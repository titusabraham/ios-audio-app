/*
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * Qualcomm Technologies, Inc. ("Proprietary Information").  You shall
 * not disclose such Proprietary Information, and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with Qualcomm Technologies, Inc.
 *
 * FILE:QiOSAudioHandler.cpp
 *
 * DESCRIPTION:
 */
#include "QiOSAudioHandler.h"
#include "VocCommon.h"
#import <CoreTelephony/CTCallCenter.h>
#import <CoreTelephony/CTCall.h>
#import <UIKit/UIDevice.h>

AudioUnit *audioUnit = NULL;
AudioHandler* AudioHandler::m_pMyInstance = NULL;

AudioHandler* AudioHandler::GetInstance(void)
{
    if(!m_pMyInstance)
        m_pMyInstance = (AudioHandler*)new AudioHandler;
    
    return m_pMyInstance;
}

AudioHandler::AudioHandler()
{
    printf("[Media-AH]: AudioHandler\n");
    m_typePCM.pcm_rate.rate = FRAME_TYPE_PCM;
    m_typePCM.length = 0;
    mAudioAvailable = FALSE;
    mStreamInit = FALSE;
    mSessionInit = FALSE;
    mAudioUnitStarted = FALSE;
    mIsSilentModeOn = FALSE;
    mNeedToRevertAudioSessionCategoryForSilentMode = FALSE;
    mToPlayBuffer = NULL;
    m_IsRecording = FALSE;
    m_IsPlaying = FALSE;
    m_ResumeAudioOutputUnitAfterInterruption = FALSE;
    allocatePlayBuffer();
}

AudioHandler::~AudioHandler()
{
    printf("[Media-AH]: ~AudioHandler\n");
    stopAudioSession();
    deAllocatePlayBuffer();
    *audioUnit = NULL;
    delete m_pMyInstance;
}

Boolean AudioHandler::GetAudioStatus()
{
    printf("[Media-AH]: T-AS GetAudioStatus %d\n",mAudioAvailable);
    if (mAudioAvailable == FALSE)
    {
        //try initializing audio unit again
        startAudioSession();
    }
    return mAudioAvailable;
}

void AudioHandler::mute()
{
    
}

UInt32 AudioHandler::getBufferSize()
{
    int bufferSize = getNoOfFrames() * AUDIO_BYTES_PER_FRAME;
#ifdef ENABLE_DEBUG_LOGGING
    printf("[Media-AH]: getBufferSize = %d\n",bufferSize);
#endif
    return bufferSize;
}

UInt32 AudioHandler::getNoOfFrames()
{
    int milliSecs= 1000 * AUDIO_DURATION;
    int noOfFrame = milliSecs * 8;
#ifdef ENABLE_DEBUG_LOGGING
    printf("[Media-AH]: getNoOfFrames = %d\n",noOfFrame);
#endif
    return noOfFrame;
}

Float32 AudioHandler::getAudioDuration()
{
#ifdef ENABLE_DEBUG_LOGGING
    printf("[Media-AH]: getAudioDuration\n");
#endif
    Float32 bufferDuration = 0;
    UInt32 propSize = sizeof(Float32);
    AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareIOBufferDuration,
                            &propSize,
                            &bufferDuration);
#ifdef ENABLE_DEBUG_LOGGING
    printf("[Media-AH]: getAudioDuration is %f\n",bufferDuration);
#endif
    return bufferDuration;
}

void AudioHandler::allocatePlayBuffer()
{
    deAllocatePlayBuffer();
    mToPlayBuffer = (UInt8*)malloc(sizeof(UInt8) * getBufferSize());
    memsetPlayBuffer();
}

void AudioHandler::memsetPlayBuffer()
{
    if(mToPlayBuffer)
    {
        memset(mToPlayBuffer, 0, sizeof(UInt8) * getBufferSize());
    }
    m_typePCM.length = 0;
}

void AudioHandler::deAllocatePlayBuffer()
{
    if(mToPlayBuffer)
    {
        free(mToPlayBuffer);
        mToPlayBuffer = NULL;
    }
    m_typePCM.length = 0;
}

