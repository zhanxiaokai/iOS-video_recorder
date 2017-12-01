#include "micro_pitchshift.h"
#include <iostream>
#include <math.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

MicroPitchShift::MicroPitchShift()
{
	m_rateBuffer[0] = NULL;
	m_rateBuffer[1] = NULL;
	m_depositQ = NULL;
	m_stretchBuffer = NULL;
	m_tmpBufferA = NULL;
	m_tmpBufferB = NULL;
	m_tmpBufferC = NULL;
	m_tmpOutBuf = NULL;
	m_tmpSegmentBuf = NULL;

}

MicroPitchShift::~MicroPitchShift()
{
	if (m_rateBuffer[0])
	{
		delete[] m_rateBuffer[0];
		m_rateBuffer[0] = NULL;
	}

	if (m_rateBuffer[1])
	{
		delete[] m_rateBuffer[1];
		m_rateBuffer[1] = NULL;
	}

	if (m_depositQ)
	{
		delete[] m_depositQ;
		m_depositQ = NULL;
	}

	if (m_stretchBuffer)
	{
		delete[] m_stretchBuffer;
		m_stretchBuffer = NULL;
	}

	if (m_tmpBufferA)
	{
		delete[] m_tmpBufferA;
		m_tmpBufferA = NULL;
	}

	if (m_tmpBufferB)
	{
		delete[] m_tmpBufferB;
		m_tmpBufferB = NULL;
	}

	if (m_tmpBufferC)
	{
		delete[] m_tmpBufferC;
		m_tmpBufferC = NULL;
	}

	if (m_tmpOutBuf)
	{
		delete[] m_tmpOutBuf;
		m_tmpOutBuf = NULL;
	}

	if (m_tmpSegmentBuf)
	{
		delete[] m_tmpSegmentBuf;
		m_tmpSegmentBuf = NULL;
	}
}

void MicroPitchShift::ResetDeposit()
{
    memset(m_depositQ, 0, sizeof(int16_t) * (m_depositQCap));
    m_depositQFront = 0;
    m_depositQRear = m_maxOverlap + m_stretchBufCap + 5;
}

void MicroPitchShift::Init(int semitone, int sampleRate)
{
	m_semiton = semitone;
	if (semitone == 0)
	{
		m_stretchBufCap = -1;
		return ;
	}
	
	double deltaIdx = pow(1.000577789506555, semitone);
	m_stretchBufCap = round(1 / fabs( 1- deltaIdx));
	m_stretchBufSize = 0; 
	m_stretchStartPos = 0;
	m_stretchBuffer = new int16_t[m_stretchBufCap];
	memset(m_stretchBuffer, 0, sizeof(int16_t) * m_stretchBufCap);

	m_minOverlap = round(sampleRate / 10000) * 2; //动态overlap 按过零次数估计
	m_defaultOverlap = round(sampleRate / 250) * 2; //动态overlap 按过零次数估计
	m_maxOverlap = round(sampleRate / 125) * 2; //动态overlap 按过零次数估计
	m_overlap[0] = m_defaultOverlap;
	m_overlap[1] = m_defaultOverlap;
	m_defaultRateBufCap = m_defaultOverlap * m_stretchBufCap ;
	m_maxRateBufCap = m_maxOverlap * m_stretchBufCap ;
	m_rateBufCap[0] = m_defaultRateBufCap;
	m_rateBufCap[1] = m_defaultRateBufCap;
	m_zeroCrossSize = m_maxOverlap * 2;
	m_rateBufSize = 0;  
	m_rateBufDrainPos = 0;
	m_curSegIdx = 0;
	m_rateBuffer[0] = new int16_t[m_maxRateBufCap];
	memset(m_rateBuffer[0] , 0, sizeof(int16_t) * m_maxRateBufCap);
	m_rateBuffer[1] = new int16_t[m_maxRateBufCap];
	memset(m_rateBuffer[1] , 0, sizeof(int16_t) * m_maxRateBufCap);

	m_depositQCap = m_maxRateBufCap * 2+ m_maxOverlap + m_stretchBufCap;
	m_depositQ = new int16_t[m_depositQCap];
    ResetDeposit();
	m_total = 0;

	int tmpSize = m_stretchBufCap;
	tmpSize = tmpSize > m_maxOverlap * 2 ? tmpSize : m_maxOverlap * 2;
	tmpSize++;
	m_tmpBufferA = new float[tmpSize];
	m_tmpBufferB = new float[tmpSize];
	m_tmpBufferC = new float[tmpSize];
	m_tmpOutBuf = new int16_t[m_stretchBufCap];
	m_tmpSegmentBuf = new int16_t[m_maxRateBufCap];
}

