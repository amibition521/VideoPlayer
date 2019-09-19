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

struct buffer_data {
    uint8_t *ptr;
    size_t size;
};
static int read_packet(void *opaque, uint8_t *buf, int buf_size);
static void av_dump_format2(AVFormatContext *ic, int index,const char *url, int is_output);

static void syslog_print(void *ptr, int level, const char *fmt, va_list vl) {
//    LOGE("%s, %d", "call back log", level);

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
    av_log_set_callback(syslog_print);

    AVFormatContext *fmt_ctx = NULL;
    AVIOContext *avio_ctx = NULL;
    uint8_t *buffer = NULL, *avio_ctx_buffer = NULL;
    size_t buffer_size, avio_ctx_buffer_size = 4096;

    char input_filename[500] = {0};
    int ret = 0;
    sprintf(input_filename, "%s", src);
//    LOGE("%s    ---", input_filename);

    struct buffer_data bd = { 0 };


    LOGE("usage: %s input_file\n"
                    "API example program to show how to read from a custom buffer "
                    "accessed through AVIOContext.\n",input_filename);
    av_register_all();
    avformat_network_init();

    /* slurp file content into buffer */
    ret = av_file_map(input_filename, &buffer, &buffer_size, 0, NULL);
    if (ret < 0)
        goto end;

    /* fill opaque structure used by the AVIOContext read callback */
    bd.ptr  = buffer;
    bd.size = buffer_size;

    if (!(fmt_ctx = avformat_alloc_context())) {
        LOGE("%s", "2 couldn't open input file .");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    avio_ctx_buffer = (uint8_t *)av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        LOGE("%s", "3 couldn't open input file .");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, &bd, &read_packet, NULL,NULL);
    if (!avio_ctx) {
        LOGE("%s", "4 couldn't open input file .");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    fmt_ctx->pb = avio_ctx;

    ret = avformat_open_input(&fmt_ctx, input_filename, NULL, NULL);
    if (ret != 0) {
        LOGE("%s", "5 Could not open input");
        LOGE("error %d,code: %s", ret, av_err2str(ret));
        goto end;
    }
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        LOGE("%s", "6 Could not find stream information \n");
        goto end;
    }
    LOGE("%s", "end   1");
    //TODO:导致崩溃
    av_dump_format2(fmt_ctx, 0, input_filename, 0);
    LOGE("%s", "end   2");

end:
    avformat_close_input(&fmt_ctx);
    if (avio_ctx) {
        av_freep(&avio_ctx->buffer);
        av_freep(&avio_ctx);
    }
    av_file_unmap(buffer, buffer_size);

    if (ret < 0) {
        LOGE("Error occured:%s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}


JNIEXPORT jint JNICALL
Java_com_zhangzd_video_MainActivity_sysloginit(JNIEnv *env, jobject instance) {

    syslog_init();

    return NULL;
}
}
int read_packet(void *opaque, uint8_t *buf, int buf_size) {
    struct buffer_data *bd = (struct buffer_data *) opaque;
    buf_size = FFMIN(buf_size, bd->size);

//    LOGE("ptr:%p, size:%zu\n", bd->ptr, bd->size);

    memcpy(buf, bd->ptr, buf_size);
    bd->ptr += buf_size;
    bd->size -= buf_size;

    return buf_size;
}


void av_dump_format2(AVFormatContext *ic, int index,
                    const char *url, int is_output)
{
    int i;
    uint8_t *printed = static_cast<uint8_t *>(ic->nb_streams ? av_mallocz(ic->nb_streams) : NULL);
    if (ic->nb_streams && !printed)
        return;

    LOGE("%s #%d, %s, %s '%s':\n",
           is_output ? "Output" : "Input",
           index,
           is_output ? ic->oformat->name : ic->iformat->name,
           is_output ? "to" : "from", url);
//    dump_metadata(NULL, ic->metadata, "  ");

    if (!is_output) {
        if (ic->duration != AV_NOPTS_VALUE) {
            int hours, mins, secs, us;
            int64_t duration = ic->duration + (ic->duration <= INT64_MAX - 5000 ? 5000 : 0);
            secs  = duration / AV_TIME_BASE;
            us    = duration % AV_TIME_BASE;
            mins  = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;
            LOGE("Duration: %02d:%02d:%02d.%02d", hours, mins, secs,
                   (100 * us) / AV_TIME_BASE);
        } else {
            LOGE("%s", "N/A");
        }
        if (ic->start_time != AV_NOPTS_VALUE) {
            long long secs, us;
            av_log(NULL, AV_LOG_INFO, ", start: ");
            secs = llabs(ic->start_time / AV_TIME_BASE);
            us   = llabs(ic->start_time % AV_TIME_BASE);
            LOGE("%s%d.%06d",
                   ic->start_time >= 0 ? "" : "-",
                   secs,
                   (int) av_rescale(us, 1000000, AV_TIME_BASE));
        }
        if (ic->bit_rate)
            LOGE("bitrate: %" PRId64 " kb/s", ic->bit_rate / 1000);
        else
            LOGE("bitrate: %s", "N/A");
    }

    for (i = 0; i < ic->nb_chapters; i++) {
        AVChapter *ch = ic->chapters[i];
        LOGE("Chapter #%d:%d: ", index, i);
        LOGE("start %f, ", ch->start * av_q2d(ch->time_base));
        LOGE("end %f\n", ch->end * av_q2d(ch->time_base));
//        dump_metadata(NULL, ch->metadata, "    ");
    }

    if (ic->nb_programs) {
        int j, k, total = 0;
        for (j = 0; j < ic->nb_programs; j++) {
            AVDictionaryEntry *name = av_dict_get(ic->programs[j]->metadata,
                                                  "name", NULL, 0);
            LOGE("  Program %d %s\n", ic->programs[j]->id,
                   name ? name->value : "");
//            dump_metadata(NULL, ic->programs[j]->metadata, "    ");
//            for (k = 0; k < ic->programs[j]->nb_stream_indexes; k++) {
//                dump_stream_format(ic, ic->programs[j]->stream_index[k],
//                                   index, is_output);
//                printed[ic->programs[j]->stream_index[k]] = 1;
//            }
            total += ic->programs[j]->nb_stream_indexes;
        }
        if (total < ic->nb_streams)
            LOGE("%s","No Program\n");
    }

//    for (i = 0; i < ic->nb_streams; i++)
//        if (!printed[i])
//            dump_stream_format(ic, i, index, is_output);

    av_free(printed);
}