void AudioHandler::InitializeiOSAudioHandler()
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        if(VOC_RET_SUCCESS != initAudioSession())
        {
            printf("[Media-VOC]: **************Failed to Init Audio Session**************\n");
        }
        if(VOC_RET_SUCCESS != initAudioStreams())
        {
            printf("[Media_VOC]: **************Failed to Init Audio Streams***************\n");
        }
    });
}

int AudioHandler::initAudioSession()
{
    printf("[Media-AH]: initAudioSession\n");
    if(mSessionInit)
    {
        printf("[Media-AH]: Session Already Inited\n");
        return 0;
    }
    audioUnit = (AudioUnit*)malloc(sizeof(AudioUnit));
    
    OSStatus retVal = AudioSessionInitialize(NULL, NULL, AudioHandler::AudioSessionInterruptionListener, this);
    if (retVal != kAudioSessionNoError)
    {
        printf("[Media-AH]: AudioSessionInitialize failed\n");
        if (retVal == kAudioSessionAlreadyInitialized) {
            printf("[Media-AH]: Audio session already initialized");
        }
        return 1;
    }
    else{
        printf("[Media-AH]: AudioSessionInitialize successful\n");
    }
    
    UInt32 sessionCategory = kAudioSessionCategory_PlayAndRecord;
    if(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory,
                               sizeof(UInt32), &sessionCategory) != noErr) {
        printf("[Media-AH]: AudioSessionSetProperty audioCategory failed\n");
        return 1;
    }
    
    Float32 bufferSizeInSec = AUDIO_DURATION;
    if(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration,
                               sizeof(Float32), &bufferSizeInSec) != noErr) {
        printf("[Media-AH]: AudioSessionSetProperty pref h/w i/o duration failed\n");
        return 1;
    }
    
    Float64 preferredHardwareSampleRate = AUDIO_SAMPLE_RATE_TYPE_8;
    if(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareSampleRate,
                               sizeof(Float64), &preferredHardwareSampleRate) != noErr) {
        printf("[Media-AH]: AudioSessionSetProperty pref h/w Sample rate failed\n");
        return 1;
    }

#if 0
    UInt32 overrideCategory = 1;
    if(AudioSessionSetProperty(kAudioSessionProperty_OverrideCategoryDefaultToSpeaker,
                               sizeof(UInt32), &overrideCategory) != noErr) {
        printf("[Media-AH]: AudioSessionSetProperty Set To Speaker failed\n");
        return 1;
    }
#endif

    //To optimize device's tonal equalization for voice
    UInt32 mode = kAudioSessionMode_VoiceChat;
    OSStatus error = AudioSessionSetProperty(kAudioSessionProperty_Mode, sizeof(mode), &mode);
    if (error != noErr)
    {
        printf("[Media-AH] Failed to set VoiceChat audio session mode!\n");
    }
    
    Float32 Volume = 0;
    UInt32 propSize = sizeof(Float32);
    AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareOutputVolume,
                            &propSize,
                            &Volume);
    printf("[Media-AH]: CurrentVolume is %f\n",Volume);
    
    UInt32 doSetProperty = 1;
    AudioSessionSetProperty (kAudioSessionProperty_OverrideCategoryMixWithOthers,sizeof (doSetProperty),&doSetProperty);
    
    UInt32 allowBluetoothInput = 1;
    OSStatus BTStat = AudioSessionSetProperty (
                                               kAudioSessionProperty_OverrideCategoryEnableBluetoothInput,
                                               sizeof(allowBluetoothInput),
                                               &allowBluetoothInput
                                              );
    if (BTStat != noErr) {
        printf("[Media-AH] MEDIA_ERROR Failed to enable Bluetooth Input\n");
    }
    

    // There are many properties you might want to provide callback functions for:
    // kAudioSessionProperty_AudioRouteChange
    // kAudioSessionProperty_OverrideCategoryEnableBluetoothInput
    // etc.
    mSessionInit = TRUE;
    return 0;
}

