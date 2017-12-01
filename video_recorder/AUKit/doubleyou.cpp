#include <inttypes.h>
#include <iostream>
#include "doubleyou.h"
#include <assert.h>
#include <string.h>
#include <math.h>

using namespace std;

#define PI 3.141592653589793

DoubleYou::DoubleYou()
	:m_leftChordSamples(NULL), m_rightChordSamples(NULL), 
	m_tmpLeftBuf(NULL), m_tmpRightBuf(NULL),m_tmpOutputBuf(NULL),
	m_tmpLeftBuf1(NULL), m_tmpRightBuf1(NULL),m_tmpOutputBuf1(NULL),
	m_tmpLeftPanRateList(NULL), m_tmpRightPanRateList(NULL), m_tmpMidPanRateList(NULL),
	m_volumeRateList(NULL),
	m_tmpBuffer(NULL) ,
	m_eq(NULL), 
	m_pitchShifterLeft(NULL), m_pitchShifterRight(NULL)
{

}

DoubleYou::~DoubleYou()
{
	if (m_pitchShifterRight != NULL)
	{
		delete m_pitchShifterRight;
	}
	m_pitchShifterRight = NULL;

	if (m_pitchShifterLeft != NULL)
	{
		delete m_pitchShifterLeft;
	}
	m_pitchShifterLeft = NULL;

	if (m_eq != NULL)
	{
		delete m_eq;
	}
	m_eq = NULL;

	if (m_rightChordSamples != NULL)
	{
		delete[] m_rightChordSamples;
	}
	m_rightChordSamples = NULL;

	if (m_leftChordSamples != NULL)
	{
		delete[] m_leftChordSamples;
	}

	if (m_leftPanRateDict != NULL)
	{
		delete[] m_leftPanRateDict;
	}
	m_leftPanRateDict = NULL;
	if (m_rightPanRateDict != NULL)
	{
		delete[] m_rightPanRateDict ;
	}
	m_rightPanRateDict = NULL;
	if (m_midPanRateDict != NULL)
	{
		delete[] m_midPanRateDict ;
	}
	m_midPanRateDict = NULL;
	if (m_volumeRateDict != NULL)
	{
		delete[] m_volumeRateDict ;
	}
	m_volumeRateDict = NULL;
	if (m_tmpBuffer)
	{
		delete[] m_tmpBuffer;
	}
	m_tmpBuffer = NULL;

	if (m_tmpLeftBuf)
	{
		delete[] m_tmpLeftBuf;
	}
	m_tmpLeftBuf = NULL;
	
	if (m_tmpRightBuf)
	{
		delete[] m_tmpRightBuf;
	}
	m_tmpRightBuf = NULL;
	
	if (m_tmpOutputBuf)
	{
		delete[] m_tmpOutputBuf;
	}
	m_tmpOutputBuf = NULL;
	
	if (m_tmpLeftBuf1)
	{
		delete[] m_tmpLeftBuf1;
	}
	m_tmpLeftBuf1 = NULL;
	
	if (m_tmpRightBuf1)
	{
		delete[] m_tmpRightBuf1;
	}
	m_tmpRightBuf1 = NULL;
	
	if (m_tmpOutputBuf1)
	{
		delete[] m_tmpOutputBuf1;
	}
	m_tmpOutputBuf1 = NULL;

	if (m_tmpLeftPanRateList)
	{
		delete[] m_tmpLeftPanRateList;
	}
	m_tmpLeftPanRateList = NULL;
	
	if (m_tmpRightPanRateList)
	{
		delete[] m_tmpRightPanRateList;
	}
	m_tmpRightPanRateList = NULL;
	
	if (m_tmpMidPanRateList)
	{
		delete[] m_tmpMidPanRateList;
	}
	m_tmpMidPanRateList = NULL;

	if (m_volumeRateList)
	{
		delete[] m_volumeRateList;
	}
	m_volumeRateList = NULL;
	
}

