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

AVFormatContext *aFormatCtx = NULL;
AVCodecContext *aCodecCtx = NULL;
AVCodec *pCodec = NULL;
AVPacket *packet;
int audioStream_idx;
AVFrame *aFrame;
SwrContext *swr;
int out_channer_nb;

int rate;
int channel;


int initFFmpeg(int *rate, int *channel, char *url) {
    av_register_all();
    avformat_network_init();

    aFormatCtx = avformat_alloc_context();

    LOGE(">> ffmpeg get url=%s", url);

    char *file_name = url;
    int ret;
    //Step 1
    if ((ret = avformat_open_input(&aFormatCtx, file_name, NULL, NULL)) < 0) {
        LOGE(">> Couldn't open file:%s\n", av_err2str(ret));
        return -1;
    }
    //step 2
    if (avformat_find_stream_info(aFormatCtx, NULL) < 0) {
        LOGE(">> Could not find stream information.\n");
        return 0;
    }


    int64_t duration = aFormatCtx->duration / AV_TIME_BASE;
    LOGD("file duration:%lld\n", duration);

    // step 3 Find the first video stream
    audioStream_idx = av_find_best_stream(aFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    // 老方式
//    int i;
//    for (i = 0; i < aFormatCtx->nb_streams;i++){
//        if (aFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audioStream_idx < 0){
//            audioStream_idx = i;
//            break;
//        }
//    }

    if (audioStream_idx < 0) {
        LOGE(" >> Could not find %s stream in input file.\n",
             av_get_media_type_string(AVMEDIA_TYPE_AUDIO));
        return ret;
    }

    //step 4 AVStream AVCodec
    AVStream *audioStream = aFormatCtx->streams[audioStream_idx];
    if (!audioStream) {
        LOGD(">> audio stream error\n");
        return -1;
    }

    int codec_id = aFormatCtx->streams[audioStream_idx]->codecpar->codec_id;
    LOGE(">> Could open audio index:%d, codecid:%d.\n", audioStream_idx, codec_id);

    pCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    if (!pCodec) {
        LOGD(">> unsupported codec.\n");
        return -1;
    }
    //step 5 AVCodecContext
    aCodecCtx = avcodec_alloc_context3(pCodec);
    ret = avcodec_parameters_to_context(aCodecCtx, audioStream->codecpar);
    if (ret < 0) {
        avcodec_free_context(&aCodecCtx);
        return -1;
    }
    if (avcodec_open2(aCodecCtx, pCodec, NULL) < 0) {
        LOGD(">> Could not open codec");
        return -1;
    }
    LOGD(">> decode audio success");

    //step 6 init AVPacket AVFrame
    aFrame = av_frame_alloc();
    packet = av_packet_alloc();
    packet->size = 0;
    packet->data = NULL;
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
//    输出采样位数  16位
    enum AVSampleFormat out_formart = AV_SAMPLE_FMT_S16;
//输出的采样率必须与输入相同
    int out_sample_rate = aCodecCtx->sample_rate;
    // 音频处理
    swr = swr_alloc();
    swr_alloc_set_opts(swr, out_ch_layout, out_formart,
                       out_sample_rate, aCodecCtx->channel_layout,
                       aCodecCtx->sample_fmt, aCodecCtx->sample_rate, 0, NULL);
    swr_init(swr);

    outputBuffer = (uint8_t *) av_malloc(44100 * 2);
    out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    // 获取采样率和通道数
    *rate = aCodecCtx->sample_rate;
    *channel = aCodecCtx->channels;
    LOGE(">> audio channel_nb:%d, sample_rate:%d, channles:%d", out_channer_nb, aCodecCtx->sample_rate,
         aCodecCtx->channels);
    return 0;
}

/*
int getPCM(void **pcm, size_t *pcmSize) {
    LOGD("getPcm");
    int ret;
    while (av_read_frame(aFormatCtx, &packet) >= 0) {
        int frameFinished = 1;
        if (packet.stream_index == audioStream_idx) {
            //TODO:
//            ret = avcodec_send_packet(aCodecCtx, &packet);
//            if (ret < 0) {
//                LOGD("avcodec_send_packet error");
//                break;
//            }
//            while (ret >= 0) {
//                ret = avcodec_receive_frame(aCodecCtx, aFrame);
//            }
            avcodec_decode_audio4(aCodecCtx, aFrame, &frameFinished, &packet);
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
*/
void getPCM(void **pcm, size_t *pcm_size) {

    unsigned int index = 0;
    //循环读取数据帧
    while (av_read_frame(aFormatCtx, packet) == 0) {
        //如果是视频数据包
        if (packet->stream_index == audioStream_idx) {
            int pkt_ret = avcodec_send_packet(aCodecCtx, packet);
            if (pkt_ret != 0) {
                av_packet_unref(packet);
            }

            if (avcodec_receive_frame(aCodecCtx, aFrame) == 0) {
                LOGE("读取音频:%d", index++);

                LOGE("音频声道数:%d 采样率：%d 数据格式: %d", aFrame->channels, aFrame->sample_rate,
                     aFrame->format);
                //                缓冲区的大小
                int data_size = av_samples_get_buffer_size(aFrame->linesize, out_channer_nb,
                                                      aFrame->nb_samples,
                                                      AV_SAMPLE_FMT_S16, 1);

//                if (data_size > outputBufferSize) {
//                    outputBufferSize = data_size;
//                    outputBuffer = (uint8_t *) av_realloc(outputBuffer,
//                                                          sizeof(uint8_t) * outputBufferSize);
//                }
                swr_convert(swr, &outputBuffer, aFrame->nb_samples, (const uint8_t **) aFrame->data,
                            aFrame->nb_samples);

                *pcm = outputBuffer;
                *pcm_size = data_size;
                break;
            }
        }
    }
}

int releaseFFmpeg() {
    av_packet_unref(packet);
    av_free(outputBuffer);
    av_frame_unref(aFrame);
    avcodec_close(aCodecCtx);
    avformat_close_input(&aFormatCtx);
    avformat_free_context(aFormatCtx);
    return 0;
}