int AudioHandler::initAudioStreams()
{
    printf("[Media-AH]: initAudioStreams\n");
    if(mStreamInit)
    {
        printf("[Media-AH]: Streams Already Inited\n");
        return 0;
    }
    AudioComponentDescription componentDescription;
    componentDescription.componentType = kAudioUnitType_Output;
    componentDescription.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
    componentDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
    componentDescription.componentFlags = 0;
    componentDescription.componentFlagsMask = 0;
    AudioComponent component = AudioComponentFindNext(NULL, &componentDescription);
    if(AudioComponentInstanceNew(component, audioUnit) != noErr) {
        printf("[Media-AH]: initAudioStreams Failed in instantiating New Component\n");
        return 1;
    }
    
    UInt32 enable = 1;
    if(AudioUnitSetProperty(*audioUnit,
                            kAudioOutputUnitProperty_EnableIO,
                            kAudioUnitScope_Input,
                            kInputBus,
                            &enable,
                            sizeof(UInt32)) != noErr) {
        printf("[Media-AH]: initAudioStreams Failed to Set Property for enabling IO\n");
        return 1;
    }

    // Set input callback
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = recordingCallback;
    callbackStruct.inputProcRefCon = this;
    if(AudioUnitSetProperty(*audioUnit,
                                  kAudioOutputUnitProperty_SetInputCallback,
                                  kAudioUnitScope_Global,
                                  kInputBus,
                                  &callbackStruct,
                                  sizeof(callbackStruct))!= noErr)
    {
        printf("[Media-AH]: initAudioStreams Failed to Set Call Back Function for kInputBus\n");
        return 1;
    }
    
    // Set output callback
    callbackStruct.inputProc = playbackCallback;
    callbackStruct.inputProcRefCon = this;
    if(AudioUnitSetProperty(*audioUnit,
                                  kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Global,
                                  kOutputBus,
                                  &callbackStruct,
                                  sizeof(callbackStruct))!= noErr)
    {
        printf("[Media-AH]: initAudioStreams Failed to Set Call Back Function for kInputBus\n");
        return 1;
    }
    
    UInt32 enableBufferAllocflag = 1;
    if(AudioUnitSetProperty(*audioUnit,
                            kAudioUnitProperty_ShouldAllocateBuffer,
                            kAudioUnitScope_Output,
                            kInputBus,
                            &enableBufferAllocflag,
                            sizeof(enableBufferAllocflag)) != noErr) {
        printf("[Media-AH]: initAudioStreams Failed to set enableBufferAllocflag \n");
        return 1;
    }
    
    AudioStreamBasicDescription streamDescription;
    // You might want to replace this with a different value, but keep in mind that the
    // iPhone does not support all sample rates. 8kHz, 22kHz, and 44.1kHz should all work.
    streamDescription.mSampleRate = AUDIO_SAMPLE_RATE_TYPE_8;
    // Yes, I know you probably want floating point samples, but the iPhone isn't going
    // to give you floating point data. You'll need to make the conversion by hand from
    // linear PCM <-> float.
    streamDescription.mFormatID = kAudioFormatLinearPCM;
    // This part is important!
    streamDescription.mFormatFlags = kAudioFormatFlagsCanonical;
    // Not sure if the iPhone supports recording >16-bit audio, but I doubt it.
    streamDescription.mBitsPerChannel = 16;
    // 1 sample per frame, will always be 2 as long as 16-bit samples are being used
    streamDescription.mBytesPerFrame = AUDIO_BYTES_PER_FRAME;
    // Record in mono. Use 2 for stereo, though I don't think the iPhone does true stereo recording
    streamDescription.mChannelsPerFrame = 1;
    streamDescription.mBytesPerPacket = streamDescription.mBytesPerFrame *
    streamDescription.mChannelsPerFrame;
    // Always should be set to 1
    streamDescription.mFramesPerPacket = 1;
    // Always set to 0, just to be sure
    streamDescription.mReserved = 0;
    
    // Set up input stream with above properties
    if(AudioUnitSetProperty(*audioUnit,
                            kAudioUnitProperty_StreamFormat,
                            kAudioUnitScope_Input,
                            kOutputBus,
                            &streamDescription,
                            sizeof(streamDescription)) != noErr) {
        printf("[Media-AH]: initAudioStreams Failed to Set Stream Format for input\n");
        return 1;
    }
    
    // Ditto for the output stream, which we will be sending the processed audio to
    if(AudioUnitSetProperty(*audioUnit,
                            kAudioUnitProperty_StreamFormat,
                            kAudioUnitScope_Output,
                            kInputBus,
                            &streamDescription,
                            sizeof(streamDescription)) != noErr) {
        printf("[Media-AH]: initAudioStreams Failed to Set Stream Format for output\n");
        return 1;
    }
    
    //set the flag
    startAudioSession();
    mStreamInit = TRUE;
    return 0;
}

