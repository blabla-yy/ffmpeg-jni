//
//  main.c
//  FFmpeg-test
//
//  Created by AdrianQaQ on 2019/1/7.
//  Copyright © 2019 adrian. All rights reserved.
//

#include "audio_transcoder.h"
#include "transcode.h"

struct Params {
    JNIEnv *cache_env;
    jobject *input_stream;
    jobject *output_stream;
};

int fill_iobuffer(void *opaque, uint8_t *buf, int buf_size)
{
    JNIEnv *cache_env = ((struct Params*)opaque)->cache_env;
    jobject *input_stream = ((struct Params*)opaque)->input_stream;

    /* invoke java method read() */
    jclass clz = (*cache_env)->FindClass(cache_env, "java/io/InputStream");
    jbyteArray bytes = (*cache_env)->NewByteArray(cache_env, buf_size);
    jmethodID method_id = (*cache_env)->GetMethodID(cache_env, clz, "read", "([B)I");
    jint res = (*cache_env)->CallIntMethod(cache_env, *input_stream, method_id, bytes);
    int len = (*cache_env)->GetArrayLength(cache_env, bytes); //todo: delete
    (*cache_env)->GetByteArrayRegion(cache_env, bytes, 0, len, (jbyte*) buf);

    /* delete local ref */
    (*cache_env)->DeleteLocalRef(cache_env, bytes);
    return res;
}

int write_stream(void *opaque, uint8_t *buf, int buf_size)
{
    JNIEnv *cache_env = ((struct Params*)opaque)->cache_env;
    jobject *output_stream = ((struct Params*)opaque)->output_stream;
    jbyteArray bytes = (*cache_env)->NewByteArray(cache_env, buf_size);

    /* invoke java method wirte() */
    jclass clz = (*cache_env)->FindClass(cache_env, "java/io/OutputStream");
    (*cache_env)->SetByteArrayRegion(cache_env, bytes, 0, buf_size, (jbyte*) buf);
    jmethodID method_id = (*cache_env)->GetMethodID(cache_env, clz, "write", "([B)V");
    (*cache_env)->CallVoidMethod(cache_env, *output_stream, method_id, bytes);

     /* invoke java method flush() */
    jmethodID flush_id = (*cache_env)->GetMethodID(cache_env, clz, "flush", "()V");
    (*cache_env)->CallVoidMethod(cache_env, *output_stream, flush_id);

    /* delete local ref */
    (*cache_env)->DeleteLocalRef(cache_env, bytes);
    return buf_size;
}

JNIEXPORT jint JNICALL Java_AudioTranscoder_transcodeToPCM(JNIEnv *env, jobject obj, jobject stream, jobject output_stream)
{
    struct Params params = {env, &stream, &output_stream};
    int res = to_pcm(&params, fill_iobuffer, write_stream, NULL, NULL);
    fflush(stdout);
    return res;
}

JNIEXPORT jint JNICALL Java_AudioTranscoder_transcodeToPCMFile
(JNIEnv *env, jobject obj, jstring src, jstring output)
{
    const char *src_path = (*env)->GetStringUTFChars(env, src, 0);
    const char *output_path = (*env)->GetStringUTFChars(env, output, 0);
    printf("file path src:%s output:%s \n", src_path, output_path);
    int res = to_pcm(NULL, NULL, NULL, src_path, output_path);
    (*env)->ReleaseStringUTFChars(env, src, src_path);
    (*env)->ReleaseStringUTFChars(env, src, output_path);
    fflush(stdout);
    return res;
}
