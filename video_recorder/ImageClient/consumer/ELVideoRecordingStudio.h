//
//  ELVideoRecordingStudio.h
//  liveDemo
//
//  Created by apple on 16/3/4.
//  Copyright © 2016年 changba. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ELPushStreamMetadata.h"

@protocol ELVideoRecordingStudioDelegate <NSObject>

@required
- (void) onConnectSuccess;

- (void) onConnectFailed;

- (void) adaptiveVideoQuality:(int) videoQuality;

- (void) adaptiveVideoMaxBitrate:(int)maxBitrate avgBitrate:(int)avgBitrate fps:(int)fps;

- (void) publishTimeOut;

@optional
- (void) statisticsCallbackWithStartTimeMills:(double) startTimeMills connectTimeMills:(int) connectTimeMills publishDurationInSec:(int)publishDurationInSec discardFrameRatio:(float) discardFrameRatio publishAVGBitRate:(float)publishAVGBitRate expectedBitRate:(float) expectedBitRate adaptiveBitrateChart:(NSString*)bitRateChangeChart;

- (void) statisticsCallbackWithSendBitrate:(int)sendBitrate compressedBitrate:(int)compressedBitrate;

@end

@interface ELVideoRecordingStudio : NSObject

@property (nonatomic, weak) id<ELVideoRecordingStudioDelegate> recordingStudioDelegate;

- (id) initWithMeta:(ELPushStreamMetadata *)metadata;

- (void) start;

- (void) stop;

- (float) getDiscardFrameRatio;

- (float)getRealTimePublishBitRate;

- (float)getRealTimeCompressedBitRate;

- (double) startConnectTimeMills;

- (BOOL)isStarted;

@end