int AudioHandler::startAudioSession()
{
    printf("[Media-AH]: T-AS startAudioSession\n");
    if(mAudioAvailable)
    {
        printf("[Media-AH]: T-AS AudioSession is already Active\n");
        return 0;
    }
    printf("[Media-AH]: T-AS startAudioSession is now being Activated\n");
    if(AudioSessionSetActive(true) != noErr) {
        mAudioAvailable = false;
        printf("[Media-AH]: T-AS AudioSessionSetActive true failed\n");
        return 1;
    }
    
    if(AudioUnitInitialize(*audioUnit) != noErr) {
        mAudioAvailable = false;
        printf("[Media-AH]: T-AS AudioUnitInitialize failed\n");
        return 1;
    }
    //set the flag
    mAudioAvailable = TRUE;
    return 0;
}

int AudioHandler::stopAudioSession()
{
    printf("[Media-AH]: T-AS stopAudioSession\n");
    if(!mAudioAvailable)
    {
        printf("[Media-AH]: T-AS AudioSession is already DeActive\n");
        return 0;
    }
    if(AudioSessionSetActive(false) != noErr) {
        printf("[Media-AH]: T-AS AudioSessionSetActive false failed\n");
    }
    
    if(AudioUnitUninitialize(*audioUnit) != noErr) {
        printf("[Media-AH]: T-AS AudioUnitUninitialize false failed\n");
    }
    return 0;
}

int AudioHandler::startAudioUnit()
{
    printf("[Media-AH]: T-AS startAudioUnit mAudioUnitStarted=%d\n",mAudioUnitStarted);
    memsetPlayBuffer();
    if((mAudioUnitStarted == FALSE)&&(AudioOutputUnitStart(*audioUnit) != noErr))
    {
        printf("[Media-AH]: MEDIA_ERROR AudioOutputUnitStart failed\n");
        return 1;
    }

    //To prevent Siri from getting launched during a call when user brings the device close to the ear
    [UIDevice currentDevice].proximityMonitoringEnabled = YES;
    printf("[Media-AH]: Enable proximityMonitoringEnabled, disable it when call ends\n");

    if (mAudioUnitStarted == FALSE)
    {

        UInt32 otherMixableAudioShouldDuck = 1;
        UInt32 propSize = sizeof(UInt32);
        OSStatus error = AudioSessionSetProperty(kAudioSessionProperty_OtherMixableAudioShouldDuck, propSize, &otherMixableAudioShouldDuck);
        if(error != noErr)
        {
            printf("[Media-AH] Failed to enable otherMixableAudioShouldDuck\n");
        }
        else
        {
            printf("[Media-AH] Enabled otherMixableAudioShouldDuck, play other audio players at ducked volume now\n");
        }
        printf("[Media-AH]: T-AS Audio Sesssion being forcefully started \n");
        AudioSessionSetActive(true);
    }
    mAudioUnitStarted = TRUE;
    return 0;
}

