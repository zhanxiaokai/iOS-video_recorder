//
//  gearshift_box.cpp
//  Pods
//
//  Created by wxl on 2017/5/26.
//
//

#include "gearshift_box.h"
#define LOG_TAG "GearShiftBox"

BitrateGear* BitrateGear::clone(BitrateGear* target){
    BitrateGear *g = new BitrateGear();
    g->index     = target->index;
    g->fear      = target->fear;
    g->guilty    = target->guilty;
    g->stayCount = target->stayCount;
    g->bitrate   = target->bitrate;
    g->fps       = target->fps;
    g->exempt    = target->exempt;
    return g;
}

int GearTrending::trending(){
    if(NULL == from || NULL == to || from->bitrate == to->bitrate)
        return 0;
    else if(to->bitrate > from->bitrate)
        return 1;
    else
        return -1;
}

GearShiftingBox::GearShiftingBox(int defaultBr, int minBr, int maxBr) {
    deltaBitrate = 30;
    minBitrate   = minBr?:300;
    maxBitrate   = maxBr?:800;
    LOGI("码率表:");
    int index = 0;
    for(int it = this->minBitrate; it <= this->maxBitrate; it += this->deltaBitrate){
        BitrateGear *gear = new BitrateGear();
        gear->fear = defaultFearValue;
        gear->stayCount = 0;
        gear->bitrate = it;
        gear->fps = round(float(it-minBitrate)/(maxBitrate-minBitrate)*(24-15))+15;
        gear->exempt = 0;
        gear->index = index;
        index += 1;
        this->mapping[it] = gear;
        LOGI("[Init] 码率表行： bitrate: %d fps: %d",gear->bitrate,gear->fps);
        if(abs(it-defaultBr)<deltaBitrate){
            this->curGear = this->mapping[it];
        }
    }
    this->latestDownTrending = NULL;
    this->latestUpTrending = NULL;
}

GearShiftingBox::~GearShiftingBox() {
    if(NULL != this->curGear)
        this->curGear = NULL;
    for(std::map<int, BitrateGear*>::iterator it = this->mapping.begin(); it != this->mapping.end(); it++){
        delete it->second;
        this->mapping.erase(it);
    }
    this->releaseTrending(this->latestUpTrending);
    this->releaseTrending(this->latestDownTrending);
}

/*
 * 最好只在一开始用,最快速度找到一个近似能接受的较低档位
 */
BitrateGear* GearShiftingBox::shiftGearFast(std::map<std::string, WriteTimeInfo*> writeTimeMapping ,int avgSendBitrate){
    for(auto it=writeTimeMapping.begin();it!=writeTimeMapping.end();++it){
        LOGI("[START MODE] :3秒回调~~~~ %s ms区间命中次数:      %d次    ,耗费%d毫秒",it->first.c_str(),it->second->count,it->second->totalms);
    }
    int targetBitrate = 0;
    //伪binary search，网络恶化情况下，只在在败部不断二分寻找合适档位。
    if ((writeTimeMapping[kRTT_1000Plus]->count>0 ||
         writeTimeMapping[kRTT_300_1000]->count>0 ||
         writeTimeMapping[kRTT_200_300]->count>0  ||
         writeTimeMapping[kRTT_100_200]->count>0) &&
         writeTimeMapping[kRTT_0_30]->count<180){
        int targetBitrate = avgSendBitrate == 0 ?: ((this->curGear->bitrate + this->minBitrate)/2);
        this->resetDefaultGear(MAX(targetBitrate,this->minBitrate));
        //print change log
        if(NULL != this->curGear){
            if (writeTimeMapping[kRTT_1000Plus]->count>0){
                LOGI("[START MODE] :重置当前档位到：%dkbps  命中区间: %s",this->curGear->bitrate,kRTT_1000Plus);
            }else if(writeTimeMapping[kRTT_300_1000]->count>0){
                LOGI("[START MODE] :重置当前档位到：%dkbps  命中区间: %s",this->curGear->bitrate,kRTT_300_1000);
            }else if(writeTimeMapping[kRTT_200_300]->count>0){
                LOGI("[START MODE] :重置当前档位到：%dkbps  命中区间: %s",this->curGear->bitrate,kRTT_200_300);
            }else if(writeTimeMapping[kRTT_100_200]->count>0){
                LOGI("[START MODE] :重置当前档位到：%dkbps  命中区间: %s",this->curGear->bitrate,kRTT_100_200);
            }
        }
        return this->curGear;
    }
    return NULL;
}

/*
 * 直播中急速恶化的情况下用，直接降10%
 */
