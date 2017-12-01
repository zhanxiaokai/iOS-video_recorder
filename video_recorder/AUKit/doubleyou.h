#ifndef DOUBLEYOU_H
#define DOUBLEYOU_H

#include <stdlib.h>
#include "hsfeq.h"
#include "micro_pitchshift.h"

class DoubleYou
{
private:
	int m_fs;
	int m_channels;

	int m_delaySampleLeft; // 左和声延迟时间
	int m_delaySampleRight; // 右和声延迟时间

	int m_maxShiftSampleLeft; // 左和声最大容许误差
	int m_maxShiftSampleRight; //

	int64_t m_totalSamples;
	int m_cbPos;
	int m_cbSize;

	int m_stride;

	int16_t * m_leftChordSamples;
	int16_t * m_rightChordSamples;

	float * m_tmpLeftBuf;
	float * m_tmpRightBuf;
	float * m_tmpOutputBuf;
	float * m_tmpLeftBuf1;
	float * m_tmpRightBuf1;
	float * m_tmpOutputBuf1;

	float * m_tmpLeftPanRateList;
	float * m_tmpRightPanRateList;
	float * m_tmpMidPanRateList;
	float * m_volumeRateList;
	
	int16_t * m_tmpBuffer;

	int m_tmpBufSize;
	int m_panPos;
	//float  m_maxPanPos;
	int  m_maxPanPos;

	//float * m_basePanRateDict ;
	float * m_leftPanRateDict ;
	float * m_rightPanRateDict ;
	float * m_midPanRateDict ;
	float * m_volumeRateDict ;
	HSFEQ * m_eq;
	MicroPitchShift * m_pitchShifterLeft;
	MicroPitchShift * m_pitchShifterRight;


public:
	DoubleYou();
	~DoubleYou();
	void Init(int sampleRate, int channels);
	int DYFlow(int16_t inBuffer[], int16_t outBuffer[], int nSamples);
    void ResetShiftDeposit();

private: 
	void _MixDelayedChordBatch(int16_t inBuffer[], int16_t outBuffer[], int nSamples);
	void _GetVolumeRate(float &leftPanRate, float &rightPanRate, float &midPanRate, float &volumeRate);
	int16_t _Mix(float a, float b);
	void _GetVolumeRateBatch(float * leftPanRateList, float * rightPanRateList, float * midPanRateList, float * volumeRate, int nSamples);
	void _VectorMul(float * inBufferA, int strideA, float * inBufferB, float * outBuffer, int nSamples);
	void _VectorScale(float * inBuffer, float scale, int nSamples);
	void _ConstSub(float a, float*  inBuffer, float * outBuffer, int nSamples);
	void _MixBatch(float * inBufferA, int strideA, float * inBufferB, int strideB, float * outBuffer, int strideC, int nSamples);
	

};

#endif
