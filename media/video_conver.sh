#!/bin/bash

OUTPUT=encoded

test -d ${OUTPUT} && rm -rf ${OUTPUT}
mkdir -p ${OUTPUT}

for file in $(ls *.mp4);do
    filename=${file%%.mp4}
    #${OUTPUT}t audio + video for SD card
    #MP3 Audio 44.1 kHz Mono
    ffmpeg -i ${file} -ar 44100 -ac 1 -ab 32k -filter:a loudnorm -filter:a "volume=-5dB" ${OUTPUT}/${filename}.mp3
    #MJPEG Video 288x240@30fps
    ffmpeg -i ${file} -vf "fps=30,scale=-1:240:flags=lanczos,crop=288:in_h:(in_w-288)/2:0" -q:v 11 ${OUTPUT}/${filename}.mjpeg
    count=$((counter+1))
done

