#include "scale_count.h"
#include <fstream>
using namespace std;
ScaleCount::ScaleCount(int sampleRate)
{
	m_sampleRate = sampleRate;
}

ScaleCount::~ScaleCount()
{
}

void ScaleCount::InitByMelFile(string filename)
{

	m_scaleIdx = 0;
	m_sampleCount = -1;
	m_mel.clear();
	m_scale.clear();
	m_scalePartIdxList.clear();
	m_scaleSetList.clear();

	if (filename != "")
	{
		ifstream is(filename.c_str(), ios::binary);
		is.read(reinterpret_cast<char *>(&m_diffMax), 4);
		int32_t secureDiffMax = 285714 ^ (~m_diffMax);

		int beginTime = 0;
		while (!is.eof())
		{
			vector<int> itMel(4);
			int32_t tmpInt;
			is.read(reinterpret_cast<char *>(&tmpInt), 4);
			itMel[BEGIN_TIME] = tmpInt ^ secureDiffMax;
			if (itMel[BEGIN_TIME] < beginTime)
			{
				break;
			}
			is.read(reinterpret_cast<char *>(&tmpInt), 4);
			itMel[END_TIME] = tmpInt ^ secureDiffMax;
			is.read(reinterpret_cast<char *>(&tmpInt), 4);
			itMel[NOTE] = tmpInt ^ secureDiffMax;
			is.read(reinterpret_cast<char *>(&tmpInt), 4);
			itMel[POSITION] = tmpInt ^ secureDiffMax;
			m_mel.push_back(itMel);
			beginTime = itMel[BEGIN_TIME];
		}
	}
	_AnalyzePartScale();
}

void ScaleCount::InitByMelFileMock(string filename)
{
	m_scaleIdx = 0;
	m_sampleCount = -1;
	m_mel.clear();
	m_scale.clear();
	m_scalePartIdxList.clear();
	m_scaleSetList.clear();

	if (filename != "")
	{
		ifstream is (filename.c_str());

		int beginTime = 0;
		while (!is.eof())
		{
			vector<int> itMel(4);
			is>>itMel[BEGIN_TIME]
				>>itMel[END_TIME]
				>>itMel[NOTE]
				>>itMel[POSITION];
			if (itMel[BEGIN_TIME] < beginTime)
			{
				break;
			}
			beginTime = itMel[BEGIN_TIME];
			m_mel.push_back(itMel);
		}
	}
	_AnalyzePartScale();
}

void ScaleCount::_AnalyzePartScale()
{
	set<NoteType> tmpScale;
	int kneePoint = 0;
	int steadyScaleNum = 0;
	size_t precSize = 0;
	vector<int> seprators;
	seprators.clear();

	for (vector<vector<int> >::iterator it = m_mel.begin(); 
			it != m_mel.end(); 
			it++)
	{
		tmpScale.insert(NoteType(it->at(NOTE)));
		if (tmpScale.size() > precSize)
		{
			steadyScaleNum = 0;
			if (tmpScale.size() > 8)
			{
				seprators.push_back(kneePoint);
				tmpScale.clear();
			}
		}
		else
		{
			steadyScaleNum ++;
			if (steadyScaleNum > 1)
			{
				kneePoint = it - m_mel.begin();
			}
		}
		precSize = tmpScale.size();
	}

	tmpScale.clear();
	seprators.push_back(m_mel.size() - 1);
	vector<int>::iterator itSp = seprators.begin();

	m_scalePartIdxList.clear();
	m_scalePartIdxList.push_back(0);

	m_scaleSetList.clear();
	for (vector<vector<int> >::iterator it = m_mel.begin();
			it != m_mel.end();
			it++)
	{
		tmpScale.insert(NoteType((it->at(NOTE) + N_C - N_A) % 12));
		if ((*itSp) <= (it - m_mel.begin()))
		{
			itSp++;
			if (tmpScale.size() < 5)
			{
				for (int note = int(N_A); note <= int(N_Ab); note++)
				{
					tmpScale.insert(NoteType(note));
				}
			}
			m_scaleSetList.push_back(tmpScale);
			tmpScale.clear();
			m_scalePartIdxList.push_back(it->at(END_TIME));
		}
	}

	if (m_scalePartIdxList.size() > 1)
	{
		m_scalePartIdxList.pop_back();
	}
	else
	{
		tmpScale.clear();
		for (int note = int(N_A); note <= int(N_Ab); note++)
		{
			tmpScale.insert(NoteType(note));
		}
		m_scaleSetList.push_back(tmpScale);
	}
}

int ScaleCount::GetScaleIdx(int sampleCount)
{
	if (sampleCount == m_sampleCount)
	{
		return m_scaleIdx - 1;
	}
	if (sampleCount < m_sampleCount)
	{
		m_scaleIdx = 0;
	}

	m_sampleCount = sampleCount;
	for (int curTime = (sampleCount / ((float)m_sampleRate/1000.0)); 
			(m_scaleIdx < int(m_scalePartIdxList.size())) 
				&& (m_scalePartIdxList[m_scaleIdx] <= curTime);
			m_scaleIdx++);
	return m_scaleIdx - 1;
}

set<ScaleCount::NoteType> ScaleCount::GetScaleSet(int idx)
{
	return m_scaleSetList.at(idx);
}

