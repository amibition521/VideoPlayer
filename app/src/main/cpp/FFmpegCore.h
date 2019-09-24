//
// Created by 再东 on 2019-09-22.
//

#ifndef VIDEOPLAYER_FFMPEGCORE_H
#define VIDEOPLAYER_FFMPEGCORE_H

#include "stdio.h"
int initFFmpeg(int *rate, int *channel, char *url);
void getPCM(void **pcm, size_t *pcmSize);
int releaseFFmpeg();


#endif //VIDEOPLAYER_FFMPEGCORE_H