int AudioHandler::stopProcessingAudio()
{
    printf("[Media-AH]: AudioHandler::stopAudioUnit()\n");
    
    [UIDevice currentDevice].proximityMonitoringEnabled = NO;
    printf("[Media-AH]: Disable proximityMonitoringEnabled\n");
    
    memsetPlayBuffer();
    if (mAudioUnitStarted == TRUE) {
        if(AudioOutputUnitStop(*audioUnit) != noErr) {
            printf("[Media-AH]: AudioOutputUnitStop failed\n");
            return 1;
        }
    }
    mAudioUnitStarted = FALSE;
    stopAudioSession();
    //once the call ends, if silent mode is enabled and client was a sender,
    //then now is the time to switch back to Ambient category
    //for possible incoming calls in silent mode
    if(mIsSilentModeOn && mNeedToRevertAudioSessionCategoryForSilentMode)
    {
        printf("[Media-AH]: Call end for sender in silent mode, revert to Ambient category\n");
        mNeedToRevertAudioSessionCategoryForSilentMode = FALSE;
        setAudioSessionCategory(kAudioSessionCategory_AmbientSound);
    }
    return 0;
}

void AudioHandler::setCBFunctions(void* acquiredCBFunc, void* playoutFrameCBFunc)
{
    printf("[Media-AH]: setCBFunctions\n");
    m_acquiredCBFunc = (acquiredCB)acquiredCBFunc;
    m_playoutFrameCBFunc = (playoutFrameCB)playoutFrameCBFunc;
}

void AudioHandler::setRecordingState(Boolean isRecording)
{
    m_IsRecording = isRecording;
}

void AudioHandler::setPlayingState(Boolean isPlaying)
{
    m_IsPlaying = isPlaying;
}

OSStatus AudioHandler::recordingCallback(void *inRefCon,
                                  AudioUnitRenderActionFlags *ioActionFlags,
                                   const AudioTimeStamp *inTimeStamp,
                                  UInt32 inBusNumber,
                                  UInt32 inNumberFrames,
                                  AudioBufferList *ioData) {
    AudioHandler *au = (AudioHandler *)inRefCon;
    if (au->m_IsRecording == FALSE) {
#ifdef ENABLE_DEBUG_LOGGING
        printf("[Media-AH]: Not in recording mode\n");
#endif
        return noErr;
    }
    OSStatus status = noErr;
    
#ifdef ENABLE_DEBUG_LOGGING
    printf("[Media-AH]: recordingCallback No of frames=%ld computed Frames=%ld\n",inNumberFrames,au->getNoOfFrames());
    printf("[Media-AH]: recordingCallback noOfFrames=%ld  predef framesize=%ld bus number=%ld \n",inNumberFrames,au->getBufferSize(),inBusNumber);
#endif
    
    if (au->mAudioUnitStarted == FALSE)
    {
        printf("[Media-AH]: Audio unit is stopped");
        return status;
    }

    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0].mData = NULL;
    bufferList.mBuffers[0].mDataByteSize = 0;
    
    status = AudioUnitRender(*audioUnit,
                             ioActionFlags,
                             inTimeStamp,
                             kInputBus,
                             inNumberFrames,
                             &bufferList);
    if(status != noErr)
    {
        printf("[Media-AH]: recordingCallback Error is %ld\n",status);
        return status;
    }
    
    if((!bufferList.mBuffers[0].mData)||(bufferList.mBuffers[0].mDataByteSize <= 0))
    {
        printf("[Media-AH]: recordingCallback with Empty Bufer");
        return status;
    }

    UInt8 *inputFrames = (UInt8*)(bufferList.mBuffers[0].mData);
    au->m_typePCM.length = bufferList.mBuffers[0].mDataByteSize;
    au->m_acquiredCBFunc(inputFrames, &au->m_typePCM, 2 * inNumberFrames, &au->m_mvs_pkt_status);
    return noErr;
}

