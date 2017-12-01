//
//  KTVAUGraphRecorder.m
//  ktv
//
//  Created by liumiao on 11/25/14.
//
//

#import "KTVAUGraphRecorder.h"
#import "CAXException.h"
#import <mach/mach.h>
#import <mach/mach_time.h>
#import "AVAudioSession+RouteUtils.h"
#import "AQPlayer.h"
#import "autotune.h"
#import "doubleyou.h"
#import "dynamic_delay.h"

static OSStatus	ioUnitCallBack (void                        *inRefCon,
                                AudioUnitRenderActionFlags 	*ioActionFlags,
                                const AudioTimeStamp 		*inTimeStamp,
                                UInt32 						inBusNumber,
                                UInt32 						inNumberFrames,
                                AudioBufferList              *ioData)
{
    __unsafe_unretained KTVAUGraphRecorder *THIS = (__bridge KTVAUGraphRecorder *)inRefCon;
    OSStatus result = noErr;
    //将字节写入文件
    AudioUnitRender(THIS->lastUnit,
                    ioActionFlags,
                    inTimeStamp,
                    0,
                    inNumberFrames,
                    ioData);
    if (!THIS->isHeadSet) {
        for (UInt32 i=0; i<ioData->mNumberBuffers; ++i)
            memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);
    }
    return result;
}

static OSStatus playMusicRenderNotify(void *							inRefCon,
                                      AudioUnitRenderActionFlags *	ioActionFlags,
                                      const AudioTimeStamp *			inTimeStamp,
                                      UInt32							inBusNumber,
                                      UInt32							inNumberFrames,
                                      AudioBufferList *				ioData)
{
    // !!! this method is timing sensitive, better not add any wasting time code here, even nslog
    if (*ioActionFlags & kAudioUnitRenderAction_PostRender) {
        __unsafe_unretained KTVAUGraphRecorder *THIS = (__bridge KTVAUGraphRecorder*)inRefCon;
        [THIS musicPlayFrames:inNumberFrames];
    }
    return noErr;
}

// 负责包房边录边存的rendernotify，录音时并不添加这个listener
static OSStatus mixerRenderNotify(void *							inRefCon,
                                  AudioUnitRenderActionFlags *	ioActionFlags,
                                  const AudioTimeStamp *			inTimeStamp,
                                  UInt32							inBusNumber,
                                  UInt32							inNumberFrames,
                                  AudioBufferList *				ioData)
{
    __unsafe_unretained KTVAUGraphRecorder *THIS = (__bridge KTVAUGraphRecorder *)inRefCon;
    OSStatus result = noErr;
    //将字节写入文件
    if (*ioActionFlags & kAudioUnitRenderAction_PostRender) {
        
//        result = ExtAudioFileWriteAsync(THIS->audioOrigianlAudioFile, inNumberFrames, ioData);
        if (THIS.delegate)
        {
            AudioBuffer buffer = ioData->mBuffers[0];
            [THIS.delegate recordDidReceiveBuffer:buffer];
        }
    }
    return result;
}

static OSStatus	dspUnitCallBack (void                        *inRefCon,
                                 AudioUnitRenderActionFlags 	*ioActionFlags,
                                 const AudioTimeStamp 		*inTimeStamp,
                                 UInt32 						inBusNumber,
                                 UInt32 						inNumberFrames,
                                 AudioBufferList              *ioData)
{
    __unsafe_unretained KTVAUGraphRecorder *THIS = (__bridge KTVAUGraphRecorder *)inRefCon;
    OSStatus result = noErr;
    //将字节写入文件
    AudioUnitRender(THIS->dspUnit,
                    ioActionFlags,
                    inTimeStamp,
                    0,
                    inNumberFrames,
                    ioData);
    [THIS runDSP:ioData];
    return result;
}

// 监测声音输出Route 改变，目前主要用途是监测耳机的插拔状态，来控制声音是否通过手机本身的喇叭外放
static void propListener_routechange(void *                  inClientData,
                                     AudioSessionPropertyID  inID,
                                     UInt32                  inDataSize,
                                     const void *            inData)
{
    KTVAUGraphRecorder *THIS = (__bridge KTVAUGraphRecorder*)inClientData;
    if (inID == kAudioSessionProperty_AudioRouteChange)
    {
        if (THIS != NULL) {
            [THIS adjustOnRouteChange];
        }
    }
}

@implementation KTVAUGraphRecorder
{
    AUNode _C32fTo16iNode;
    AUNode _C16iTo32fNode;
    AUNode dsp_C32fTo16iNode;
    AUNode dsp_C16iTo32fNode;
    AUNode IO_C32iTo32fNode;
    AUNode _IONode;
    AUNode _musicPlayerNode;
    
    AUNode _reverbNode;
    AUNode _pitchNode;
    AUNode _bandEQNode;
    AUNode _limiterNode;
    AUNode _compressorNode;
    AUNode _timePitchNode;
    
    UInt32 playedFrames;
    UInt32 totalFramesToPlay;
    
//    NSString *_destinationFilePath;
    
    //伴奏相关
    NSString *_musicFilePath;
    ExtAudioFileRef _musicExtFileRef;
    AudioFileID _musicFileID;
    AudioStreamBasicDescription _musicFileASBD;
    UInt32 _musicFileTotalFrames;
    
    AQPlayer *_aqplayer;
    
    AutoTune *_autoTune;
    
    DoubleYou *_doubleYou;
    
    KTVEffectMode _effectMode;
    
    BOOL _isGraphAddedEffectNodes;
    BOOL _isMusicPlaying;
}


