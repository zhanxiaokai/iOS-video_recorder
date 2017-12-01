//
//  ELVideoRecordingStudio.m
//  liveDemo
//
//  Created by apple on 16/3/4.
//  Copyright © 2016年 changba. All rights reserved.
//

#import "ELVideoRecordingStudio.h"
#import "video_consumer_thread.h"

static int on_publish_timeout_callback(void *context) {
    NSLog(@"PublishTimeoutCallback...");
    ELVideoRecordingStudio* recordingStudio = (__bridge ELVideoRecordingStudio*) context;
    [[recordingStudio recordingStudioDelegate] publishTimeOut];
    return 1;
}

static int on_publish_statistics_callback(long startTimeMills, int connectTimeMills, int publishDurationInSec, float discardFrameRatio, float publishAVGBitRate, float expectedBitRate, char* adaptiveBitrateChart, void *context) {
    ELVideoRecordingStudio* recordingStudio = (__bridge ELVideoRecordingStudio*) context;
    NSString* bitRateChangeChart = [NSString stringWithUTF8String:adaptiveBitrateChart];
    if ([recordingStudio recordingStudioDelegate] && [[recordingStudio recordingStudioDelegate] respondsToSelector:@selector(statisticsCallbackWithStartTimeMills:connectTimeMills:publishDurationInSec:discardFrameRatio:publishAVGBitRate:expectedBitRate:adaptiveBitrateChart:)]) {
        [[recordingStudio recordingStudioDelegate] statisticsCallbackWithStartTimeMills:[recordingStudio startConnectTimeMills] connectTimeMills:connectTimeMills publishDurationInSec:publishDurationInSec discardFrameRatio:discardFrameRatio publishAVGBitRate:publishAVGBitRate expectedBitRate:expectedBitRate adaptiveBitrateChart:bitRateChangeChart];
    }
    
    return 1;
}

static int on_adaptive_bitrate_callback(PushVideoQuality videoQuality, void *context) {
    ELVideoRecordingStudio* recordingStudio = (__bridge ELVideoRecordingStudio*) context;
    [[recordingStudio recordingStudioDelegate] adaptiveVideoQuality:videoQuality];
    return 1;
}

static int hot_adaptive_bitrate_callback(int maxBitrate, int avgBitrate, int fps, void *context) {
    ELVideoRecordingStudio* recordingStudio = (__bridge ELVideoRecordingStudio*) context;
    if (avgBitrate < 0) {
        [[recordingStudio recordingStudioDelegate] adaptiveVideoQuality:INVALID_LIVE_FLAG];
    } else {
        [[recordingStudio recordingStudioDelegate] adaptiveVideoMaxBitrate:maxBitrate avgBitrate:avgBitrate fps:fps];
    }
    
    return 1;
}

static int statistics_bitrate_callback(int sendBitrate, int compressedBitrate, void *context) {
    ELVideoRecordingStudio* recordingStudio = (__bridge ELVideoRecordingStudio*) context;
    if ([recordingStudio recordingStudioDelegate] && [[recordingStudio recordingStudioDelegate] respondsToSelector:@selector(statisticsCallbackWithSendBitrate:compressedBitrate:)]) {
        [[recordingStudio recordingStudioDelegate] statisticsCallbackWithSendBitrate:sendBitrate compressedBitrate:compressedBitrate];
    }
    
    return 1;
}


@implementation ELVideoRecordingStudio
{
    VideoConsumerThread*                _consumer;
    dispatch_queue_t                    _consumerQueue;
    
    ELPushStreamMetadata*               _metaData;
	
	double                              _startConnectTimeMills;
    int                                 _historyBitrate;
    NSTimeInterval                      _lastStopTime;
}

- (double) startConnectTimeMills;{
    return _startConnectTimeMills;
}

- (id) initWithMeta:(ELPushStreamMetadata *)metadata;
{
    self = [super init];
    if(self){
        _metaData = metadata;
        _consumerQueue = dispatch_queue_create("com.easylive.RecordingStudio.consumerQueue", NULL);
    }
    return self;
}