void MicroPitchShift::Shift(int16_t inBuffer[], int16_t outBuffer[], int nSamples)
{
	if (m_semiton == 0)
	{
		memcpy(outBuffer, inBuffer, sizeof(int16_t) * nSamples);
	}

	int t = 0;
	for (int i = 0; i < nSamples;)
	{
		int startPos;
		int bufSize;
		int size = _Stretch(inBuffer + i, nSamples - i, m_tmpOutBuf, startPos, bufSize);
		for (int j = 0; j < bufSize;)
		{
			int bufSize1;
			int startPos1;
			int size1 = _ChangeRate(m_tmpOutBuf + startPos + j, bufSize -j, m_tmpSegmentBuf, startPos1, bufSize1);
			int outSize = _Push2OutputBuffer(m_tmpSegmentBuf + startPos1, bufSize1, outBuffer + t, i + size - t);
			t+= outSize;
			j += size1;
		}
		i += size;
		m_total += size;
	}
	int16_t ts = outBuffer[t-1];
	for (int i = t-1; i < nSamples; i++)
	{
		outBuffer[i] = ts;
	}
}

int MicroPitchShift::_Push2OutputBuffer(int16_t inBuffer[], int inSize, int16_t outBuffer[], int outSize)
{
	int t = 0;
	while (t < inSize)
	{
		int tSize = inSize - t;
		if ((m_depositQRear + tSize) > m_depositQCap)
		{
			tSize = m_depositQCap - m_depositQRear;
		}
		memcpy(m_depositQ + m_depositQRear, inBuffer + t, tSize * sizeof(int16_t));
		m_depositQRear += tSize;
		if (m_depositQRear == m_depositQCap)
		{
			m_depositQRear = 0;
		}
		t += tSize;
	}

	t = 0;
	while (t < outSize)
	{
		int tSize = m_depositQRear - m_depositQFront;
		if (tSize >= 0 && tSize < 2)
		{
			cout<<"FUCK1!!!" <<endl;
			break;
		}
		if (tSize < 0)
		{
			tSize = m_depositQCap - m_depositQFront;
		}
		if (tSize > (outSize - t))
		{
			tSize = (outSize - t);
		}

		memcpy(outBuffer + t, m_depositQ + m_depositQFront, tSize * sizeof(int16_t));
		m_depositQFront += tSize;
		if (m_depositQFront == m_depositQCap)
		{
			m_depositQFront = 0;
		}
		t += tSize;
	}
	return t;
}

