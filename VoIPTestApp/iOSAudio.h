//
//  iOSAudio.h
//  VoIPTestApp
//
//  Created by Abraham, Titus on 3/20/14.
//  Copyright (c) 2014 Qualcomm. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "audiohandler.h"

@interface iOSAudio : NSObject
- (id)init;
-(void)playPCMVOIP;
-(void)playPCMRemote;
-(void)playRandomTone;
@end