- (void) start;
{
    if(NULL == _consumer){
        _consumer = new VideoConsumerThread();
    }
    __weak __typeof(self) weakSelf = self;
    dispatch_async(_consumerQueue, ^(void) {
        __strong __typeof(weakSelf) strongSelf = weakSelf;
        char* videoOutputURI = [ELPushStreamMetadata nsstring2char:strongSelf->_metaData.rtmpUrl];
        char* audioCodecName = [ELPushStreamMetadata nsstring2char:strongSelf->_metaData.audioCodecName];
        strongSelf->_startConnectTimeMills = [[NSDate date] timeIntervalSince1970] * 1000;
        LivePacketPool::GetInstance()->initRecordingVideoPacketQueue();
        LivePacketPool::GetInstance()->initAudioPacketQueue((int)strongSelf->_metaData.audioSampleRate);
        LiveAudioPacketPool::GetInstance()->initAudioPacketQueue();
        std::map<std::string, int> configMap;
        configMap["adaptiveBitrateWindowSizeInSecs"] = (int)strongSelf->_metaData.adaptiveBitrateWindowSizeInSecs;
        configMap["adaptiveBitrateEncoderReconfigInterval"] = (int)strongSelf->_metaData.adaptiveBitrateEncoderReconfigInterval;
        configMap["adaptiveBitrateWarCntThreshold"] = (int)strongSelf->_metaData.adaptiveBitrateWarCntThreshold;
        configMap["adaptiveMinimumBitrate"] = (int)strongSelf->_metaData.adaptiveMinimumBitrate/1024;
        configMap["adaptiveMaximumBitrate"] = (int)strongSelf->_metaData.adaptiveMaximumBitrate/1024;
        if (_historyBitrate != 0 && _historyBitrate != -1           && _historyBitrate>=configMap["adaptiveMinimumBitrate"]    &&
            _lastStopTime                                           &&
            ([NSDate date].timeIntervalSince1970-_lastStopTime)<60) {
            configMap["adaptiveHistoryBitrate"] = _historyBitrate;
        }
        
        int consumerInitCode = strongSelf->_consumer->init(videoOutputURI, (int)strongSelf->_metaData.videoWidth, (int)strongSelf->_metaData.videoHeight, (int)strongSelf->_metaData.videoFrameRate, (int)strongSelf->_metaData.videoBitRate, (int)strongSelf->_metaData.audioSampleRate, (int)strongSelf->_metaData.audioChannels, (int)strongSelf->_metaData.audioBitRate, audioCodecName,
            (int)strongSelf->_metaData.qualityStrategy, configMap);
        delete[] audioCodecName;
        delete[] videoOutputURI;
        
        if(consumerInitCode >= 0) {
            strongSelf->_consumer->registerPublishTimeoutCallback(on_publish_timeout_callback, (__bridge void *)self);
            strongSelf->_consumer->registerPublishStatisticsCallback(on_publish_statistics_callback, (__bridge void *)self);
            strongSelf->_consumer->registerAdaptiveBitrateCallback(on_adaptive_bitrate_callback, (__bridge void *)self);
            strongSelf->_consumer->registerHotAdaptiveBitrateCallback(hot_adaptive_bitrate_callback, (__bridge void *)self);
            strongSelf->_consumer->registerStatisticsBitrateCallback(statistics_bitrate_callback, (__bridge void *)self);
            strongSelf->_consumer->startAsync();
            NSLog(@"cosumer open video output success...");
            [strongSelf.recordingStudioDelegate onConnectSuccess];
        } else {
            NSLog(@"cosumer open video output failed...");
            LivePacketPool::GetInstance()->destoryRecordingVideoPacketQueue();
            LivePacketPool::GetInstance()->destoryAudioPacketQueue();
            LiveAudioPacketPool::GetInstance()->destoryAudioPacketQueue();
            [strongSelf.recordingStudioDelegate onConnectFailed];
        }
    });
}


- (float) getDiscardFrameRatio;
{
    if (_consumer) {
        return _consumer->getDiscardFrameRatio();
    }
    return 0.0f;
}

- (float)getRealTimePublishBitRate {
    if (_consumer) {
        return _consumer->getRealTimePublishBitRate();
    }
    return 0.0f;
}

- (float)getRealTimeCompressedBitRate {
    if (_consumer) {
        return _consumer->getRealTimeCompressedBitRate();
    }
    return 0.0f;
}

- (void) stop;
{
    _historyBitrate = PublisherRateFeedback::GetInstance()->getQualityAgent()->getBitrate();
    _lastStopTime = [NSDate date].timeIntervalSince1970;
    
    if (_consumer) {
        _consumer->stop();
        delete _consumer;
        _consumer = NULL;
    }
}

- (BOOL)isStarted {
    return _consumer != NULL;
}

@end
