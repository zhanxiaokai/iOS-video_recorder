//
//  KTVAUGraphController.m
//  ktv
//
//  Created by liumiao on 11/22/14.
//
//

#import "KTVAUGraphController.h"
#import "CAXException.h"

static const UInt32 kMaximumFramesPerSlice = 4096;
static const int kMaximumConnectionPerNode = 10;
static const int kMaximumHeadInputNode = 3;
@interface KTVAUGraphController()
{
    AUNode _headNodeInput[kMaximumHeadInputNode];
    AUNode _headNodeOutput;
}

@property (nonatomic, strong) NSArray *recordParamArray;
@property (nonatomic, strong) NSArray *playParamArray;

@end
@implementation KTVAUGraphController

- (id)init
{
    self = [super init];
    if (self)
    {
        outputMixerGain = 1.0f;
        [self configureDataFormat];
    }
    return self;
}

- (void)dealloc
{
    [self tearDownAUGraph];
    _mixerNode = 0;
    for(int i = 0; i < kMaximumHeadInputNode; i++)
    {
        _headNodeInput[i] = 0;
    }
    _headNodeOutput = 0;
}

#pragma mark Configure Work

- (void)configureAudioSession
{
    [NSException raise:@"KTVMethodNotImplementedException" format:@"Subclass has to implement this method"];
/*
    sample:
    NSError *error = nil;
    
    // Configure the audio session
    AVAudioSession *sessionInstance = [AVAudioSession sharedInstance];
    
    // our default category -- we change this for conversion and playback appropriately
    [sessionInstance setCategory:AVAudioSessionCategoryPlayAndRecord error:&error];
    XThrowIfError(error.code, "couldn't set audio category");
    
    // activate the audio session
    [sessionInstance setActive:YES error:&error];
    XThrowIfError(error.code, "couldn't set audio session active\n");
 */
    
}

- (void)configureDataFormat
{
    // stereo format
    UInt32 bytesPerSample = sizeof (AudioUnitSampleType);
    _clientFormat32float.mFormatID          = kAudioFormatLinearPCM;
    _clientFormat32float.mFormatFlags       = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
    _clientFormat32float.mBytesPerPacket    = bytesPerSample;
    _clientFormat32float.mFramesPerPacket   = 1;
    _clientFormat32float.mBytesPerFrame     = bytesPerSample;
    _clientFormat32float.mChannelsPerFrame  = 2;
    _clientFormat32float.mBitsPerChannel    = 8 * bytesPerSample;
    _clientFormat32float.mSampleRate        = [KTVAUGraphController hardwareSampleRate];
    
    _clientFormat32int.mFormatID          = kAudioFormatLinearPCM;
    _clientFormat32int.mFormatFlags       = kAudioFormatFlagsAudioUnitCanonical | kAudioFormatFlagIsNonInterleaved;
    _clientFormat32int.mBytesPerPacket    = bytesPerSample;
    _clientFormat32int.mFramesPerPacket   = 1;
    _clientFormat32int.mBytesPerFrame     = bytesPerSample;
    _clientFormat32int.mChannelsPerFrame  = 2;
    _clientFormat32int.mBitsPerChannel    = 8 * bytesPerSample;
    _clientFormat32int.mSampleRate        = [KTVAUGraphController hardwareSampleRate];
    
    _clientFormat16int.mFormatID          = kAudioFormatLinearPCM;
    _clientFormat16int.mFormatFlags       = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    _clientFormat16int.mBytesPerPacket    = bytesPerSample;
    _clientFormat16int.mFramesPerPacket   = 1;
    _clientFormat16int.mBytesPerFrame     = bytesPerSample;
    _clientFormat16int.mChannelsPerFrame  = 2;
    _clientFormat16int.mBitsPerChannel    = 16;
    _clientFormat16int.mSampleRate        = [KTVAUGraphController hardwareSampleRate];
}

- (void)configureAUGraph
{
    try {
        XThrowIfError(NewAUGraph(&_graph), "create graph error");
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        _graph = nil;
    }
    
    _mixerNode = [self graphAddNodeOfType:kAudioUnitType_Mixer SubType:kAudioUnitSubType_MultiChannelMixer];
    
    [self openAUGraph];
    
    [self configureStandardProperty:_mixerNode];
    
    [self showAUGraph];
}


#pragma mark- Node Management

/*
 改变graph流程 ：stop graph（must）-> edit graph(remove/add/connect/disconnect node..., update to be changed) -> graph update（graph changed）->start graph(optional)
 改变property： stop graph（must）-> change property ->start graph(optional)
 改变pramater：change pramater
 */