BitrateGear* GearShiftingBox::shiftGearEmergency(std::map<std::string, WriteTimeInfo*> writeTimeMapping ,int avgSendBitrate,shiftStateEnum &shiftState){
    BitrateGear *targetGear = NULL;
    if (writeTimeMapping[kRTT_1000Plus]->count > 0 ||
        writeTimeMapping[kRTT_300_1000]->totalms >= 1000){
        BitrateGear* lowerGear = this->roundWithBitrate(MAX(0.9*avgSendBitrate,(avgSendBitrate+this->curBitrate())/2));
        if (NULL != this->curGear && NULL != lowerGear &&
            lowerGear->index >= this->curGear->index) {
            lowerGear = getLowerGear(1);
        }
        if(NULL == lowerGear || NULL == this->curGear){
            shiftState = ShiftStateNone;
            return NULL;
        }
        int subCount = this->curGear->index-lowerGear->index;
        if(subCount>0){
            for(auto it=writeTimeMapping.begin();it!=writeTimeMapping.end();++it){
                LOGI("[EMERGENCY MODE] :3秒回调~~~~ %s ms区间命中次数:      %d次    ,耗费%d毫秒",it->first.c_str(),it->second->count,it->second->totalms);
            }
            int originBitrate = this->curGear->bitrate;
            targetGear = this->shiftGearDown(subCount,4);
            LOGI("[EMERGENCY MODE] :3秒紧急情况处理~~~~ 从 %d 下降到 %d",originBitrate,targetGear?targetGear->bitrate:originBitrate);
        }
        else if (0 == subCount) {
            shiftState = ShiftStateBelowLowest;
        }
    }
    return targetGear;
}

BitrateGear* GearShiftingBox::autoShiftGear(std::map<std::string, WriteTimeInfo*> writeTimeMapping,shiftStateEnum &shiftState){
    if (!writeTimeMapping.empty()) {
        LOGI("12秒统计结果如下");
        for(auto it=writeTimeMapping.begin();it!=writeTimeMapping.end();++it){
            LOGI("[NORMAL MODE]~~~ %s ms区间命中次数:      %d次    ,耗费%d毫秒",it->first.c_str(),it->second->count,it->second->totalms);;
        }
    }
    
    BitrateGear* targetGear = NULL;
    if (writeTimeMapping[kRTT_1000Plus]->count>0 ||
        writeTimeMapping[kRTT_300_1000]->totalms >= 1000) {
        int curStayCount = this->curStayCount();
        int subGearCount = 0;
        if (curStayCount>10)
            subGearCount = 1;
        else if (curStayCount>=5 && curStayCount<=10)
            subGearCount = 2;
        else if (curStayCount<5)
            subGearCount = 3;
        
        if(subGearCount >0){
            int originIndex = this->curGear->index;
            BitrateGear* gear = this->shiftGearDown(subGearCount,6);
            if (NULL != gear){
                targetGear = gear;
                subGearCount = originIndex - targetGear->index;
                shiftState = ShiftStateDown;
                if(writeTimeMapping[kRTT_1000Plus]->count>0){
                    LOGI("[NORMAL MODE] 降码率 %d 个档位，设置码率:%d  帧率:%d  生效区间:%s",subGearCount,gear->bitrate,gear->fps, kRTT_1000Plus);
                }
                else if(writeTimeMapping[kRTT_300_1000]->totalms >= 1000){
                    LOGI("[NORMAL MODE] 降码率 %d 个档位，设置码率:%d  帧率:%d  生效区间:%s 耗费：%dms",subGearCount,gear->bitrate,gear->fps, kRTT_300_1000,writeTimeMapping[kRTT_300_1000]->totalms);
                }
            }
            else{
                shiftState = ShiftStateBelowLowest;
            }
        }
        return 0;
    }
    else if (writeTimeMapping[kRTT_300_1000]->count>0) {
        int curStayCount = this->curStayCount();
        int subGearCount = 0;
        if (curStayCount>10)
            subGearCount = 1;
        else if (curStayCount>=5 && curStayCount<=10)
            subGearCount = 2;
        else if (curStayCount<5)
            subGearCount = 3;
        
        if(subGearCount >0){
            int originIndex = this->curGear->index;
            BitrateGear* gear = this->shiftGearDown(subGearCount,3);
            if (NULL != gear){
                targetGear = gear;
                subGearCount = originIndex - targetGear->index;
                shiftState = ShiftStateDown;
                LOGI("[NORMAL MODE] 降码率 %d 个档位，设置码率为:%d  帧率:%d  生效区间:%s",subGearCount,gear->bitrate,gear->fps,kRTT_300_1000);
            }else{
                shiftState = ShiftStateBelowLowest;
            }
        }
    }
    else if (writeTimeMapping[kRTT_200_300]->count > 0) {
        int curStayCount = this->curStayCount();
        int subGearCount = 0;
        if (curStayCount>=5 && curStayCount<=10)
            subGearCount = 1;
        else if (curStayCount<5)
            subGearCount = 2;
        
        if(subGearCount >0){
            int originIndex = this->curGear->index;
            BitrateGear* gear = this->shiftGearDown(subGearCount,2);
            if (NULL != gear){
                targetGear = gear;
                subGearCount = originIndex - targetGear->index;
                shiftState = ShiftStateDown;
                LOGI("[NORMAL MODE] 降码率 %d 个档位，设置码率为:%d  帧率:%d  生效区间:%s",subGearCount,gear->bitrate,gear->fps,kRTT_200_300);
            }else{
                shiftState = ShiftStateBelowLowest;
            }
        }
    }
    else if (writeTimeMapping[kRTT_100_200]->count > 0) {
        int curStayCount = this->curStayCount();
        int subGearCount = 0;
        if (curStayCount<5)
            subGearCount = 1;
        
        if(subGearCount >0){
            int originIndex = this->curGear->index;
            BitrateGear* gear = this->shiftGearDown(subGearCount,1);
            if (NULL != gear){
                targetGear = gear;
                subGearCount = originIndex - targetGear->index;
                shiftState = ShiftStateDown;
                LOGI("[NORMAL MODE] 降码率 %d 个档位，设置码率为:%d  帧率:%d  生效区间:%s",subGearCount,gear->bitrate,gear->fps,kRTT_100_200);
            }else{
                shiftState = ShiftStateBelowLowest;
            }
        }
    }
    else if (writeTimeMapping[kRTT_30_100]->count > 0) {
        this->increaseCurStayCount();
        shiftState = ShiftStatePeace;
        LOGI("[NORMAL MODE] 码率不变 ，生效区间:%s",kRTT_30_100);
    }
    else if (writeTimeMapping[kRTT_0_30]->count > 0) {
        //未达到升档条件，但情况乐观，降低自己和下两级升档恐惧
        BitrateGear *upGear = this->shiftGearUp();
        if (NULL == upGear) {
            this->increaseCurStayCount();
            this->subCurFear();
            this->subBelowGearFear();
            LOGI("[NORMAL MODE] 尝试升码率失败, 当前档位恐惧度:%d, 当前档位码率:%d", this->curGearFear(), this->curGear->bitrate);
        }
        else
        {
            targetGear = upGear;
            shiftState = ShiftStateUp;
            LOGI("[NORMAL MODE] 升码率，设置码率为:%d 帧率:%d",upGear->bitrate,upGear->fps);
        }
    }
    return targetGear;
}