void DoubleYou::Init(int sampleRate, int channels)
{
	m_fs = sampleRate;
	m_channels = channels;
	m_tmpBuffer = NULL;
	m_tmpBufSize = 0;

	m_delaySampleRight = 828; //TODO 左和声延迟时间
	m_delaySampleLeft = 2901; //TODO 右和声延迟时间

	m_cbPos = 0;
	m_stride = (m_delaySampleLeft > m_delaySampleRight? m_delaySampleLeft : m_delaySampleRight) + 1;
	m_cbSize = m_stride * 3;
	m_stride *= m_channels;

	m_leftChordSamples = new int16_t[m_cbSize];
	memset(m_leftChordSamples, 0, sizeof(int16_t) * m_cbSize);
	m_rightChordSamples = new int16_t[m_cbSize];
	memset(m_rightChordSamples, 0, sizeof(int16_t) * m_cbSize);
	m_tmpLeftBuf = new float[m_cbSize];
	m_tmpRightBuf = new float[m_cbSize];
	m_tmpOutputBuf = new float[m_cbSize];
	m_tmpLeftBuf1 = new float[m_cbSize];
	m_tmpRightBuf1 = new float[m_cbSize];
	m_tmpOutputBuf1 = new float[m_cbSize];

	m_tmpLeftPanRateList = new float[m_cbSize];
	m_tmpRightPanRateList = new float[m_cbSize];
	m_tmpMidPanRateList = new float[m_cbSize];
	m_volumeRateList = new float[m_cbSize];

	//TODO init eq;
	m_eq = new HSFEQ(6, 1306, m_fs);
	m_pitchShifterLeft = new MicroPitchShift();
	m_pitchShifterLeft->Init(-15, sampleRate);
	m_pitchShifterRight = new MicroPitchShift();
	m_pitchShifterRight->Init(15, sampleRate);
	
	m_maxPanPos = sampleRate * 8;
	m_panPos = -m_maxPanPos;

	int dictSize = m_maxPanPos +1;
	m_leftPanRateDict = new  float[dictSize];
	m_rightPanRateDict = new float[dictSize];
	m_midPanRateDict = new   float[dictSize];
	m_volumeRateDict = new   float[dictSize];
	memset(m_leftPanRateDict, 0xffff, sizeof(float) *  dictSize);
	memset(m_rightPanRateDict, 0xffff, sizeof(float) * dictSize);
	memset(m_midPanRateDict, 0xffff, sizeof(float) *   dictSize);
	memset(m_volumeRateDict, 0xffff, sizeof(float) *   dictSize);

}

void DoubleYou::ResetShiftDeposit()
{
    m_pitchShifterLeft->ResetDeposit();
    m_pitchShifterRight->ResetDeposit();
}

int DoubleYou::DYFlow(int16_t inBuffer[], int16_t outBuffer[], int nSamples)
{
	assert((nSamples % m_channels) == 0);
	if (m_tmpBufSize < nSamples)
	{
		if (m_tmpBuffer)
		{
			delete[] m_tmpBuffer;
			m_tmpBuffer = NULL;
		}
		m_tmpBuffer = new int16_t[nSamples];
		m_tmpBufSize = nSamples;
	}

	for (int i = 0; i < nSamples; i+=m_channels)
	{

		int16_t tmpSample = inBuffer[i];
		tmpSample *= 0.5;
		tmpSample = m_eq->BiQuad(tmpSample);
		m_tmpBuffer[i/m_channels] = tmpSample;
	}

	int size = nSamples;
	int todoSize = 0;

	int i; 
	for (i = 0; i < nSamples; i += todoSize)
	{
		if ((m_cbPos + size/m_channels) < m_cbSize)
		{
			todoSize = size;
		}
		else
		{
			todoSize = (m_cbSize - m_cbPos) * m_channels;
		}
		todoSize = todoSize < m_stride? todoSize: m_stride;
		size -= todoSize;

		m_pitchShifterLeft->Shift(m_tmpBuffer + i/m_channels, m_leftChordSamples + m_cbPos, todoSize/m_channels);
		m_pitchShifterRight->Shift(m_tmpBuffer + i/m_channels, m_rightChordSamples + m_cbPos, todoSize/m_channels);

		_MixDelayedChordBatch(inBuffer + i, outBuffer + (2 * i/m_channels), todoSize);//TODO cur position delay
		m_cbPos += todoSize / m_channels;
		if (m_cbPos >= m_cbSize)
		{
			m_cbPos -= m_cbSize;
		}


	}

	return (nSamples * 2 / m_channels);
}

