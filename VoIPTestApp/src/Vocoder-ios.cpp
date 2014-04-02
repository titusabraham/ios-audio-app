/*
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * Qualcomm Technologies, Inc. ("Proprietary Information").  You shall
 * not disclose such Proprietary Information, and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with Qualcomm Technologies, Inc.
 * 
 * FILE:Vocoder-ios.cpp
 *
 * DESCRIPTION:
 */

#include "VocCommon.h"
#include "GenerateFrame.h"
#include "Vocoder-ios.h"
#include "frame.h"
#include "ring.h"
#include "timesec.h"
#include "iOSInterface.h"

/****************Enum Declarations**********************/

//recording states
typedef enum e_VocoderRecState
{
    VOC_REC_STATE_STOPPED = 0,
    VOC_REC_STATE_RECORDING = 1,
    VOC_REC_STATE_PAUSED = 2
} e_VocoderRecState;

typedef enum e_VocoderFrameReadState
{
    FRAME_READ_INITIAL,
    FRAME_READ_IN_PROGRESS,
    FRAME_READ_COMPLETE
} e_VocoderFrameReadState;

e_VocoderFrameReadState frameReadState;

/****************Vocoder Configuration******************/
// Vocoder configuration
#define DECODER_WAIT_MILLIS 60
#define ENCODER_WAIT_MILLIS 60
#define PLAYER_WAIT_MILLIS 20
#define RINGDQ_WAIT_MILLIS 60     //when dequeing, wait up to 60ms for input

// to enable volume amplification for playback, turn doEarGain to true
static bool doEarGain = false;
// to enable volume amplification for record, turn doMicGain to true
static bool doMicGain = false;
// how much to amplify playback PCM, 2 is about 3 dB
static float playMultiplier = 1.00;
// how much to amplify recorded PCM, 2 is about 3 dB
static float recordMultiplier = 1.00;

static int codecArgs[Codec::LAST_CODEC_ARG];

#ifdef USE_FILE_IO
char playFileName[50];
FILE *playFile;
    
char playFileNameBG[50];
FILE *playFileBG;

char recordFileName[50];
FILE *recordFile;
    
char recordFileNameBG[50];
FILE *recordFileBG;

char recordEncTxFileName[50];
FILE *recordEncTxFile;

char playDecRxFileName[50];
FILE *playEncRxFile;

int gPCMSize;
#endif

/****************Variable Declarations******************/
// Vocoder
static ICodec *codec;
static GenerateFrame *PCMEncGenerator;
static GenerateFrame *PCMDecGenerator;

// Vocoder callbacks
static VocoderRecorderCallback recorderCallback;
static VocoderPlayerCallback playerCallback;
static VocoderTimerCallback timerCallback;
static unsigned long timerMillis;
static pthread_mutex_t recorderCBMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t timerCBMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t playerCBMutex = PTHREAD_MUTEX_INITIALIZER;

// Vocoder threads
static pthread_t encoderThread;
static int encoderExitPending;
static int encoderThread_create_rc=-1;

static pthread_t decoderThread;
static int decoderExitPending;
static int decoderThread_create_rc=-1;

static pthread_t timerThread;
static int timerExitPending;
static int timerThread_create_rc=-1;

//Vocoder Rings
static RingBuffer<Frame> encoderRing(1050); // 50 is 1 sec of frame data
static RingBuffer<Frame> decoderRing(1050);
static RingBuffer<Frame> playerRing(1050);
static RingBuffer<Frame> recorderRing(1050);
    
//PlayBack Optimizations
static unsigned long decode_cnt = 0;
static unsigned long playIn = 0;
static unsigned long playOut = 0;
static long playInStartedTime = 0;
static long lastPlayOutTime = 0;
static uint32_t playbackDurationMs = 0;
static bool playOutRequest = FALSE;

//Vocoder Lib Load Status
static bool isLoaded = FALSE;

// current state of the recorder
e_VocoderRecState recorderState;
/*******************************************************/
void InitializeiOSAudioHandler();
void StartAudioUnit();
void StopAudioUnit();
void CreateCodec(Vocoder_ConfigParamType param);
void DeleteCodec();
void resetPlaybackOptimizationCounters();
void flushRecordBuffer();
/****************Thread Management**********************/

static void *doTimer(void *param)
{
    VocoderTimerCallback tCallback = NULL;
    timerExitPending = 0;
    unsigned long usecs = msec_to_usec(timerMillis);

    if (usecs)
    {
        while (!timerExitPending)
        {
            usleep((useconds_t)usecs);
            if (timerExitPending)
            {
                continue;
            }
            pthread_mutex_lock (&timerCBMutex);
            tCallback = timerCallback;
            pthread_mutex_unlock (&timerCBMutex);
            if (tCallback)
            {
                (*tCallback)();
            }
        }
    }
    pthread_exit(NULL);
    return NULL;
}

