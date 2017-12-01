//
//  ELPushStreamMetadata.h
//  liveDemo
//
//  Created by 曾陆洋 on 16/2/1.
//  Copyright © 2016年 changba. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface ELPushStreamMetadata : NSObject

@property(nonatomic,copy)  NSString *rtmpUrl;

@property(nonatomic,assign) NSInteger videoWidth;

@property(nonatomic,assign) NSInteger videoHeight;

@property(nonatomic,assign) NSInteger videoFrameRate;

@property(nonatomic,assign) NSInteger videoBitRate;

@property(nonatomic,assign) NSInteger audioSampleRate;

@property(nonatomic,assign) NSInteger audioChannels;

@property(nonatomic,assign) NSInteger audioBitRate;

@property(nonatomic,copy) NSString *audioCodecName;

@property(nonatomic,assign) NSInteger adaptiveBitrateWindowSizeInSecs;
@property(nonatomic,assign) NSInteger adaptiveBitrateEncoderReconfigInterval;
@property(nonatomic,assign) NSInteger adaptiveBitrateWarCntThreshold;
@property(nonatomic,assign) NSInteger adaptiveMinimumBitrate;
@property(nonatomic,assign) NSInteger adaptiveMaximumBitrate;

@property (nonatomic, assign) NSInteger qualityStrategy;


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

+ (char*)nsstring2char:(NSString *)path;

@end