/*
    向mixer node所对应element所对应scope的末端添加node
    node -> node -
                  \
                    ->mixer -> node ->node
                  /
    node -> node -
    new node ->node -> node -
                             \
                               ->mixer -> node ->node
                             /
               node -> node -
*/
- (void)graphAppendHeadNode:(AUNode)node inScope:(AudioUnitScope)scope inElement:(AudioUnitElement)element
{
    try {
        if (!_graph || ![self isNodeInGraph:node])
        {
            return;
        }
        if (scope == kAudioUnitScope_Input)
        {
            if (element > kMaximumHeadInputNode -1)
            {
                return;
            }
            AUNode relatedHeadNode = _headNodeInput[element];
            
            if (relatedHeadNode == 0)
            {
                XThrowIfError(AUGraphConnectNodeInput(_graph, node, [self isRemoteIONode:node] ? 1 : 0, _mixerNode, element), "graph connect error");
            }
            else
            {
                XThrowIfError(AUGraphConnectNodeInput(_graph, node, [self isRemoteIONode:node] ? 1 : 0, relatedHeadNode, 0), "graph connect error");
            }
            _headNodeInput[element] = node;
        }
        else if(scope == kAudioUnitScope_Output)
        {
            if (element != 0)
            {
                return;
            }
            if (_headNodeOutput == 0)
            {
                XThrowIfError(AUGraphConnectNodeInput(_graph, _mixerNode, 0, node, 0), "graph connect error");
            }
            else
            {
                XThrowIfError(AUGraphConnectNodeInput(_graph, _headNodeOutput, 0, node, 0), "graph connect error");
            }
            _headNodeOutput = node;
        }
        [self updateAUGraph];
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}
/*
     向mixer node所对应element所对应scope插入node
     node -> node -
                   \
                    ->mixer -> node ->node
                   /
     node -> node -
     node ->new node -> node -
                              \
                               ->mixer -> node ->node
                              /
                node -> node -
*/
- (void)graphInsertNode:(AUNode)node inScope:(AudioUnitScope)scope inElement:(AudioUnitElement)element
{
    try {
        if (!_graph || ![self isNodeInGraph:node])
        {
            return;
        }
        if (scope == kAudioUnitScope_Input)
        {
            if (element > kMaximumHeadInputNode - 1)
            {
                return;
            }
            AUNode relatedHeadNode = _headNodeInput[element];
            if (relatedHeadNode == 0)
            {
                [self graphAppendHeadNode:node inScope:scope inElement:element];
                return;
            }
            else
            {
                AUNodeConnection downConnection = [self graphGetNodeDownConnection:relatedHeadNode];
                if (![self isValidConnection:downConnection])
                {
                    return;
                }
                XThrowIfError(AUGraphDisconnectNodeInput(_graph, downConnection.destNode, downConnection.destInputNumber), "graph disconnect error");
                XThrowIfError(AUGraphConnectNodeInput(_graph, downConnection.sourceNode, downConnection.sourceOutputNumber, node, 0), "graph connect error");
                XThrowIfError(AUGraphConnectNodeInput(_graph, node, 0, downConnection.destNode, downConnection.destInputNumber), "graph connect error");
            }
        }
        else if(scope == kAudioUnitScope_Output)
        {
            if (element != 0)
            {
                return;
            }
            if (_headNodeOutput == 0)
            {
                [self graphAppendHeadNode:node inScope:scope inElement:element];
                return;
            }
            else
            {
                AUNodeConnection upConnection = [self graphGetNodeUpConnection:_headNodeOutput];
                if (![self isValidConnection:upConnection])
                {
                    return;
                }
                XThrowIfError(AUGraphDisconnectNodeInput(_graph, upConnection.destNode, upConnection.destInputNumber), "graph disconnect error");
                XThrowIfError(AUGraphConnectNodeInput(_graph, upConnection.sourceNode, upConnection.sourceOutputNumber, node, 0), "graph connect error");
                XThrowIfError(AUGraphConnectNodeInput(_graph, node, 0, upConnection.destNode, upConnection.destInputNumber), "graph connect error");
            }
        }
        [self updateAUGraph];
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (void)graphRemoveNode:(AUNode)removeNode
{
    AUNode emptyNode = 0;
    [self graphReplaceNode:removeNode withNode:emptyNode shouldRemoveReplacedNode:YES];
}

- (void)graphDisconnectNode:(AUNode)disconnectNode
{
    AUNode emptyNode = 0;
    [self graphReplaceNode:disconnectNode withNode:emptyNode shouldRemoveReplacedNode:NO];
}

- (void)graphReplaceNode:(AUNode)removeNode withNode:(AUNode)replaceNode shouldRemoveReplacedNode:(BOOL)shouldRemoveReplacedNode
{
    try {
        if (!_graph || ![self isNodeInGraph:removeNode])
        {
            return;
        }
        if (replaceNode != 0 && ![self isNodeInGraph:replaceNode])
        {
            return;
        }
        // 遍历node的connection，找到node的pre node和next node
        NodeConnectionState nodeConnectionState = NodeConnectionNone;
        UInt32 numInteractions = kMaximumConnectionPerNode * 2;
        AUNodeInteraction interactions[numInteractions];
        
        XThrowIfError(AUGraphGetNodeInteractions(_graph, removeNode, &numInteractions, interactions), "AUGraphGetNodeInteractions");
        
        AUNodeConnection upConnection = [self graphGetNodeUpConnection:removeNode];
        AUNodeConnection downConnection = [self graphGetNodeDownConnection:removeNode];
        if ([self isValidConnection:upConnection])
        {
            nodeConnectionState |= NodeConnectionOnlyUp;
        }
        if ([self isValidConnection:downConnection])
        {
            nodeConnectionState |= NodeConnectionOnlyDown;
        }
        
        // connect pre and next node
        switch (nodeConnectionState)
        {
            case NodeConnectionNone:
                break;
            case NodeConnectionOnlyUp:
            {
                XThrowIfError(AUGraphDisconnectNodeInput(_graph, upConnection.destNode, upConnection.destInputNumber), "graph disconnect error");
                if (_headNodeOutput == removeNode)
                {
                    //valid case
                    if (replaceNode == 0)
                    {
                        //delete only
                        _headNodeOutput = upConnection.sourceNode == _mixerNode ? 0 : upConnection.sourceNode;
                    }
                    else
                    {
                        //replace
                        XThrowIfError(AUGraphConnectNodeInput(_graph, upConnection.sourceNode, upConnection.sourceOutputNumber, replaceNode, 0), "graph connect error");
                        _headNodeOutput = replaceNode;
                    }
                }
            }
                break;
            case NodeConnectionOnlyDown:
            {
                XThrowIfError(AUGraphDisconnectNodeInput(_graph, downConnection.destNode, downConnection.destInputNumber), "graph disconnect error");
                int index = 0;
                BOOL isHeadNode = NO;
                for (; index < kMaximumHeadInputNode; index++)
                {
                    if (_headNodeInput[index] == removeNode)
                    {
                        isHeadNode = YES;
                        break;
                    }
                }
                if(isHeadNode)
                {
                    //valid case
                    if (replaceNode == 0)
                    {
                        //delete only
                        _headNodeInput[index] = downConnection.destNode == _mixerNode ? 0 : downConnection.destNode;
                    }
                    else
                    {
                        //replace
                        XThrowIfError(AUGraphConnectNodeInput(_graph, replaceNode, [self isRemoteIONode:replaceNode] ? 1 : 0, downConnection.destNode, downConnection.destInputNumber), "graph connect error");
                        _headNodeInput[index] = replaceNode;
                    }
                }
            }
                break;
            case NodeConnectionUpAndDown:
            {
                XThrowIfError(AUGraphDisconnectNodeInput(_graph, upConnection.destNode, upConnection.destInputNumber), "graph disconnect error");
                XThrowIfError(AUGraphDisconnectNodeInput(_graph, downConnection.destNode, downConnection.destInputNumber), "graph disconnect error");
                if(replaceNode == 0)
                {
                    //delete only
                    if (![self isRemoteIONode:removeNode])
                    {
                        // remote io node does not need to reconnect it's pre and next node
                        XThrowIfError(AUGraphConnectNodeInput(_graph, upConnection.sourceNode, upConnection.sourceOutputNumber, downConnection.destNode, downConnection.destInputNumber), "graph connect error");
                    }
                    if ([self isRemoteIONode:removeNode])
                    {
                        // find new out head node
                        _headNodeOutput = upConnection.sourceNode == _mixerNode ? 0 : upConnection.sourceNode;
                        // find new in head node
                        int index = 0 ;
                        BOOL isHeadNode = NO;
                        for (; index < kMaximumHeadInputNode; index++)
                        {
                            if (_headNodeInput[index] == removeNode)
                            {
                                isHeadNode = YES;
                                break;
                            }
                        }
                        if(isHeadNode)
                        {
                            _headNodeInput[index] = downConnection.destNode == _mixerNode ? 0 : downConnection.destNode;
                        }
                    }
                }
                else
                {
                    //replace , io can't replace not io, not io also can't replace io
                    if ((![self isRemoteIONode:removeNode] && ![self isRemoteIONode:replaceNode]) || ([self isRemoteIONode:removeNode] && [self isRemoteIONode:replaceNode]))
                    {
                        XThrowIfError(AUGraphConnectNodeInput(_graph, upConnection.sourceNode, upConnection.sourceOutputNumber, replaceNode, 0), "graph connect error");
                        XThrowIfError(AUGraphConnectNodeInput(_graph, replaceNode, [self isRemoteIONode:replaceNode] ? 1 : 0, downConnection.destNode, downConnection.destInputNumber), "graph connect error");
                    }
                    if ([self isRemoteIONode:removeNode] && [self isRemoteIONode:replaceNode])
                    {
                        // set new out head node
                        if (_headNodeOutput == removeNode)
                        {
                            _headNodeOutput = replaceNode;
                        }
                        // set new in head node
                        int index = 0;
                        BOOL isHeadNode = NO;
                        for (; index < kMaximumHeadInputNode; index++)
                        {
                            if (_headNodeInput[index] == removeNode)
                            {
                                isHeadNode = YES;
                                break;
                            }
                        }
                        if(isHeadNode)
                        {
                            _headNodeInput[index] = replaceNode;
                        }
                    }
                }
            }
                break;
            default:
                break;
        }
        if (shouldRemoveReplacedNode) {
            XThrowIfError(AUGraphRemoveNode(_graph, removeNode), "graph remove node error");
        }
        [self updateAUGraph];
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

// 工具方法，供其它方法使用
- (AUNode)graphAddNodeOfType:(OSType)type SubType:(OSType)subType
{
    AUNode node = 0;
    try {
        if (_graph)
        {
            AudioComponentDescription acd;
            acd.componentType = type;
            acd.componentSubType = subType;
            acd.componentManufacturer = kAudioUnitManufacturer_Apple;
            acd.componentFlags = 0;
            acd.componentFlagsMask = 0;
            XThrowIfError(AUGraphAddNode(_graph, &acd, &node), "graph added node error");
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
    return node;
}

#pragma mark- Utilities

- (AUNodeConnection)graphGetNodeUpConnection:(AUNode)node
{
    AUNodeConnection upConnection = {0};
    try {
        if (!_graph || ![self isNodeInGraph:node])
        {
            return upConnection;
        }
        UInt32 numInteractions = kMaximumConnectionPerNode * 2;
        AUNodeInteraction interactions[numInteractions];
        
        XThrowIfError(AUGraphGetNodeInteractions(_graph, node, &numInteractions, interactions), "AUGraphGetNodeInteractions");
        
        for (int i = 0; i < numInteractions; i++)
        {
            if (interactions[i].nodeInteractionType == kAUNodeInteraction_Connection && interactions[i].nodeInteraction.connection.destNode == node)
            {
                upConnection = interactions[i].nodeInteraction.connection;
                break;
            }
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
    return upConnection;
}

- (AUNodeConnection)graphGetNodeDownConnection:(AUNode)node
{
    AUNodeConnection downConnection = {0};
    try {
        if (!_graph || ![self isNodeInGraph:node])
        {
            return downConnection;
        }
        UInt32 numInteractions = kMaximumConnectionPerNode * 2;
        AUNodeInteraction interactions[numInteractions];
        
        XThrowIfError(AUGraphGetNodeInteractions(_graph, node, &numInteractions, interactions), "AUGraphGetNodeInteractions");
        
        for (int i = 0; i < numInteractions; i++)
        {
            if (interactions[i].nodeInteractionType == kAUNodeInteraction_Connection && interactions[i].nodeInteraction.connection.sourceNode == node)
            {
                downConnection = interactions[i].nodeInteraction.connection;
                break;
            }
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
    return downConnection;
}

- (void)resetNodeRenderState:(AUNode)node inScope:(AudioUnitScope)inScope inElement:(AudioUnitElement)inElement
{
    try {
        if (_graph && [self isNodeInGraph:node])
        {
            AudioUnit unit = [self graphUnitOfNode:node];
            XThrowIfError(AudioUnitReset(unit, inScope, inElement), "augraph unit reset error");
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (BOOL)isValidConnection:(AUNodeConnection)connection
{
    return (connection.sourceNode != 0 || connection.sourceOutputNumber != 0 || connection.destNode != 0 || connection.destInputNumber != 0);
}

- (BOOL)isRemoteIONode:(AUNode)node
{
    AudioComponentDescription acd = {0};
    try {
        if (_graph && [self isNodeInGraph:node])
        {
            XThrowIfError(AUGraphNodeInfo(_graph, node, &acd, NULL), "get node info error");
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
    return (acd.componentType == kAudioUnitType_Output && acd.componentSubType == kAudioUnitSubType_RemoteIO);
}

- (BOOL)isRemovebleNode:(AUNode)node
{
    if(!_graph || node == _mixerNode || ![self isNodeInGraph:node])
        return NO;
    return YES;
}

- (BOOL)isNodeInGraph:(AUNode)node
{
    BOOL nodeInGraph = NO;
    try {
        if (!_graph || node == 0)
        {
            return NO;
        }
        UInt32 nodeCount = 0;
        XThrowIfError(AUGraphGetNodeCount(_graph, &nodeCount), "get node count error");
        AUNode tempNode = 0;
        for (int i = 0; i < nodeCount; i++)
        {
            XThrowIfError(AUGraphGetIndNode(_graph, i, &tempNode), "get node for index error");
            if (tempNode == node)
            {
                nodeInGraph = YES;
                break;
            }
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
    return nodeInGraph;
}

#pragma Node Property and Parameter Configure

- (AudioUnit)graphUnitOfNode:(AUNode)node
{
    AudioUnit unit = {0};
    try {
        if (!_graph || ![self isNodeInGraph:node])
        {
            return unit;
        }
        XThrowIfError(AUGraphNodeInfo(_graph, node, NULL, &unit), "get node info error");
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
    return unit;
}

- (void)configureStandardProperty:(AUNode)node
{
    try {
        if (!_graph || ![self isNodeInGraph:node])
        {
            return;
        }
        AudioUnit unit;
        AudioComponentDescription acd;
        XThrowIfError(AUGraphNodeInfo(_graph, node, &acd, &unit), "get node info error");
        
        if (acd.componentType == kAudioUnitType_Output && acd.componentSubType == kAudioUnitSubType_RemoteIO)
        {
            UInt32 one = 1;
            XThrowIfError(AudioUnitSetProperty(unit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &one, sizeof(one)), "remote unit enable io error");
            XThrowIfError(AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &_clientFormat32int, sizeof(_clientFormat32int)), "remote unit set client format error");
        }
        else
        {
            XThrowIfError(AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &_clientFormat32float, sizeof(_clientFormat32float)), "normal unit set client format error");
        }
        XThrowIfError(AudioUnitSetProperty (unit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &kMaximumFramesPerSlice, sizeof (kMaximumFramesPerSlice)), "unit set MaximumFramesPerSlice error");
        
        if (acd.componentType == kAudioUnitType_Effect && acd.componentSubType == kAudioUnitSubType_NBandEQ)
        {
            UInt32 bandNum = EQ_NODE_NUM;
            XThrowIfError(AudioUnitSetProperty(unit, kAUNBandEQProperty_NumberOfBands, kAudioUnitScope_Global, 0, &bandNum, sizeof(bandNum)), "set NumberOfBands error");
            for(int i = 0; i < bandNum; i++)
            {
                XThrowIfError(AudioUnitSetParameter(unit, kAUNBandEQParam_BypassBand + i, kAudioUnitScope_Global, 0, 1, 0), "set BypassBand volume error");
            }
            XThrowIfError(AudioUnitSetParameter(unit, kAUNBandEQParam_GlobalGain, kAudioUnitScope_Global, 0, 0, 0), "set BypassBand volume error");
        }

    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (void)setNodeParameter:(AUNode)node
                    inID:(AudioUnitParameterID)inID
                 inScope:(AudioUnitScope)inScope
               inElement:(AudioUnitElement)inElement
                 inValue:(AudioUnitParameterValue)inValue
                inOffset:(UInt32)inBufferOffsetInFrames
{
    try {
        if (!_graph || ![self isNodeInGraph:node])
        {
            return;
        }
        AudioUnit unit = {0};
        XThrowIfError(AUGraphNodeInfo(_graph, node, NULL, &unit), "get node info error");
        XThrowIfError(AudioUnitSetParameter(unit, inID, inScope, inElement, inValue, inBufferOffsetInFrames), "set Parameter value error");

    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (AudioUnitParameterValue)getNodeParameter:(AUNode)node
                    inID:(AudioUnitParameterID)inID
                 inScope:(AudioUnitScope)inScope
               inElement:(AudioUnitElement)inElement
{
    AudioUnitParameterValue value = 0;
    try {
        if (_graph && [self isNodeInGraph:node])
        {
            AudioUnit unit = {0};
            XThrowIfError(AUGraphNodeInfo(_graph, node, NULL, &unit), "get node info error");
            
            XThrowIfError(AudioUnitGetParameter(unit, inID, inScope, inElement, &value), "get Parameter value error");
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
    return value;
}

- (AudioUnitParameterID)getParameterID:(NSString*)parameterKey
{
    NSArray *paramSpliter = [parameterKey componentsSeparatedByString:@"_"];
    if ([paramSpliter count] < 2)
    {
        return MAGIC_PARAM_ID;
    }
    NSString *typeKey = [paramSpliter firstObject];
    NSString *paramKey = [paramSpliter lastObject];
    if ([typeKey isEqual:@"compressor"])
    {
        if ([paramKey isEqual:@"attackTime"])
        {
            return kDynamicsProcessorParam_AttackTime;
        }
        else if ([paramKey isEqual:@"expensionRatio"])
        {
            return kDynamicsProcessorParam_ExpansionRatio;
        }
        else if ([paramKey isEqual:@"expensionThreshold"])
        {
            return kDynamicsProcessorParam_ExpansionThreshold;
        }
        else if ([paramKey isEqual:@"headRoom"])
        {
            return kDynamicsProcessorParam_HeadRoom;
        }
        else if ([paramKey isEqual:@"masterGain"])
        {
            return kDynamicsProcessorParam_MasterGain;
        }
        else if ([paramKey isEqual:@"releaseTime"])
        {
            return kDynamicsProcessorParam_ReleaseTime;
        }
        else if ([paramKey isEqual:@"threshold"])
        {
            return kDynamicsProcessorParam_Threshold;
        }
        
    }
    else if ([typeKey isEqual:@"equalizer"])
    {
        int index = 0;
        if ([paramSpliter count] >= 3)
        {
            NSString *indexStr = paramSpliter[1];
            index = [indexStr intValue] - 1;
            if (index >= EQ_NODE_NUM)
            {
                return MAGIC_PARAM_ID;
            }
        }
        if ([paramKey isEqual:@"band"])
        {
            return kAUNBandEQParam_Bandwidth + index;
        }
        else if ([paramKey isEqual:@"disable"])
        {
            return kAUNBandEQParam_BypassBand + index;
        }
        else if ([paramKey isEqual:@"samplerate"])
        {
            return kAUNBandEQParam_Frequency + index;
        }
        else if ([paramKey isEqual:@"gain"])
        {
            return kAUNBandEQParam_Gain + index;
        }
        else if ([paramKey isEqual:@"type"])
        {
            return kAUNBandEQParam_FilterType + index;
        }
        
    }
    else if ([typeKey isEqual:@"reverb"])
    {
        if ([paramKey isEqual:@"drywetmix"])
        {
            return kReverb2Param_DryWetMix;
        }
        else if ([paramKey isEqual:@"decay0hz"])
        {
            return kReverb2Param_DecayTimeAt0Hz;
        }
        else if ([paramKey isEqual:@"decaynyquist"])
        {
            return kReverb2Param_DecayTimeAtNyquist;
        }
        else if ([paramKey isEqual:@"gain"])
        {
            return kReverb2Param_Gain;
        }
        else if ([paramKey isEqual:@"maxdelay"])
        {
            return kReverb2Param_MaxDelayTime;
        }
        else if ([paramKey isEqual:@"mindelay"])
        {
            return kReverb2Param_MinDelayTime;
        }
        else if ([paramKey isEqual:@"randomize"])
        {
            return kReverb2Param_RandomizeReflections;
        }
    }
    else if ([typeKey isEqual:@"limiter"])
    {
        if ([paramKey isEqual:@"attackTime"])
        {
            return kLimiterParam_AttackTime;
        }
        else if ([paramKey isEqual:@"decayTime"])
        {
            return kLimiterParam_DecayTime;
        }
        else if ([paramKey isEqual:@"preGain"])
        {
            return kLimiterParam_PreGain;
        }
    }
    else if ([typeKey isEqual:@"pitch"])
    {
        if ([paramKey isEqual:@"enablePeakLocking"])
        {
            return kNewTimePitchParam_EnablePeakLocking;
        }
        else if ([paramKey isEqual:@"overlap"])
        {
            return kNewTimePitchParam_Overlap;
        }
        else if ([paramKey isEqual:@"pitch"])
        {
            return kNewTimePitchParam_Pitch;
        }
        else if ([paramKey isEqual:@"rate"])
        {
            return kNewTimePitchParam_Rate;
        }
    }
    else if ([typeKey isEqual:@"inputmixer"])
    {
        if ([paramKey isEqual:@"gain"])
        {
            return kMultiChannelMixerParam_Volume;
        }
    }
    else if ([typeKey isEqual:@"outputmixer"])
    {
        if ([paramKey isEqual:@"gain"])
        {
            return kMultiChannelMixerParam_Volume;
        }
    }
    return MAGIC_PARAM_ID;
}

#pragma mark- Render Notify and Property Listener

- (void)addRenderNofityForNode:(AUNode)node callback:(AURenderCallback)callback
{
    try {
        if (_graph && [self isNodeInGraph:node])
        {
            AudioUnit unit = [self graphUnitOfNode:node];
            XThrowIfError(AudioUnitAddRenderNotify(unit, callback, (__bridge void *)self), "augraph add render notify error");
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (void)addPropertyListenerForNode:(AUNode)node listener:(AudioUnitPropertyListenerProc)listener propertyId:(AudioUnitPropertyID)propertyId
{
    try {
        if (_graph && [self isNodeInGraph:node])
        {
            AudioUnit unit = [self graphUnitOfNode:node];
            XThrowIfError(AudioUnitAddPropertyListener(unit, propertyId, listener, (__bridge void *)self), "augraph add PropertyListener notify error");
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (void)removeRenderNofifyForNode:(AUNode)node renderNotify:(AURenderCallback)callback
{
    try {
        if (_graph && [self isNodeInGraph:node])
        {
            AudioUnit unit = [self graphUnitOfNode:node];
            XThrowIfError(AudioUnitRemoveRenderNotify(unit, callback, (__bridge void *)self), "augraph remove render notify error");
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

- (void)removePropertyListenerForNode:(AUNode)node listener:(AudioUnitPropertyListenerProc)listener  propertyId:(AudioUnitPropertyID)propertyId
{
    try {
        if (_graph && [self isNodeInGraph:node])
        {
            AudioUnit unit = [self graphUnitOfNode:node];
            XThrowIfError(AudioUnitRemovePropertyListenerWithUserData(unit, propertyId, listener, (__bridge void *)self), "augraph add PropertyListener notify error");
        }
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

+ (float)adjustDBToParamater:(float)volumeDB minDB:(float)minDB {
    NSLog(@"volumeDB is %@, minDB is %@", @(volumeDB), @(minDB));
    float rate = 0;
    if (volumeDB > minDB + 0.1f) {
        rate = powf(10, ((float)volumeDB / 20.0f));
    }
    return rate;
}

+ (double)hardwareSampleRate
{
#warning 需要依赖
//    if ([CommonTools GetIntSettingsByKey:SETTING_KEY_HIGHSAMPLINGMODE defaultInt:0] == 0)
//        return 22050;
    return 44100;
}

+ (double)liveRoomHardwareSampleRate
{
#warning 需要依赖
    NSString* modelIdentifier = [[UIDevice currentDevice] modelIdentifier];
    if (([modelIdentifier rangeOfString:@"iPhone"].location != NSNotFound) && ([modelIdentifier compare:@"iPhone8,0"] == NSOrderedDescending)) {
        return 48000;
    }
//    if ([CommonTools GetIntSettingsByKey:SETTING_KEY_HIGHSAMPLINGMODE defaultInt:0] == 0)
//        return 22050;
    return 44100;
}

+ (AudioUnitParameterValue)calAutoFadeVolumeOfFrame:(UInt32)currentFrame
{
    Float32 index = fmaxf(0.0, fminf(1.0, 1.0 - currentFrame / FADEOUT_DATA_SIZE));
    AudioUnitParameterValue volume = 1 - sinf(index * M_PI / 2);
//    if (volume < ZERO_VOLUME_BOUNDRY) {
//        volume = 0.0;
//    }
    return volume;
}

+ (BOOL)isPitchSupported
{
    return YES;
#warning 需要依赖
//    return [[UIDevice currentDevice] isRecordingPitchSupported];
}

#pragma mark- Graph Management

- (void)initializeAUGraph
{
    if(_graph)
    {
        try {
            Boolean graphIsInitialized;
            XThrowIfError(AUGraphIsInitialized(_graph, &graphIsInitialized), "get graph initialize state error");
            if (!graphIsInitialized)
            {
                XThrowIfError(AUGraphInitialize(_graph), "initialize graph error");
            }
        } catch (CAXException e) {
            char buf[256];
            fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        }
    }
}

- (void)uninitializeAUGraph
{
    if (_graph)
    {
        try {
            Boolean graphIsInitialized;
            XThrowIfError(AUGraphIsInitialized(_graph, &graphIsInitialized), "get graph initialize state error");
            if (graphIsInitialized)
            {
                XThrowIfError(AUGraphUninitialize(_graph), "uninitialize graph error");
            }
        } catch (CAXException e) {
            char buf[256];
            fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        }
    }
}

- (void)startAUGraph
{
    if (_graph)
    {
        try {
            Boolean graphIsRunning;
            XThrowIfError(AUGraphIsRunning(_graph, &graphIsRunning), "get graph running state error");
            if (!graphIsRunning)
            {
                XThrowIfError(AUGraphStart(_graph), "start graph error");
            }
        } catch (CAXException e) {
            char buf[256];
            fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        }
    }
}

- (void)stopAUGraph
{
    if (_graph)
    {
        try {
            Boolean graphIsRunning;
            XThrowIfError(AUGraphIsRunning(_graph, &graphIsRunning), "get graph running state error");
            if (graphIsRunning)
            {
                XThrowIfError(AUGraphStop(_graph), "stop graph error");
            }
        } catch (CAXException e) {
            char buf[256];
            fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        }
    }
}

- (void)openAUGraph
{
    if (_graph)
    {
        try {
            Boolean graphIsOpen;
            XThrowIfError(AUGraphIsOpen(_graph, &graphIsOpen), "get graph open state error");
            if (!graphIsOpen)
            {
                XThrowIfError(AUGraphOpen(_graph), "open graph error");
            }
        } catch (CAXException e) {
            char buf[256];
            fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        }
    }
}

- (void)closeAUGraph
{
    if (_graph)
    {
        try {
            Boolean graphIsOpen;
            XThrowIfError(AUGraphIsOpen(_graph, &graphIsOpen), "get graph open state error");
            if (graphIsOpen)
            {
                XThrowIfError(AUGraphClose(_graph), "close graph error");
            }
        } catch (CAXException e) {
            char buf[256];
            fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        }
    }
}

- (void)updateAUGraph
{
    if (_graph)
    {
        try {
            XThrowIfError(AUGraphUpdate(_graph, NULL), "graph update error");
        } catch (CAXException e) {
            char buf[256];
            fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        }
    }
}

- (void)showAUGraph
{
//    if (_graph)
//    {
//        CAShow(_graph);
//    }
}

- (void)disposeAUGraph
{
    if (_graph)
    {
        try {
            XThrowIfError(DisposeAUGraph(_graph), "deallocates graph error");
        } catch (CAXException e) {
            char buf[256];
            fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        }
    }
}

- (void)tearDownAUGraph
{
    [self stopAUGraph];
    [self uninitializeAUGraph];
    [self closeAUGraph];
    [self disposeAUGraph];
    _graph = NULL;
}

#pragma mark- Controll

- (void)start
{
    [self startAUGraph];
}

- (void)stop
{
    [self stopAUGraph];
}

- (void)setMuted:(BOOL)muted forInput:(AudioUnitElement)inputNum
{
    AudioUnitParameterValue isEnableValue = muted ? 0 : 1 ;
    [self setNodeParameter:_mixerNode inID:kMultiChannelMixerParam_Enable inScope:kAudioUnitScope_Input inElement:inputNum inValue:isEnableValue inOffset:0];
}

- (void)setInputVolume:(AudioUnitElement)inputNum value:(AudioUnitParameterValue)value
{
    [self setNodeParameter:_mixerNode inID:kMultiChannelMixerParam_Volume inScope:kAudioUnitScope_Input inElement:inputNum inValue:value inOffset:0];
}

- (AudioUnitParameterValue)volumeForInput:(AudioUnitElement)inputNum
{
    return [self getNodeParameter:_mixerNode inID:kMultiChannelMixerParam_Volume inScope:kAudioUnitScope_Input inElement:inputNum];
}

- (void)setOutputVolume:(AudioUnitParameterValue)value
{
    //NSLog(@"output volume %@", @(value));
    [self setNodeParameter:_mixerNode inID:kMultiChannelMixerParam_Volume inScope:OUTPUT_VOICE_SCOPE inElement:OUTPUT_VOICE_ELEMENT inValue:value inOffset:0];
}

- (AudioUnitParameterValue)volumeForOutput
{
    return [self getNodeParameter:_mixerNode inID:kMultiChannelMixerParam_Volume inScope:OUTPUT_VOICE_SCOPE inElement:OUTPUT_VOICE_ELEMENT];
}

- (BOOL)isMutedForInput:(AudioUnitElement)inputNum
{
    AudioUnitParameterValue value = [self getNodeParameter:_mixerNode inID:kMultiChannelMixerParam_Enable inScope:kAudioUnitScope_Input inElement:inputNum];
    return value == 1 ? NO : YES;
}

#pragma mark- Effect Mode

- (void)graphApplyEffectMode:(KTVEffectMode)effectMode
{
    NSArray *effectSettingArray = [self buildEffectParamArrayForMode:effectMode];
    Float64 halfSampleRate = _clientFormat32float.mSampleRate / 2;
    [effectSettingArray enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        
        NSDictionary *aNodeSetting = (NSDictionary *)obj;
        NSString *key = [[aNodeSetting allKeys] firstObject];
        AUNode node = [self findNodeForKey:key];
        if (node == 0) {
            return ;
        }
        
        BOOL isEQ =  [[[key componentsSeparatedByString:@"_"] firstObject] isEqualToString:@"equalizer"];
        if (isEQ) {
            for(int i = 0; i < EQ_NODE_NUM; i++)
            {
                [self setNodeParameter:node inID:kAUNBandEQParam_BypassBand + i inScope:kAudioUnitScope_Global inElement:0 inValue:1 inOffset:0];
            }
        }
        
        NSMutableSet *invalidEQIndex = [NSMutableSet set]; // 存放已经禁掉的eq的index
        
        [[aNodeSetting allKeys]enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
            NSString *paramKey = (NSString *)obj;
            NSNumber *number = aNodeSetting[paramKey];
            
            // 下面针对eq加了一次特殊过滤
            // 当eq的samplerate值 > (1/2 format samplerate)时, 要把该eq禁掉(bypass设为1)
            NSArray *paramSpliter = [paramKey componentsSeparatedByString:@"_"];
            NSString *typeKey = [paramSpliter firstObject];
            if ([paramSpliter count] >= 2)
            {
                NSString *paramKey = [paramSpliter lastObject];
                
                if ([typeKey isEqualToString:@"equalizer"])
                {
                    int index = (int)([paramSpliter count] > 2 ? [paramSpliter[1] integerValue] : 1);
                    if ([paramKey isEqualToString:@"samplerate"])
                    {
                        if ((Float32)[number floatValue] - halfSampleRate > 0)
                        {
                            [invalidEQIndex addObject:@(index)];
                            [self setNodeParameter:node inID:kAUNBandEQParam_BypassBand + index - 1 inScope:kAudioUnitScope_Global inElement:0 inValue:1 inOffset:0];
                            return;
                        }
                    }
                    else if ([paramKey isEqualToString:@"disable"])
                    {
                        if ([invalidEQIndex containsObject:@(index)])
                        {
                            return;
                        }
                    }
                }
            }
            
            // 正常设置参数值
            AudioUnitParameterID paramId = [self getParameterID:paramKey];
            if(paramId != MAGIC_PARAM_ID)
            {
                // 针对inputmixer_gain 和outputmixer_gain 要进行特殊处理
                if ([typeKey isEqualToString:@"inputmixer"]) {
                    [self setNodeParameter:node inID:paramId inScope:kAudioUnitScope_Input inElement:0 inValue:(Float32)[number floatValue] inOffset:0];
                } else if ([typeKey isEqualToString:@"outputmixer"]) {
                    [self setNodeParameter:node inID:paramId inScope:kAudioUnitScope_Input inElement:0 inValue:(Float32)([number floatValue] * [self humanLinearDB]) inOffset:0];
                    outputMixerGain = [number floatValue];
                } else {
                    [self setNodeParameter:node inID:paramId inScope:kAudioUnitScope_Global inElement:0 inValue:(Float32)[number floatValue] inOffset:0];
                }
            }
        }];
    }];
#if DEBUG
    //[self printAllNodeInfo:effectMode];
#endif
}

- (Float32)humanLinearDB
{
    return 1.0f;
}

- (void)printAllNodeInfo:(KTVEffectMode)effectMode
{
    [self showAUGraph];
    NSArray *effectSettingArray = [self buildEffectParamArrayForMode:effectMode];
    [effectSettingArray enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        NSDictionary *aNodeSetting = (NSDictionary *)obj;
        NSString *key = [[aNodeSetting allKeys]firstObject];
        AUNode node = [self findNodeForKey:key];
        NSLog(@"debug find node: %d", (int)node);
        [[aNodeSetting allKeys]enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
            NSString *paramKey = (NSString *)obj;
            AudioUnitParameterID paramId = [self getParameterID:paramKey];
            if(paramId != MAGIC_PARAM_ID)
            {
                AudioUnitParameterValue value = [self getNodeParameter:node inID:paramId inScope:kAudioUnitScope_Global inElement:0];
                NSLog(@"debug key: %@ value: %f", paramKey, value);
            }
        }];
    }];
}

- (AUNode)findNodeForKey:(NSString*)paramKey
{
    [NSException raise:@"KTVMethodNotImplementedException" format:@"Subclass has to implement this method"];
    AUNode node = 0;
    return node;
}

- (NSArray*)buildEffectParamArrayForMode:(KTVEffectMode)effectMode
{
    BOOL isForRec = isEffectModeForRec(effectMode);
    NSArray *effectSettingsArray = nil;
    NSString *plistPath = nil;
    if (isForRec)
    {
        if (!_recordParamArray)
        {
            plistPath = [[NSBundle mainBundle] pathForResource: @"record_effect_param" ofType:@"plist"];
            _recordParamArray = [NSArray arrayWithContentsOfURL:[NSURL fileURLWithPath: plistPath]];
        }
        effectSettingsArray = _recordParamArray;
    }
    else
    {
        if (!_playParamArray)
        {
            plistPath = [[NSBundle mainBundle] pathForResource: @"play_back_effect_param" ofType:@"plist"];
            _playParamArray = [NSArray arrayWithContentsOfURL:[NSURL fileURLWithPath: plistPath]];
        }
        effectSettingsArray = _playParamArray;
    }
    
    NSArray *conrrespondingEffect = [NSArray array];
    if (isForRec)
    {
        // 录音参数 一维数组
        if (effectMode.category < [effectSettingsArray count])
        {
            conrrespondingEffect = effectSettingsArray[effectMode.category];
        }
        return conrrespondingEffect;
    }
    
    //回放、保存参数 二维数组
    NSArray *effectSettingArray = [NSArray array];
    NSUInteger categotyIndex = effectMode.category - KTVPlayStartModeCategory();
    if (categotyIndex < [effectSettingsArray count])
    {
        effectSettingArray = effectSettingsArray[categotyIndex];
    }
    if (isEffectModeHasEQLevel(effectMode))
    {
        if (effectMode.eqLevel < [effectSettingArray count])
        {
            conrrespondingEffect = effectSettingArray[effectMode.eqLevel];
        }
    }
    else
    {
        conrrespondingEffect = [effectSettingArray firstObject];
    }
    return conrrespondingEffect;
}


@end