- (id)initWithRecordFilePath:(NSString *)filePath
{
    self = [super init];
    if (self) {
//        _destinationFilePath = filePath;
        _humanVolumeDB  = 0;
        _musicVolumeDB = 0;
        _effectMode = KTVEffectModeMake(KTVEffectCategoryPlayOrigin, EQ_LEVEL_DEAULT);
        
        totalFramesToPlay = 0;
        playedFrames = 0;
        _pitchLevel = 0;
        _isMusicPlaying = NO;
        _isGraphAddedEffectNodes = NO;
        _clientFormat32float.mSampleRate = [KTVAUGraphController liveRoomHardwareSampleRate];
        _clientFormat32int.mSampleRate = [KTVAUGraphController liveRoomHardwareSampleRate];
        _clientFormat16int.mSampleRate = [KTVAUGraphController liveRoomHardwareSampleRate];
        [self configureAudioSession];
        [self configureAUGraph];
        [self configureRecorderGraph];
        [self initAutoTune];
        [self initDoubleYou];
        [self initHarmonic];
        [self initDynamicDelays];
        [self prepareForRecord];
    }
    
    return self;
}

- (void)prepareForRecord
{
    [self applyEffect:_effectMode.category];
    self.musicVolumeDB = _musicVolumeDB;
    self.humanVolumeDB = _humanVolumeDB;
}

