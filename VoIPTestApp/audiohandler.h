//
//  audiohandler.h
//  VoIPTestApp
//
//  Created by Abraham, Titus on 3/20/14.
//  Copyright (c) 2014 Qualcomm. All rights reserved.
//

#ifndef __VoIPTestApp__audiohandler__
#define __VoIPTestApp__audiohandler__

#ifdef __cplusplus
extern "C" {
#endif

    void InitializeiOSAudioHandler();
    void StartRecording();
    void StopRecording();
    void StartPlayingRemote();
    void StartPlayingVOIP();
    void StartPlayingRandomTone();
    void StopPlaying();
    int ArePCMThereToBePlayed();
    
#ifdef __cplusplus
}
#endif

#endif /* defined(__VoIPTestApp__audiohandler__) */
