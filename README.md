# FFmpeg-JNI

This project transcode mp3 files into pcm files by using FFmpeg native method.


## Why
- Native methods can improve transcoding efficiency and reduce the cost of process switching.
- Most of voice evaluation system require pcm source files, and this format is not commonly used. So audio transcoding will be used frequently.


## Build
1. Install FFmpeg
2. Modify shell variable
3. Compile to binary using linux.sh or mac.sh
4. Modify AudioTranscoder.java
5. Run AudioTranscoder main method