int MicroPitchShift::_ChangeRate(int16_t * inBuffer, int inSize, int16_t * outBuf, int &outStartPos, int &outBufSize)
{
	int size = _PushSample2RateBuffer(inBuffer, inSize); 
	_UpdateOverlap();

	int tmpIdx = (m_curSegIdx + 1)% 2;
	if (m_semiton > 0)
	{
		if (m_rateBufDrainPos >= m_rateBufCap[m_curSegIdx])
		{
			m_rateBufDrainPos -= m_rateBufCap[m_curSegIdx] - m_overlap[m_curSegIdx];
		}

		int tSize = m_rateBufSize - (m_rateBufCap[m_curSegIdx] - 2* m_overlap[m_curSegIdx]);
		if (tSize > 0)
		{
			memcpy(m_rateBuffer[tmpIdx], m_rateBuffer[m_curSegIdx] + (m_rateBufCap[m_curSegIdx] - 2 * m_overlap[m_curSegIdx]), tSize * sizeof(int16_t)); //TODO 优化
		}

		//输出结果
		int tPos = m_rateBufDrainPos;
		int tSize1 = m_rateBufSize - tPos;
		if (tPos < (m_rateBufCap[m_curSegIdx] - m_overlap[m_curSegIdx]))
		{
			memcpy(outBuf + tPos, m_rateBuffer[m_curSegIdx] + tPos, sizeof(int16_t) * tSize1);
			tPos = m_rateBufCap[m_curSegIdx] - m_overlap[m_curSegIdx];
		}
		if (tPos  < m_rateBufSize)
		{
			_MergeSegement(m_rateBuffer[m_curSegIdx] + (m_rateBufCap[m_curSegIdx] - m_overlap[m_curSegIdx]), m_rateBuffer[tmpIdx], outBuf + (m_rateBufCap[m_curSegIdx] - m_overlap[m_curSegIdx]), m_overlap[m_curSegIdx]);
		}
		outStartPos = m_rateBufDrainPos;
		outBufSize = m_rateBufSize - m_rateBufDrainPos;
		//buffer满，换一个buffer
		m_rateBufDrainPos = outStartPos + outBufSize;
		if (m_rateBufSize == m_rateBufCap[m_curSegIdx])
		{
			m_rateBufSize = 2* m_overlap[m_curSegIdx];
			m_rateBufDrainPos = m_overlap[m_curSegIdx]; 
			m_curSegIdx = tmpIdx;
			m_overlap[m_curSegIdx] = m_defaultOverlap;
			m_rateBufCap[m_curSegIdx] = m_defaultRateBufCap;
		}
	}
	else
	{
		//输出结果
		int tPos = m_rateBufSize - size;
		if (tPos < m_overlap[tmpIdx])
		{
			_MergeSegement(m_rateBuffer[tmpIdx] + (m_rateBufCap[tmpIdx] - m_overlap[tmpIdx]), m_rateBuffer[m_curSegIdx], outBuf, m_overlap[tmpIdx]);
			tPos = m_overlap[tmpIdx];
		}
		if (tPos < m_rateBufSize)
		{
			memcpy(outBuf + tPos, m_rateBuffer[m_curSegIdx] + tPos, sizeof(int16_t) * (m_rateBufSize - tPos));
		}
		
		outStartPos = m_rateBufSize - size;
		outBufSize = size;
		if (m_rateBufSize > (m_rateBufCap[m_curSegIdx] - m_overlap[m_curSegIdx]))
		{
			outBufSize = (m_rateBufCap[m_curSegIdx] - m_overlap[m_curSegIdx]) - outStartPos;
		}
		if (outBufSize < 0)
		{
			outBufSize = 0;
		}

		m_rateBufDrainPos = outStartPos + outBufSize;
		if (m_rateBufDrainPos >= (m_rateBufCap[m_curSegIdx] - m_overlap[m_curSegIdx])) //TODO
		{
			m_rateBufDrainPos -= m_rateBufCap[m_curSegIdx] - m_overlap[m_curSegIdx];
		}
		//buffer满，换一个buffer
		if (m_rateBufSize == m_rateBufCap[m_curSegIdx])
		{
			m_rateBufSize = 0;
			m_curSegIdx = tmpIdx;
			m_overlap[m_curSegIdx] = m_defaultOverlap;
			m_rateBufCap[m_curSegIdx]  = m_defaultRateBufCap;
		}
	}
	return size;
}

void MicroPitchShift::_UpdateOverlap()
{
	if ((m_rateBufSize >= m_rateBufCap[m_curSegIdx]/2) && ((m_rateBufSize + 2* m_overlap[m_curSegIdx]) < m_rateBufCap[m_curSegIdx] ))
	{
		int16_t * zeroCrossingWindow = m_rateBuffer[m_curSegIdx] + m_rateBufSize - m_zeroCrossSize;
		//int cnt = _CountOverZero(m_rateBuffer[m_curSegIdx] + m_rateBufSize - m_zeroCrossSize, m_zeroCrossSize);
		int cnt = _CountOverZero(zeroCrossingWindow, m_zeroCrossSize);
        if (cnt <= 0) {
            return;
        }
		int tOverlap = m_zeroCrossSize / cnt / 2 * 2;
		int bestOverlap;
		if ((tOverlap > m_minOverlap) && (tOverlap < m_defaultOverlap/2)) 
		{
			int step = tOverlap << 1;
			while (tOverlap < m_defaultOverlap)
			{
				tOverlap <<= 1;
			}
			int maxStep = tOverlap >> 1;
			int minDiff;
			int tMinOverlap = 1 + ceil(m_rateBufCap[m_curSegIdx]/ (m_stretchBufCap - 2)) ; 
			if (tOverlap < tMinOverlap)
			{
				bestOverlap = -1;
				minDiff = INT_MAX;
			}
			else
			{
				bestOverlap = tOverlap ;
				minDiff = _WaveDifference(zeroCrossingWindow, zeroCrossingWindow + tOverlap, tOverlap);
			}
			for (; step <= maxStep; step <<= 1)
			{
				for (int k = -1; k <=1; k+=2)
				{
					int overlap = tOverlap + k * step ;
					if ((overlap < tMinOverlap) || (overlap > m_maxOverlap)) 
					{
						continue;
					}
					int diff = _WaveDifference(zeroCrossingWindow, zeroCrossingWindow + overlap, overlap);
					if (diff < minDiff)
					{
						bestOverlap = overlap;
						minDiff = diff;
					}
				}
			}
			if (bestOverlap > 0)
			{
				m_overlap[m_curSegIdx] = bestOverlap;
				m_rateBufCap[m_curSegIdx] = m_overlap[m_curSegIdx] * m_stretchBufCap;
			}
		}
	}
}

