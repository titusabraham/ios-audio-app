//
//  ViewController.m
//  VoIPTestApp
//
//  Created by Abraham, Titus on 3/20/14.
//  Copyright (c) 2014 Qualcomm. All rights reserved.
//

#import "ViewController.h"
#import "audiohandler.h"
#import <AudioToolbox/AudioToolbox.h>

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    InitializeiOSAudioHandler();
	// Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)recordStart:(UIButton *)sender {
    NSLog(@"recordStart");
    /*Invoke the audio unit to give the PCM files to be queued for later playing*/
    StartRecording();
}

- (IBAction)recordStop:(UIButton *)sender {
    NSLog(@"recordStop");
    /*Stop audio unit*/
    StopRecording();
}

- (IBAction)voIPPlay:(UIButton *)sender {
    NSLog(@"voIPPlay");
    if(ArePCMThereToBePlayed())
    {
        StartPlayingVOIP();
    }
    else
    {
        NSLog(@"no PCM Frames available");
    }
}

- (IBAction)randomTone:(UIButton *)sender {
    NSLog(@"play Random Tone");
    StartPlayingRandomTone();
}

- (IBAction)stopPlaying:(UIButton *)sender {
}
@end
