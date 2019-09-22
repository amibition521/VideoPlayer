//
// Created by 再东 on 2019-09-22.
//

#ifndef VIDEOPLAYER_LOG_H
#define VIDEOPLAYER_LOG_H

#ifdef ANDROID
#include <android/log.h>
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "zzd", format, ##__VA_ARGS__)
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, "zzd", ##__VA_ARGS__)
#endif

#endif //VIDEOPLAYER_LOG_H
