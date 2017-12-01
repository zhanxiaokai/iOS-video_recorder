#ifndef DYNAMIC_DELAY_H
#define DYNAMIC_DELAY_H

#include <inttypes.h>

class DynamicDelay
{
private:
    int16_t* m_delayBuf[2];
    int m_pos;
    int m_dbSize;
	int m_channel;
	int m_sampleRate;
	int m_bufferFlag;
	int m_maxDbSize;
public:
    DynamicDelay(int sampleRate, int delayMicroSecond, int channel);
    void Delay(int16_t* inputSamples, int16_t * outputSamples, uint32_t sampleCount);
	void ResetDelayTime(int delayMicroSecond);
	~DynamicDelay();
};

#endif