void *doDecode(void *param)
{
    printf("[Media-VOC]: doDecode ready\n");
    decoderExitPending = 0;
    int decodePCMSize = AudioHandler::GetInstance()->getBufferSize();
#ifdef ENABLE_DEBUG_LOGGING
    int decSize = 0;
    unsigned long start, stop;
#endif
    while (!decoderExitPending)
    {
        //Step1: Deque from Decoder Ring
        Frame *PCMframe = decoderRing.dequeue(DECODER_WAIT_MILLIS);
        if (!PCMframe)
        {
#ifdef ENABLE_DEBUG_LOGGING
            printf("[Media-VOC]: doDecode decoderRing empty, decoderExitPending=%d\n", decoderExitPending);
#endif
            continue;
        }

        //Step2: Fail safe check just in case we were asked to exit while we were blocked
        if (decoderExitPending)
        {
            printf("[Media-VOC]: doDecode have frame but exit pending skip processing\n");
            if(PCMDecGenerator)
            {
                PCMDecGenerator->ResetGenData();
            }
            delete PCMframe;
            PCMframe = NULL;
            break;
        }

        //Step3: Check if DecGenerator is valid
        if(!PCMDecGenerator)
        {
            printf("[Media-VOC]:doDecode PCM Generator is null\n");
            break;
        }
        
        //Step4: Decode the EVRC frame to PCM
#ifdef ENABLE_DEBUG_LOGGING
        start = time_in_millis();
#endif

#ifdef ENABLE_DEBUG_LOGGING
        decSize = codec->decode(PCMframe->getEncBuffer(), PCMframe->getEncSize(),(short *) PCMframe->getPcmBuffer());
#else
        codec->decode(PCMframe->getEncBuffer(), PCMframe->getEncSize(),(short *) PCMframe->getPcmBuffer());
#endif
#ifdef ENABLE_DEBUG_LOGGING
        stop = time_in_millis();
        printf("[Media-VOC]: doDecode DecodedSize=%ld from EncSize=%d\n",decSize*sizeof(short),PCMframe->getEncSize());
        printf("[Media-VOC]: doDecode DecodeTime=%ld\n",(stop-start));
#endif

        //Step5: Write to File the Encoded Buffer
#ifdef USE_FILE_IO
        if(playEncRxFile != NULL)
        {
            int fRetVal = 0;
            fRetVal = fwrite((short *)PCMframe->getEncBuffer(),PCMframe->getEncSize(), 1, playEncRxFile);
        }
#endif
        
        //Step6: Get PCM Frames of Appropriate Size for AudioHandler
        PCMGenType retVal = PCM_FRAME_UNAVAILABLE;
        char *inputPCMBuffer = PCMframe->getPcmBuffer();
        do {
            if (decoderExitPending)
            {
                printf("[Media-VOC]: doDecodeGeneratePCMFrame have frame but exit pending skip processing\n");
                break;
            }
            
#ifdef USE_FILE_IO
            if(playFileBG != NULL)
            {
                int fRetVal = 0;
                fRetVal = fwrite((short *)inputPCMBuffer,PCMframe->getPcmSize(), 1, playFileBG);
            }
#endif
            retVal = PCMDecGenerator->GeneratePCMFrame(inputPCMBuffer);
            if(retVal == PCM_FRAME_UNAVAILABLE)
            {
                break;
            }
            GenerateFrameDataType* data = PCMDecGenerator->GetGeneratedFrame();
            if(!data || !data->isAvailable || !data->GenBuffer)
            {
                printf("[Media-VOC]: MEDIA_ERROR - NULL generated frame");
                //assert(0);
                break;
            }
            Frame *GenFrame = new Frame(decodePCMSize,decodePCMSize);
            if(!GenFrame)
            {
#ifdef ENABLE_DEBUG_LOGGING
                printf("[Media-VOC]: Ran out of memory for GenFrame\n");
#endif
            }
            memcpy(GenFrame->getPcmBuffer(),data->GenBuffer,decodePCMSize);
#ifdef USE_FILE_IO
            if(playFile != NULL)
            {
                int fRetVal = 0;
                fRetVal = fwrite((short *)GenFrame->getPcmBuffer(),decodePCMSize, 1, playFile);
            }
#endif
            // queue for playing
            decode_cnt++;
            playerRing.enqueue(GenFrame);
#ifdef ENABLE_DEBUG_LOGGING
            printf("[Media-VOC]:doDecode Dec Ring Size=[%d] Play Ring Size=[%d] with GenFrameSize=%d\n",decoderRing.getQueueSize(),playerRing.getQueueSize(),decSize);
#endif
            inputPCMBuffer = NULL;
        } while (retVal == PCM_FRAME_ACQUIRED_WITH_MORE);
        if(PCMframe != NULL)
        {
            delete PCMframe;
            PCMframe = NULL;
        }
    }
    //Lets clear it all.
    decoderRing.clear();
    playerRing.clear();
    if(PCMDecGenerator)
    {
        PCMDecGenerator->ResetGenData();
    }
    printf("[Media-VOC]: doDecode exit\n");
    pthread_exit(NULL);
    return NULL;
}

