/*
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * Qualcomm Technologies, Inc. ("Proprietary Information").  You shall
 * not disclose such Proprietary Information, and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with Qualcomm Technologies, Inc.
 *
 * FILE:QiOSAudioHandler.h
 *
 * DESCRIPTION:
 */
#ifndef __iOSVocoder__QiOSAudioHandler__
#define __iOSVocoder__QiOSAudioHandler__

#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#include "VocDataTypeDef.h"


#define kOutputBus 0
#define kInputBus 1
#define AUDIO_DURATION .064f//[.002f .004f .008f .016f .032f .064f]
#define AUDIO_BYTES_PER_FRAME 2
typedef enum
{
    AUDIO_SAMPLE_RATE_TYPE_8 = 8000,
    AUDIO_SAMPLE_RATE_TYPE_22 = 22000,
    AUDIO_SAMPLE_RATE_TYPE_44 = 44100
} AUDIO_SAMPLE_RATE_TYPE;

typedef struct
{
    int rate;
} pcm_frame_info_type;

enum frames_types
{
    FRAME_TYPE_PCM,
    FRAME_TYPE_MAX
};

typedef struct
{
    pcm_frame_info_type pcm_rate;
    int length;
} frame_info_type;

typedef  void(*acquiredCB)     (UInt8 *data, frame_info_type *frame_info, UInt16 len, int *status);
typedef void (*playoutFrameCB) (UInt8 *data, frame_info_type *frame_info, int *status);


class AudioHandler
{
public:
    static AudioHandler* GetInstance(void);
    static OSStatus recordingCallback(void *inRefCon,
                                      AudioUnitRenderActionFlags *ioActionFlags,
                                      const AudioTimeStamp *inTimeStamp,
                                      UInt32 inBusNumber,
                                      UInt32 inNumberFrames,
                                      AudioBufferList *ioData);
    static OSStatus playbackCallback(void *inRefCon,
                                     AudioUnitRenderActionFlags *ioActionFlags,
                                     const AudioTimeStamp *inTimeStamp,
                                     UInt32 inBusNumber,
                                     UInt32 inNumberFrames,
                                     AudioBufferList *ioData);
    UInt32 getBufferSize();
    void setCBFunctions(void* acquiredCBFunc, void* playoutFrameCBFunc);
    void InitializeiOSAudioHandler();
    int initAudioSession();
    int configureAudioSession();
    int initAudioStreams();
    int startAudioSession();
    int stopAudioSession();
    int startAudioUnit();
    int stopProcessingAudio();
    int stopProcessingAudioAsync();
    Boolean GetAudioStatus();
    void setRecordingState(Boolean isRecording);
    void setPlayingState(Boolean isPlaying);
    
private:
    AudioHandler();
    ~AudioHandler();
    void allocatePlayBuffer();
    void memsetPlayBuffer();
    void deAllocatePlayBuffer();
    UInt32 getNoOfFrames();
    
    Boolean mAudioAvailable;
    Boolean mAudioUnitStarted;
    Boolean mStreamInit;
    Boolean mSessionInit;
    acquiredCB     m_acquiredCBFunc;
    playoutFrameCB m_playoutFrameCBFunc;
    
    int m_mvs_pkt_status;
    frame_info_type m_typePCM;
    
    UInt8 *mToPlayBuffer;
    static AudioHandler* m_pMyInstance;
    Boolean m_IsRecording;
    Boolean m_IsPlaying;
    Boolean m_ResumeAudioOutputUnitAfterInterruption;
};

#endif /* defined(__iOSVocoder__QiOSAudioHandler__) */
