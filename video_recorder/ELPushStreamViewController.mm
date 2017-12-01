#import "ELPushStreamViewController.h"
#import "ELImageVideoScheduler.h"
#import "KTVAUGraphRecorder.h"
#import "live_packet_pool.h"
#import "video_consumer_thread.h"
#import "audio_encoder_adapter.h"
#import "ELPushStreamConfigeration.h"
#import "ELVideoRecordingStudio.h"
#import "LoadingView.h"

#define buttonWidth 50.0f

@interface ELPushStreamViewController () <ELVideoEncoderStatusDelegate, KTVAUGraphRecorderDelegate, ELVideoRecordingStudioDelegate>
{
    BOOL _started;
    BOOL _userStarted;
    
    BOOL                            _isRecordingFlag;
    double                          startRecordTimeMills;
    int64_t                         totalSampleCount;
    
    ELImageVideoScheduler*          _videoScheduler;
    AudioEncoderAdapter*            _audioEncoder;
    KTVAUGraphRecorder*             _audioRecorder;
    ELVideoRecordingStudio*         _recordingStudio;
}

@property(nonatomic,strong) ELPushStreamMetadata *metadata;
@property(nonatomic,strong) UIButton *frontBackSwitchButton;
@property(nonatomic,strong) UIButton *pushButton;

@end

@implementation ELPushStreamViewController

#pragma -mark life cycle

- (void)viewDidLoad {
    [super viewDidLoad];
    [self pushButton];
    [self frontBackSwitchButton];
    CGRect bounds = self.view.bounds;
    _videoScheduler = [[ELImageVideoScheduler alloc] initWithFrame:bounds videoFrameRate:kFrameRate];
    [self.view insertSubview:[_videoScheduler previewView] atIndex:0];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillResignActive:) name:UIApplicationWillResignActiveNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];
    if(!_metadata)
    {
        NSArray *documentsPathArr = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *document = [documentsPathArr lastObject];
        NSString* pushURL = [document stringByAppendingPathComponent:@"recording.flv"];
        //        NSString* pushURL = kFakePushURL;
        _metadata = [[ELPushStreamMetadata alloc] initWithRtmpUrl:pushURL videoWidth:kDesiredWidth videoHeight:kDesiredHeight videoFrameRate:kFrameRate videoBitRate:kAVGVideoBitRate audioSampleRate:[KTVAUGraphController liveRoomHardwareSampleRate] audioChannels:kAudioChannels audioBitRate:kAudioBitRate audioCodecName:kAudioCodecName
                                                  qualityStrategy:0
                                  adaptiveBitrateWindowSizeInSecs:WINDOW_SIZE_IN_SECS adaptiveBitrateEncoderReconfigInterval:NOTIFY_ENCODER_RECONFIG_INTERVAL adaptiveBitrateWarCntThreshold:PUB_BITRATE_WARNING_CNT_THRESHOLD
                                           adaptiveMinimumBitrate:300 * 1024
                                           adaptiveMaximumBitrate:1000 * 1024];
    }
}