int GearShiftingBox::resetDefaultGear(int defaultGearKey){
    for(int it = this->minBitrate; it <= this->maxBitrate; it += this->deltaBitrate){
        if(abs(it-defaultGearKey)<=deltaBitrate){
            this->curGear = this->mapping[it];
            return this->curGear->bitrate;
        }
    }
    return 0;
}

int GearShiftingBox::curStayCount(){
    if(curGear)
        return curGear->stayCount;
    return 0;
}

/*
 *  档位停留次数+1,根据码率
 */
bool GearShiftingBox::increaseCurStayCount(){
    if(NULL != curGear){
        increaseStayCount(curGear);
        return true;
    }
    return false;
}

int GearShiftingBox::curGearFear(){
    if(NULL != this->curGear)
        return this->curGear->fear;
    return -1;
}

int GearShiftingBox::curBitrate(){
    if(NULL != this->curGear)
        return this->curGear->bitrate;
    return -1;
}
/*
 *  档位停留次数减1
 */
bool GearShiftingBox::subFear(BitrateGear* gear){
    if(NULL == gear)
        return false;
    if(gear->fear-1 >= 0)
        gear->fear -= 1;
    return true;
}

void GearShiftingBox::subBelowGearFear(){
    BitrateGear *below1 = getLowerGear(1);
    if(NULL != below1){
        subFear(below1);
    }
}

void GearShiftingBox::resetCurFear(){
    if(this->curGear->guiltyCount>0)
        this->curGear->fear = defaultFearValue*(1+this->curGear->guilty)<=25?defaultFearValue*(1+this->curGear->guilty):25;
    else
        this->curGear->fear = defaultFearValue;
}

/*
 *  当前档位停留次数减1
 */
