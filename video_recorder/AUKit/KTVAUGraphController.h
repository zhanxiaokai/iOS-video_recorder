//
//  KTVAUGraphController.h
//  ktv
//
//  Created by liumiao on 11/22/14.
//
//

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import "UIDevice-Hardware.h"

#define HUMAN_VOICE_ELEMENT 0
#define MUSIC_VOICE_ELEMENT 1
#define OUTPUT_VOICE_ELEMENT 0

#define HUMAN_VOICE_SCOPE kAudioUnitScope_Input
#define MUSIC_VOICE_SCOPE kAudioUnitScope_Input
#define OUTPUT_VOICE_SCOPE kAudioUnitScope_Output

#define EQ_NODE_NUM 9

#define EQ_LEVEL_DEAULT 5

// if we can not find paramid or want to skip a parami, we return this magic id
#define MAGIC_PARAM_ID 201314

// use for [[AVAudioSession sharedInstance]setPreferredIOBufferDuration]
#define AUDIO_DEFAULT_BUFFER_DURATION 0.02322 //for play and save
#define AUDIO_RECORD_BUFFER_DURATION 0.005 //for record

#define FILE_SIZE_PER_SECOND 176400  //176.4*8000b = 176.4KB 对应16位 双声道 44.1kHz PCM wav文件每秒钟数据size

#define FADEOUT_DURATION 3.0 //淡出效果时间

#define FADEOUT_DATA_SIZE 132300.0 //FILE_SIZE_PER_SECOND * FADEOUT_DURATION / 4 由于用在AU的callback中，需要尽量减少不必要计算，所以用常数替代

#define ZERO_VOLUME_BOUNDRY 0.05 // 音量下调阈值，一旦低于该值，音量调为0

// AccompanyDefault 和服务器的acompanyMax一起控制伴奏的音量, RecordAccompanyDefault用于录音，PlayAccompanyDefault用于回放和保存
#define PlayAccompanyDefault 1.0f
#define RecordAccompanyDefault 1.0f

#define VocalLinearGainDefault 0.95f
#define AccompanyLinearGainDefault 0.95f
#define SingOnlyLinearGainDefault 1.0f

typedef NS_OPTIONS(NSUInteger, NodeConnectionState) {
    NodeConnectionNone      = 0,
    NodeConnectionOnlyUp    = 1 << 0,
    NodeConnectionOnlyDown  = 1 << 1,
    NodeConnectionUpAndDown = NodeConnectionOnlyUp | NodeConnectionOnlyDown
};

typedef NS_ENUM(NSUInteger, KTVEffectCategory)
{
    // 录音效果
    KTVEffectCategoryMoYin = 0,     //魔音怪咖
    KTVEffectCategoryChangJiang,    //实力唱将
    KTVEffectCategoryGeShou,        //专业歌手
    KTVEffectCategoryGeShen,        //顶级歌神
    KTVEffectCategoryRecOrigin,     //录音原声
    
    // 回放、保存录音效果
    KTVEffectCategoryRB = 50,       //R&B
    KTVEffectCategoryRock,          //摇滚
    KTVEffectCategoryPopular,       //流行
    KTVEffectCategoryDance,         //舞曲
    KTVEffectCategoryNewAge,        //新世纪
    KTVEffectCategoryPlayOrigin,    //回放原声
    KTVEffectCategoryPhono,         //留声机   注意：留声机是只有eq
    KTVEffectCategoryCatty,         //萌猫
    KTVEffectCategoryDoggy,         //狗宝宝
    KTVEffectCategoryBaby,          //娃娃音
    KTVEffectCategoryAutoTone,      //电音
    KTVEffectCategoryDoubleYou,     //DoubleYou
    KTVEffectCategoryRobot,         //机器人
    KTVEffectCategoryFreeReverb,    //自由混响
    KTVEffectCategoryHarmonic       //和声音效
};

// 目前几种效果对应的AUGraph结构
typedef NS_ENUM(NSUInteger, KTVEffectGraph)
{
    KTVEffectGraphPitch = 0,        //只有pitch node 的结构
    KTVEffectGraphEQ,               //limiter->eq->compressor->reverb 结构
    KTVEffectGraphDSP,              //limiter->eq->compressor->dsp 结构
    KTVEffectGraphDSPReverb,        //limiter->eq->compressor->dsp->reverb 结构
    KTVEffectGraphEQPitch           //limiter->eq->compressor->reverb->pitch 结构
};


typedef struct KTVEffectMode {
    KTVEffectCategory category;
    UInt16 eqLevel;             //对于无EQ level的，eqLevel 为0，对于有EQ level的 0->10（低沉->明亮）, 默认5 DEFAULT_EQ_LEVEL
} KTVEffectMode;

static inline KTVEffectMode KTVEffectModeMake(KTVEffectCategory category,UInt16 eqLevel)
{
    KTVEffectMode mode = {category, eqLevel};
    return mode;
}

static inline KTVEffectCategory KTVPlayStartModeCategory()
{
    return KTVEffectCategoryRB;
}

static inline BOOL isEffectModeForRec(KTVEffectMode mode)
{
    return mode.category < KTVPlayStartModeCategory();
}

static inline BOOL isEffectModeHasEQLevel(KTVEffectMode mode)
{
    BOOL hasEQ = NO;
    switch (mode.category) {
        case KTVEffectCategoryRB:
        case KTVEffectCategoryRock:
        case KTVEffectCategoryPopular:
        case KTVEffectCategoryDance:
        case KTVEffectCategoryNewAge:
        case KTVEffectCategoryPlayOrigin:
        case KTVEffectCategoryAutoTone:
        case KTVEffectCategoryDoubleYou:
        case KTVEffectCategoryFreeReverb:
        case KTVEffectCategoryHarmonic:
            hasEQ = YES;
            break;
        default:
            break;
    }
    return hasEQ;
}

