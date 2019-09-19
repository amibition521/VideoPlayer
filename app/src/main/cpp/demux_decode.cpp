//
// Created by 张再东 on 2019-09-19.
//


#include "native-lib.h"
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>


extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>


static const char *src_filename = NULL;
static const char *video_dst_filename = NULL;
static const char *audio_dst_filename = NULL;

static uint8_t *video_dst_data[4] = {NULL};
static int video_dst_linesize[4];
static int video_dst_bufsize;

static int video_stream_idx = -1, audio_stream_idx = -1;
static AVFrame *frame = NULL;
static AVPacket pkt;
static int video_frame_count = 0;
static int audio_frame_count = 0;

static int refcount = 0;
/*
static int decode_packet2(int *got_frame, int cached)
{
    int ret = 0;
    int decoded = pkt.size;
    *got_frame = 0;

    if (pkt.stream_index == video_stream_idx){
        ret = avcodec_send_packet(video_dec_ctx, &pkt);
        if (ret < 0){
            LOGE("Error decoding video frame (%s)\n", av_err2str(ret));
            return ret;
        }

        while (ret >= 0){
            ret = avcodec_receive_frame(video_dec_ctx, frame);
            if (ret < 0){
                LOGE("%s", "Error during decoding\n");
                return ret;
            }
        }

//        if (*got_frame){
//            if (frame->width != width || frame->height != height ||
//            frame->format != pix_fmt) {
//                LOGE("Error: Width, height and pixel format have to be "
//                                "constant in a rawvideo file, but the width, height or "
//                                "pixel format of the input video changed:\n"
//                                "old: width = %d, height = %d\n"
//                                "new: width = %d, height = %d\n",
//                        width, height,frame->width, frame->height);
//                return -1;
//            }
//            LOGE("video_frame%s n:%d coded_n:%d\n",
//                   cached ? "(cached)" : "",
//                   video_frame_count++, frame->coded_picture_number);
//        }
    } else if (pkt.stream_index == audio_stream_idx){
        ret = avcodec_send_packet(video_dec_ctx, &pkt);
        if (ret < 0){
            LOGE("Error decoding video frame (%s)\n", av_err2str(ret));
            return ret;
        }

        while (ret >= 0){
            ret = avcodec_receive_frame(video_dec_ctx, frame);
            if (ret < 0){
                LOGE("%s", "Error during decoding\n");
                return ret;
            }
        }
//        ret = avcodec_decode_audio4(audio_dex_ctx, frame, got_frame, &pkt);
//        if (ret < 0) {
//            LOGE("Error decoding audio frame (%s)\n", av_err2str(ret));
//            return ret;
//        }
//        decoded = FFMIN(ret, pkt.size);
//
//        if (*got_frame) {
//            size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)frame->format);
//            printf("audio_frame%s n:%d nb_samples:%d\n",
//                   cached ? "(cached)" : "",
//                   audio_frame_count++, frame->nb_samples);
//        }
    }
    if (*got_frame && refcount)
        av_frame_unref(frame);

    return 0;
}
*/
static int open_codec_context2(int *stream_idx, AVCodecContext **dec_ctx,
                               AVFormatContext *fmt_ctx, enum AVMediaType type) {
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;
    AVCodecParserContext *parser;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        LOGE("Could not find %s stream in input file.'%s'\n", av_get_media_type_string(type),
             src_filename);
        return ret;
    } else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            LOGE("Failed to find %s codec \n", av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }
        parser = av_parser_init(dec->id);
        if (!parser) {
            LOGE("%s", "parser not found.");
            return 0;
        }

        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            LOGE("Failed to allocate the %s codec context\n", av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar);
        if (ret < 0) {
            LOGE("Failed to copy %s codec parameters to decoder context\n",
                 av_get_media_type_string(type));
            return ret;
        }

        av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
        ret = avcodec_open2(*dec_ctx, dec, &opts);
        if (ret < 0) {
            LOGE("Failed to open %s codec \n", av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }
    return 0;
}

