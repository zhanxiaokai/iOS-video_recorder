#ifndef MICRO_PITCHSHIFT_H
#define MICRO_PITCHSHIFT_H

#include <stdlib.h>
#include <inttypes.h>
#include <Accelerate/Accelerate.h>

class MicroPitchShift
{
private:

	int m_semiton;
	int m_overlap[2];
	int m_minOverlap;
	int m_maxOverlap;
	int m_defaultOverlap;

	int m_rateBufCap[2];
	//int m_minRateBufCap;
	int m_defaultRateBufCap;
	int m_maxRateBufCap;
	int m_rateBufSize;
	int m_curSegIdx;
	int m_rateBufDrainPos;
	int m_zeroCrossSize;
	int16_t * m_rateBuffer[2];

	int m_stretchBufCap;
	int m_stretchBufSize;
	int16_t * m_stretchBuffer;
	int m_stretchStartPos;

	int m_depositQCap;
	int m_depositQFront;
	int m_depositQRear;
	int16_t * m_depositQ;

	float * m_tmpBufferA;
	float * m_tmpBufferB;
	float * m_tmpBufferC;

	int16_t * m_tmpOutBuf;
	int16_t * m_tmpSegmentBuf;


	int m_total;

	int _PushSample2RateBuffer(int16_t * inBuffer, int nSamples);
	int _Stretch(int16_t* inBuffer, int inSize, int16_t * outBuf, int &outStartPos, int &outBufSize);
	int _PushSample2StretchBuffer(int16_t * inBuffer, int nSamples);
	int _ChangeRate(int16_t * inBuffer, int inSize, int16_t * outBuf, int &outStartPos, int &outBufSize);
	void _MergeSegement(int16_t* bufferA, int16_t *bufferB, int16_t* outBuffer, int nSamples);
	int _CountOverZero(int16_t *m_rateBuffer, int m_rateBufSize);
	int _Push2OutputBuffer(int16_t * inBuffer, int inSize, int16_t * outBuffer, int outSize);
	void _UpdateOverlap();
	int _WaveDifference(int16_t bufferA[], int16_t bufferB[], int nSamples);
public:
	MicroPitchShift();
	~MicroPitchShift();
	void Init(int semitone, int sampleRate);
	void Shift(int16_t*  inBuffer, int16_t * outBuffer, int nSamples);
    void ResetDeposit();
};

#endif
