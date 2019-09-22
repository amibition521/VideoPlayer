//
// Created by 再东 on 2019-09-22.
//


#include <stdint.h>
#include "FFmpegCore.h"
#include "log.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "libswresample/swresample.h"

}
uint8_t *outputBuffer;
size_t outputBufferSize;

AVPacket packet;
int audioStream_idx;
AVFrame *aFrame;
SwrContext *swr;
AVFormatContext *aFormatCtx;
AVCodecContext *aCodecCtx;

int initFFmpeg(int *rate, int *channel, char *url) {
    av_register_all();
    avformat_network_init();

    aFormatCtx = avformat_alloc_context();

    LOGE("ffmpeg get url=%s", url);

    char *file_name = url;
    int ret;
    if ((ret = avformat_open_input(&aFormatCtx, file_name, NULL, NULL)) < 0) {
        LOGE("Couldn't open file:%s\n", av_err2str(ret));
        return -1;
    }
    LOGE("ffmpeg get2  url=%s", url);

//    ret = avformat_find_stream_info(aFormatCtx, NULL);
//    int video_stream_idx = av_find_best_stream(aFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (avformat_find_stream_info(aFormatCtx, NULL) < 0) {
        LOGE("Could not find stream information.\n");
        return 0;
    }
    LOGE("Could not open codec:%d, codecid:.\n",audioStream_idx);
    int i;
    ret = av_find_best_stream(aFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
//    for (i = 0; i < aFormatCtx->nb_streams;i++){
//        if (aFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audioStream < 0){
//            audioStream_idx = i;
//            break;
//        }
//    }
    LOGE("Could find %s stream in input file.\n",
         av_get_media_type_string(AVMEDIA_TYPE_AUDIO));

    if (ret < 0) {
        LOGE("Could not find %s stream in input file.\n",
             av_get_media_type_string(AVMEDIA_TYPE_AUDIO));
        return ret;
    }
    aFrame = av_frame_alloc();

    audioStream_idx = ret;

    AVStream *audioStream = aFormatCtx->streams[audioStream_idx];
    if (!audioStream){
        LOGD("audio stream error\n");
        return -1;
    }
    LOGD("audio stream error\n");

    int codec_id = aFormatCtx->streams[audioStream_idx]->codecpar->codec_id;
    LOGE("Could not open codec:%d, codecid:%d.\n",audioStream_idx, codec_id);

    AVCodec *avCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    if (!avCodec) {
        LOGD("unsupported codec.\n");
        return -1;
    }
    LOGD("2   unsupported codec.\n");

    aCodecCtx = avcodec_alloc_context3(avCodec);

    ret = avcodec_parameters_to_context(aCodecCtx, audioStream->codecpar);
    if (ret < 0) {
        avcodec_free_context(&aCodecCtx);
        return -1;
    }

    if (avcodec_open2(aCodecCtx, avCodec, NULL) < 0) {
        LOGD("Could not open codec");
        return -1;
    }
    LOGD("Could not open codec");


    swr = swr_alloc();
//    swr = swr_alloc_set_opts(NULL, aCodecCtx->channel_layout, aCodecCtx->sample_fmt,
//                             aCodecCtx->sample_rate, aCodecCtx->channel_layout,
//                             aCodecCtx->sample_fmt, aCodecCtx->sample_rate, 0, NULL);

    outputBufferSize = 8196;
    outputBuffer = (uint8_t *) av_malloc(sizeof(uint8_t) * outputBufferSize);

    *rate = aCodecCtx->sample_rate;
    *channel = aCodecCtx->channels;
    return 0;
}

int getPCM(void **pcm, size_t *pcmSize) {
    int ret;
    while (av_read_frame(aFormatCtx, &packet) >= 0) {
        int frameFinished = 1;
        if (packet.stream_index == audioStream_idx) {
            //TODO:
            ret = avcodec_send_packet(aCodecCtx, &packet);
            if (ret < 0) {
                LOGD("avcodec_send_packet error");
                break;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(aCodecCtx, aFrame);
            }
//            avcodec_decode_audio4(aCodecCtx, aFrame, &frameFinished, &packet);
            if (frameFinished) {
                int data_size = av_samples_get_buffer_size(aFrame->linesize, aFrame->channels,
                                                           aFrame->nb_samples,
                                                           aCodecCtx->sample_fmt, 1);
                LOGE(">> getPcm data_size=%d", data_size);

                if (data_size > outputBufferSize) {
                    outputBufferSize = data_size;
                    outputBuffer = (uint8_t *) av_realloc(outputBuffer,
                                                          sizeof(uint8_t) * outputBufferSize);

                }

                swr_convert(swr, &outputBuffer, aFrame->nb_samples,
                            (uint8_t const **) aFrame->extended_data, aFrame->nb_samples);

                *pcm = outputBuffer;
                *pcmSize = data_size;
                return 0;
            }
        }
    }
    return -1;
}

int releaseFFmpeg() {
    av_packet_unref(&packet);
    av_free(outputBuffer);
    av_frame_unref(aFrame);
    avcodec_close(aCodecCtx);
    avformat_close_input(&aFormatCtx);
}