void DoubleYou::_MixDelayedChordBatch(int16_t inBuffer[], int16_t outBuffer[], int nSamples)
{
	//left
	int nDiv2 = nSamples /2;

	int t = 0;
	int t1 = m_cbPos + 1 - m_delaySampleLeft;
	if (t1 < 0)
	{
        vDSP_vflt16(m_leftChordSamples + (t1 + m_cbSize), 1, m_tmpLeftBuf, 1, -t1);
		t = -t1;
	}
	if (t < nDiv2)
	{
        vDSP_vflt16(m_leftChordSamples + t1 + t, 1, m_tmpLeftBuf + t, 1, nDiv2 - t);
	}

	
	t = 0;
	t1 = m_cbPos + 1 - m_delaySampleRight;
	if (t1 < 0)
	{
        vDSP_vflt16(m_rightChordSamples + (t1 + m_cbSize), 1, m_tmpRightBuf, 1, -t1);
		t = -t1;
	}
	if (t < nDiv2)
	{
        vDSP_vflt16(m_rightChordSamples + t1 + t, 1, m_tmpRightBuf + t, 1, nDiv2 - t);
	}

	_GetVolumeRateBatch(m_tmpLeftPanRateList, m_tmpRightPanRateList, m_tmpMidPanRateList, m_volumeRateList, nDiv2);

	_VectorMul(m_tmpLeftBuf, 1, m_volumeRateList, m_tmpLeftBuf, nDiv2);
	_VectorMul(m_tmpRightBuf, 1, m_volumeRateList, m_tmpRightBuf, nDiv2);
    
    float *fInBuffer = new float[nDiv2];
    vDSP_vflt16(inBuffer, 2, fInBuffer, 1, nDiv2);
    
	_VectorMul(fInBuffer, 1, m_volumeRateList, m_tmpOutputBuf, nDiv2);
	_VectorScale(m_tmpOutputBuf, 0.8, nDiv2);

	_VectorMul(m_tmpLeftBuf, 1, m_tmpLeftPanRateList, m_tmpLeftBuf1, nDiv2);
	_VectorMul(m_tmpRightBuf, 1, m_tmpRightPanRateList, m_tmpRightBuf1, nDiv2);
	_VectorMul(m_tmpOutputBuf, 1, m_tmpMidPanRateList, m_tmpOutputBuf1, nDiv2); 
	
    float *fOutBuffer = new float[nSamples];
    
    _MixBatch(m_tmpLeftBuf1, 1, m_tmpRightBuf1, 1, fOutBuffer, 2, nDiv2);
	_MixBatch(fOutBuffer, 2, m_tmpOutputBuf1, 1, fOutBuffer, 2, nDiv2);

	_ConstSub(1, m_tmpLeftPanRateList, m_tmpLeftPanRateList, nDiv2);
	_ConstSub(1, m_tmpRightPanRateList, m_tmpRightPanRateList, nDiv2);
	_ConstSub(1, m_tmpMidPanRateList, m_tmpMidPanRateList, nDiv2);
	_VectorMul(m_tmpLeftBuf, 1, m_tmpLeftPanRateList, m_tmpLeftBuf1, nDiv2);
	_VectorMul(m_tmpRightBuf, 1, m_tmpRightPanRateList, m_tmpRightBuf1, nDiv2);
	_VectorMul(m_tmpOutputBuf, 1, m_tmpMidPanRateList, m_tmpOutputBuf1, nDiv2); 
	_MixBatch(m_tmpLeftBuf1, 1, m_tmpRightBuf1, 1, fOutBuffer + 1, 2, nDiv2);
	_MixBatch(fOutBuffer + 1, 2, m_tmpOutputBuf1, 1, fOutBuffer + 1, 2, nDiv2);
    
    vDSP_vfixr16(fOutBuffer, 1, outBuffer, 1, nSamples);
    delete[] fInBuffer;
    delete[] fOutBuffer;
}

void DoubleYou::_VectorMul(float * inBufferA, int strideA, float * inBufferB, float * outBuffer, int nSamples)
{
    vDSP_vmul(inBufferA, strideA, inBufferB, 1, outBuffer, 1, nSamples);
}

void DoubleYou::_VectorScale(float * inBuffer, float scale, int nSamples)
{
    vDSP_vsmul(inBuffer, 1, &scale, inBuffer, 1, nSamples);
}

void DoubleYou::_ConstSub(float a, float*  inBuffer, float * outBuffer, int nSamples)
{
    float scalar = -1;
    vDSP_vsmsa(inBuffer, 1, &scalar, &a, outBuffer, 1, nSamples);
}