void *doEncode(void *param)
{
#ifdef ENABLE_DEBUG_LOGGING
    unsigned long start;
#endif
    unsigned long stop;
    encoderExitPending = 0;
    VocoderRecorderCallback recCallbackFn = NULL;
    int encodePCMSize = codec->getPcmSize()*sizeof(short);
    int encSize = 0;
    while (!encoderExitPending)
    {
        Frame *PCMframe = encoderRing.dequeue(ENCODER_WAIT_MILLIS);
        if (!PCMframe)
        {
#ifdef ENABLE_DEBUG_LOGGING
            printf("[Media-VOC]:doEncode encoderRing empty, encoderExitPending=%d\n",
                   encoderExitPending);
#endif
            continue;
        }
        
        //just in case we were asked to exit while we were blocked
        if (encoderExitPending)
        {
            printf("[Media-VOC]: doEncode have frame but exit pending skip processing. No of left over frames=%d\n",encoderRing.getQueueSize());
            if(PCMEncGenerator)
            {
                PCMEncGenerator->ResetGenData();
            }
            delete PCMframe;
            PCMframe = NULL;
            break;
        }
        
        if(!PCMEncGenerator)
        {
            printf("[Media-VOC]:doEncode PCM Generator is null\n");
            break;
        }
        
#ifdef USE_FILE_IO
        int fRetVal = 0;
        if (recordFileBG != NULL)
        {
            fRetVal = fwrite(PCMframe->getPcmBuffer(),PCMframe->getEncSize(), 1, recordFileBG);
        }
#endif
        PCMGenType retVal = PCM_FRAME_UNAVAILABLE;
        char *inputPCMBuffer = PCMframe->getPcmBuffer();
        do {
            if (encoderExitPending)
            {
                printf("[Media-VOC]: doEncodeGeneratePCMFrame have frame but exit pending skip processing. No of left over frames\n");
                break;
            }
            retVal = PCMEncGenerator->GeneratePCMFrame(inputPCMBuffer);
            if(retVal == PCM_FRAME_UNAVAILABLE)
            {
                break;
            }
            GenerateFrameDataType* data = PCMEncGenerator->GetGeneratedFrame();
            if(!data || !data->isAvailable || !data->GenBuffer)
            {
                printf("[Media-VOC]: MEDIA_ERROR - NULL generated frame");
                //assert(0);
                break;
            }
            Frame *GenFrame = new Frame(encodePCMSize,encodePCMSize);
            if(!GenFrame)
            {
#ifdef ENABLE_DEBUG_LOGGING
                printf("[Media-VOC]: Ran out of memory for GenFrame\n");
#endif
            }
            memcpy(GenFrame->getPcmBuffer(),data->GenBuffer,encodePCMSize);
#ifdef USE_FILE_IO
            int fRetVal = 0;
            if (recordFile != NULL)
            {
                fRetVal = fwrite((short *)GenFrame->getPcmBuffer(),encodePCMSize, 1, recordFile);
            }
#endif
#ifdef ENABLE_DEBUG_LOGGING
            start = time_in_millis();
#endif
            encSize =  codec->encode((short*) GenFrame->getPcmBuffer(),GenFrame->getEncBuffer());
            stop = time_in_millis();
#ifdef ENABLE_DEBUG_LOGGING
            printf("[Media-VOC]: doEncode EncodeTime=%ld\n",(stop-start));
#endif
            GenFrame->setEncSize(encSize);
            GenFrame->setTimestamp(stop);
#ifdef USE_FILE_IO
            int fEncRetVal = 0;
            if (recordEncTxFile != NULL)
            {
                fEncRetVal = fwrite((short *)GenFrame->getEncBuffer(),GenFrame->getEncSize(), 1, recordEncTxFile);
            }
#endif
            //printf("encSize = %d\n",encSize);
            if (encSize > 0) {
                recorderRing.enqueue(GenFrame);
                //printf("recorderRing qSize = %d\n",recorderRing.getQueueSize());
            }
            if(frameReadState == FRAME_READ_INITIAL || frameReadState == FRAME_READ_COMPLETE)
            {
                if (recorderRing.getQueueSize() > 0 && recorderRing.getQueueSize()%6 == 0)
                {
                    pthread_mutex_lock (&recorderCBMutex);
                    recCallbackFn = recorderCallback;
                    pthread_mutex_unlock (&recorderCBMutex);
                    
                    if (recCallbackFn)
                    {
                        frameReadState = FRAME_READ_IN_PROGRESS;
                        (*recCallbackFn)(GenFrame->getPcmBuffer(), GenFrame->getEncSize());
                    }
                }
            }
            inputPCMBuffer = NULL;
        } while (retVal == PCM_FRAME_ACQUIRED_WITH_MORE);
        if(PCMframe != NULL)
        {
            delete PCMframe;
            PCMframe = NULL;
        }
    }
    //Lets clear it all.
    encoderRing.clear();
    recorderRing.clear();
    if(PCMEncGenerator)
    {
        PCMEncGenerator->ResetGenData();
    }
    printf("[Media-VOC]: doEncode exit. No of left over frames=%d\n",encoderRing.getQueueSize());
    pthread_exit(NULL);
    return NULL;
}
/****************Public Functions***********************/
void VocoderInit(Vocoder_ConfigParamType param)
{
    printf("[Media_VOC]: NAME:%s  VERSION:%s\n",(char *)DISP_VOC_PRODUCT_NAME, (char *)DISP_VOC_VERSION);
    frameReadState = FRAME_READ_INITIAL;
    VocoderInitFull(param);
    return;
}
    
