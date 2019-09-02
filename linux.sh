#!/bin/bash

ProjectPath=/home/ffmpeg-jni
JavaHome=/usr/java/jdk1.8.0_161
FFmpegPath=/ProjectPath/ffmpeg/ffmpeg

JavaHPath=${JavaHome}/bin/javah

echo 'Generate header...'
rm ${ProjectPath}/jni/audio_transcoder.h
${JavaHPath} -jni -classpath ${ProjectPath}/src/ -o ${ProjectPath}/jni/audio_transcoder.h AudioTranscoder

echo 'Compile C...'

gcc -fPIC -shared \
${ProjectPath}/jni/main.c ${ProjectPath}/jni/transcode.c \
${FFmpegPath}/lib/libswscale.a \
${FFmpegPath}/lib/libavformat.a \
${FFmpegPath}/lib/libavcodec.a \
${FFmpegPath}/lib/libswresample.a \
${FFmpegPath}/lib/libavutil.a \
-lm -lz -lpthread -ldl -lrt \
-o ${ProjectPath}/jni/transcode \
-I ${JavaHome}/include \
-I ${JavaHome}/include/linux \
-I ${FFmpegPath}/include \
-Wl,-Bsymbolic

echo 'Complete!'
