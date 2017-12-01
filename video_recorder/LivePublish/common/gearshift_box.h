//
//  gearshift_box.hpp
//  Pods
//
//  Created by wxl on 2017/5/26.
//
//

#ifndef gearshift_box_h
#define gearshift_box_h

#include "../platform_dependent/platform_4_live_common.h"
#include <map>

#define STRATEGY_REDUCTION_RISE_PLAN_B 2

#define kRTT_0_30     "0-30"
#define kRTT_30_100   "30_100"
#define kRTT_100_200  "100-200"
#define kRTT_200_300  "200-300"
#define kRTT_300_1000  "300-1000"
#define kRTT_1000Plus "1000Plus"

enum shiftStateEnum{
    ShiftStateNone        = 0,
    ShiftStateUp          = 1,
    ShiftStateDown        = 2,
    ShiftStatePeace       = 3,
    ShiftStateBelowLowest = 4,
    ShiftStateAboveHighest= 5,
};

typedef struct WriteTimeInfo {
    int count;
    int totalms;
    
    WriteTimeInfo() {
        this->count = 0;
        this->totalms = 0;
    }
    
    ~WriteTimeInfo() {
    }
} WriteTimeInfo;

typedef struct WriteTimeMappingHelper {
    static void merge(std::map<std::string, WriteTimeInfo*> &source,std::map<std::string, WriteTimeInfo*> &target){
        for(auto it=target.begin();it!=target.end();++it){
            source[it->first]->count += it->second->count;
            source[it->first]->totalms += it->second->totalms;
        }
    }
    
    static void reset(std::map<std::string, WriteTimeInfo*> &source){
        for(auto it=source.begin();it!=source.end();++it){
            it->second->count = 0;
            it->second->totalms = 0;
        }
    }
} WriteTimeMapping;

typedef struct BitrateGear {
    int index;
    int fear;         //恐惧度
    int guilty;       //内疚度
    int exempt;             //豁免
    int guiltyCount;
    int stayCount;
    int bitrate;
    int fps;
    
    static BitrateGear* clone(BitrateGear* target);
    
} BitrateGear;

typedef struct GearTrending {
    BitrateGear *from;
    BitrateGear *to;
    long timeStamp;
    
    /*
     *  判断是升档还是降档、还是没升没降
     */
    int trending();
} GearTrending;

typedef struct GearShiftingBox {
    
private:
    
    const int defaultFearValue = 3;
    BitrateGear *curGear;
    std::map<int,BitrateGear*> mapping;
    GearTrending *latestUpTrending;
    GearTrending *latestDownTrending;
public:
    
    int deltaBitrate;
    int minBitrate;
    int maxBitrate;
    
public:
    GearShiftingBox(int defaultBr, int minBr, int maxBr);
    
    ~GearShiftingBox();
    
    BitrateGear* getCurGear();
    
    /*
     * 最好只在一开始用,最快速度找到一个近似能接受的较低档位
     */
    BitrateGear* shiftGearFast(std::map<std::string, WriteTimeInfo*> writeTimeMapping ,int avgSendBitrate);
    
    /*
     * 直播中急速恶化的情况下用，直接降10%
     */
    BitrateGear* shiftGearEmergency(std::map<std::string, WriteTimeInfo*> writeTimeMapping ,int avgSendBitrate,shiftStateEnum &shiftState);
    
    BitrateGear* autoShiftGear(std::map<std::string, WriteTimeInfo*> writeTimeMapping,shiftStateEnum &shiftState);
    
    int resetDefaultGear(int defaultGearKey);
    
    int curStayCount();
    
    int curBitrate();
    /*
     *  档位停留次数+1,根据码率
     */
    bool increaseCurStayCount();
    
    int curGearFear();
    
    /*
     *  档位停留次数减1
     */
    bool subFear(BitrateGear* gear);
    
    void subBelowGearFear();
    
    void resetCurFear();
    
    /*
     *  当前档位停留次数减1
     */
    bool subCurFear();
    /*
     *  重置档位停留次数
     */
    bool resetStayCount(BitrateGear* gear);
    
    /*
     *  从当前档位升档,返回升高一级的档位,升档只允许升一档
     *  升档前把当前档位fear恢复
     */
    BitrateGear* shiftGearUp();
    
    /*
     *  从当前档位降档，,返回升档后的建议sendbitrate
     *  decreaseGearCount: 下降几档
     */
    BitrateGear* shiftGearDown(int decreaseGearCount,int fearAddition);
    
    /*
     *  获取当前档位更高的档位
     *  addCount是高几档
     *  结果NULL没找到
     */
    BitrateGear* getHigherGear(int addCount);
    
    /*
     *  获取当前档位更低的档位
     *  subCount是低几档
     *  结果NULL没找到
     */
    BitrateGear* getLowerGear(int subCount);
private:
    
    /*
     * bitrate最接近的较低档位
     */
    BitrateGear* roundWithBitrate(int bitrate);
    
    /*
     *  档位停留次数+1,根据码率
     */
    bool increaseStayCount(BitrateGear* gear);
    
    /*
     *  是否含有对应码率的档位
     */
    bool hasBitrateGear(int bitrate,BitrateGear* &theGear);
    
    /*
     *  码率对应档位的Key
     */
    int getKey(int bitrate);
    
    /*
     *  判断当前条件是否允许升档
     *  往上那个档位
     */
    bool canShiftUp(BitrateGear* & upGear);
    
    /*
     *  判断当前条件是否允许降档
     *  往上那个档位
     */
    bool canShiftDown(int decreaseGearCount,BitrateGear* & downGear);
    
    void releaseTrending(GearTrending* trending);
} GearShiftingBox;


#endif /* gearshift_box_h */