bool GearShiftingBox::subCurFear(){
    return subFear(curGear);
}
/*
 *  重置档位停留次数
 */
bool GearShiftingBox::resetStayCount(BitrateGear* gear){
    if(NULL == gear)
        return false;
    gear->stayCount = 0;
    return true;
}

/*
 *  从当前档位升档,返回升高一级的档位,升档只允许升一档
 *  升档前把当前档位fear恢复
 */
BitrateGear* GearShiftingBox::shiftGearUp(){
    BitrateGear *upGear = NULL;
    if(this->canShiftUp(upGear) && NULL != upGear){
        //更新latestUptrending
        this->releaseTrending(this->latestUpTrending);
        this->latestUpTrending =  new GearTrending();
        this->latestUpTrending->from = BitrateGear::clone(this->curGear);
        this->latestUpTrending->to = BitrateGear::clone(upGear);
        this->latestUpTrending->timeStamp = platform_4_live::getCurrentTimeMills();
        
        //升档前重置当前档位恐惧度到合理值（guilty relative）
        this->resetCurFear();
        //升档前恢复停留次数
        this->resetStayCount(this->curGear);
        this->curGear = upGear;
    }
    return upGear;
}

/*
 *  从当前档位降档，,返回升档后的建议sendbitrate
 *  decreaseGearCount: 下降几档
 */
BitrateGear* GearShiftingBox::shiftGearDown(int decreaseGearCount,int fearAddition){
    BitrateGear *downGear = NULL;
    int subGearCount = decreaseGearCount;
    //支持触底，找到个能下降到的档位
    while(!this->canShiftDown(subGearCount, downGear)){
        subGearCount--;
    }
    
    //升了又降回来的档位有一次豁免权，免去一次继续下降档位的惩罚,因为编码器的反射弧长，升了降回来还会继续下探，但这样的档位往往再经历一个周期就会起死回生，因为编码器这事彻底反应过来了
    if(1 == downGear->exempt){
        downGear->exempt = 0;
        LOGI("%d档位使用一次豁免权 , 避免了一次继续降档",downGear->bitrate);
        downGear = NULL;
    }
    
    
    if(subGearCount>0 && NULL != downGear){
        //更新latestUptrending
        this->releaseTrending(this->latestDownTrending);
        this->latestDownTrending =  new GearTrending();
        this->latestDownTrending->from = BitrateGear::clone(this->curGear);
        this->latestDownTrending->to = BitrateGear::clone(downGear);
        this->latestDownTrending->timeStamp = platform_4_live::getCurrentTimeMills();
        
        BitrateGear* upFromGear = NULL;
        BitrateGear* upToGear = NULL;
        //升了又降，更新内疚度,惩罚升高发起档位和升高到达档位
        if(NULL != this->latestUpTrending &&
           this->latestUpTrending->timeStamp < this->latestDownTrending->timeStamp &&
           this->latestUpTrending->to->bitrate == this->latestDownTrending->from->bitrate &&
           this->latestUpTrending->from->bitrate >= this->latestDownTrending->to->bitrate){
            upFromGear = this->mapping[this->latestUpTrending->from->bitrate];
            upFromGear->guiltyCount += 1;
            upFromGear->exempt = 1;//升了又降回来的档位有一次豁免权，免去一次继续下降档位的惩罚
            if(upFromGear->guiltyCount >=3){
                //暂时禁升此档位，设置成30分钟内不准升码率超过此档位
                upFromGear->guilty = MAX(upFromGear->guilty,30);
            }
            else
            {
                //降幅越大越内疚
                upFromGear->guilty += subGearCount;
            }
            LOGI("[down]-------- %dkbps档位的内疚度增加到%d , 已经内疚了%d次",
                   upFromGear->bitrate,
                   upFromGear->guilty,
                   upFromGear->guiltyCount);
            
            
            upToGear = this->mapping[this->latestUpTrending->to->bitrate];
            upToGear->guiltyCount += 1;
            if(upToGear->guiltyCount >=3){
                //暂时禁升此档位，设置成9分钟内不准升码率超过这个此档位
                upToGear->guilty = MAX(upToGear->guilty,30);
            }
            else
            {
                //降幅越大越内疚
                upToGear->guilty += subGearCount;
            }
            LOGI("[down]-------- %dkbps档位的内疚度增加到%d , 已经内疚了%d次",
                   upToGear->bitrate,
                   upToGear->guilty,
                   upToGear->guiltyCount);
            //升了再降最低降到升之前的档位
            downGear = upFromGear;
        }
        //不是升了又降，对当前档位惩罚较轻
        bool upThenDown = (NULL != upFromGear);
        if(!upThenDown)
            this->resetCurFear();
        
        //降档前恢复停留次数
        this->resetStayCount(this->curGear);
        this->curGear = downGear;
        this->curGear->fear += fearAddition;
        
        //升了又降的惩罚,25是个trade off，因为平均直播时长10分钟左右
        if(upFromGear){
            if(NULL != upFromGear){
                upFromGear->fear += (fearAddition*(1+upFromGear->guilty));
                if (upFromGear->fear > 25) {
                    upFromGear->fear = 25;
                }
            }
            if(NULL != upToGear){
                upToGear->fear += (fearAddition*(1+upToGear->guilty));
                if (upToGear->fear > 25) {
                    upToGear->fear = 25;
                }
            }
        }
    }
    return downGear;
}

