//
// Created by 张再东 on 2019-09-18.
//

#include "native-lib.h"
#include <stdio.h>
#include <jni.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
//Log
#ifdef ANDROID
#include <android/log.h>
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "zzd", format, ##__VA_ARGS__)
#else
#define LOGD(format, ...)  printf("(>_<) " format "\n", ##__VA_ARGS__)
#endif

//#ifndef _SYS_LOG_
//#define _SYS_LOG_
//
//#include <libavutil/log.h>
//#define LOGD(format, ...) av_log(NULL, AV_LOG_DEBUG, format, ##__VA_ARGS__)
//#define LOGV(format, ...) av_log(NULL, AV_LOG_VERSON, format, ##__VA_ARGS__)
//#define LOGI(format, ...) av_log(NULL, AV_LOG_INFO, format, ##__VA_ARGS__)
//#define LOGW(format, ...) av_log(NULL, AV_LOG_WARING, format, ##__VA_ARGS__)
//#define LOGE(format, ...) av_log(NULL, AV_LOG_ERROR, format, ##__VA_ARGS__)
//
//#endif

#ifdef  __ANDROID_API__
#include <android/log.h>
#include <libavutil/file.h>

#define ALOG(level, TAG, ...) ((void)__android_log_vprint(level, TAG, ##__VA_ARGS__))
#define SYS_LOG_TAG "zzdplayer"

static void syslog_print(void *ptr, int level, const char *fmt, va_list vl)
{
    switch (level){
        case AV_LOG_DEBUG:
            ALOG(AV_LOG_DEBUG, SYS_LOG_TAG, fmt, vl);
            break;
        case AV_LOG_VERBOSE:
            ALOG(AV_LOG_VERBOSE, SYS_LOG_TAG, fmt, vl);
            break;
        case AV_LOG_INFO:
            ALOG(AV_LOG_INFO, SYS_LOG_TAG, fmt, vl);
            break;
        case AV_LOG_WARNING:
            ALOG(AV_LOG_WARNING, SYS_LOG_TAG, fmt, vl);
            break;
        case AV_LOG_ERROR:
            ALOG(AV_LOG_ERROR, SYS_LOG_TAG, fmt, vl);
            break;
    }
}

static void syslog_init()
{
    av_log_set_callback(syslog_print);
}

#endif

//FIX
struct URLProtocol;

//avcodecinfo
JNIEXPORT jstring JNICALL
Java_com_zhangzd_video_MainActivity_avcodecinfo(JNIEnv *env, jobject obj) {
    char info[40000] = {0};

    av_register_all();

    AVCodec *c_temp = av_codec_next(NULL);

    while (c_temp != NULL) {
        if (c_temp->decode != NULL) {
            sprintf(info, "%s[Dec]", info);
        } else {
            sprintf(info, "%s[Enc]", info);
        }
        switch (c_temp->type) {
            case AVMEDIA_TYPE_VIDEO:
                sprintf(info, "%s[Video]", info);
                break;
            case AVMEDIA_TYPE_AUDIO:
                sprintf(info, "%s[Audio]", info);
                break;
            default:
                sprintf(info, "%s[Other]", info);
                break;
        }
        sprintf(info, "%s[%10s]\n", info, c_temp->name);


        c_temp = c_temp->next;
    }
    LOGE("%s", info);

    return (*env)->NewStringUTF(env, info);
}

JNIEXPORT jstring JNICALL
Java_com_zhangzd_video_MainActivity_avioreading(JNIEnv *env, jobject instance, jstring src_) {
    const char *src = (*env)->GetStringUTFChars(env, src_, 0);
    LOGE("%s", src);

    avio_read(src);
//    (*env)->ReleaseStringUTFChars(env, src_, src);

    return (*env)->NewStringUTF(env, src);
}

JNIEXPORT jstring JNICALL
Java_com_zhangzd_video_MainActivity_sysloginit(JNIEnv *env, jobject instance) {

    syslog_init();

    return (*env)->NewStringUTF(env, "");
}

struct buffer_data {
    uint8_t *ptr;
    size_t size;
};

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    struct buffer_data *bd = (struct buffer_data *)opaque;
    buf_size = FFMIN(buf_size, bd->size);

    LOGE("%p %zu\n", bd->ptr, bd->size);

    memcpy(buf, bd->ptr, buf_size);
    bd->ptr += buf_size;
    bd->size -= buf_size;

    return buf_size;
}

int avio_read(const char *src) {
    AVFormatContext *fmt_ctx = NULL;
    AVIOContext *avio_ctx = NULL;
    uint8_t *buffer = NULL, *avio_ctx_buffer = NULL;
    size_t buffer_size, avio_ctx_buffer_size = 4096;
//    const char *sr = "/storage/emulated/0/DCIM/Camera/e7a9c442d1cda4462fc03459d71d8b7c.mp4";

    const char *input_filename = src;
    int ret = 0;
    struct buffer_data bd = {0};
    LOGE("%s", "start");

    av_register_all();
//    ret = av_file_map(input_filename, &buffer, &buffer_size, 0, NULL);
//    if (ret < 0) {
//        LOGE("%s", "couldn't open input file .");
//        goto end;
//    }

    bd.ptr = buffer;
    bd.size = buffer_size;

    if (!(fmt_ctx = avformat_alloc_context())) {
        LOGE("%s", "2 couldn't open input file .");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        LOGE("%s", "3 couldn't open input file .");
        ret = AVERROR(ENOMEM);

        goto end;
    }

    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, NULL, &read_packet, NULL,
                                  NULL);
    if (!avio_ctx) {
        LOGE("%s", "4 couldn't open input file .");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    fmt_ctx->pb = avio_ctx;

    ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    if (ret < 0) {
        LOGE("%s", "5 Could not open input");
        goto end;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        LOGE("%s", "6 Could not find stream information \n");
        goto end;
    }

    av_dump_format(fmt_ctx, 0, input_filename, 0);
    LOGE("%s","end");

    end:
    avformat_close_input(&fmt_ctx);
    if (avio_ctx){
        av_freep(&avio_ctx->buffer);
        av_freep(&avio_ctx);
    }
    av_file_unmap(buffer, buffer_size);
    LOGE("%s", "error.");

    if (ret < 0){
        fprintf(stderr, "Error occured:%s\n", av_err2str(ret));
        LOGE("%s", "error.");
        return 1;
    }

    return 0;

}