void VocoderInitFull(Vocoder_ConfigParamType param)
{
    printf("[Media_VOC]: VocoderInitFull\n");
    DeleteCodec();
    CreateCodec(param);
#ifdef USE_FILE_IO
    gPCMSize = codec->getPcmSize();
    printf("[Media_VOC]: Codec Size->%d\n",gPCMSize);
    GetiOSFileName(playFileName,"PCMInput");
    playFile = fopen(playFileName,"w");
    GetiOSFileName(playFileNameBG,"PCMInputBG");
    playFileBG = fopen(playFileNameBG,"w");
    GetiOSFileName(playDecRxFileName,"EncodedInput");
    playEncRxFile = fopen(playDecRxFileName,"w");
    GetiOSFileName(recordFileName,"PCMOutput");
    recordFile = fopen(recordFileName,"w");
    GetiOSFileName(recordFileNameBG,"PCMOutputBG");
    recordFileBG = fopen(recordFileNameBG,"w");
    GetiOSFileName(recordEncTxFileName,"EncodedOutput");
    recordEncTxFile = fopen(recordEncTxFileName,"w");
#endif
    return;
}

void VocoderInitCodecArgs()
{
    printf("[Media-VOC]: VocoderInitCodecArgs\n");
    codecArgs[Codec::NOISESUPPRESION] = 1;
    codecArgs[Codec::POSTFILTER] = 1;
    codecArgs[Codec::TTY] = 0;
    codecArgs[Codec::FEC_OFFSET] = 0;
    codecArgs[Codec::FEC_PROTECTION] = 0;
    codecArgs[Codec::FEC_REDUNDANCY] = 0;
    codecArgs[Codec::TIME_WARPING] = 0;
    return;
}

void VocoderSetCodecArg(Codec::Args key, int value)
{
    codecArgs[key]= value;
}

void VocoderLoadCodecs(const char *pLibsPathStr)
{
    printf("[Media-VOC]: VocoderLoadCodecs UnSupported\n");
    return;
}

void VocoderUnloadCodecs()
{
    printf("[Media-VOC]: VocoderUnloadCodecs UnSupported\n");
    return;
}