OSStatus AudioHandler::playbackCallback(void *inRefCon,
                                 AudioUnitRenderActionFlags *ioActionFlags,
                                 const AudioTimeStamp *inTimeStamp,
                                 UInt32 inBusNumber,
                                 UInt32 inNumberFrames,
                                 AudioBufferList *ioData) {
#ifdef ENABLE_DEBUG_LOGGING
    printf("[Media-AH]: playbackCallback bus number=%ld with getsize =%ld\n",inBusNumber,ioData->mBuffers->mDataByteSize);
#endif
    AudioHandler *au = (AudioHandler *)inRefCon;
    if(au->m_IsPlaying == FALSE)
    {
        memset(ioData->mBuffers->mData, 0, au->getBufferSize());
        ioData->mBuffers->mDataByteSize = 0;
#ifdef ENABLE_DEBUG_LOGGING
        printf("[Media-AH]: Not in playback mode\n");
#endif
        return noErr;
    }
    
    OSStatus status = noErr;
    au->memsetPlayBuffer();
    
    if ((au->mAudioUnitStarted == FALSE) || (au->mToPlayBuffer == NULL))
    {
        printf("[Media-AH]: AudioOutputUnit has been stopped or Play Buffer is Null\n");
        return noErr;
    }
    
    au->m_playoutFrameCBFunc(au->mToPlayBuffer, &au->m_typePCM, &au->m_mvs_pkt_status);
    if ((au->m_typePCM.length == 0)||(au->mAudioUnitStarted == FALSE))
    {
#ifdef ENABLE_DEBUG_LOGGING
        printf("[Media-AH]: playSize is 0 !!!!\n");
#endif
        if(au->mAudioUnitStarted == FALSE)
        {
            printf("[Media-AH]: AudioUnit is stopped!!!!\n");
        }
        memset(ioData->mBuffers->mData, 0, au->getBufferSize());
        ioData->mBuffers->mDataByteSize = 0;
        return status;
    }
    UInt8 *inputFrames = (UInt8*)(ioData->mBuffers->mData);
    memcpy(inputFrames, au->mToPlayBuffer, au->getBufferSize());
    ioData->mBuffers->mDataByteSize = au->getBufferSize();
    return noErr;
}

void AudioHandler::setAudioSessionCategory(UInt32 sessionCategory)
{
    if(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory,
                               sizeof(UInt32), &sessionCategory) != noErr) {
        printf("[Media-AH]: MEDIA_ERROR AudioSessionSetProperty audioCategory to %d failed\n",(unsigned int)sessionCategory);
    }
    else
    {
#ifdef ENABLE_DEBUG_LOGGING
        printf("[Media-AH]: AudioSessionSetProperty audioCategory successfully set to %d\n",(unsigned int)sessionCategory);
#endif
    }
}

void AudioHandler::setAudioSessionProperty(UInt32 sessionProperty)
{
    UInt32 overrideCategory = 1;
    OSStatus error = AudioSessionSetProperty(sessionProperty,
                                             sizeof(UInt32), &overrideCategory);
    if(error != noErr)
    {
        printf("[Media-AH]: MEDIA_ERROR AudioSessionSetProperty sessionProperty Set To %d failed\n",(unsigned int)sessionProperty);
    }
    else
    {
#ifdef ENABLE_DEBUG_LOGGING
        printf("[Media-AH]: AudioSessionSetProperty sessionProperty Set To %d success\n",(unsigned int)sessionProperty);
#endif
    }
}

void AudioHandler::setAudioSessionMode(UInt32 sessionMode)
{
    OSStatus error = AudioSessionSetProperty(kAudioSessionProperty_Mode, sizeof(sessionMode), &sessionMode);
    if (error != noErr)
    {
        printf("[Media-AH] MEDIA_ERROR AudioSessionSetProperty sessionMode Set To %d failed\n",(unsigned int)sessionMode);
    }
    else
    {
#ifdef ENABLE_DEBUG_LOGGING
        printf("[Media-AH] AudioSessionSetProperty sessionProperty Set To %d success\n",(unsigned int)sessionMode);
#endif
    }
}

void AudioHandler::updateSilentModeSettings(bool isON)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        mIsSilentModeOn = isON;
        printf("[Media-AH]: silent mode is %d\n",mIsSilentModeOn);
        
        if(mIsSilentModeOn)
        {
            setAudioSessionCategory(kAudioSessionCategory_AmbientSound);
        }
        else
        {
            setAudioSessionCategory(kAudioSessionCategory_PlayAndRecord);
            setAudioSessionProperty(kAudioSessionProperty_OverrideCategoryDefaultToSpeaker);
            setAudioSessionMode(kAudioSessionMode_VoiceChat);
        }
    });
   
}

