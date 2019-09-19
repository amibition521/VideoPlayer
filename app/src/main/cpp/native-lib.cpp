//
// Created by 张再东 on 2019-09-19.
//

#include "native-lib.h"
#include <jni.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
//Log
#ifdef ANDROID
#include <android/log.h>
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_DEBUG, "zzd", format, ##__VA_ARGS__)
#endif

#include <libavutil/file.h>

#ifdef  __ANDROID__
#include <android/log.h>
#define ALOG(level, TAG, format, ...) ((void)__android_log_print(level, TAG,format, ##__VA_ARGS__))
#define SYS_LOG_TAG "zzd---player"

static void syslog_print(void *ptr, int level, const char *fmt, va_list vl) {
    LOGE("%s", "call back log");

    switch (level) {
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

static void syslog_init() {
    LOGE("%s", "init log");
    av_log_set_callback(syslog_print);
}

#endif

//FIX
struct URLProtocol;

//avcodecinfo
JNIEXPORT jint JNICALL
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

    return 0;
}

JNIEXPORT jint JNICALL
Java_com_zhangzd_video_MainActivity_avioreading(JNIEnv *env, jobject instance, jstring src_) {
    const char *src = env->GetStringUTFChars(src_, NULL);
    LOGE("%s", src);
    av_log_set_callback(syslog_print);
    AVFormatContext *fmt_ctx = NULL;
    AVIOContext *avio_ctx = NULL;
    uint8_t *buffer = NULL, *avio_ctx_buffer = NULL;
    size_t buffer_size, avio_ctx_buffer_size = 4096;
//    const char *sr = "/storage/emulated/0/DCIM/Camera/e7a9c442d1cda4462fc03459d71d8b7c.mp4";

    char input_filename[500] = {0};
    int ret = 0;
    sprintf(input_filename, "%s", src);
//    struct buffer_data bd = {0};
    LOGE("%s    ---", input_filename);

    av_register_all();
    avformat_network_init();

//    bd.ptr = buffer;
//    bd.size = buffer_size;

    LOGE("%s", "2 couldn't open input file .");
    if (!(fmt_ctx = avformat_alloc_context())) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    LOGE("%s", "3 couldn't open input file .");
    avio_ctx_buffer = static_cast<uint8_t *>(av_malloc(avio_ctx_buffer_size));
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);

        goto end;
    }

    LOGE("%s", "4 couldn't open input file .");
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, NULL, NULL,
                                  NULL,
                                  NULL);
    if (!avio_ctx) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    fmt_ctx->pb = avio_ctx;

    LOGE("%s", "5 Could not open input");
    ret = avformat_open_input(&fmt_ctx, input_filename, NULL, NULL);
    LOGE("error %d,code: %s", ret, av_err2str(ret));
    if (ret != 0) {
        goto end;
    }
    LOGE("%s", "6 Could not find stream information \n");
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        goto end;
    }

    av_dump_format(fmt_ctx, 0, input_filename, 0);
    LOGE("%s", "end");

    end:
    avformat_close_input(&fmt_ctx);
    if (avio_ctx) {
        av_freep(&avio_ctx->buffer);
        av_freep(&avio_ctx);
    }
    av_file_unmap(buffer, buffer_size);
    LOGE("%s", "error.");

    if (ret < 0) {
        fprintf(stderr, "Error occured:%s\n", av_err2str(ret));
        LOGE("%s", "error.");
        return 1;
    }

    return 0;
}


JNIEXPORT jint JNICALL
Java_com_zhangzd_video_MainActivity_sysloginit(JNIEnv *env, jobject instance) {

    syslog_init();

    return NULL;
}

struct buffer_data {
    uint8_t *ptr;
    size_t size;
};

static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
    struct buffer_data *bd = (struct buffer_data *) opaque;
    buf_size = FFMIN(buf_size, bd->size);

//    LOGE("%p %zu\n", bd->ptr, bd->size);

    memcpy(buf, bd->ptr, buf_size);
    bd->ptr += buf_size;
    bd->size -= buf_size;

    return buf_size;
}

int avio_read2(char *src) {
    AVFormatContext *fmt_ctx = NULL;
    AVIOContext *avio_ctx = NULL;
    uint8_t *buffer = NULL, *avio_ctx_buffer = NULL;
    size_t buffer_size, avio_ctx_buffer_size = 4096;
//    const char *sr = "/storage/emulated/0/DCIM/Camera/e7a9c442d1cda4462fc03459d71d8b7c.mp4";

    char input_filename[500] = {0};
    int ret = 0;
    sprintf(input_filename, "%s", src);
//    struct buffer_data bd = {0};
    LOGE("%s    ---", input_filename);

    av_register_all();
    avformat_network_init();

//    bd.ptr = buffer;
//    bd.size = buffer_size;

    LOGE("%s", "2 couldn't open input file .");
    if (!(fmt_ctx = avformat_alloc_context())) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    LOGE("%s", "3 couldn't open input file .");
    avio_ctx_buffer = static_cast<uint8_t *>(av_malloc(avio_ctx_buffer_size));
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);

        goto end;
    }

    LOGE("%s", "4 couldn't open input file .");
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, NULL, &read_packet,
                                  NULL,
                                  NULL);
    if (!avio_ctx) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    fmt_ctx->pb = avio_ctx;

    LOGE("%s", "5 Could not open input");
    ret = avformat_open_input(&fmt_ctx, input_filename, NULL, NULL);
    LOGE("error %code, %s", ret, av_err2str(ret));
    if (ret != 0) {
        goto end;
    }
    LOGE("%s", "6 Could not find stream information \n");
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        goto end;
    }

    av_dump_format(fmt_ctx, 0, input_filename, 0);
    LOGE("%s", "end");

    end:
    avformat_close_input(&fmt_ctx);
    if (avio_ctx) {
        av_freep(&avio_ctx->buffer);
        av_freep(&avio_ctx);
    }
    av_file_unmap(buffer, buffer_size);
    LOGE("%s", "error.");

    if (ret < 0) {
        fprintf(stderr, "Error occured:%s\n", av_err2str(ret));
        LOGE("%s", "error.");
        return 1;
    }

    return 0;

}

}