void VocoderShutdown()
{
    printf("[Media-VOC]: VocoderShutdown exit pending (1): encoder %d - decoder %d - timer %d\n",
           encoderExitPending, decoderExitPending, timerExitPending);
    
    frameReadState = FRAME_READ_INITIAL; //ensure this doesnt affect any pending recorded frames in queue
    //wait for the threads to die (assuming they were started)
    void *ptread_returned;
    int rc=0;
    AudioHandler *au = AudioHandler::GetInstance();
    au->setRecordingState(FALSE);
    au->setPlayingState(FALSE);
    StopAudioUnit();
    printf("[Media-VOC]: VocoderShutdown - check on threads, wait for them to end\n");
    if (encoderThread_create_rc == 0)
    {
        rc = pthread_join(encoderThread, &ptread_returned);
        printf("[Media-VOC]: VocoderShutdown- encoderThread join (%d)\n", rc);
    } else {
        printf("[Media-VOC]: VocoderShutdown- encoderThread was not started - don't wait\n");
    }
    encoderThread_create_rc = -1;
    
    if (decoderThread_create_rc == 0)
    {
        rc = pthread_join(decoderThread, &ptread_returned);
        printf("[Media-VOC]: VocoderShutdown - decoderThread join (%d)\n", rc);
    } else {
        printf("[Media-VOC]: VocoderShutdown - decoderThread was not started - don't wait\n");
    }
    decoderThread_create_rc = -1;
    
    if (timerThread_create_rc == 0)
    {
        rc = pthread_join(timerThread, &ptread_returned);
        printf("[Media-VOC]: VocoderShutdown - timerThread join (%d)\n", rc);
    } else {
        printf("[Media-VOC]: VocoderShutdown - timerThread was not started - don't wait\n");
    }
    timerThread_create_rc = -1;
    
    //reset internal recorder state
    recorderState = VOC_REC_STATE_STOPPED;
    
    // clear the rings
    encoderRing.clear();
    decoderRing.clear();
    playerRing.clear();
    recorderRing.clear();

    if(PCMDecGenerator)
    {
        PCMDecGenerator->ResetGenData();
    }
    if(PCMEncGenerator)
    {
        PCMEncGenerator->ResetGenData();
    }
    //Invalidating registered call backs
    timerCallback = NULL;
    recorderCallback = NULL;
    playerCallback = NULL;
    
    printf("[Media-VOC]: VocoderShutdown exit pending (2): encoder %d - decoder %d - timer %d\n",
           encoderExitPending, decoderExitPending, timerExitPending);
#ifdef USE_FILE_IO
    if(recordFile != NULL)
    {
        fflush(recordFile);
        fclose(recordFile);
    }
    if(recordFileBG != NULL)
    {
        fflush(recordFileBG);
        fclose(recordFileBG);
    }
    if(recordEncTxFile != NULL)
    {
        fflush(recordEncTxFile);
        fclose(recordEncTxFile);
    }
    if(playFile != NULL)
    {
        fflush(playFile);
        fclose(playFile);
    }
    if(playFileBG != NULL)
    {
        fflush(playFileBG);
        fclose(playFileBG);
    }
    if(playEncRxFile != NULL)
    {
        fflush(playEncRxFile);
        fclose(playEncRxFile);
    }
#endif
    printf("[Media]: VocoderShutdown complete\n");
    return;
}
void VocoderCreateEchoCanceller(const char *configFile)
{
    printf("[Media-VOC]: VocoderCreateEchoCanceller NOT SUPPORTED\n");
    return;
}
void VocoderStartEchoCanceling()
{
    printf("[Media-VOC]:VocoderStartEchoCanceling NOT SUPPORTED\n");
    return;
}
void VocoderStopEchoCanceling()
{
    printf("[Media-VOC]: VocoderStopEchoCanceling NOT SUPPORTED\n");
    return;
}
void VocoderSetECNSConfigFileName(const char *configFile, int namelength)
{
    printf("[Media-VOC]: VocoderSetECNSConfigFileName NOT SUPPORTED\n");
    return;
}
void VocoderCreateAudioPlayer()
{
    printf("[Media-VOC]: VocoderCreateAudioPlayer Not Supported\n");
    return;
}