- (void)dealloc
{
    try {
//        if (audioOrigianlAudioFile) {
//            XThrowIfError(ExtAudioFileDispose(audioOrigianlAudioFile), "augraph recorder closing human voice file failed");
//        }
        if (_musicExtFileRef) {
            XThrowIfError(ExtAudioFileDispose(_musicExtFileRef), "augraph recorder dispose music file failed");
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
    [self clearAllDspTool];
    _IONode = 0;
    _C32fTo16iNode = 0;
    _C16iTo32fNode = 0;
    _musicPlayerNode = 0;
    _bandEQNode = 0;
    _limiterNode = 0;
    _compressorNode = 0;
    _reverbNode = 0;
    _pitchNode = 0;
    _timePitchNode = 0;
    dsp_C16iTo32fNode = 0;
    dsp_C32fTo16iNode = 0;
}

- (void)configureAudioSession
{
    NSError *error = nil;
    
    // Configure the audio session
    AVAudioSession *sessionInstance = [AVAudioSession sharedInstance];
    
    // our default category -- we change this for conversion and playback appropriately
    [sessionInstance setCategory:AVAudioSessionCategoryPlayAndRecord error:&error];
    
    [sessionInstance setPreferredIOBufferDuration:AUDIO_RECORD_BUFFER_DURATION error:&error];
    
    [sessionInstance setPreferredSampleRate:[KTVAUGraphController liveRoomHardwareSampleRate] error:&error];
    
    // activate the audio session
    [sessionInstance setActive:YES error:&error];
}

- (void)configureRecorderGraph
{
    BOOL isPitchSupported = [KTVAUGraphController isPitchSupported];
    
    _IONode = [self graphAddNodeOfType:kAudioUnitType_Output SubType:kAudioUnitSubType_RemoteIO];
    [self configureStandardProperty:_IONode];
    
    _C32fTo16iNode = [self graphAddNodeOfType:kAudioUnitType_FormatConverter SubType:kAudioUnitSubType_AUConverter];
    [self configureStandardProperty:_C32fTo16iNode];
    
    _C16iTo32fNode = [self graphAddNodeOfType:kAudioUnitType_FormatConverter SubType:kAudioUnitSubType_AUConverter];
    [self configureStandardProperty:_C16iTo32fNode];
    
    IO_C32iTo32fNode = [self graphAddNodeOfType:kAudioUnitType_FormatConverter SubType:kAudioUnitSubType_AUConverter];
    [self configureStandardProperty:IO_C32iTo32fNode];
    
    _musicPlayerNode = [self graphAddNodeOfType:kAudioUnitType_Generator SubType:kAudioUnitSubType_AudioFilePlayer];
    [self configureStandardProperty:_musicPlayerNode];
    
    if (isPitchSupported) {
        _timePitchNode = [self graphAddNodeOfType:kAudioUnitType_FormatConverter SubType:kAudioUnitSubType_NewTimePitch];
        [self configureStandardProperty:_timePitchNode];
    }
    
    dsp_C16iTo32fNode = [self graphAddNodeOfType:kAudioUnitType_FormatConverter SubType:kAudioUnitSubType_AUConverter];
    [self configureStandardProperty:dsp_C16iTo32fNode];
    dsp_C32fTo16iNode = [self graphAddNodeOfType:kAudioUnitType_FormatConverter SubType:kAudioUnitSubType_AUConverter];
    [self configureStandardProperty:dsp_C32fTo16iNode];
    
    _reverbNode = [self graphAddNodeOfType:kAudioUnitType_Effect SubType:kAudioUnitSubType_Reverb2];
    [self configureStandardProperty:_reverbNode];
    
    _limiterNode = [self graphAddNodeOfType:kAudioUnitType_Effect SubType:kAudioUnitSubType_PeakLimiter];
    [self configureStandardProperty:_limiterNode];
    
    _compressorNode = [self graphAddNodeOfType:kAudioUnitType_Effect SubType:kAudioUnitSubType_DynamicsProcessor];
    [self configureStandardProperty:_compressorNode];
    
    _bandEQNode = [self graphAddNodeOfType:kAudioUnitType_Effect SubType:kAudioUnitSubType_NBandEQ];
    [self configureStandardProperty:_bandEQNode];
    
    _pitchNode = [self graphAddNodeOfType:kAudioUnitType_FormatConverter SubType:kAudioUnitSubType_NewTimePitch];
    [self configureStandardProperty:_pitchNode];
    
    try {
        AudioUnit C32fTo16iUnit = [self graphUnitOfNode:_C32fTo16iNode];
        // spectial format for converter
        XThrowIfError(AudioUnitSetProperty(C32fTo16iUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &_clientFormat32float, sizeof(_clientFormat32float)), "augraph recorder normal unit set client format error");
        XThrowIfError(AudioUnitSetProperty(C32fTo16iUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &_clientFormat16int, sizeof(_clientFormat16int)), "augraph recorder normal unit set client format error");
        
        AudioUnit C16iTo32fUnit = [self graphUnitOfNode:_C16iTo32fNode];
        // spectial format for converter
        XThrowIfError(AudioUnitSetProperty(C16iTo32fUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &_clientFormat16int, sizeof(_clientFormat16int)), "augraph recorder normal unit set client format error");
        XThrowIfError(AudioUnitSetProperty(C16iTo32fUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &_clientFormat32float, sizeof(_clientFormat32float)), "augraph recorder normal unit set client format error");
        
        AudioUnit musicC16iTo32fUnit = [self graphUnitOfNode:dsp_C16iTo32fNode];
        // spectial format for converter
        XThrowIfError(AudioUnitSetProperty(musicC16iTo32fUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &_clientFormat16int, sizeof(_clientFormat16int)), "augraph recorder normal unit set client format error");
        XThrowIfError(AudioUnitSetProperty(musicC16iTo32fUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &_clientFormat32float, sizeof(_clientFormat32float)), "augraph recorder normal unit set client format error");
        
        AudioUnit musicC32fTo16iUnit = [self graphUnitOfNode:dsp_C32fTo16iNode];
        // spectial format for converter
        XThrowIfError(AudioUnitSetProperty(musicC32fTo16iUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &_clientFormat32float, sizeof(_clientFormat32float)), "augraph recorder normal unit set client format error");
        XThrowIfError(AudioUnitSetProperty(musicC32fTo16iUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &_clientFormat16int, sizeof(_clientFormat16int)), "augraph recorder normal unit set client format error");
        
        AudioUnit IO_C32iTo32fUnit = [self graphUnitOfNode:IO_C32iTo32fNode];
        // spectial format for converter
        XThrowIfError(AudioUnitSetProperty(IO_C32iTo32fUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &_clientFormat32int, sizeof(_clientFormat32int)), "augraph recorder normal unit set client format error");
        XThrowIfError(AudioUnitSetProperty(IO_C32iTo32fUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &_clientFormat32float, sizeof(_clientFormat32float)), "augraph recorder normal unit set client format error");
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
    
    // 连接声音输入和输出
    [self graphAppendHeadNode:IO_C32iTo32fNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    [self graphAppendHeadNode:_IONode inScope:OUTPUT_VOICE_SCOPE inElement:OUTPUT_VOICE_ELEMENT];
    
    // 连接32float转16int转换器
    [self graphInsertNode:_C32fTo16iNode inScope:OUTPUT_VOICE_SCOPE inElement:OUTPUT_VOICE_ELEMENT];
    [self graphInsertNode:_C16iTo32fNode inScope:OUTPUT_VOICE_SCOPE inElement:OUTPUT_VOICE_ELEMENT];
    [self graphInsertNode:dsp_C16iTo32fNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    [self graphInsertNode:dsp_C32fTo16iNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    try {
        XThrowIfError(AUGraphConnectNodeInput(_graph, _IONode, 1, IO_C32iTo32fNode, 0), "graph connect error");
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
    
    // 连接music player unit
    [self graphAppendHeadNode:_musicPlayerNode inScope:MUSIC_VOICE_SCOPE inElement:MUSIC_VOICE_ELEMENT];
    
    if (isPitchSupported) {
        [self graphInsertNode:_timePitchNode inScope:MUSIC_VOICE_SCOPE inElement:MUSIC_VOICE_ELEMENT];
        self.pitchLevel = _pitchLevel;
    }

    mixerUnit = [self graphUnitOfNode:_mixerNode];
    lastUnit = [self graphUnitOfNode:_C16iTo32fNode];
    dspUnit = [self graphUnitOfNode:dsp_C32fTo16iNode];
    
    try {
        XThrowIfError(AUGraphDisconnectNodeInput(_graph, _IONode, 0), "graph disconnect error");
        AURenderCallbackStruct renderCallback;
        renderCallback.inputProc = &ioUnitCallBack;
        renderCallback.inputProcRefCon = (__bridge void *)self;
        XThrowIfError(AUGraphSetNodeInputCallback(_graph, _IONode, 0, &renderCallback), "graph set node input call back error");
        
        XThrowIfError(AUGraphDisconnectNodeInput(_graph, dsp_C16iTo32fNode, 0), "graph disconnect error");
        AURenderCallbackStruct dspRenderCallback;
        dspRenderCallback.inputProc = &dspUnitCallBack;
        dspRenderCallback.inputProcRefCon = (__bridge void *)self;
        XThrowIfError(AUGraphSetNodeInputCallback(_graph, dsp_C16iTo32fNode, 0, &dspRenderCallback), "graph set node input call back error");
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
    
    [self initializeAUGraph];
    [self showAUGraph];
}

#pragma mark - Utilities

//- (void)prepareAudioOrigianlWriteFile
//{
//    try {
//        
//        
//        CFURLRef destinationURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
//                                                                (CFStringRef)_destinationFilePath,
//                                                                kCFURLPOSIXPathStyle,
//                                                                false);
//        // specify codec Saving the output in .m4a format
//        AudioStreamBasicDescription ima4DataFormat;
//        UInt32 formatSize = sizeof(ima4DataFormat);
//        memset(&ima4DataFormat, 0, sizeof(ima4DataFormat));
//        ima4DataFormat.mSampleRate = _clientFormat32float.mSampleRate;
//        ima4DataFormat.mChannelsPerFrame = 2;
//        ima4DataFormat.mFormatID = kAudioFormatAppleIMA4;
//        XThrowIfError(AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, NULL, &formatSize, &ima4DataFormat), "couldn't create IMA4 destination data format");
//        XThrowIfError(ExtAudioFileCreateWithURL(destinationURL,
//                                                kAudioFileCAFType,
//                                                &ima4DataFormat,
//                                                NULL,
//                                                kAudioFileFlags_EraseFile,
//                                                &audioOrigianlAudioFile), "augraph recorder create url error");
//        CFRelease(destinationURL);
//        
//        // set the audio data format of mixer Unit
//        XThrowIfError(ExtAudioFileSetProperty(audioOrigianlAudioFile,
//                                              kExtAudioFileProperty_ClientDataFormat,
//                                              sizeof(_clientFormat16int),
//                                              &_clientFormat16int), "augraph recorder set file format error");
//        
//        // specify codec
//        UInt32 codec = kAppleHardwareAudioCodecManufacturer;
//        XThrowIfError(ExtAudioFileSetProperty(audioOrigianlAudioFile,
//                                              kExtAudioFileProperty_CodecManufacturer,
//                                              sizeof(codec),
//                                              &codec), "augraph recorder set file codec error");
//        
//        XThrowIfError(ExtAudioFileWriteAsync(audioOrigianlAudioFile, 0, NULL), "augraph recorder write file error");
//    } catch (CAXException e) {
//        char buf[256];
//        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
//    }
//}

- (void)addMixerRenderNofity
{
    try {
        if (_graph && [self isNodeInGraph:_C32fTo16iNode])
        {
            AudioUnit unit = [self graphUnitOfNode:_C32fTo16iNode];
            XThrowIfError(AudioUnitAddRenderNotify(unit, &mixerRenderNotify, (__bridge void *)self), "augraph recorder unit add render notify error");
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (void)removeMixerRenderNofify
{
    try {
        if (_graph && [self isNodeInGraph:_C32fTo16iNode])
        {
            AudioUnit unit = [self graphUnitOfNode:_C32fTo16iNode];
            XThrowIfError(AudioUnitRemoveRenderNotify(unit, &mixerRenderNotify, (__bridge void *)self), "augraph recorder remove render notify error");
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (void)removePlayRenderNotify
{
    try {
        if (_graph && [self isNodeInGraph:_musicPlayerNode]) {
            AudioUnit playerUnit = [self graphUnitOfNode:_musicPlayerNode];
            XThrowIfError(AudioUnitRemoveRenderNotify(playerUnit, &playMusicRenderNotify, (__bridge void *)self), "augraph recorder remove render notify error");
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (void)enableEarReturn
{
    [self muteHumanTrack:NO];
}

- (void)disableEarReturn
{
    [self muteHumanTrack:YES];
}

- (void)enableUnitPlayer
{
    [self muteMusicTrack:NO];
}

- (void)disableUnitPlayer
{
    [self muteMusicTrack:YES];
}

- (void)muteHumanTrack:(BOOL)muteTrack
{
    [self setMuted:muteTrack forInput:0];
}

- (void)muteMusicTrack:(BOOL)muteTrack
{
    [self setMuted:muteTrack forInput:1];
}

- (void)musicPlayFrames:(UInt32)numberFrames
{
    playedFrames += numberFrames;
    if (playedFrames >= totalFramesToPlay)
    {
        [self stopMusicPlay];
    }
}

- (float)musicPlayingTime
{
    return (playedFrames / _clientFormat32float.mSampleRate);
}

- (void)resetParam
{
    [self setPitchLevel:0];
}

- (AudioUnitParameterValue)currentMusicVolume
{
    return [self getNodeParameter:_mixerNode inID:kMultiChannelMixerParam_Volume inScope:kAudioUnitScope_Input inElement:1];
}

#pragma mark - Record Control

- (void)startRecord
{
    // 开始直播
    [self addRouteChangeListener];
    [self addMixerRenderNofity];
    [self start];
}

- (void)stopRecord
{
    // 结束直播
    [self removeRouteChangeListener];
    [self removeMixerRenderNofify];
    [self stopMusicPlay];
    [self stop];
}

#pragma mark- Player Unit Play Control

// 播放伴奏文件，支持循环/非循环播放
- (void)playMusicFile:(NSString *)filePath
{
    // 新播放一首伴奏，都要停止之前播放的伴奏
    [self stopMusicPlay];
    
    _musicFilePath = filePath;
    
    playedFrames = 0;
    totalFramesToPlay = 0;
    _isMusicPlaying = YES;
    
    [self playMusic:_musicFilePath startOffset:0.f];
    [self aqplayerPlayMusic:_musicFilePath];
}

// 暂停播放当前伴奏
- (void)pauseMusicPlay
{
    _isMusicPlaying = NO;
    [self stopMusicUnitPlayer];
    [self pauseAQPlayer];
    // 暂停的时间点为playedFrames / _clientFormat32float.mSampleRate
}

-(void)stopMusicUnitPlayer
{
    [self resetNodeRenderState:_musicPlayerNode inScope:kAudioUnitScope_Global inElement:0];
    [self removePlayRenderNotify];
}

// 继续播放当前伴奏
- (void)resumeMusicPlay
{
    // 从上次暂停的点开始播放 playedFrames / _clientFormat32float.mSampleRate
    _isMusicPlaying = YES;
    [self playMusic:_musicFilePath startOffset:(playedFrames / _clientFormat32float.mSampleRate)];
    [self resumeAQPlayer];
}

// 停止播放当前伴奏
- (void)stopMusicPlay
{
    _isMusicPlaying = NO;
    [self stopMusicUnitPlayer];
    [self stopAQPlayer];
}

- (void)playMusic:(NSString *)filePath startOffset:(NSTimeInterval)startOffset
{
    try {
        if (filePath) {
            NSURL *url = [NSURL fileURLWithPath:filePath];
            XThrowIfError(ExtAudioFileOpenURL((__bridge CFURLRef)url, &_musicExtFileRef), "augraph recorder open music file failed");
            
            if (_musicExtFileRef) {
                UInt32 size = sizeof(_musicFileID);
                XThrowIfError(ExtAudioFileGetProperty(_musicExtFileRef, kExtAudioFileProperty_AudioFile, &size, &_musicFileID), "augraph recorder get music kExtAudioFileProperty_AudioFile failed");
                //获取长度
                size = sizeof(SInt64);
                XThrowIfError(ExtAudioFileGetProperty(_musicExtFileRef, kExtAudioFileProperty_FileLengthFrames, &size, &_musicFileTotalFrames), "augraph recorder get music kExtAudioFileProperty_FileLengthFrames failed");
                //获取格式
                if (_musicFileID) {
                    size = sizeof(AudioStreamBasicDescription);
                    XThrowIfError(AudioFileGetProperty(_musicFileID, kAudioFilePropertyDataFormat, &size, &_musicFileASBD), "augraph recorder get music kAudioFilePropertyDataFormat failed");
                }
            }
            
            if (_graph && [self isNodeInGraph:_musicPlayerNode] && _musicFileID) {
                AudioUnit playerUnit = [self graphUnitOfNode:_musicPlayerNode];
                //指定AU文件
                if (playerUnit) {
                    
                    XThrowIfError(AudioUnitSetProperty(playerUnit, kAudioUnitProperty_ScheduledFileIDs, kAudioUnitScope_Global, 0, &_musicFileID, sizeof(AudioFileID)), "augraph recorder music AudioUnitSetProperty kAudioUnitProperty_ScheduledFileIDs failed");
                    
                    // 设置region
                    ScheduledAudioFileRegion region = {0};
                    region.mAudioFile = _musicFileID;
                    region.mCompletionProc = NULL;
                    region.mCompletionProcUserData = NULL;
                    region.mLoopCount = 0;
                    region.mStartFrame = (SInt64)(startOffset * _musicFileASBD.mSampleRate);
                    region.mFramesToPlay = MAX(1, _musicFileTotalFrames - (UInt32)region.mStartFrame);
                    region.mTimeStamp.mFlags = kAudioTimeStampSampleTimeValid;
                    region.mTimeStamp.mSampleTime = 0;
                    
                    totalFramesToPlay = region.mFramesToPlay * (_clientFormat32float.mSampleRate / _musicFileASBD.mSampleRate) + startOffset * _clientFormat32float.mSampleRate;
                    playedFrames = startOffset * _clientFormat32float.mSampleRate;
                    
                    XThrowIfError(AudioUnitSetProperty(playerUnit, kAudioUnitProperty_ScheduledFileRegion, kAudioUnitScope_Global, 0, &region, sizeof(ScheduledAudioFileRegion)), "augraph recorder music AudioUnitSetProperty kAudioUnitProperty_ScheduledFileRegion failed");
                    
                    UInt32 primeFrames = 0;
                    XThrowIfError(AudioUnitSetProperty(playerUnit, kAudioUnitProperty_ScheduledFilePrime, kAudioUnitScope_Global, 0, &primeFrames, sizeof(primeFrames)), "augraph recorder set file prime error");
                    
                    // DO REMEMBER TO REMOVE render notify when stop the player !!!
                    
                    XThrowIfError(AudioUnitAddRenderNotify(playerUnit, &playMusicRenderNotify, (__bridge void *)self), "augraph recorder unit add render notify error");
                    
                    // 开始播放
                    AudioTimeStamp theTimeStamp = {0};
                    theTimeStamp.mFlags = kAudioTimeStampHostTimeValid;
                    theTimeStamp.mHostTime = 0;
                    XThrowIfError(AudioUnitSetProperty(playerUnit, kAudioUnitProperty_ScheduleStartTimeStamp, kAudioUnitScope_Global, 0, &theTimeStamp, sizeof(theTimeStamp)), "augraph recorder music AudioUnitSetProperty kAudioUnitProperty_ScheduleStartTimeStamp failed");
                }
            }
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

#pragma mark- AQPlayer Play Control

- (void)aqplayerPlayMusic:(NSString *)filePath
{
    if (_aqplayer) {
        _aqplayer->DisposeQueue(true);
        delete _aqplayer;
        _aqplayer = NULL;
    }
    _aqplayer = new AQPlayer();
    _aqplayer->CreateQueueForFile((__bridge CFStringRef)filePath);
    if (isHeadSet) {
        [self changeToHeadSetMode];
    } else {
        [self changeToSpeakerMode];
    }
}

- (void)stopAQPlayer
{
    if (_aqplayer) {
        if (_aqplayer->IsRunning()) {
            _aqplayer->StopQueue();
        }
    }
}

- (void)pauseAQPlayer
{
    if (_aqplayer) {
        if (_aqplayer->IsRunning()) {
            _aqplayer->PauseQueue();
        }
    }
}

- (void)resumeAQPlayer
{
    if (_aqplayer) {
        if (_aqplayer->IsRunning()) {
            _aqplayer->StartQueue(true);
        }
    }
}

#pragma mark- Route Change Handle

- (void)addRouteChangeListener
{
    try {
        XThrowIfError(AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange, propListener_routechange, (__bridge void *)self), "augraph recorder couldn't set property listener");
        [self adjustOnRouteChange];
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (void)removeRouteChangeListener
{
    try {
        XThrowIfError(AudioSessionRemovePropertyListenerWithUserData(kAudioSessionProperty_AudioRouteChange, propListener_routechange, (__bridge void *)self), "augraph recorder remove AudioRouteChange listener error") ;
    } catch (CAXException &e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (void)adjustOnRouteChange
{
    try {
        CFStringRef audioRoute;
        UInt32 size = sizeof(CFStringRef);
        XThrowIfError(AudioSessionGetProperty(kAudioSessionProperty_AudioRoute, &size, &audioRoute), "augraph recorder couldn't get audio route");
        if (audioRoute)
        {
            if ([[AVAudioSession sharedInstance] usingWiredMicrophone]) {
                isHeadSet = YES;
                [self enableEarReturn];
                [self changeToHeadSetMode];
            } else {
                isHeadSet = NO;
                if (![[AVAudioSession sharedInstance] usingBlueTooth]) {
                    UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;
                    XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_OverrideAudioRoute, sizeof(audioRouteOverride), &audioRouteOverride), "augraph recorder set audio route error");
                }
                [self changeToSpeakerMode];
            }
            CFRelease(audioRoute);
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (void)changeToSpeakerMode
{
    if (_aqplayer) {
        if (!_aqplayer->IsRunning() && _isMusicPlaying) {
            _aqplayer->StartQueue(false);
        }
        _aqplayer-> SetVolume([self currentMusicVolume]);
        [self disableUnitPlayer];
    }
}

- (void)changeToHeadSetMode
{
    if (_aqplayer) {
        if (!_aqplayer->IsRunning() && _isMusicPlaying) {
            _aqplayer->StartQueue(false);
        }
        _aqplayer-> SetVolume(0);
        [self enableUnitPlayer];
    }
}

#pragma mark- 音效相关

- (void)setPitchLevel:(NSInteger)pitchLevel
{
    _pitchLevel = pitchLevel;
    if ([KTVAUGraphController isPitchSupported]) {
        [self setNodeParameter:_timePitchNode inID:kNewTimePitchParam_Pitch inScope:kAudioUnitScope_Global inElement:0 inValue:(_pitchLevel * 100.f) inOffset:0];
        [self setNodeParameter:_timePitchNode inID:kNewTimePitchParam_Rate inScope:kAudioUnitScope_Global inElement:0 inValue:1.0 inOffset:0];
        [self setNodeParameter:_timePitchNode inID:kNewTimePitchParam_Overlap inScope:kAudioUnitScope_Global inElement:0 inValue:8.0 inOffset:0];
        [self setNodeParameter:_timePitchNode inID:kNewTimePitchParam_EnablePeakLocking inScope:kAudioUnitScope_Global inElement:0 inValue:1.0 inOffset:0];
        
        if (_aqplayer) {
            _aqplayer->SetPitch(_pitchLevel);
        }
        // 和声需要做pitch shift
        //        if (_harmonic) {
        //            _harmonic->PitchShift((int)_pitchLevel);
        //        }
        if (_autoTune) {
            _autoTune->PitchShift((int)_pitchLevel);
        }
    }
}

- (void)setHumanVolumeDB:(Float32)humanVolumeDB
{
    _humanVolumeDB = humanVolumeDB;
    [self setInputVolume:HUMAN_VOICE_ELEMENT value:[KTVAUGraphController adjustDBToParamater:_humanVolumeDB minDB:VocalParamHumanVolumeMin]];
}

- (void)setMusicVolumeDB:(Float32)musicVolumeDB
{
    _musicVolumeDB = musicVolumeDB;
    [self setInputVolume:MUSIC_VOICE_ELEMENT value:[KTVAUGraphController adjustDBToParamater:_musicVolumeDB minDB:VocalParamMusicVolumeMin]];
    if (!isHeadSet) {
        if (_aqplayer) {
            _aqplayer->SetVolume([KTVAUGraphController adjustDBToParamater:_musicVolumeDB minDB:VocalParamMusicVolumeMin]);
        }
    }
}

- (void)clearAllEffectNodes
{
    [self graphDisconnectNode:_pitchNode];
    [self graphDisconnectNode:_reverbNode];
    [self graphDisconnectNode:_limiterNode];
    [self graphDisconnectNode:_compressorNode];
    [self graphDisconnectNode:_bandEQNode];
    [self updateAUGraph];
}

- (void)changeToPitchMode
{
    [self clearAllEffectNodes];
    [self addNodesForPitch];
}

- (void)changeToEQModeWithPitch:(BOOL)withPitch
{
    [self clearAllEffectNodes];
    [self addNodesForEQWithPitch:withPitch];
}

- (void)changeToDSPModeWithReverb:(BOOL)withReverb
{
    [self clearAllEffectNodes];
    [self adjustForCurrentDSPMode];
    [self addNodesForDSPWithReverb:withReverb];
}

- (void)initDynamicDelays
{
//    if (!_humanVoiceDelay) {
//        _humanVoiceDelay = new DynamicDelay([KTVAUGraphController hardwareSampleRate], 300000, 2);
//        [self resetHumanDanymicDelay:0.0f];
//    }
//    if (!_musicVoiceDelay) {
//        _musicVoiceDelay = new DynamicDelay([KTVAUGraphController hardwareSampleRate], 300000, 2);
//        [self resetMusicDanymicDelay:0.0f];
//    }
}

- (void)initAutoTune
{
    if (!_autoTune) {
        _autoTune = new AutoTune();
//        if (_songMelPathString) {
//            _autoTune->Init([KTVAUGraphController hardwareSampleRate], 2, [_songMelPathString UTF8String]);
//            _autoTune->PitchShift((int)self.pitchLevel);
//        } else {
        _autoTune->Init([KTVAUGraphController hardwareSampleRate], 2, "");
        _autoTune->PitchShift((int)self.pitchLevel);
//        }
    }
}

- (void)initDoubleYou
{
    if (!_doubleYou) {
        _doubleYou = new DoubleYou();
        _doubleYou->Init([KTVAUGraphController hardwareSampleRate], 2);
    }
}

- (void)initHarmonic
{
//    if (!_harmonic) {
//        if (_songMelPathString) {
//            _harmonic = new HarmonicMix();
//            BOOL isFastMode = ![[UIDevice currentDevice] isIphone5Upper];
//            _harmonic->Init([KTVAUGraphController hardwareSampleRate], 2, [_songMelPathString UTF8String], self.isHarmonicOnlyChorus, isFastMode);
//            // 4和4s为了效率，使用fast 模式
//            _harmonic->PitchShift((int)self.pitchLevel);
//        }
//    }
}

- (void)clearAllDspTool
{
    [self clearAutoTune];
    [self clearDoubleYou];
    [self clearHarmonic];
    [self clearDynamicDelays];
}

- (void)clearDynamicDelays
{
//    if (_humanVoiceDelay) {
//        delete _humanVoiceDelay;
//        _humanVoiceDelay = NULL;
//    }
//    if (_musicVoiceDelay) {
//        delete _musicVoiceDelay;
//        _musicVoiceDelay = NULL;
//    }
}

- (void)clearAutoTune
{
    if (_autoTune) {
        delete _autoTune;
        _autoTune = NULL;
    }
}

- (void)clearDoubleYou
{
    if (_doubleYou) {
        delete _doubleYou;
        _doubleYou = NULL;
    }
}

- (void)clearHarmonic
{
//    if (_harmonic) {
//        delete _harmonic;
//        _harmonic = NULL;
//    }
}

- (void)adjustForCurrentDSPMode
{
    if (_effectMode.category == KTVEffectCategoryAutoTone) {
        [self initAutoTune];
        _autoTune->SeekFromStart(playedFrames);
    } else if (_effectMode.category == KTVEffectCategoryDoubleYou) {
        [self initDoubleYou];
        _doubleYou->ResetShiftDeposit();
    }
//    else if (_effectMode.category == KTVEffectCategoryHarmonic) {
//        [self initHarmonic];
//        _harmonic->SeekFromStart(_playedFrames);
//        _harmonic->SetOnlyChorus(_isHarmonicOnlyChorus);
//    }
}

- (void)addNodesForPitch
{
    [self graphInsertNode:_pitchNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    [self updateAUGraph];
}

- (void)addNodesForEQWithPitch:(BOOL)withPitch
{
    if (withPitch) {
        [self graphInsertNode:_pitchNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    }
    
    [self graphInsertNode:_reverbNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    [self graphInsertNode:_compressorNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    [self graphInsertNode:_bandEQNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    [self graphInsertNode:_limiterNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    [self updateAUGraph];
}

- (void)addNodesForDSPWithReverb:(BOOL)withReverb
{
    if (withReverb) {
        [self graphInsertNode:_reverbNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    }
    
    [self graphInsertNode:_compressorNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    [self graphInsertNode:_bandEQNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    [self graphInsertNode:_limiterNode inScope:HUMAN_VOICE_SCOPE inElement:HUMAN_VOICE_ELEMENT];
    [self updateAUGraph];
}

- (void)applyEffect:(KTVEffectCategory)effectCategory
{
    BOOL shouldChangeMode = NO;
    if (!_isGraphAddedEffectNodes) {
        _isGraphAddedEffectNodes = YES;
        shouldChangeMode = YES;
    } else {
        shouldChangeMode = !isTwoEffectModesHasSameGraph(KTVEffectModeMake(effectCategory, EQ_LEVEL_DEAULT), _effectMode);
    }
    
    _effectMode.category = effectCategory;
    
    if (shouldChangeMode) {
        switch (getEffectModesGraph(_effectMode)) {
            case KTVEffectGraphPitch:
                [self changeToPitchMode];
                break;
            case KTVEffectGraphDSP:
                [self changeToDSPModeWithReverb:NO];
                break;
            case KTVEffectGraphDSPReverb:
                [self changeToDSPModeWithReverb:YES];
                break;
            case KTVEffectGraphEQ:
                [self changeToEQModeWithPitch:NO];
                break;
            case KTVEffectGraphEQPitch:
                [self changeToEQModeWithPitch:YES];
                break;
            default:
                break;
        }
    } else {
        if (getEffectModesGraph(_effectMode) == KTVEffectGraphDSPReverb) {
            [self adjustForCurrentDSPMode];
        }
    }
    
    //    NSLog(@"self.dynamicDelayFromRecorder is %@", @(self.dynamicDelayFromRecorder));
//    if (fabs(self.dynamicDelayFromRecorder) > 0.04f) {
//        self.dynamicDelayFromRecorder = 0.0f;
//    }
//    switch (_effectMode.category) {
//        case KTVEffectCategoryRobot:
//            [self resetHumanDanymicDelay:0.0925f + self.dynamicDelayFromRecorder / 2.f];
//            break;
//        case KTVEffectCategoryAutoTone:
//            [self resetHumanDanymicDelay:0.052f + self.dynamicDelayFromRecorder / 2.f];
//            break;
//        case KTVEffectCategoryDoubleYou:
//            [self resetHumanDanymicDelay:0.0175f + self.dynamicDelayFromRecorder / 2.f];
//            break;
//        case KTVEffectCategoryHarmonic:
//            [self resetHumanDanymicDelay:0.052f + self.dynamicDelayFromRecorder / 2.f];
//            break;
//        default:
//            [self resetHumanDanymicDelay:-0.0075f + self.dynamicDelayFromRecorder / 2.f];
//            break;
//    }
    
    [self graphApplyEffectMode:_effectMode];
    [self updateAUGraph];
    [self showAUGraph];
}

- (void)runDSP:(AudioBufferList *)ioData
{
    if (_effectMode.category == KTVEffectCategoryAutoTone) {
        if (_autoTune) {
            for (int i = 0; i < ioData->mNumberBuffers; i++) {
                UInt32 bufSize = ioData->mBuffers[i].mDataByteSize;
                SInt16 *outBuf = new SInt16[bufSize/sizeof(SInt16)];
                _autoTune->runAutoTune((SInt16*)(ioData->mBuffers[i].mData), outBuf, bufSize / sizeof(SInt16));
//                if (_humanVoiceDelay) {
//                    SInt16 *delayedBuf = new SInt16[bufSize/sizeof(SInt16)];
//                    _humanVoiceDelay->Delay(outBuf, delayedBuf, bufSize / sizeof(SInt16));
//                    memcpy((SInt16*)(ioData->mBuffers[i].mData), delayedBuf, bufSize);
//                    delete [] delayedBuf;
//                    delete [] outBuf;
//                } else {
                    memcpy((SInt16*)(ioData->mBuffers[i].mData), outBuf, bufSize);
                    delete[] outBuf;
//                }
            }
        }
    } else if (_effectMode.category == KTVEffectCategoryDoubleYou) {
        if (_doubleYou) {
            for (int i = 0; i < ioData->mNumberBuffers; i++) {
                UInt32 bufSize = ioData->mBuffers[i].mDataByteSize;
                SInt16 *outBuf = new SInt16[bufSize/sizeof(SInt16)];
                _doubleYou->DYFlow((SInt16*)(ioData->mBuffers[i].mData), outBuf, bufSize / sizeof(SInt16));
//                if (_humanVoiceDelay) {
//                    SInt16 *delayedBuf = new SInt16[bufSize/sizeof(SInt16)];
//                    _humanVoiceDelay->Delay(outBuf, delayedBuf, bufSize / sizeof(SInt16));
//                    memcpy((SInt16*)(ioData->mBuffers[i].mData), delayedBuf, bufSize);
//                    delete [] delayedBuf;
//                    delete [] outBuf;
//                } else {
                    memcpy((SInt16*)(ioData->mBuffers[i].mData), outBuf, bufSize);
                    delete[] outBuf;
//                }
            }
        }
    }
//    else if (_effectMode.category == KTVEffectCategoryHarmonic) {
//        if (_harmonic) {
//            for (int i = 0; i < ioData->mNumberBuffers; i++) {
//                UInt32 bufSize = ioData->mBuffers[i].mDataByteSize;
//                SInt16 *outBuf = new SInt16[bufSize/sizeof(SInt16)];
//                _harmonic->runHarmonicMix((SInt16*)(ioData->mBuffers[i].mData), outBuf, bufSize / sizeof(SInt16));
//                if (_humanVoiceDelay) {
//                    SInt16 *delayedBuf = new SInt16[bufSize/sizeof(SInt16)];
//                    _humanVoiceDelay->Delay(outBuf, delayedBuf, bufSize / sizeof(SInt16));
//                    memcpy((SInt16*)(ioData->mBuffers[i].mData), delayedBuf, bufSize);
//                    delete [] delayedBuf;
//                    delete [] outBuf;
//                } else {
//                    memcpy((SInt16*)(ioData->mBuffers[i].mData), outBuf, bufSize);
//                    delete[] outBuf;
//                }
//            }
//        }
//    }
//    else {
//        if (_humanVoiceDelay) {
//            for (int i = 0; i < ioData->mNumberBuffers; i++) {
//                UInt32 bufSize = ioData->mBuffers[i].mDataByteSize;
//                SInt16 *outBuf = new SInt16[bufSize/sizeof(SInt16)];
//                _humanVoiceDelay->Delay((SInt16*)(ioData->mBuffers[i].mData), outBuf, bufSize / sizeof(SInt16));
//                memcpy((SInt16*)(ioData->mBuffers[i].mData), outBuf, bufSize);
//                delete[] outBuf;
//            }
//        }
//    }
}

- (AUNode)findNodeForKey:(NSString*)paramKey
{
    NSArray *paramSpliter = [paramKey componentsSeparatedByString:@"_"];
    NSString *typeKey = [paramSpliter firstObject];
    if ([typeKey isEqual:@"compressor"])
    {
        return _compressorNode;
    }
    else if ([typeKey isEqual:@"equalizer"])
    {
        return _bandEQNode;
    }
    else if ([typeKey isEqual:@"reverb"])
    {
        return _reverbNode;
    }
    else if ([typeKey isEqual:@"limiter"])
    {
        return _limiterNode;
    }
    else if ([typeKey isEqual:@"pitch"])
    {
        return _pitchNode;
    }
    return 0;
}

#pragma mark- test

- (void)testEffect:(NSInteger)testCaseIndex
{
    NSLog(@"testEffect %@", @(testCaseIndex));
    switch (testCaseIndex) {
        case 0:
            [self applyEffect:KTVEffectCategoryRock];
            break;
        case 1:
            [self applyEffect:KTVEffectCategoryRB];
            break;
        case 2:
            [self applyEffect:KTVEffectCategoryPopular];
            break;
        case 3:
            [self applyEffect:KTVEffectCategoryDance];
            break;
        case 4:
            [self applyEffect:KTVEffectCategoryNewAge];
            break;
        case 5:
            [self applyEffect:KTVEffectCategoryAutoTone];
            break;
        case 6:
            [self applyEffect:KTVEffectCategoryPhono];
            break;
        case 7:
            [self applyEffect:KTVEffectCategoryPlayOrigin];
            break;
        case 8:
            [self applyEffect:KTVEffectCategoryDoubleYou];
            break;
            
        default:
            break;
    }
    
}

- (void)testPitch:(NSInteger)testCaseIndex
{
    NSLog(@"testPitch %@", @(testCaseIndex - 3));
    self.pitchLevel = testCaseIndex - 3;
}

@end