-(UIButton*)pushButton
{
    if(!_pushButton)
    {
        _pushButton = [UIButton buttonWithType:UIButtonTypeCustom];
        [self.view addSubview: _pushButton];
        [_pushButton setTitle:@"Push" forState:UIControlStateNormal];
        [_pushButton setTitle:@"Stop" forState:UIControlStateSelected];
        [_pushButton setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
        [_pushButton setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected];
        CGFloat screenWidth = self.view.bounds.size.width;
        CGFloat screenHeight = self.view.bounds.size.height;
        CGRect huge = CGRectMake((screenWidth - buttonWidth) / 2, screenHeight - buttonWidth - 30, buttonWidth, buttonWidth);
        [_pushButton setFrame:huge];
        _pushButton.layer.cornerRadius = buttonWidth/2.0f;
        _pushButton.layer.borderWidth = 1.0f;
        _pushButton.layer.borderColor = [UIColor blackColor].CGColor;
        [_pushButton addTarget:self action:@selector(OnStartStop:) forControlEvents:UIControlEventTouchUpInside];
        //按钮初始状态
        [_pushButton setSelected:NO];
    }
    return _pushButton;
}

#pragma -mark publish Control logic

-(void)start
{
    _isRecordingFlag = false;
    totalSampleCount = 0;
    [self.pushButton setEnabled:NO];
    [[self recordingStudio] start];
    [[LoadingView shareLoadingView] show];
}

- (ELVideoRecordingStudio*) recordingStudio;
{
    if(_recordingStudio == NULL)
    {
        _recordingStudio = [[ELVideoRecordingStudio alloc] initWithMeta:self.metadata];
        _recordingStudio.recordingStudioDelegate = self;
    }
    return _recordingStudio;
}

-(void)stop
{
    //stop producer
    [_videoScheduler stopEncode];
    
    [self stopAudioRecord];
    
    //stop consumer
    [[self recordingStudio] stop];
    
    [self.pushButton setSelected:NO];
    _started = NO;
    [self bringButtonsToFront];
}

- (void)startAudioRecord
{
    [self.audioRecorder startRecord];
    _audioEncoder = new AudioEncoderAdapter();
    char* audioCodecName = [ELPushStreamMetadata nsstring2char:kAudioCodecName];
    _audioEncoder->init(LivePacketPool::GetInstance(), [KTVAUGraphController liveRoomHardwareSampleRate], kAudioChannels, kAudioBitRate, audioCodecName);
    delete[] audioCodecName;
}

- (void)stopAudioRecord
{
    [self.audioRecorder stopRecord];
    if(NULL != _audioEncoder){
        _audioEncoder->destroy();
        delete _audioEncoder;
        _audioEncoder = NULL;
    }
}

// Called when start/stop button is pressed
- (void)OnStartStop:(id)sender {
    if (_started)
    {
        _userStarted = NO;
        [self stop];
        [UIApplication sharedApplication].idleTimerDisabled = NO;
    }
    else
    {
        _userStarted = YES;
        [self start];
        [UIApplication sharedApplication].idleTimerDisabled = YES;
    }
}
-(UIButton*)frontBackSwitchButton
{
    if(!_frontBackSwitchButton)
    {
        //前后摄像头切换
        _frontBackSwitchButton = [UIButton buttonWithType:UIButtonTypeCustom];
        [self.view addSubview: _frontBackSwitchButton];
        [_frontBackSwitchButton setTitle:@"Back" forState:UIControlStateNormal];
        [_frontBackSwitchButton setTitle:@"Front" forState:UIControlStateSelected];
        [_frontBackSwitchButton setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
        [_frontBackSwitchButton setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected];
        [_frontBackSwitchButton setFrame:CGRectMake(100, self.view.frame.size.height - buttonWidth - 30, buttonWidth, buttonWidth)];
        _frontBackSwitchButton.layer.cornerRadius = buttonWidth/2.0f;
        _frontBackSwitchButton.layer.borderWidth = 1.0f;
        _frontBackSwitchButton.layer.borderColor = [UIColor blackColor].CGColor;
        [_frontBackSwitchButton addTarget:self action:@selector(OnFrontBackSwitch:) forControlEvents:UIControlEventTouchUpInside];
        //按钮初始状态
        [_frontBackSwitchButton setSelected:NO];
    }
    return _frontBackSwitchButton;
}

- (void)OnFrontBackSwitch:(id)sender {
    [_videoScheduler switchFrontBackCamera];
    [self bringButtonsToFront];
    [self.frontBackSwitchButton setSelected:!self.frontBackSwitchButton.selected];
}
- (KTVAUGraphRecorder *)audioRecorder
{
    if (!_audioRecorder) {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *documentsDirectory = paths[0];
        NSString *recordFolderPath = [documentsDirectory stringByAppendingPathComponent:@"record"];
        NSFileManager *fm = [NSFileManager defaultManager];
        
        if (![fm fileExistsAtPath:recordFolderPath isDirectory:NULL])
        {
            //if folder notfound, create one
            [fm createDirectoryAtPath:recordFolderPath withIntermediateDirectories:YES attributes:nil error:nil];
        }
        
        _audioRecorder = [[KTVAUGraphRecorder alloc] initWithRecordFilePath:[recordFolderPath stringByAppendingPathComponent:@"temp.wav"]];
        _audioRecorder.delegate = self;
    }
    return _audioRecorder;
}
- (void)bringButtonsToFront
{
    [self.view bringSubviewToFront:self.frontBackSwitchButton];
    [self.view bringSubviewToFront:_pushButton];
}


-(void)applicationWillResignActive:(NSNotification*)note
{
    if (_started) {
        [self stop];
    }
}

-(void)applicationDidBecomeActive:(NSNotification*)note
{
    if (_userStarted && !_started) {
        [self start];
    }
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [_videoScheduler startPreview];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [_videoScheduler stopPreview];
}

-(void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - Audio callback

- (void)recordDidReceiveBuffer:(AudioBuffer)buffer
{
    //    NSLog(@"Audio Record Receive %d buffer",buffer.mDataByteSize);
    if(!_isRecordingFlag){
        _isRecordingFlag = true;
        startRecordTimeMills = CFAbsoluteTimeGetCurrent() * 1000;
    }
    double audioSamplesTimeMills = CFAbsoluteTimeGetCurrent() * 1000 - startRecordTimeMills;
    double audioSampleRate = [KTVAUGraphController liveRoomHardwareSampleRate];
    double dataAccumulateTimeMills = (double)totalSampleCount * 1000 / audioSampleRate / kAudioChannels;
    if(dataAccumulateTimeMills <= (audioSamplesTimeMills - MAX_DIFF_TIME_MILLS)){
        int correctTimeMills = audioSamplesTimeMills - dataAccumulateTimeMills - MIN_DIFF_TIME_MILLS;
        int correctBufferSize = ((float)correctTimeMills / 1000.0f) * audioSampleRate * kAudioChannels;
        LiveAudioPacket * audioPacket = new LiveAudioPacket();
        audioPacket->buffer = new short[correctBufferSize];
        memset(audioPacket->buffer, 0, correctBufferSize * sizeof(short));
        audioPacket->size = correctBufferSize;
        LivePacketPool::GetInstance()->pushAudioPacketToQueue(audioPacket);
        totalSampleCount+=correctBufferSize;
        NSLog(@"Correct Time Mills is %d", correctTimeMills);
        NSLog(@"audioSamplesTimeMills is %lf, dataAccumulateTimeMills is %lf", audioSamplesTimeMills, dataAccumulateTimeMills);
    }
    int sampleCount = buffer.mDataByteSize / 2;
    totalSampleCount += sampleCount;
    short *packetBuffer = new short[sampleCount];
    memcpy(packetBuffer, buffer.mData, buffer.mDataByteSize);
    LiveAudioPacket *audioPacket = new LiveAudioPacket();
    audioPacket->buffer = packetBuffer;
    audioPacket->size = buffer.mDataByteSize/2;
    LivePacketPool::GetInstance()->pushAudioPacketToQueue(audioPacket);
}

#pragma -mark Encoder Delegate
- (void) onEncoderInitialFailed{
    dispatch_async(dispatch_get_main_queue(), ^{
        [_videoScheduler stopEncode];
        //您的直播无法正常播放(编码器初始化失败)，请立即联系客服人员
        UIAlertView *alterView = [[UIAlertView alloc] initWithTitle:@"提示信息"
                                                            message:@"您的直播无法播放，请立即联系客服人员"
                                                           delegate:self
                                                  cancelButtonTitle:@"取消"
                                                  otherButtonTitles: nil];
        [alterView show];
    });
}

- (void) onEncoderEncodedFailed{
    dispatch_async(dispatch_get_main_queue(), ^{
        [_videoScheduler stopEncode];
        //您的直播无法正常播放(编码器编码视频失败)，请立即联系客服人员
        UIAlertView *alterView = [[UIAlertView alloc] initWithTitle:@"提示信息"
                                                            message:@"您的直播无法正常播放了，请立即联系客服人员"
                                                           delegate:self cancelButtonTitle:@"取消" otherButtonTitles: nil];
        [alterView show];
    });
}

#pragma -mark Recording Studio Delegate
- (void) publishTimeOut;
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self stop];
        UIAlertView *alterView = [[UIAlertView alloc] initWithTitle:@"提示信息" message:@"由于网络原因, 发送超时" delegate:self cancelButtonTitle:@"取消" otherButtonTitles: nil];
        [alterView show];
    });
}


