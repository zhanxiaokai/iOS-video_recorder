#include "dynamic_delay.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

DynamicDelay::DynamicDelay(int sampleRate, int maxDelayMicroSecond, int channel)
{
	m_sampleRate = sampleRate;
	m_channel = channel;
    m_maxDbSize = int((double)m_sampleRate * maxDelayMicroSecond / 1000000.0) * m_channel;
    m_maxDbSize += m_channel;
	m_dbSize = m_maxDbSize;
    m_pos = 0;
	m_delayBuf[0] = NULL;
	m_delayBuf[1] = NULL;
    m_delayBuf[0] = new int16_t[m_maxDbSize];
	m_delayBuf[1] = new int16_t[m_maxDbSize];
	m_bufferFlag = 0;
    memset(m_delayBuf[0], 0, sizeof(int16_t) * m_maxDbSize);
    memset(m_delayBuf[1], 0, sizeof(int16_t) * m_maxDbSize);
}

void DynamicDelay::Delay(int16_t* inputSamples, int16_t * outputSamples, uint32_t sampleCount)
{
	for (uint32_t i = 0; i< sampleCount; i++)
	{
		outputSamples[i] = m_delayBuf[m_bufferFlag][m_pos];
		m_delayBuf[m_bufferFlag][m_pos] = inputSamples[i];
		m_pos++;
		if (m_pos >= m_dbSize)
		{
			m_pos -= m_dbSize;
		}
	}
}

void DynamicDelay::ResetDelayTime(int delayMicroSecond)
{
	int dbSize = int((double)m_sampleRate * delayMicroSecond / 1000000.0) * m_channel;
	dbSize += m_channel;
	if (dbSize > m_maxDbSize)
	{
		dbSize = m_maxDbSize;
	}
	int flag = (m_bufferFlag + 1) % 2;
	memcpy(m_delayBuf[flag], m_delayBuf[m_bufferFlag] + m_pos, (m_dbSize - m_pos) * sizeof(int16_t)); 
	memcpy(m_delayBuf[flag] + (m_dbSize - m_pos), m_delayBuf[m_bufferFlag], m_pos * sizeof(int16_t));
	memset(m_delayBuf[flag] + m_dbSize, 0, sizeof(int16_t) * (m_maxDbSize - m_dbSize));
	m_dbSize = dbSize;
	m_bufferFlag = flag;
	m_pos = 0;
}

DynamicDelay::~DynamicDelay()
{
	if (m_delayBuf[0] != NULL)
	{
		delete[] m_delayBuf[0];
		m_delayBuf[0] = NULL;
	}
	if (m_delayBuf[1] != NULL)
	{
		delete[] m_delayBuf[1];
		m_delayBuf[1] = NULL;
	}
}