static inline KTVEffectGraph getEffectModesGraph(KTVEffectMode mode)
{
    KTVEffectGraph conrrespodingGraph = KTVEffectGraphEQ;
    switch (mode.category) {
        case KTVEffectCategoryCatty:
        case KTVEffectCategoryDoggy:
        case KTVEffectCategoryBaby:
            conrrespodingGraph = KTVEffectGraphPitch;
            break;
        case KTVEffectCategoryAutoTone:
        case KTVEffectCategoryDoubleYou:
        case KTVEffectCategoryHarmonic:
            conrrespodingGraph = KTVEffectGraphDSPReverb;
            break;
        case KTVEffectCategoryRobot:
            conrrespodingGraph = KTVEffectGraphEQPitch;
            break;
        default:
            break;
    }
    return conrrespodingGraph;
}

static inline BOOL isTwoEffectModesHasSameGraph(KTVEffectMode mode1, KTVEffectMode mode2)
{
    return (getEffectModesGraph(mode1) == getEffectModesGraph(mode2));
}

@interface KTVAUGraphController : NSObject
{
    AUGraph _graph;
    AUNode _mixerNode;
    AudioStreamBasicDescription _clientFormat32int;
    AudioStreamBasicDescription _clientFormat32float;
    AudioStreamBasicDescription _clientFormat16int;
    Float32 outputMixerGain;
}

#pragma mark- Control

- (void)start;
- (void)stop;

- (void)setMuted:(BOOL)muted forInput:(AudioUnitElement)inputNum;
- (void)setInputVolume:(AudioUnitElement)inputNum value:(AudioUnitParameterValue)value;
- (AudioUnitParameterValue)volumeForInput:(AudioUnitElement)inputNum;
- (void)setOutputVolume:(AudioUnitParameterValue)value;
- (AudioUnitParameterValue)volumeForOutput;
- (BOOL)isMutedForInput:(AudioUnitElement)inputNum;

- (void)graphApplyEffectMode:(KTVEffectMode)effectMode;

#pragma mark- Node Management
//节点操作
- (AUNode)graphAddNodeOfType:(OSType)type SubType:(OSType)subType;
- (void)graphAppendHeadNode:(AUNode)node inScope:(AudioUnitScope)scope inElement:(AudioUnitElement)element;
- (void)graphInsertNode:(AUNode)node inScope:(AudioUnitScope)scope inElement:(AudioUnitElement)element;
- (void)graphRemoveNode:(AUNode)removeNode;
- (void)graphDisconnectNode:(AUNode)disconnectNode;
- (void)graphReplaceNode:(AUNode)removeNode withNode:(AUNode)replaceNode shouldRemoveReplacedNode:(BOOL)shouldRemoveReplacedNode;
- (void)configureStandardProperty:(AUNode)node;
- (AudioUnit)graphUnitOfNode:(AUNode)node;

#pragma mark- Render Notify and Property Listener

- (void)addRenderNofityForNode:(AUNode)node callback:(AURenderCallback)callback;
- (void)addPropertyListenerForNode:(AUNode)node listener:(AudioUnitPropertyListenerProc)listener propertyId:(AudioUnitPropertyID)propertyId;
- (void)removeRenderNofifyForNode:(AUNode)node renderNotify:(AURenderCallback)callback;
- (void)removePropertyListenerForNode:(AUNode)node listener:(AudioUnitPropertyListenerProc)listener  propertyId:(AudioUnitPropertyID)propertyId;

#pragma mark- Graph Management

- (void)initializeAUGraph;
- (void)uninitializeAUGraph;
- (void)startAUGraph;
- (void)stopAUGraph;
- (void)openAUGraph;
- (void)closeAUGraph;
- (void)updateAUGraph;
- (void)showAUGraph;
- (void)disposeAUGraph;
- (void)tearDownAUGraph;
- (void)configureAUGraph;

#pragma mark- Utilities

- (AUNodeConnection)graphGetNodeUpConnection:(AUNode)node;
- (AUNodeConnection)graphGetNodeDownConnection:(AUNode)node;
- (void)resetNodeRenderState:(AUNode)node inScope:(AudioUnitScope)inScope inElement:(AudioUnitElement)inElement;
- (BOOL)isValidConnection:(AUNodeConnection)connection;
- (BOOL)isRemoteIONode:(AUNode)node;
- (BOOL)isRemovebleNode:(AUNode)node;
- (BOOL)isNodeInGraph:(AUNode)node;
- (void)setNodeParameter:(AUNode)node
                          inID:(AudioUnitParameterID)inID
                       inScope:(AudioUnitScope)inScope
                     inElement:(AudioUnitElement)inElement
                       inValue:(AudioUnitParameterValue)inValue
                      inOffset:(UInt32)inBufferOffsetInFrames;
- (AudioUnitParameterValue)getNodeParameter:(AUNode)node
                                       inID:(AudioUnitParameterID)inID
                                    inScope:(AudioUnitScope)inScope
                                  inElement:(AudioUnitElement)inElement;
- (AudioUnitParameterID)getParameterID:(NSString*)parameterKey;
+ (float)adjustDBToParamater:(float)volumeDB minDB:(float)minDB;
+ (double)hardwareSampleRate;
+ (double)liveRoomHardwareSampleRate;
+ (AudioUnitParameterValue)calAutoFadeVolumeOfFrame:(UInt32)currentFrame;
+ (BOOL)isPitchSupported;
@end
