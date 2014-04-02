//
//  audiohandler.mm
//  VoIPTestApp
//
//  Created by Abraham, Titus on 3/20/14.
//  Copyright (c) 2014 Qualcomm. All rights reserved.
//

#include "audiohandler.h"
#import "QiOSAudioHandler.h"
#include "frame.h"
#include "ring.h"
#import <AVFoundation/AVFoundation.h>

AVAudioPlayer *audioPlayerTone;
NSURL *toneURL;

static RingBuffer<Frame> RecorderRing(5000);


static AudioHandler* myaudio= NULL;
static void AudioHandlerFrameAcquiredCB(uint8 *data, frame_info_type *frame_info, uint16 len, int *status);
static void AudioHandlerFramePlayedCB(uint8 *data, frame_info_type *frame_info, int *status);

void AudioHandlerFrameAcquiredCB(uint8 *data, frame_info_type *frame_info, uint16 len, int *status)
{
    printf("AudioHandlerFrameAcquiredCB\n");
    Frame *frame = new Frame(len, len);
    memcpy(frame->getPcmBuffer(), data, len);
    frame->setEncSize(frame_info->length);
    RecorderRing.enqueue(frame);
}

void AudioHandlerFramePlayedCB(uint8 *data, frame_info_type *frame_info, int *status)
{
    printf("AudioHandlerFramePlayedCB\n");
    Frame *frame = RecorderRing.dequeueNonblock();
    if (frame)
    {
        memcpy(data, frame->getPcmBuffer(), frame->getEncSize());
        frame_info->length = frame->getEncSize();
        *status = 0;
        delete frame;
    }
    else
    {
        myaudio->setPlayingState(FALSE);
    }
}

void StartAudioUnit()
{
    if(0 != myaudio->startAudioUnit())
    {
        printf("*********************Failed to StartAudioUnit***************\n");
    }
}

void StopAudioUnit()
{
    printf("[Media-VOC]: StopAudioUnit\n");
    myaudio->stopProcessingAudio();
}

///external
int ArePCMThereToBePlayed()
{
    return RecorderRing.getQueueSize();
}
void StartRecording()
{
    printf("StartRecording\n");
    RecorderRing.clear();
    myaudio->setRecordingState(TRUE);
    StartAudioUnit();
}

void StopRecording()
{
    myaudio->setRecordingState(FALSE);
    StopAudioUnit();
    printf("Size of the recorder Ring is %d",RecorderRing.getQueueSize());
}

void StartPlayingRemote()
{
    printf("StartPlayingRemote\n");
    myaudio->setRecordingState(FALSE);
    myaudio->setPlayingState(TRUE);
}

void StartPlayingVOIP()
{
    printf("StartPlayingVOIP\n");
    myaudio->setRecordingState(FALSE);
    myaudio->setPlayingState(TRUE);
    StartAudioUnit();
}

void StartPlayingRandomTone()
{
    printf("StartPlayingRandomTone\n");
    toneURL = [[NSBundle mainBundle] URLForResource:@"Tone2" withExtension:@"wav"];
    NSError *error;
    audioPlayerTone = [[AVAudioPlayer alloc] initWithContentsOfURL:toneURL error:&error];
    if(!audioPlayerTone)
    {
        printf("nil audioplayer tone, cant play tones\n");
    }
    audioPlayerTone.numberOfLoops = 0; //0 means play only once
    [audioPlayerTone prepareToPlay];
    [audioPlayerTone play];
}

void StopPlaying()
{
    printf("StopPlaying\n");
    myaudio->setPlayingState(FALSE);
    StopAudioUnit();
}

void InitializeiOSAudioHandler()
{
    printf("InitializeiOSAudioHandler\n");
    if(myaudio!= NULL)
    {
        return;
    }
    myaudio = AudioHandler::GetInstance();
    myaudio->setCBFunctions((void *)AudioHandlerFrameAcquiredCB, (void *)AudioHandlerFramePlayedCB);
    myaudio->InitializeiOSAudioHandler();
    RecorderRing.clear();
}