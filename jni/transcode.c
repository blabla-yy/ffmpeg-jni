//
//  transcode.c
//  FFmpeg-test
//
//  Created by AdrianQaQ on 2019/1/4.
//  Copyright © 2019 adrian. All rights reserved.
//

#include "transcode.h"

int64_t dst_ch_layout = AV_CH_LAYOUT_MONO;
int dst_rate = 16000;
enum AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16;

static void decode(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame, uint8_t **dst_data,
                   int *dst_nb_samples, int *max_dst_nb_samples, int *dst_nb_channels, int *dst_linesize,
                   SwrContext *swr_ctx, FILE *outfile, void *opaque, int (*write_packet)(void *opaque, uint8_t *buf, int buf_size))
{
    int dst_bufsize;
    /* send the packet with the compressed data to the decoder */
    int ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error submitting the packet to the decoder\n");
        exit(1);
    }

    /* read all the output frames (in general there may be any number of them */
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }

        if (!dst_data) {
            *max_dst_nb_samples = *dst_nb_samples =
                av_rescale_rnd(frame->nb_samples, dst_rate, dec_ctx->sample_rate, AV_ROUND_UP);
            *dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
            ret = av_samples_alloc_array_and_samples(&dst_data, dst_linesize, *dst_nb_channels,
                                                     *dst_nb_samples, dst_sample_fmt, 0);
            if (ret < 0) {
                fprintf(stderr, "Could not allocate destination samples\n");
                exit(1);
            }
        }
        /* compute destination number of samples */
        *dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, dec_ctx->sample_rate) +
                                         frame->nb_samples, dst_rate, dec_ctx->sample_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            av_freep(&dst_data[0]);
            ret = av_samples_alloc(dst_data, dst_linesize, *dst_nb_channels,
                                   *dst_nb_samples, dst_sample_fmt, 1);
            if (ret < 0) {
                break;
            }
            max_dst_nb_samples = dst_nb_samples;
        }

        ret = swr_convert(swr_ctx, dst_data, *dst_nb_samples, frame->data, frame->nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            return;
        }

        dst_bufsize = av_samples_get_buffer_size(dst_linesize, *dst_nb_channels,
                                                 ret, dst_sample_fmt, 1);
        if (dst_bufsize < 0) {
            fprintf(stderr, "Could not get sample buffer size\n");
            return;
        }
        if (outfile != NULL) {
            fwrite(dst_data[0], 1, dst_bufsize, outfile);
        } else if (write_packet != NULL) {
            write_packet(opaque, dst_data[0], dst_bufsize);
        } else {
            fprintf(stderr, "Empty ouput config\n");
            return;
        }
    }
}

static int open_codec_context(int *stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx,
                              enum AVMediaType type, const char *src_filename)
{
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(type), src_filename);
        return ret;
    } else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }

        /* Init the decoders, with or without reference counting */
        if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

int to_pcm(void *opaque,
           int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
           int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
           const char *src_filename,
           const char *audio_dst_filename)
{
    /* local variable */
    int ret;
    FILE *audio_dst_file = NULL;
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *audio_dec_ctx = NULL;
    int audio_stream_idx = -1;
    AVStream *audio_stream = NULL;
    AVFrame *frame = NULL;
    AVPacket pkt;

    struct SwrContext *swr_ctx;
    uint8_t **dst_data = NULL;
    int dst_nb_channels = 0;
    int dst_linesize = 0;
    int dst_nb_samples = 0;
    int max_dst_nb_samples = 0;

    /* convert config */
    int64_t dst_ch_layout = AV_CH_LAYOUT_MONO;
    int dst_rate = 16000;
    enum AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16;

    /* read_packet */

    printf("read_packet\n");
    AVIOContext *avio = NULL;

    /* file_path优先，其次回调函数 */
    if (src_filename == NULL && read_packet != NULL) {
        printf("read from source stream\n");
        unsigned char *iobuffer = (unsigned char *)av_malloc(32768);
        fmt_ctx = avformat_alloc_context();
        avio = avio_alloc_context(iobuffer, 32768, 0, opaque, read_packet, NULL, NULL);
        fmt_ctx->pb = avio;
    }

    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        if (avio != NULL) {
            avformat_close_input(&fmt_ctx);
            av_free(avio->buffer);
        }
        return 1;
    }
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        if (avio != NULL) {
            avformat_close_input(&fmt_ctx);
            av_free(avio->buffer);
        }
        return 1;
    }
    if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO, src_filename) >= 0) {
        audio_stream = fmt_ctx->streams[audio_stream_idx];
        if (audio_dst_filename != NULL) {
            audio_dst_file = fopen(audio_dst_filename, "wb");
            if (!audio_dst_file) {
                fprintf(stderr, "Could not open destination file %s\n", audio_dst_filename);
                ret = 1;
                goto end;
            }
        }
    }

    av_dump_format(fmt_ctx, 0, src_filename, 0);

    if (!audio_stream) {
        fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
        ret = 1;
        goto end;
    }

    /* create resampler context */
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    av_opt_set_int(swr_ctx, "in_channel_layout", audio_dec_ctx->channel_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", audio_dec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", audio_dec_ctx->sample_fmt, 0);

    av_opt_set_int(swr_ctx, "out_channel_layout", dst_ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", dst_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

    if ((ret = swr_init(swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        goto end;
    }

    /* init frame and packet */
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    /* read packets */
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        AVPacket orig_pkt = pkt;
        decode(audio_dec_ctx,
               &pkt, frame,
               dst_data,
               &dst_nb_samples,
               &max_dst_nb_samples,
               &dst_nb_channels,
               &dst_linesize,
               swr_ctx,
               audio_dst_file,
               opaque,
               write_packet);
        av_packet_unref(&orig_pkt);
    }

    /* flush the decoder */
    pkt.data = NULL;
    pkt.size = 0;
    decode(audio_dec_ctx,
           &pkt,
           frame,
           dst_data,
           &dst_nb_samples,
           &max_dst_nb_samples,
           &dst_nb_channels,
           &dst_linesize,
           swr_ctx,
           audio_dst_file,
           opaque,
           write_packet);

    printf("Done.\n");

 end:
    printf("release ffmpeg variable\n");
    if (avio != NULL) {
        av_free(avio->buffer);
    }

    avcodec_free_context(&audio_dec_ctx);
    avformat_close_input(&fmt_ctx);

    if (audio_dst_file) {
        fclose(audio_dst_file);
    }
    av_frame_free(&frame);
    return 0;
}