/*
 *  获取当前档位更高的档位
 *  addCount是高几档
 *  结果NULL没找到
 */
BitrateGear* GearShiftingBox::getHigherGear(int addCount){
    BitrateGear* upGear = NULL;
    //当前档位还有没有更高的档位可升
    if(this->curGear &&
       (this->curGear->bitrate+(addCount*this->deltaBitrate)) < this->maxBitrate){
        upGear = this->mapping[this->curGear->bitrate+(addCount*this->deltaBitrate)];
    }
    return upGear;
}

/*
 *  获取当前档位更低的档位
 *  subCount是低几档
 *  结果NULL没找到
 */
BitrateGear* GearShiftingBox::getLowerGear(int subCount){
    BitrateGear* subGear = NULL;
    //当前档位还有没有更高的档位可升
    if(this->curGear &&
       (this->curGear->bitrate-(subCount*this->deltaBitrate)) >= this->minBitrate){
        subGear = this->mapping[this->curGear->bitrate-(subCount*this->deltaBitrate)];
    }
    return subGear;
}


/*
 * bitrate最接近的较低档位
 */
BitrateGear* GearShiftingBox::roundWithBitrate(int bitrate){
    BitrateGear *gear = NULL;
    for(int it = this->minBitrate; it <= this->maxBitrate; it += this->deltaBitrate){
        //            this->mapping[it]->bitrate;
        if(abs(this->mapping[it]->bitrate-bitrate)<this->deltaBitrate){
            gear = this->mapping[it];
            break;
        }
    }
    if (NULL == gear && bitrate <= this->minBitrate) {
        gear = this->mapping[this->minBitrate];
    }
    return gear;
}

/*
 *  档位停留次数+1,根据码率
 */
bool GearShiftingBox::increaseStayCount(BitrateGear* gear){
    if(!gear)
        return false;
    gear->stayCount++;
    return true;
}

/*
 *  是否含有对应码率的档位
 */
bool GearShiftingBox::hasBitrateGear(int bitrate,BitrateGear* &theGear)
{
    if(bitrate < this->minBitrate ||
       bitrate > this->maxBitrate)
        return false;
    std::map<int, BitrateGear*> :: iterator it = this->mapping.find(this->getKey(bitrate)) ;
    bool result = (it != this->mapping.end());
    if(result)
        theGear = it->second;
    return result;
}

/*
 *  码率对应档位的Key
 */
int GearShiftingBox::getKey(int bitrate){
    return this->minBitrate + ((bitrate - this->minBitrate)/this->deltaBitrate)*this->deltaBitrate;
}

/*
 *  判断当前条件是否允许升档
 *  往上那个档位
 */
bool GearShiftingBox::canShiftUp(BitrateGear* & upGear){
    //当前档位的恐惧值是否为0
    if(NULL == this->curGear || (*(this->curGear)).fear>0)
        return false;
    //当前档位还有没有更高的档位可升
    if((this->curGear->bitrate+this->deltaBitrate) <= this->maxBitrate){
        upGear = this->mapping[this->curGear->bitrate+this->deltaBitrate];
        return true;
    }
    return false;
}

/*
 *  判断当前条件是否允许降档
 *  往上那个档位
 */
bool GearShiftingBox::canShiftDown(int decreaseGearCount,BitrateGear* & downGear){
    //当前档位是否空
    if(NULL == this->curGear)
        return false;
    //当前档位还有没有更低的档位可降
    int key = this->curGear->bitrate-(decreaseGearCount*this->deltaBitrate);
    if(key >= this->minBitrate && this->hasBitrateGear(key, downGear)){
        return true;
    }
    return false;
}

void GearShiftingBox::releaseTrending(GearTrending* trending){
    if(NULL != trending){
        delete trending;
        trending = NULL;
    }
}

BitrateGear*  GearShiftingBox::getCurGear(){
    return this->curGear;
}
