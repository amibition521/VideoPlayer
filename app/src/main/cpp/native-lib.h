//
// Created by 张再东 on 2019-09-19.
//

#ifndef VIDEODEMO_NATIVE_LIB_H
#define VIDEODEMO_NATIVE_LIB_H

#ifdef ANDROID
#include <android/log.h>
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_DEBUG, "zzd", format, ##__VA_ARGS__)
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, "zzd", ##__VA_ARGS__)
#endif

#endif //VIDEODEMO_NATIVE_LIB_H