void VocoderStartPlaying()
{
    printf("[Media-VOC]: VocoderStartPlaying entry\n");
    resetPlaybackOptimizationCounters();
    StartAudioUnit();
    AudioHandler *audioHandler = AudioHandler::GetInstance();
    audioHandler->setPlayingState(TRUE);
    // Start the decoder thread
    decoderThread_create_rc = pthread_create(&decoderThread, NULL, doDecode, NULL);
#ifdef ENABLE_LOOPBACK
    //int PCMBufferSize = audioHandler->getBufferSize();
    printf("[Media-VOC]:VocoderStartPlaying decoderRing Ring Size %d\n",decoderRing.getQueueSize());
    while(recorderRing.getQueueSize())
    {
        printf("[Media-VOC]: VocoderStartPlaying Loop Back Rec Ring Size=[%d] Decoder Ring Size=[%d]\n",recorderRing.getQueueSize(),decoderRing.getQueueSize());
        Frame *tempframe = recorderRing.dequeueNonblock();
        if(tempframe)
        {
            Frame *frame = new Frame(codec->getPcmSize() * sizeof(short), codec->getEncSize());
            frame->setEncSize(tempframe->getEncSize());
            printf("[Media-VOC]: VocoderStartPlaying LoopEncSize=%d\n",frame->getEncSize());
            memcpy(frame->getEncBuffer(),tempframe->getEncBuffer(), tempframe->getEncSize());
            decoderRing.enqueue(frame);
            printf("[Media-VOC]: VocoderStartPlaying Loop Back Decoder Ring Size=[%d]\n",decoderRing.getQueueSize());
            delete tempframe;
        }
    }
#endif
    printf("[Media-VOC]: VocoderStartPlaying exit\n");
    return;
}
void VocoderStopPlaying()
{
    printf("[Media-VOC]: VocoderStopPlaying\n");
    AudioHandler *audioHandler = AudioHandler::GetInstance();
    audioHandler->setPlayingState(FALSE);
    // Stop the decoder Thread
    decoderExitPending++;
    return;
}
void VocoderDoPlaybackPostProc(bool enable)
{
    printf("[Media-VOC]: VocoderDoPlaybackPostProc enable[%d]\n",enable);
    doEarGain = enable;
    return;
}
void VocoderSetPlaybackGain(float multiplier)
{
    printf("[Media_VOC]: VocoderSetPlaybackGain\n");
    playMultiplier = multiplier;
    return;
}
void VocoderDecodeAndPlay(const char *encFrame, int frameSize)
{
#ifdef ENABLE_DEBUG_LOGGING
    printf("[Media-VOC]: VocoderDecodeAndPlay with FrameSize=[%d]\n",frameSize);
#endif
    Frame *frame = new Frame(codec->getPcmSize() * sizeof(short), codec->getEncSize());
    frame->setEncSize(frameSize);
    memcpy(frame->getEncBuffer(), encFrame, frameSize);
    decoderRing.enqueue(frame);
    return;
}
void VocoderPlay(const char *pcmFrame, int frameSize)
{
#ifdef ENABLE_DEBUG_LOGGING
    printf("[Media-VOC]: VocoderPlay with FrameSize=[%d]\n",frameSize);
#endif
    Frame *frame = new Frame(codec->getPcmSize() * sizeof(short), codec->getEncSize());
    frame->setEncSize(frameSize);
    memcpy(frame->getPcmBuffer(), pcmFrame, frameSize);
    decoderRing.enqueue(frame);
    return;
}
void VocoderRegisterPlayerCallback(VocoderPlayerCallback callback)
{
    printf("[Media-VOC]: VocoderRegisterPlayerCallback\n");
    pthread_mutex_lock (&playerCBMutex);
    playerCallback = callback;
    pthread_mutex_unlock (&playerCBMutex);
    return;
}
// Vocoder audio recording

void VocoderUseRecorderPreset(signed int type)
{
    printf("[Media-VOC]: VocoderUseRecorderPreset Not Supported (%d)\n",type);
    return;
}
void VocoderSetRecordGain(float multiplier)
{
    printf("[Media-VOC]: VocoderSetRecordGain\n");
    recordMultiplier = multiplier;
    return;
}
void VocoderDoRecordPreProc(bool enable)
{
    printf("[Media-VOC]: VocoderDoRecordPreProc enable[%d]\n",enable);
    doMicGain = enable;
    return;
}
int VocoderCreateAudioRecorder()
{
    printf("[Media-VOC]: VocoderCreateAudioRecorder NOT SUPPORTED\n");
    return VOC_RET_FAIL;
}
    
void VocoderStartRecording(bool autoStart)
{
    printf("[Media-VOC]: VocoderStartRecording entry start=%d\n",autoStart);
    frameReadState = FRAME_READ_INITIAL;
    StartAudioUnit();
    
    //reset internal recorder state
    recorderState = VOC_REC_STATE_STOPPED;
    AudioHandler::GetInstance()->setRecordingState(FALSE);
    
    // clear the rings
    encoderRing.clear();
    decoderRing.clear();
    playerRing.clear();
    recorderRing.clear();
    
    //reset Counters
    decode_cnt = 0;
    
    // Start the encoder thread
    encoderThread_create_rc = pthread_create(&encoderThread, NULL, doEncode, NULL);
    
    // update internal recorder state
    if(autoStart)
    {
        recorderState = VOC_REC_STATE_RECORDING;
        AudioHandler::GetInstance()->setRecordingState(TRUE);
    }
    else
    {
        recorderState = VOC_REC_STATE_PAUSED;
        AudioHandler::GetInstance()->setRecordingState(FALSE);
    }
    printf("[Media-VOC]: VocoderStartRecording exit\n");
    return;
}

void VocoderPauseRecording()
{
    printf("[Media-VOC]: VocoderPauseRecording Rec Ring Size=[%d] Enc Ring Size=[%d]\n",recorderRing.getQueueSize(),encoderRing.getQueueSize());
    frameReadState = FRAME_READ_INITIAL; //ensure this doesnt affect any pending recorded frames in queue
    // check that the state is playing, otherwise, just ignore the request
    if (__sync_bool_compare_and_swap(&recorderState, VOC_REC_STATE_RECORDING,
                                     VOC_REC_STATE_PAUSED))
    {
        AudioHandler::GetInstance()->setRecordingState(FALSE);
    }
    else
    {
        printf("[Media-VOC]: VocoderPauseRecording ignored\n");
    }
    return;
}
    
