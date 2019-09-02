//
//  transcode.h
//  FFmpeg-test
//
//  Created by AdrianQaQ on 2019/1/4.
//  Copyright © 2019 adrian. All rights reserved.
//

#ifndef transcode_h
#define transcode_h

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/samplefmt.h"
//#include <sys/buf.h>
int to_pcm(void *opaque,
           int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
           int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
           const char *src_filename,
           const char *audio_dst_filename);
#endif /* transcode_h */