- (void) statisticsCallbackWithStartTimeMills:(double) startTimeMills connectTimeMills:(int) connectTimeMills publishDurationInSec:(int)publishDurationInSec discardFrameRatio:(float) discardFrameRatio publishAVGBitRate:(float)publishAVGBitRate expectedBitRate:(float) expectedBitRate adaptiveBitrateChart:(NSString*)bitRateChangeChart;
{
    NSLog(@"startTimeMills is %lf connectTimeMills is %d publishDurationInSec is %d", startTimeMills, connectTimeMills, publishDurationInSec);
    NSLog(@"discardFrameRatio is %f publishAVGBitRate is %f expectedBitRate is %f", discardFrameRatio, publishAVGBitRate, expectedBitRate);
    NSLog(@"adaptive bitrate chart is %@", bitRateChangeChart);
}

- (void) onConnectSuccess;
{
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [[LoadingView shareLoadingView] close];
        [self startAudioRecord];
        [_videoScheduler startEncodeWithFPS:kFrameRate maxBitRate:kMaxVideoBitRate avgBitRate:kAVGVideoBitRate encoderWidth:kDesiredWidth encoderHeight:kDesiredHeight encoderStatusDelegate:self];
        _started = YES;
        [self.pushButton setSelected:YES];
        [self bringButtonsToFront];
        [self.pushButton setEnabled:YES];
    });
    
}

