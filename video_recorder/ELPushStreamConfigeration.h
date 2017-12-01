//
//  ELPushStreamConfigeration.h
//  liveDemo
//
//  Created by 曾陆洋 on 16/2/22.
//  Copyright © 2016年 changba. All rights reserved.
//

#ifndef stream_configeration_h
#define stream_configeration_h

#endif /* stream_configeration_h */

#define kCommonCaptureSessionPreset                                 AVCaptureSessionPreset640x480
#define kHighCaptureSessionPreset                                   AVCaptureSessionPreset1280x720
#define kDesiredWidth                                               360.0f
#define kDesiredHeight                                              640.0f
#define kFrameRate                                                  24.0f
#define kMaxVideoBitRate                                            650 * 1024
#define kAVGVideoBitRate                                            600 * 1024
#define kAudioSampleRate                                            44100
#define kAudioChannels                                              2
#define kAudioBitRate                                               64000
#define kAudioCodecName                                             @"libfdk_aac"
//#define kAudioCodecName                                             @"libvo_aacenc"

#define WINDOW_SIZE_IN_SECS                                         3
#define NOTIFY_ENCODER_RECONFIG_INTERVAL                            15
#define PUB_BITRATE_WARNING_CNT_THRESHOLD                           10

#define LOW_QUALITY_FRAME_RATE                                      15
#define LOW_QUALITY_LIMITS_BIT_RATE                                 300 * 1024
#define LOW_QUALITY_BIT_RATE                                        280 * 1024

#define MIDDLE_QUALITY_FRAME_RATE                                   20
#define MIDDLE_QUALITY_LIMITS_BIT_RATE                              425 * 1024
#define MIDDLE_QUALITY_BIT_RATE                                     400 * 1024


#define kFakePushURL @"rtmp://wspush01.live.changbalive.com/easylive/1098225?wsSecret=d0dcbe7f7c7952b045e948bad02f3ace&wsTime=57bd1aa5"
