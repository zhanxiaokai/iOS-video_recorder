#import "H264HwEncoderHandler.h"
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#import "CommonUtil.h"
#import "live_video_packet_queue.h"
#import "live_packet_pool.h"

#define HEAD_NALU_SEI       [NSData dataWithBytes:"\x06" length:1]
#define HEAD_NALU_I         [NSData dataWithBytes:"\x25" length:1]

@implementation H264HwEncoderHandler

#pragma mark - H264HwEncoderImplDelegate delegare

- (void)gotSpsPps:(NSData*)sps pps:(NSData*)pps timestramp:(Float64)miliseconds fromEncoder:(H264HwEncoderImpl*)encoder
{
    const char bytesHeader[] = "\x00\x00\x00\x01";
    size_t headerLength = 4; //string literals have implicit trailing '\0'
    
    LiveVideoPacket* spsPpsVideoPacket = new LiveVideoPacket();
    size_t length = 2*headerLength+sps.length+pps.length;
    spsPpsVideoPacket->buffer = new unsigned char[length];
    spsPpsVideoPacket->size = int(length);
    memcpy(spsPpsVideoPacket->buffer, bytesHeader, headerLength);
    memcpy(spsPpsVideoPacket->buffer + headerLength, (unsigned char*)[sps bytes], sps.length);
    memcpy(spsPpsVideoPacket->buffer + headerLength + sps.length, bytesHeader, headerLength);
    memcpy(spsPpsVideoPacket->buffer + headerLength*2 + sps.length, (unsigned char*)[pps bytes], pps.length);
    spsPpsVideoPacket->timeMills = 0;
    LivePacketPool::GetInstance()->pushRecordingVideoPacketToQueue(spsPpsVideoPacket);
}

- (void)gotEncodedData:(NSData*)data isKeyFrame:(BOOL)isKeyFrame timestramp:(Float64)miliseconds pts:(int64_t) pts dts:(int64_t) dts fromEncoder:(H264HwEncoderImpl *)encoder
{
    const char bytesHeader[] = "\x00\x00\x00\x01";
    size_t headerLength = 4; //string literals have implicit trailing '\0'
    
    LiveVideoPacket* videoPacket = new LiveVideoPacket();
    
    videoPacket->buffer = new unsigned char[headerLength+data.length];
    videoPacket->size = int(headerLength+data.length);
    memcpy(videoPacket->buffer,bytesHeader, headerLength);
    memcpy(videoPacket->buffer + headerLength, (unsigned char*)[data bytes], data.length);
    videoPacket->timeMills = miliseconds;
    //    videoPacket->pts = pts;
    //    videoPacket->dts = dts;
    LivePacketPool::GetInstance()->pushRecordingVideoPacketToQueue(videoPacket);
}


@end
