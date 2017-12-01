//
//  KTVAUGraphRecorder.h
//  ktv
//
//  Created by liumiao on 11/25/14.
//
//

#import "KTVAUGraphController.h"

#define KeyVocalParamHumanVolume @"KeyVocalParamHumanVolume"
#define KeyVocalParamMusicVolume @"KeyVocalParamMusicVolume"
#define KeyVocalParamEffect @"KeyVocalParamEffect"

#define VocalParamHumanVolumeMin -30.f
#define VocalParamHumanVolumeMax 6.f
#define VocalParamHumanVolumeDefault 0.f

#define VocalParamMusicVolumeMin -30.f
#define VocalParamMusicVolumeMax 6.f
#define VocalParamMusicVolumeDefault 0.f

@protocol KTVAUGraphRecorderDelegate
- (void) recordDidReceiveBuffer:(AudioBuffer)buffer;
@end

@interface KTVAUGraphRecorder : KTVAUGraphController
{
@public
//    // 录音文件的file ref
//    ExtAudioFileRef audioOrigianlAudioFile;
    // 用户是否插入耳机
    BOOL isHeadSet;
    AudioUnit lastUnit;
    AudioUnit mixerUnit;
    AudioUnit dspUnit;
}
@property (nonatomic, weak) id<KTVAUGraphRecorderDelegate> delegate;

// 升降调  -3 -> +3, 0
@property (nonatomic, assign) NSInteger pitchLevel;
// 人声音量 DB
@property (nonatomic, assign) Float32 humanVolumeDB;
// 伴奏音量 DB
@property (nonatomic, assign) Float32 musicVolumeDB;

// 更改音效
- (void)applyEffect:(KTVEffectCategory)effectCategory;

- (id)initWithRecordFilePath:(NSString *)filePath;

- (void)startRecord;

- (void)stopRecord;

- (void)runDSP:(AudioBufferList *)ioData;

// 播放伴奏文件，支持循环/非循环播放
- (void)playMusicFile:(NSString *)filePath;
// 暂停播放当前伴奏
- (void)pauseMusicPlay;
// 继续播放当前伴奏
- (void)resumeMusicPlay;
// 停止播放当前伴奏
- (void)stopMusicPlay;
// 伴奏playedFrame的处理
- (void)musicPlayFrames:(UInt32)numberFrames;
// 插拔耳机后的处理
- (void)adjustOnRouteChange;
// 拔掉耳机进入外放模式
- (void)changeToSpeakerMode;
// 插入耳机进入耳机模式
- (void)changeToHeadSetMode;
// 获取当前伴奏播放的时间点
- (float)musicPlayingTime;

- (void)resetParam;

- (void)testEffect:(NSInteger)testCaseIndex;
- (void)testPitch:(NSInteger)testCaseIndex;

@end