void VocoderResumeRecording()
{
    printf("[Media-VOC]: VocoderResumeRecording Rec Ring Size=[%d] Enc Ring Size=[%d]\n",recorderRing.getQueueSize(),encoderRing.getQueueSize());
    // check that the state is paused, otherwise, just ignore the request
    if (__sync_bool_compare_and_swap(&recorderState, VOC_REC_STATE_PAUSED,
                                     VOC_REC_STATE_RECORDING))
    {
        flushRecordBuffer();
        AudioHandler::GetInstance()->setRecordingState(TRUE);
    }
    else
    {
        printf("[Media-VOC]: VocoderResumeRecording ignored\n");
    }
    return;
}

void VocoderStopRecording()
{
    printf("[Media-VOC]: VocoderStopRecording\n");
    frameReadState = FRAME_READ_INITIAL; //ensure this doesnt affect any pending recorded frames in queue
    // stop the encoder thread
    encoderExitPending = 1;
    // update internal recorder state
    recorderState = VOC_REC_STATE_STOPPED;
    AudioHandler::GetInstance()->setRecordingState(FALSE);
    return;
}
void VocoderRegisterRecorderCallback(VocoderRecorderCallback callback)
{
    printf("[Media-VOC]: VocoderRegisterRecorderCallback\n");
    pthread_mutex_lock (&recorderCBMutex);
    recorderCallback = callback;
    pthread_mutex_unlock (&recorderCBMutex);
    return;
}
int VocoderRead(char *encFrame, int frameSize)
{
#ifdef ENABLE_DEBUG_LOGGING
    printf("[Media-VOC]: VocoderRead\n");
#endif
    int size = 0;
    Frame *frame = recorderRing.dequeueNonblock();
    if (frame)
    {
#ifdef ENABLE_DEBUG_LOGGING
        printf("[Media-VOC]: ***********VocoderRead %d bytes into %d byte buffer\n",
               frame->getEncSize(), frameSize);
#endif
        memcpy(encFrame, frame->getEncBuffer(), frame->getEncSize());
        size = frame->getEncSize();
#ifdef ENABLE_DEBUG_LOGGING
        printf("[Media-VOC]: VocoderRead Rec Ring Size=[%d] Enc Ring Size=[%d]\n",recorderRing.getQueueSize(),encoderRing.getQueueSize());
#endif
        delete frame;
    }
    else
    {
        frameReadState = FRAME_READ_COMPLETE;
#ifdef ENABLE_DEBUG_LOGGING
        printf("[Media-VOC]: VocoderRead no frames - return 0\n");
#endif
    }
    return size;
}
void VocoderStartTimer(unsigned long msec)
{
    printf("[Media-VOC]: VocoderStartTimer\n");
    timerMillis = msec;
    // Start the timer thread
    timerThread_create_rc = pthread_create(&timerThread, NULL, doTimer, NULL);
    return;
}
void VocoderStopTimer()
{
    printf("[Media-VOC]: VocoderStopTimer\n");
    timerExitPending++;
    return;
}
void VocoderRegisterTimerCallback(VocoderTimerCallback callback)
{
    printf("[Media-VOC]:VocoderRegisterTimerCallback\n");
    pthread_mutex_lock (&timerCBMutex);
    timerCallback = callback;
    pthread_mutex_unlock (&timerCBMutex);
    return;
}
void VocoderStatus(int *status)
{
    printf("[Media-VOC]: VocoderStatus NOT SUPPORTED\n");
    return;
}

void VocoderloadLib()
{
    if(!isLoaded)
    {
        isLoaded = TRUE;
        InitializeiOSAudioHandler();
    }
}

/****************iOS Call backs*************************/
void AudioHandlerFrameAcquiredCB(uint8 *data, mvs_frame_info_type *frame_info, uint16 len, mvs_pkt_status_type *status)
{
    // This is going to get called when Audio Handler has recorded frames for us to process.
    // We need to return ASAP
    if(recorderState == VOC_REC_STATE_RECORDING)
    {
        Frame *frame = new Frame(len, len);
        if (frame == NULL) {
            printf("[Media-VOC]:AudioHandlerFrameAcquiredCB Mem alloc failed for frame, return\n");
            return;
        }
        unsigned long ts = time_in_millis();
        memcpy(frame->getPcmBuffer(), data, len);
        frame->setEncSize((int)frame_info->length);
        frame->setTimestamp(ts);
        encoderRing.enqueue(frame);
    }
    else
    {
    }
}

