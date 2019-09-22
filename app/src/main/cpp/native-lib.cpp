//
// Created by 张再东 on 2019-09-19.
//

#include "native-lib.h"
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
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
static int play(JNIEnv *env, jobject surface, const char *src);

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
   // av_dump_format2(fmt_ctx, 0, input_filename, 0);
    LOGE("%s", "end");

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

JNIEXPORT jint JNICALL
Java_com_zhangzd_video_MainActivity_play(JNIEnv *env,jobject instance, jobject surface, jstring src_) {
    const char *src = env->GetStringUTFChars(src_, NULL);
    av_log_set_callback(syslog_print);

    play(env, surface, src);
    return 0;
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


static void dump_metadata(void *ctx, AVDictionary *m, const char *indent)
{
    if (m && !(av_dict_count(m) == 1 && av_dict_get(m, "language", NULL, 0))) {
        AVDictionaryEntry *tag = NULL;

        LOGE("%sMetadata:", indent);
        while ((tag = av_dict_get(m, "", tag, AV_DICT_IGNORE_SUFFIX)))
            if (strcmp("language", tag->key)) {
                const char *p = tag->value;
                LOGE("%s  %-16s: ", indent, tag->key);
                while (*p) {
                    char tmp[256];
                    size_t len = strcspn(p, "\x8\xa\xb\xc\xd");
//                    av_strlcpy(tmp, p, FFMIN(sizeof(tmp), len+1));
                    LOGE("%s", tmp);
                    p += len;
                    if (*p == 0xd) LOGE(" ");
                    if (*p == 0xa) LOGE("\n%s  %-16s: ", indent, "");
                    if (*p) p++;
                }
            }
    }
}

static void dump_stream_format(AVFormatContext *ic, int i,
                               int index, int is_output)
{
    char buf[256];
    int flags = (is_output ? ic->oformat->flags : ic->iformat->flags);
    AVStream *st = ic->streams[i];
    AVDictionaryEntry *lang = av_dict_get(st->metadata, "language", NULL, 0);
    char *separator = reinterpret_cast<char *>(ic->dump_separator);
    AVCodecContext *avctx;
    int ret;

    avctx = avcodec_alloc_context3(NULL);
    if (!avctx)
        return;

    ret = avcodec_parameters_to_context(avctx, st->codecpar);
    if (ret < 0) {
        avcodec_free_context(&avctx);
        return;
    }

    // Fields which are missing from AVCodecParameters need to be taken from the AVCodecContext
//    avctx->properties = st->codec->properties;
//    avctx->codec      = st->codec->codec;
//    avctx->qmin       = st->codec->qmin;
//    avctx->qmax       = st->codec->qmax;
//    avctx->coded_width  = st->codec->coded_width;
//    avctx->coded_height = st->codec->coded_height;

//    if (separator)
//        av_opt_set(avctx, "dump_separator", separator, 0);
    avcodec_string(buf, sizeof(buf), avctx, is_output);
    avcodec_free_context(&avctx);

    LOGE("Stream #%d:%d", index, i);

    /* the pid is an important information, so we display it */
    /* XXX: add a generic system */
    if (flags & AVFMT_SHOW_IDS)
        LOGE("[0x%x]", st->id);
    if (lang)
        LOGE("(%s)", lang->value);
    LOGE("%d, %d/%d", st->codec_info_nb_frames,
           st->time_base.num, st->time_base.den);
    LOGE(": %s", buf);

    if (st->sample_aspect_ratio.num &&
        av_cmp_q(st->sample_aspect_ratio, st->codecpar->sample_aspect_ratio)) {
        AVRational display_aspect_ratio;
        av_reduce(&display_aspect_ratio.num, &display_aspect_ratio.den,
                  st->codecpar->width  * (int64_t)st->sample_aspect_ratio.num,
                  st->codecpar->height * (int64_t)st->sample_aspect_ratio.den,
                  1024 * 1024);
        LOGE(", SAR %d:%d DAR %d:%d",
               st->sample_aspect_ratio.num, st->sample_aspect_ratio.den,
               display_aspect_ratio.num, display_aspect_ratio.den);
    }

    if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        int fps = st->avg_frame_rate.den && st->avg_frame_rate.num;
        int tbr = st->r_frame_rate.den && st->r_frame_rate.num;
        int tbn = st->time_base.den && st->time_base.num;
//        int tbc = st->codec->time_base.den && st->codec->time_base.num;

        if (fps || tbr || tbn )
            LOGE("%s", separator);

//        if (fps)
//            print_fps(av_q2d(st->avg_frame_rate), tbr || tbn || tbc ? "fps, " : "fps");
//        if (tbr)
//            print_fps(av_q2d(st->r_frame_rate), tbn || tbc ? "tbr, " : "tbr");
//        if (tbn)
//            print_fps(1 / av_q2d(st->time_base), tbc ? "tbn, " : "tbn");
//        if (tbc)
//            print_fps(1 / av_q2d(st->codec->time_base), "tbc");
    }

    if (st->disposition & AV_DISPOSITION_DEFAULT)
        LOGE("%s", " (default)");
    if (st->disposition & AV_DISPOSITION_DUB)
        LOGE("%s", " (dub)");
    if (st->disposition & AV_DISPOSITION_ORIGINAL)
        LOGE("%s", " (original)");
    if (st->disposition & AV_DISPOSITION_COMMENT)
        LOGE("%s", " (comment)");
    if (st->disposition & AV_DISPOSITION_LYRICS)
        LOGE("%s", " (lyrics)");
    if (st->disposition & AV_DISPOSITION_KARAOKE)
        LOGE("%s", " (karaoke)");
    if (st->disposition & AV_DISPOSITION_FORCED)
        LOGE("%s", " (forced)");
    if (st->disposition & AV_DISPOSITION_HEARING_IMPAIRED)
        LOGE("%s", " (hearing impaired)");
    if (st->disposition & AV_DISPOSITION_VISUAL_IMPAIRED)
        LOGE("%s", " (visual impaired)");
    if (st->disposition & AV_DISPOSITION_CLEAN_EFFECTS)
        LOGE("%s", " (clean effects)");
    LOGE("%s",  "\n");

    dump_metadata(NULL, st->metadata, "    ");

//    dump_sidedata(NULL, st, "    ");
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
    dump_metadata(NULL, ic->metadata, "  ");

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
//            LOGE("%s%d.%06d",
//                   ic->start_time >= 0 ? "" : "-",
//                   secs,
//                   (int) av_rescale(us, 1000000, AV_TIME_BASE));
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
            dump_metadata(NULL, ic->programs[j]->metadata, "    ");
            for (k = 0; k < ic->programs[j]->nb_stream_indexes; k++) {
                dump_stream_format(ic, ic->programs[j]->stream_index[k],
                                   index, is_output);
                printed[ic->programs[j]->stream_index[k]] = 1;
            }
            total += ic->programs[j]->nb_stream_indexes;
        }
        if (total < ic->nb_streams)
            LOGE("%s","No Program\n");
    }

    for (i = 0; i < ic->nb_streams; i++)
        if (!printed[i])
            dump_stream_format(ic, i, index, is_output);

    av_free(printed);
}

