#include "QiOSAudioHandler.h"

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
    //error handling
    BOOL success;
    NSError* error;
    AVAudioSession* session = [AVAudioSession sharedInstance];
    success = [session setActive:NO error:&error];
    if(!success)
    {
        printf("[Media-AH]: T-AS AudioSessionSetActive false failed\n");
    }
    
    if(AudioUnitUninitialize(*audioUnit) != noErr) {
        printf("[Media-AH]: T-AS AudioUnitUninitialize false failed\n");
    }
    mAudioAvailable = false;
    return 0;
}

Boolean AudioHandler::GetAudioStatus()
{
    printf("[Media-AH]: GetAudioStatus, mAudioAvailable %d\n",mAudioAvailable);
    if (mAudioAvailable == FALSE) {
        //try initializing audio unit again
        startAudioSession();
    }
    return mAudioAvailable;
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
    if(0 != configureAudioSession())
    {
        printf("[Media-VOC]: **************Failed to Init Audio Session**************\n");
    }
    if(0 != initAudioStreams())
    {
        printf("[Media_VOC]: **************Failed to Init Audio Streams***************\n");
    }
}

int AudioHandler::configureAudioSession()
{
    
    //get your app's audioSession singleton object
    AVAudioSession* session = [AVAudioSession sharedInstance];
    
    //error handling
    BOOL success;
    NSError* error;
    printf("[Media-AH]: configureAudioSession\n");
    if(mSessionInit)
    {
        printf("[Media-AH]: Session Already Initialized\n");
        return 0;
    }
    
    success = [[AVAudioSession sharedInstance] setActive:YES error:&error];
    if (!success)  NSLog(@"AVAudioSession error setting category:%@",error);
    
    success = [session setCategory:AVAudioSessionCategoryPlayAndRecord
                             error:&error];
    if (!success)  NSLog(@"AVAudioSession error setting category:%@",error);

    
    
    [[AVAudioSession sharedInstance] setPreferredIOBufferDuration:AUDIO_DURATION error:nil];
    if (!success)  NSLog(@"AVAudioSession error setting category:%@",error);
    
    
    
    success =[[AVAudioSession sharedInstance] setPreferredHardwareSampleRate:AUDIO_SAMPLE_RATE_TYPE_8 error:nil];
    if (!success)  NSLog(@"AVAudioSession error setting category:%@",error);

    
    
    success = [session setMode:AVAudioSessionModeVoiceChat error:&error];
    if (!success)  NSLog(@"AVAudioSession error setting category:%@",error);
    
    success = [session overrideOutputAudioPort:AVAudioSessionPortOverrideSpeaker error:&error];
    if (!success)  NSLog(@"AVAudioSession error setting category:%@",error);
    
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
    audioUnit = (AudioUnit*)malloc(sizeof(AudioUnit));
    AudioComponentDescription componentDescription;
    componentDescription.componentType = kAudioUnitType_Output;
    componentDescription.componentSubType = kAudioUnitSubType_VoiceProcessingIO;//kAudioUnitSubType_RemoteIO;
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
        printf("[Media-AH]: MEDIA_ERROR initAudioStreams Failed to Set Property for enabling IO\n");
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
        printf("[Media-AH]: MEDIA_ERROR initAudioStreams Failed to Set Call Back Function for kInputBus\n");
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
        printf("[Media-AH]: MEDIA_ERROR initAudioStreams Failed to Set Call Back Function for kInputBus\n");
        return 1;
    }
    
    UInt32 enableBufferAllocflag = 1;
    if(AudioUnitSetProperty(*audioUnit,
                            kAudioUnitProperty_ShouldAllocateBuffer,
                            kAudioUnitScope_Output,
                            kInputBus,
                            &enableBufferAllocflag,
                            sizeof(enableBufferAllocflag)) != noErr) {
        printf("[Media-AH]: MEDIA_ERROR initAudioStreams Failed to set enableBufferAllocflag \n");
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
        printf("[Media-AH]: MEDIA_ERROR initAudioStreams Failed to Set Stream Format for input\n");
        return 1;
    }
    
    // Ditto for the output stream, which we will be sending the processed audio to
    if(AudioUnitSetProperty(*audioUnit,
                            kAudioUnitProperty_StreamFormat,
                            kAudioUnitScope_Output,
                            kInputBus,
                            &streamDescription,
                            sizeof(streamDescription)) != noErr) {
        printf("[Media-AH]: MEDIA_ERROR initAudioStreams Failed to Set Stream Format for output\n");
        return 1;
    }
    
    //Start Audio Session
    startAudioSession();
    //set the flag
    mStreamInit = TRUE;
    return 0;
}

int AudioHandler::startAudioUnit()
{
    printf("[Media-AH]: startAudioUnit mAudioUnitStarted=%d\n",mAudioUnitStarted);
    memsetPlayBuffer();
    
    if (mAudioUnitStarted == FALSE)
    {
        OSStatus auerror = AudioOutputUnitStart(*audioUnit);
        if(auerror != noErr)
        {
            printf("[Media-AH]: MEDIA_ERROR AudioOutputUnitStart failed error=%d\n",(int)auerror);
            return 1;
        }
    }
    mAudioUnitStarted = TRUE;
    return 0;
}

int AudioHandler::stopProcessingAudioAsync()
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        stopProcessingAudio();
    });
    return 0;
}

int AudioHandler::stopProcessingAudio()
{
    printf("[Media-AH]: AudioHandler::stopAudioUnit()\n");
    
    memsetPlayBuffer();
    
    if (mAudioUnitStarted == TRUE)
    {
        if(AudioOutputUnitStop(*audioUnit) != noErr) {
            printf("[Media-AH]: AudioOutputUnitStop failed\n");
            return 1;
        }
        if(AudioUnitUninitialize(*audioUnit) != noErr) {
            printf("[Media-AH]: T-AS AudioUnitUninitialize false failed\n");
        }
    }
    mAudioUnitStarted = FALSE;
    mAudioAvailable = FALSE;
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
        printf("[Media-AH]: recordingCallback Error is %d\n",(int)status);
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