static int play(JNIEnv *env, jobject surface, const char *src) {
    int ret = 0;
    src_filename = src;
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *video_dec_ctx = NULL, *audio_dex_ctx = NULL;
    int width, height;
    enum AVPixelFormat pix_fmt;
    AVStream *video_stream = NULL, *audio_stream = NULL;
    LOGE("Could open source file %s.\n", src_filename);

    av_register_all();
    avformat_network_init();

    fmt_ctx = avformat_alloc_context();
    char input_filename[500] = {0};
    sprintf(input_filename, "%s", src);
    LOGE("22  Could open source file %s.\n", input_filename);
    ret = avformat_open_input(&fmt_ctx, input_filename, NULL, NULL);
    if (ret < 0) {
        LOGE("Could not open source file %s.\n", av_err2str(ret));
        return 0;
    }

    LOGD("33  Could open source file.\n");

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        LOGE("Could not find stream information.\n");
        return 0;
    }
    LOGD("444Could open source file.\n");

    if (open_codec_context2(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        video_stream = fmt_ctx->streams[video_stream_idx];
        width = video_dec_ctx->width;
        height = video_dec_ctx->height;
        pix_fmt = video_dec_ctx->pix_fmt;
        ret = av_image_alloc(video_dst_data, video_dst_linesize, width, height, pix_fmt, 1);
    }
    LOGD("555 Could open source file.\n");

    if (open_codec_context2(&audio_stream_idx, &audio_dex_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
        audio_stream = fmt_ctx->streams[audio_stream_idx];
    }
    LOGD("666 Could open source file.\n");

    if (!audio_stream && !video_stream) {
        LOGE("%s", "Could not find audio or video stream in the input, aborting.\n");
        ret = AVERROR(ENOMEM);
        return 0;
    }

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

//    do {
//        decode_packet2(&got_frame, 1);
//    } while (got_frame);

    LOGE("%s", "Demuxing succeeded.\n");

    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    LOGD("71 Could open source file.\n");

    int videoWidth = video_dec_ctx->width;
    int videoHeight = video_dec_ctx->height;

    ANativeWindow_setBuffersGeometry(nativeWindow, videoWidth, videoHeight,
                                     WINDOW_FORMAT_RGBA_8888);
    LOGD("7 Could open source file.\n");

    ANativeWindow_Buffer window_buffer;

    AVFrame *pFrame = av_frame_alloc();

    AVFrame *pFrameRGBA = av_frame_alloc();
    if (pFrameRGBA == NULL || pFrame == NULL) {
        LOGE("%s", "Could not allocate video frame.");
        return -1;
    }
    LOGD("77 Could open source file.\n");

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, video_dec_ctx->width,
                                            video_dec_ctx->height, 1);

    uint8_t *buffer = static_cast<uint8_t *>(av_malloc(numBytes * sizeof(uint8_t)));
    av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, buffer, AV_PIX_FMT_RGBA,
                         video_dec_ctx->width, video_dec_ctx->height, 1);

    struct SwsContext *sws_ctx = sws_getContext(video_dec_ctx->width, video_dec_ctx->height,
                                                video_dec_ctx->pix_fmt, video_dec_ctx->width,
                                                video_dec_ctx->height, AV_PIX_FMT_RGBA,
                                                SWS_BILINEAR, NULL, NULL, NULL);
    LOGD("777 Could open source file.\n");

    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        ret = avcodec_send_packet(video_dec_ctx, &pkt);
        if (ret < 0) {
            LOGE("Error decoding video frame (%s)\n", av_err2str(ret));
            return ret;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(video_dec_ctx, pFrame);
            if (ret < 0) {
                LOGE("%s", "Error during decoding\n");
                return ret;
            }
//            avcodec_decode_video2(video_dec_ctx, pFrame, &frameFinished, &pkt);
            ANativeWindow_lock(nativeWindow, &window_buffer, 0);

            sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data,
                      pFrame->linesize, 0, video_dec_ctx->height, pFrameRGBA->data,
                      pFrameRGBA->linesize);

            uint8_t *dst = (uint8_t *) window_buffer.bits;
            int dstStride = window_buffer.stride * 4;
            uint8_t *stc = pFrameRGBA->data[0];
            int srcStride = pFrameRGBA->linesize[0];

            int h;
            for (h = 0; h < videoHeight; h++) {
                memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
            }
            ANativeWindow_unlockAndPost(nativeWindow);
            av_packet_unref(&pkt);
        }
    }

    av_free(buffer);
    av_free(pFrameRGBA);

    av_frame_free(&pFrame);

    avcodec_close(video_dec_ctx);
    avcodec_close(audio_dex_ctx);

    avformat_close_input(&fmt_ctx);

    return 0;
}


}



