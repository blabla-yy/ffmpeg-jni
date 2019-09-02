#!/bin/bash

ProjectPath=/Users/Documents/ffmpeg-jni
JavaHome=/Library/Java/JavaVirtualMachines/jdk1.8.0_161.jdk/Contents/Home
FFmpeg=/usr/local/Cellar/ffmpeg/4.1.3_1
JavaHPath=${JavaHome}/bin/javah

echo 'Generate Header...'
rm ${ProjectPath}/jni/audio_transcoder.h
${JavaHPath} -jni -classpath ${ProjectPath}/src/ -o ${ProjectPath}/jni/audio_transcoder.h AudioTranscoder

echo 'Start Compile...'
gcc -dynamiclib \
${ProjectPath}/jni/main.c ${ProjectPath}/jni/transcode.c \
-o ${ProjectPath}/jni/transcode \
-framework JavaVM \
-I ${JavaHome}/include \
-I ${JavaHome}/include/darwin \
-I ${FFmpeg}/include \
-L ${FFmpeg}/lib \
-lavutil -lavformat -lavcodec -lswresample -lswscale

echo 'Compile Complete!'