int MicroPitchShift::_WaveDifference(int16_t bufferA[], int16_t bufferB[], int nSamples)
{
	int ret = 0;
    float sum = 0;
    
    vDSP_vflt16(bufferA, 1, m_tmpBufferA, 1, nSamples);
    vDSP_vflt16(bufferB, 1, m_tmpBufferB, 1, nSamples);
    vDSP_vsub(m_tmpBufferA, 1, m_tmpBufferB, 1, m_tmpBufferC, 1, nSamples);
    vDSP_svemg(m_tmpBufferC, 1, &sum, nSamples);
    ret = (int)(sum);
    
	return ret;
}

int MicroPitchShift::_CountOverZero(int16_t m_rateBuffer[], int m_rateBufSize)
{
    vDSP_vflt16(m_rateBuffer, 1, m_tmpBufferC, 1, m_rateBufSize);
    vDSP_Length lastFoundIndex = 0;
    vDSP_Length totalFoundNum = 0;
    vDSP_nzcros(m_tmpBufferC, 1, INT_MAX, &lastFoundIndex, &totalFoundNum, m_rateBufSize);
    
	return (int)totalFoundNum;
}

int MicroPitchShift::_PushSample2RateBuffer(int16_t inBuffer[], int nSamples)
{
	int retSize = m_rateBufCap[m_curSegIdx] - m_rateBufSize;
	if (m_rateBufSize < m_defaultRateBufCap)
	{
		retSize = m_defaultRateBufCap - m_rateBufSize;
	}
	if (retSize > m_rateBufCap[m_curSegIdx] >> 2)
	{
		retSize = m_rateBufCap[m_curSegIdx] >> 2;
	}
	if (nSamples < retSize)
	{
		retSize = nSamples;
	}
	memcpy(m_rateBuffer[m_curSegIdx] + m_rateBufSize, inBuffer, sizeof(int16_t) * retSize);
	m_rateBufSize += retSize;

	return retSize;
}

int MicroPitchShift::_Stretch(int16_t* inBuffer, int inSize, int16_t * outBuf, int &outStartPos, int &outBufSize)
{
	int size = _PushSample2StretchBuffer(inBuffer, inSize);
	if (m_semiton > 0) // 缩短
	{
		_MergeSegement(m_stretchBuffer, m_stretchBuffer + 1, outBuf, m_stretchBufCap -1);
	}
	else //拉伸
	{
		_MergeSegement(m_stretchBuffer + 1, m_stretchBuffer, outBuf, m_stretchBufCap - 1);
	}
	outStartPos = m_stretchStartPos;
	outBufSize = m_stretchBufSize - m_stretchStartPos - 1;

	if (m_stretchBufSize == m_stretchBufCap)
	{
		if (m_semiton > 0)
		{
			m_stretchBufSize = 0;
		}
		else
		{
			m_stretchBufSize = 2;
			m_stretchBuffer[0] = m_stretchBuffer[m_stretchBufCap - 2];
			m_stretchBuffer[1] = m_stretchBuffer[m_stretchBufCap - 1];
		}
		m_stretchStartPos = 0;
	}
	else
	{
		m_stretchStartPos = m_stretchBufSize -1;
	}
	return size;


}

int MicroPitchShift::_PushSample2StretchBuffer(int16_t inBuffer[], int nSamples)
{
	int retSize = m_stretchBufCap - m_stretchBufSize;
	if (nSamples < retSize)
	{
		retSize = nSamples;
	}
	memcpy(m_stretchBuffer + m_stretchBufSize, inBuffer, sizeof(int16_t) * retSize);
	m_stretchBufSize += retSize;
	return retSize;
}

void MicroPitchShift::_MergeSegement(int16_t bufferA[], int16_t bufferB[], int16_t outBuffer[], int nSamples)
{
    vDSP_vflt16(bufferA, 1, m_tmpBufferA, 1, nSamples);
    vDSP_vflt16(bufferB, 1, m_tmpBufferB, 1, nSamples);
    vDSP_vtmerg(m_tmpBufferA, 1, m_tmpBufferB, 1, m_tmpBufferC, 1, nSamples);
    vDSP_vfixr16(m_tmpBufferC, 1, outBuffer, 1, nSamples);
    
}