void AudioHandlerFramePlayedCB(uint8 *data, mvs_frame_info_type *frame_info, mvs_pkt_status_type *status)
{
    VocoderPlayerCallback pCallback = NULL;
    long nowTimestamp = 0;
    if(playOutRequest)
    {
        lastPlayOutTime = time_in_millis();
        playOut++;
        playOutRequest = FALSE;
    }
    Frame *frame = playerRing.dequeueNonblock();
    if (frame)
    {
        //record when we started playback this time
        playInStartedTime = time_in_millis();
        playOutRequest = TRUE;
        playIn++;
        memcpy(data, frame->getPcmBuffer(), frame->getEncSize());
        frame_info->length = frame->getEncSize();
        *status = 0;
        delete frame;
    }
    else
    {
        if ((decode_cnt == playOut) &&
                (playInStartedTime > 0))
        {
            nowTimestamp = time_in_millis();
            if((nowTimestamp > playInStartedTime + playbackDurationMs) &&
               (nowTimestamp > lastPlayOutTime + playbackDurationMs))
            {
                pthread_mutex_lock (&playerCBMutex);
                pCallback = playerCallback;
                pthread_mutex_unlock (&playerCBMutex);
                bool isPlaybackDone =false;
                if(pCallback)
                {
                    isPlaybackDone=(*pCallback)(decode_cnt,playOut);
                }
                if(isPlaybackDone)
                {
                    resetPlaybackOptimizationCounters();
                }
            }
        }
    }
}
/****************Private Functions**********************/
void InitializeiOSAudioHandler()
{
    printf("[Media-VOC]: InitializeiOSAudioHandler\n");
    AudioHandler *audioHandler = AudioHandler::GetInstance();
    audioHandler->setCBFunctions((void *)AudioHandlerFrameAcquiredCB, (void *)AudioHandlerFramePlayedCB);
    audioHandler->InitializeiOSAudioHandler();
}

void StartAudioUnit()
{
    AudioHandler *audioHandler = AudioHandler::GetInstance();

#ifdef ENABLE_DEBUG_LOGGING
    uint16 newLen = audioHandler->getBufferSize();
    playbackDurationMs = audioHandler->getAudioDuration()*1000;
    printf("[Media-VOC]: StartAudioUnit bufferSize = %d and playBackDuration=%d\n",newLen,playbackDurationMs);
#endif

    if(VOC_RET_SUCCESS != audioHandler->startAudioUnit())
    {
        printf("[Media-VOC]: *********************Failed to StartAudioUnit***************\n");
    }
}
    
void StopAudioUnit()
{
    AudioHandler *audioHandler = AudioHandler::GetInstance();
    printf("[Media-VOC]: StopAudioUnit\n");
    audioHandler->stopProcessingAudioAsync();
}

void CreateCodec(Vocoder_ConfigParamType param)
{
    codec = build_codec();
    if(codec)
    {
        codec->init(param.codec, param.rate, codecArgs);
        printf("[Media-VOC]: Codec created CodecType[%d] Codec Rate[%d] SampleRate[%d] PCM Size[%d] Enc size[%d]\n",
               param.codec, param.rate,codec->getSampleRate(),codec->getPcmSize(),codec->getEncSize());
        AudioHandler *audioHandler = AudioHandler::GetInstance();
        int encSize = audioHandler->getBufferSize();
        PCMEncGenerator = new GenerateFrame(encSize,codec->getPcmSize()*sizeof(short));
        PCMDecGenerator = new GenerateFrame(codec->getPcmSize()*sizeof(short),encSize);
    }
}
    
void DeleteCodec()
{
    if (codec)
    {
        printf("[Media-VOC]: Codec Deleted\n");
        delete codec;
        codec = NULL;
        if(PCMEncGenerator)
        {
            delete PCMEncGenerator;
            PCMEncGenerator = NULL;
        }
        if(PCMDecGenerator)
        {
            delete PCMDecGenerator;
            PCMDecGenerator = NULL;
        }
    }
}

void VocoderUpdateSilentModeSettings(bool isON)
{
    AudioHandler *audioHandler = AudioHandler::GetInstance();
    audioHandler->updateSilentModeSettings(isON);
}

void VocoderInitAudioSessionPropertyForSender()
{
    AudioHandler *audioHandler = AudioHandler::GetInstance();
    if(audioHandler != NULL)
    {
        audioHandler->initAudioSessionPropertyForSender();
    }
    else
    {
        printf("[Media-VOC]: MEDIA_ERROR audioHandler is NULL\n");
    }
}

void resetPlaybackOptimizationCounters()
{
    decode_cnt = 0;
    playIn = 0;
    playOut = 0;
    playInStartedTime = 0;
    lastPlayOutTime = 0;
    playOutRequest = FALSE;
}

void flushRecordBuffer()
{
    encoderRing.clear();
    recorderRing.clear();
    if(PCMEncGenerator)
    {
        PCMEncGenerator->ResetGenData();
    }
}