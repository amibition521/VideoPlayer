//
// Created by 再东 on 2019-09-22.
//

#include "OpenSL_ES_Core.h"
#include "FFmpegCore.h"
#include "stdio.h"
#include <jni.h>
#include "log.h"
#include <string.h>
#include <assert.h>

extern "C" {
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

//engine interface
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

//output mix interface
static SLObjectItf outputMixObject = NULL;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

//buffer queue player interfaces
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLEffectSendItf bqPlayerEffectSend;
static SLMuteSoloItf bqPlayerMuteSolo;
static SLVolumeItf bqPlayerVolume;
static SLmilliHertz bqPlayerSampleRate = 0;
uint8_t *out_buffer;

// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

// pointer and size of the next player buffer to enqueue, and number of remaining buffers
static void *buffer;
static unsigned int bufferSize;

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    LOGD(">> buffere queue callback");
//    assert(bq == bqPlayerBufferQueue);
//    bufferSize = 0;
    getPCM(&buffer, &bufferSize);

    if (buffer != NULL && bufferSize != 0) {
        SLresult result;
        result = (*bq)->Enqueue(bq, buffer, bufferSize);
        LOGE("Enqueue:%d", bufferSize);
//        assert(SL_RESULT_SUCCESS == result);
//        (void) result;

        if (result != SL_RESULT_SUCCESS){
            return;
        }
    }
}

void initOpenSLES() {
    LOGD(">> initOpenSLES....");
    SLresult result;

    //1. create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    LOGD(">> initOpenSLES... step 1, result =%d", result);

    //2. realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    LOGD(">> initOpenSLES... step 2, result=%d", result);

    //3. get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    LOGD(">> initOpenSLES... step3, result = %d", result);
    //4. create output mix, with environmental reverb specialed as a non-requeired interface
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    LOGD(">> initOpenSLES... step4, result = %d", result);

    //5. realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    LOGD(">> initOpenSLES... step5 result = %d", result);


    // 6、get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    LOGD(">> initOpenSLES... step6 result = %d", result);

    result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
            outputMixEnvironmentalReverb, &reverbSettings);
    LOGD(">> initOpenSLES... step7 result =%d", result);
}

// create buffer queue audio player
void initBufferQueue(int rate, int channel, int bitsPerSample) {
    LOGD(">> init Buffer Queue");
    SLresult result;

    //config audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    // 方式不一样
//    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,channel, rate * 1000,
//                                   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
//                                   SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN};

    SLDataFormat_PCM format_pcm;
    format_pcm.formatType = SL_DATAFORMAT_PCM;
    format_pcm.numChannels = channel;
    format_pcm.samplesPerSec = rate * 1000;
//    format_pcm.samplesPerSec = SL_SAMPLINGRATE_44_1;
    format_pcm.bitsPerSample = bitsPerSample;
    format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    if (channel == 2) {
        format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    } else {
        format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
    }
    format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
    SLDataSource audioSource = {&loc_bufq, &format_pcm};

    //configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSink = {&loc_outmix, NULL};

    //create audio palyer
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSource,
                                                &audioSink, 1, ids, req);
//    assert(SL_RESULT_SUCCESS == result);
//    (void) result;

    //realize the player
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
//    assert(SL_RESULT_SUCCESS == result);
//    (void) result;

    //get the play interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
//    assert(SL_RESULT_SUCCESS == result);
//    (void) result;

    //get the buffer queue interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                             &bqPlayerBufferQueue);
//    assert(SL_RESULT_SUCCESS == result);
//    (void) result;

    //register callback on the buffer queue
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
//    assert(SL_RESULT_SUCCESS == result);
//    (void) result;

    //get the effect send interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
                                             &bqPlayerEffectSend);
//    assert(SL_RESULT_SUCCESS == result);
//    (void) result;

    //get the volume interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
//    assert(SL_RESULT_SUCCESS == result);
//    (void) result;

    //set the player's state to playing
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
//    assert(SL_RESULT_SUCCESS == result);
//    (void) result;
}

}

void stop() {
    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (bqPlayerObject != NULL) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
        bqPlayerEffectSend = NULL;
        bqPlayerMuteSolo = NULL;
        bqPlayerVolume = NULL;
    }

    // destroy output mix object, and invalidate all associated interfaces
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }

    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

    releaseFFmpeg();
}

void play(char *url) {
    int rate, channel;
    LOGD(">> get Url =%s", url);
    initFFmpeg(&rate, &channel, url);

    initOpenSLES();

    LOGD(">> rate=%d, channel:%d", rate, channel);
    initBufferQueue(rate, channel, SL_PCMSAMPLEFORMAT_FIXED_16);

    bqPlayerCallback(bqPlayerBufferQueue, NULL);

}