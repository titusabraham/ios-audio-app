//
//  QiOSAudioInterface.h
//  Vocoder-iOS
//
//  Created by Abraham, Titus on 12/11/13.
//  Copyright (c) 2013 Abraham, Titus. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface QiOSAudioInterface : NSObject
+ (QiOSAudioInterface*)sharedInstance;
-(void)setToMultiRoute;
-(void)setToPlayAndRecord;

@end