/***********
 By default, audio session category is set to PlayAndRecord on initialization
 If silent mode is disabled, then no problem, calls will go normal way
 with PlayAndRecord category
 If silent mode is enabled:
 - As originator, no problem as category is already PlayAndRecord
 - As target, at call start, change to Ambient category and revert it
   to PlayAndRecord when call ends, this is what the below function does
 ***********/
void AudioHandler::initAudioSessionPropertyForSender()
{
    printf("[Media-AH]: initAudioSessionPropertyForSender\n");
    if(mIsSilentModeOn)
    {
        printf("[Media-AH]: call started by sender in silent mode, set to playAndRecord category\n");
        setAudioSessionCategory(kAudioSessionCategory_PlayAndRecord);
        setAudioSessionProperty(kAudioSessionProperty_OverrideCategoryDefaultToSpeaker);
        setAudioSessionMode(kAudioSessionMode_VoiceChat);
        mNeedToRevertAudioSessionCategoryForSilentMode = TRUE;
    }
}

//Function invoked when CS call is received (during idle period or active QChat call)
void AudioHandler::AudioSessionInterruptionListener
(
 void *inClientData,
 UInt32 inInterruptionState
)
{
    printf("[Media-AH]: AudioSessionInterruptionListener invoked InterruptionState=%d\n",(unsigned int)inInterruptionState);
    AudioHandler *audioHandler = (AudioHandler *)inClientData;
    if(audioHandler == NULL)
    {
        printf("[Media-AH]: audioHandler is NULL, return");
        return;
    }

    if(audioHandler->IsCSCallInProgress())
    {
        audioHandler->HandleCSCallInterruption(audioHandler);
        return;
    }

    if(inInterruptionState == kAudioSessionEndInterruption)
    {
        audioHandler->RestoreQChatAudioAfterEndOfAudioSessionInterruption(audioHandler);
    }
}

Boolean AudioHandler::IsCSCallInProgress()
{
    CTCallCenter *ctCallCenter = [[CTCallCenter alloc] init];
    if (ctCallCenter.currentCalls != nil)
        return TRUE;
    else
        return FALSE;
}

void AudioHandler::HandleCSCallInterruption(AudioHandler* audioHandler)
{
#ifdef ENABLE_DEBUG_LOGGING
    printf("[Media-AH]: CS Call notification received\n");
#endif
    if(audioHandler->mAudioUnitStarted==TRUE)
    {
        if(audioHandler->stopProcessingAudio() == 0)
        {
            audioHandler->m_ResumeAudioOutputUnitAfterInterruption = TRUE;
#ifdef ENABLE_DEBUG_LOGGING
            printf("[Media-AH]: Successfully stopped the audio output unit\n");
#endif
        }
        else
        {
#ifdef ENABLE_DEBUG_LOGGING
            printf("[Media-AH]: Failed to stop audio output unit\n");
#endif
        }
    }
}

void AudioHandler::RestoreQChatAudioAfterEndOfAudioSessionInterruption(AudioHandler* audioHandler)
{
#ifdef ENABLE_DEBUG_LOGGING
    printf("[Media-AH]: End of Audio Session Interruption\n");
#endif
    if(AudioSessionSetActive(true) != noErr)
    {
        audioHandler->mAudioAvailable = FALSE;
#ifdef ENABLE_DEBUG_LOGGING
        printf("[Media-AH]: AudioSessionSetActive true failed\n");
#endif
    }
    else
    {
        audioHandler->mAudioAvailable = TRUE;
#ifdef ENABLE_DEBUG_LOGGING
        printf("[Media-AH]: AudioSessionSetActive success\n");
#endif
    }
    
    if(audioHandler->m_ResumeAudioOutputUnitAfterInterruption==TRUE)
    {
        if(audioHandler->startAudioUnit() == 0)
        {
#ifdef ENABLE_DEBUG_LOGGING
            printf("[Media-AH]: Successfully started audio output unit\n");
#endif
        }
        else
        {
#ifdef ENABLE_DEBUG_LOGGING
            printf("[Media-AH]: Failed to start audio output unit\n");
#endif
        }
    }
}