- (void) adaptiveVideoQuality:(int) videoQuality;

{
    bool invalidFlag = false;
    bool showUserTip = false;
    int bitrate = kAVGVideoBitRate;
    int bitrateLimits = kMaxVideoBitRate;
    int fps = kFrameRate;
    switch (videoQuality) {
        case HIGH_QUALITY:
            bitrate = kAVGVideoBitRate;
            bitrateLimits = kMaxVideoBitRate;
            fps = kFrameRate;
            break;
        case MIDDLE_QUALITY:
            bitrate = MIDDLE_QUALITY_BIT_RATE;
            bitrateLimits = MIDDLE_QUALITY_LIMITS_BIT_RATE;
            fps = MIDDLE_QUALITY_FRAME_RATE;
            break;
        case LOW_QUALITY:
            showUserTip = true;
            bitrate = LOW_QUALITY_BIT_RATE;
            bitrateLimits = LOW_QUALITY_LIMITS_BIT_RATE;
            fps = LOW_QUALITY_FRAME_RATE;
            break;
        case INVALID_LIVE_FLAG:
            invalidFlag = true;
            break;
        default:
            break;
    }
    if(invalidFlag){
        dispatch_async(dispatch_get_main_queue(), ^{
            [self stop];
            UIAlertView *alterView = [[UIAlertView alloc] initWithTitle:@"提示信息" message:@"由于当前网络环境过差，无法支持视频直播。请切换至其他网络或改善所处网络环境后重新开播！" delegate:self cancelButtonTitle:@"取消" otherButtonTitles: nil];
            [alterView show];
        });
    } else{
        NSLog(@"由于当前网络环境较差，已切换至流畅模式。如需使用高清模式，请改善所处网络环境后重新开播！[%dKbps, %d]", (int)(bitrate / 1024), fps);
        if(showUserTip){
            dispatch_async(dispatch_get_main_queue(), ^{
                UIAlertView *alterView = [[UIAlertView alloc] initWithTitle:@"提示信息" message:@"由于当前网络环境较差，已切换至流畅模式。如需使用高清模式，请改善所处网络环境后重新开播！" delegate:self cancelButtonTitle:@"取消" otherButtonTitles: nil];
                [alterView show];
            });
        }
        [_videoScheduler settingMaxBitRate:bitrateLimits avgBitRate:(int)bitrate fps:fps];
    }
}
- (void) adaptiveVideoMaxBitrate:(int)maxBitrate avgBitrate:(int)avgBitrate fps:(int)fps;
{
    [_videoScheduler settingMaxBitRate:maxBitrate avgBitRate:(int)avgBitrate fps:fps];
}


- (void) onConnectFailed;
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [[LoadingView shareLoadingView] close];
        [self.pushButton setEnabled:YES];
        UIAlertView *alterView = [[UIAlertView alloc] initWithTitle:@"提示信息" message:@"连接RTMP服务器失败" delegate:self cancelButtonTitle:@"取消" otherButtonTitles: nil];
        [alterView show];
    });
}
@end