void DoubleYou::_MixBatch(float * inBufferA, int strideA, float * inBufferB, int strideB, float * outBuffer, int strideC, int nSamples)
{
    float *bufferA = new float[strideA * nSamples];
    float *bufferB = new float[strideB * nSamples];
    memcpy(bufferA, inBufferA, sizeof(float) * nSamples * strideA);
    memcpy(bufferB, inBufferB, sizeof(float) * nSamples * strideB);

    /* Cc->outBuffer
     Cc = Aa*Bb;
     Cc = clip Cc;
     Db = abs(Bb);
     Ec = Aa*Db;
     Cc = Ec*Cc;
     Cc = Bb-Cc;
     Cc = Aa+Cc;
     */

    float nearZero = 0.000000001f;

    float intMinScalar = 1.f;
    float divScalar = 32767.f;

    float *bufferD = new float[strideB*nSamples];
    float *bufferE = new float[strideC*nSamples];

    vDSP_vsdiv(bufferB, strideB, &divScalar, bufferD, strideB, nSamples);

    vDSP_vmul(bufferA, strideA, bufferD, strideB, outBuffer, strideC, nSamples);


    vDSP_vthrsc(outBuffer, strideC, &nearZero, &intMinScalar, outBuffer, strideC, nSamples);


    vDSP_vthres(outBuffer, strideC, &nearZero, outBuffer, strideC, nSamples);


    vDSP_vabs(bufferD, strideB, bufferD, strideB, nSamples);
    vDSP_vmul(bufferA, strideA, bufferD, strideB, bufferE, strideC, nSamples);

    vDSP_vmul(outBuffer, strideC, bufferE, strideC, outBuffer, strideC, nSamples);

    vDSP_vsub(outBuffer, strideC, bufferB, strideB, outBuffer, strideC, nSamples);
    vDSP_vadd(bufferA, strideA, outBuffer, strideC, outBuffer, strideC, nSamples);
    delete[] bufferD;
    delete[] bufferE;
    
    delete[] bufferB;
    delete[] bufferA;

}

void DoubleYou::_GetVolumeRateBatch(float * leftPanRateList, float * rightPanRateList, float * midPanRateList, float * volumeRateList, int nSamples)
{
	for (int i = 0; i < nSamples; )
	{
		int n = nSamples - i;
		if (m_panPos >= 0)
		{
			int tn = m_maxPanPos - m_panPos;
			if (n > tn)
			{
				n = tn;
			}
			memcpy(leftPanRateList + i, m_leftPanRateDict + m_panPos, sizeof(float) * n);
			memcpy(rightPanRateList + i, m_rightPanRateDict + m_panPos, sizeof(float) * n);
			memcpy(midPanRateList + i, m_midPanRateDict + m_panPos, sizeof(float) * n);
            memcpy(volumeRateList + i, m_volumeRateDict +m_panPos, sizeof(float) * n);
		}
		else
		{
			int tn = 0 - m_panPos;
			if (n > tn)
			{
				n = tn;
			}
			for ( int j = 0 ; j < n ; j++)
			{
                int k = m_maxPanPos + m_panPos + j;
                float deg = 2 * PI * ( k / float(m_maxPanPos));
                float basePan = cosf(deg);
                midPanRateList[i+j] = basePan * 0.48 + 0.5;
                leftPanRateList[i+j] = midPanRateList[i+j] -0.02;
                rightPanRateList[i+j] = midPanRateList[i+j] + 0.02;
                volumeRateList[i+j] = fabs(sinf(deg) * 0.5) + 0.5;

                m_leftPanRateDict[m_maxPanPos + m_panPos + j] = leftPanRateList[i + j];
				m_rightPanRateDict[m_maxPanPos + m_panPos + j] = rightPanRateList[i + j];
				m_midPanRateDict[m_maxPanPos + m_panPos + j] = midPanRateList[i + j];
                m_volumeRateDict[m_maxPanPos + m_panPos +j] = volumeRateList[i+ j];
			}

		}
		m_panPos += n;
		if (m_panPos >= m_maxPanPos)
		{
			m_panPos -= m_maxPanPos;
		}
		i+= n;
	}
    return ;
}


int16_t DoubleYou::_Mix(float a, float b)
{
	int tmp = a < 0 && b < 0 ? ((int) a + (int) b) - (((int) a * (int) b) / INT16_MIN) : (a > 0 && b > 0 ? ((int) a + (int) b) - (((int) a * (int) b) / INT16_MAX) : a + b);
	return tmp > INT16_MAX ? INT16_MAX : (tmp < INT16_MIN ? INT16_MIN : tmp);
}

