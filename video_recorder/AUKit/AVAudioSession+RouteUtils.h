//
//  AVAudioSession+RouteUtils.h
//  liveDemo
//
//  Created by huang wei on 15/12/16.
//  Copyright © 2015年 changba. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>

@interface AVAudioSession (RouteUtils)

- (BOOL)usingBlueTooth;

- (BOOL)usingWiredMicrophone;

@end
