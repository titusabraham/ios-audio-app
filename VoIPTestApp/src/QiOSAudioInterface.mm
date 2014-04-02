//
//  QiOSAudioInterface.m
//  Vocoder-iOS
//
//  Created by Abraham, Titus on 12/11/13.
//  Copyright (c) 2013 Abraham, Titus. All rights reserved.
//

#import "QiOSAudioInterface.h"
#import "QiOSAudioHandler.h"

@implementation QiOSAudioInterface

+ (QiOSAudioInterface *)sharedInstance
{
    static QiOSAudioInterface * sSharedInstance;
    static dispatch_once_t onceToken;
    
    dispatch_once(&onceToken, ^{
        sSharedInstance = [[QiOSAudioInterface alloc] init];
        assert(sSharedInstance != nil);
    });
    return sSharedInstance;
}

- (id)init
{
    self = [super init];
    return self;
}

-(void)setToMultiRoute
{

}

-(void)setToPlayAndRecord
{

}
@end