static int play(JNIEnv *env, jobject surface, const char *src) {
    int ret = 0;
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *video_dec_ctx = NULL;
    AVCodecContext *audio_dex_ctx = NULL;
    int width, height;
    enum AVPixelFormat pix_fmt;
    AVStream *video_stream = NULL;
    AVStream *audio_stream = NULL;

    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;
    AVCodecParserContext *parser;

    int video_stream_idx = -1, audio_stream_idx = -1;
    AVPacket pkt;
    struct SwsContext *sws_ctx = NULL;

    uint8_t *video_dst_data[4] = {NULL};
    int video_dst_linesize[4];

    av_register_all();
    avformat_network_init();

    fmt_ctx = avformat_alloc_context();
    char input_filename[500] = {0};
    sprintf(input_filename, "%s", src);
    LOGE("22  Could open source file %s.\n", input_filename);

    //open file
    ret = avformat_open_input(&fmt_ctx, input_filename, NULL, NULL);
    if (ret < 0) {
        LOGE("Could not open source file %s.\n", av_err2str(ret));
        return 0;
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        LOGE("Could not find stream information.\n");
        return 0;
    }
    //decode
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    LOGE("Could find %s stream in input file.\n",
         av_get_media_type_string(AVMEDIA_TYPE_VIDEO));

    if (ret < 0) {
        LOGE("Could not find %s stream in input file.\n",
             av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        return ret;
    }
    int i;
    for (i = 0; i < fmt_ctx->nb_streams;i++){
        if (fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_idx < 0) {
           video_stream_idx = i;
           break;
        }
    }
    LOGE("Could not find %d stream in input file.\n", video_stream_idx);

    video_stream = fmt_ctx->streams[video_stream_idx];

    video_dec_ctx = fmt_ctx->streams[video_stream_idx]->codec;

    dec = avcodec_find_decoder(video_dec_ctx -> codec_id);
    if(dec==NULL) {
        LOGD("Codec not found.");
        return -1; // Codec not found
    }

//    av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
    ret = avcodec_open2(video_dec_ctx, dec, &opts);
    if (ret < 0) {
        LOGE("Failed to open %s codec \n", av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        return ret;
    }
    //
//    width = video_dec_ctx->width;
//    height = video_dec_ctx->height;
//    pix_fmt = video_dec_ctx->pix_fmt;
//    ret = av_image_alloc(video_dst_data, video_dst_linesize, width, height, pix_fmt, 1);
//    if (ret < 0) {
//        LOGD("555 Could open source file.%d, \n", ret);
//        return 0;
//    }
//    // init packet
//    av_init_packet(&pkt);
//    pkt.data = NULL;
//    pkt.size = 0;

    LOGE("Demuxing succeeded. video:%d, audio: %d, \n", video_stream_idx, audio_stream_idx);

    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    LOGD("71 Could open source file.\n");

    int videoWidth = video_dec_ctx->width;
    int videoHeight = video_dec_ctx->height;

    ANativeWindow_setBuffersGeometry(nativeWindow, videoWidth, videoHeight,
                                     WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer window_buffer;

    AVFrame *pFrame = av_frame_alloc();
    AVFrame *pFrameRGBA = av_frame_alloc();

    if (pFrameRGBA == NULL || pFrame == NULL) {
        LOGE("%s", "Could not allocate video frame.");
        return -1;
    }
    LOGD("7 Could open source file.\n");

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    LOGE("77 numBytes:%d.\n", numBytes);

    uint8_t *buffer = static_cast<uint8_t *>(av_malloc(numBytes * sizeof(uint8_t)));
    av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, buffer, AV_PIX_FMT_RGBA,
                         video_dec_ctx->width, video_dec_ctx->height, 1);

    LOGE("Impossible to create scale context for the conversion "
         "fmt:%s s:%dx%d -> fmt:%s s:%dx%d.\n",
         av_get_pix_fmt_name(pix_fmt), videoWidth, videoHeight,
         av_get_pix_fmt_name(AV_PIX_FMT_RGBA), videoWidth, videoHeight);
//    uint8_t *dst_data[4];
//    int dst_linesize[4];
//    av_image_alloc(dst_data, dst_linesize, video_dec_ctx->width, video_dec_ctx->height,AV_PIX_FMT_RGBA, 1);
    LOGD("7771 Could open source file.\n");

    sws_ctx = sws_getContext(video_dec_ctx->width,video_dec_ctx->height,video_dec_ctx->pix_fmt,
            video_dec_ctx->width,video_dec_ctx->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL,NULL,NULL);
    if (!sws_ctx) {
        ret = AVERROR(EINVAL);
        LOGD("77  ----\n");
        return 0;
    }
    LOGD("7772----\n");
/*
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {

        if (pkt.stream_index == video_stream_idx) {
            ret = avcodec_send_packet(video_dec_ctx, &pkt);
            av_packet_unref(&pkt);

            if (ret < 0) {
                LOGE("Error decoding video frame (%s)\n", av_err2str(ret));
                break;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(video_dec_ctx, pFrame);
                if (ret < 0) {
                    LOGE("Error during decoding %s \n", av_err2str(ret));
                    break;
                }
                LOGE("saving frame %3d\n", video_dec_ctx->frame_number);


                ANativeWindow_lock(nativeWindow, &window_buffer, 0);

                sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data,
                          pFrame->linesize, 0, video_dec_ctx->height, pFrameRGBA->data,
                          pFrameRGBA->linesize);

                uint8_t *dst = (uint8_t *) window_buffer.bits;
                int dstStride = window_buffer.stride * 4;
                uint8_t *src = pFrameRGBA->data[0];
                int srcStride = pFrameRGBA->linesize[0];

                int h;
                for (h = 0; h < videoHeight; h++) {
                    memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(nativeWindow);
            }
        }
        av_packet_unref(&pkt);
    }
*/
    int frameFinished;
    AVPacket packet;
    while(av_read_frame(fmt_ctx, &pkt) >=0) {
        // Is this a packet from the video stream?
        if(pkt.stream_index==video_stream_idx) {

            // Decode video frame
            avcodec_decode_video2(video_dec_ctx, pFrame, &frameFinished, &pkt);

            // 并不是decode一次就可解码出一帧
            if (frameFinished) {

                // lock native window buffer
                ANativeWindow_lock(nativeWindow, &window_buffer, 0);

                // 格式转换
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                          pFrame->linesize, 0, video_dec_ctx->height,
                          pFrameRGBA->data, pFrameRGBA->linesize);

                // 获取stride
                uint8_t * dst = (uint8_t*) (window_buffer.bits);
                int dstStride = window_buffer.stride * 4;
                uint8_t *src = (uint8_t*) (pFrameRGBA->data[0]);
                int srcStride = pFrameRGBA->linesize[0];

                // 由于window的stride和帧的stride不同,因此需要逐行复制
                int h;
                for (h = 0; h < videoHeight; h++) {
                    memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
                }

                ANativeWindow_unlockAndPost(nativeWindow);
            }

        }
        av_packet_unref(&pkt);
    }
    av_free(buffer);
    av_free(pFrameRGBA);

    av_frame_free(&pFrame);

    avcodec_close(video_dec_ctx);
    avcodec_close(audio_dex_ctx);

    avformat_close_input(&fmt_ctx);

    return 0;
}

