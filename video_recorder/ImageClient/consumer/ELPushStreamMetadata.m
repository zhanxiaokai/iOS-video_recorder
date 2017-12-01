//
//  ELPushStreamMetadata.m
//  liveDemo
//
//  Created by 曾陆洋 on 16/2/1.
//  Copyright © 2016年 changba. All rights reserved.
//

#import "ELPushStreamMetadata.h"

@implementation ELPushStreamMetadata

- (instancetype)initWithRtmpUrl:(NSString *)rtmpUrl
                     videoWidth:(NSInteger)videoWidth
                    videoHeight:(NSInteger)videoHeight
                 videoFrameRate:(NSInteger)videoFrameRate
                   videoBitRate:(NSInteger)videoBitRate
                audioSampleRate:(NSInteger)audioSampleRate
                  audioChannels:(NSInteger)audioChannels
                   audioBitRate:(NSInteger)audioBitRate
                 audioCodecName:(NSString *)audioCodecName
                qualityStrategy:(NSInteger)qualityStrategy
adaptiveBitrateWindowSizeInSecs:(NSInteger)adaptiveBitrateWindowSizeInSecs
adaptiveBitrateEncoderReconfigInterval:(NSInteger)adaptiveBitrateEncoderReconfigInterval
 adaptiveBitrateWarCntThreshold:(NSInteger)adaptiveBitrateWarCntThreshold
         adaptiveMinimumBitrate:(NSInteger)adaptiveMinimumBitrate
         adaptiveMaximumBitrate:(NSInteger)adaptiveMaximumBitrate;
{
    self = [super init];
    if(self)
    {
        self.rtmpUrl = rtmpUrl;
        self.videoWidth = videoWidth;
        self.videoHeight = videoHeight;
        self.videoFrameRate = videoFrameRate;
        self.videoBitRate = videoBitRate;
        self.audioSampleRate = audioSampleRate;
        self.audioChannels = audioChannels;
        self.audioBitRate = audioBitRate;
        self.audioCodecName = audioCodecName;
        self.qualityStrategy = qualityStrategy;
        self.adaptiveBitrateWindowSizeInSecs = adaptiveBitrateWindowSizeInSecs;
        self.adaptiveBitrateEncoderReconfigInterval = adaptiveBitrateEncoderReconfigInterval;
        self.adaptiveBitrateWarCntThreshold = adaptiveBitrateWarCntThreshold;
        self.adaptiveMinimumBitrate = adaptiveMinimumBitrate;
        self.adaptiveMaximumBitrate = adaptiveMaximumBitrate;
    }
    return self;
}

#pragma -mark helper

+ (char*)nsstring2char:(NSString *)path
{
    NSUInteger len = [path length];
    char *filepath = (char*)malloc(sizeof(char) * (len + 1));
    
    [path getCString:filepath maxLength:len + 1 encoding:[NSString defaultCStringEncoding]];
    
    return filepath;
}
@end
