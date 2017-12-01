#include "scale_analyze.h"
#include <fstream>
#include <iostream>
using namespace std;
/*
 * @param a bit set A
 * @param b bit set B
 * @param k move bit step
 * @return bit set
 */
t_Scale ScaleAnalyze::_XUnion(t_Scale a, t_Scale b, int k)
{
	int c = a << k;
	a = c & 0xfff;
	a |= (c - a) >> 12;
	a |= b;
	return t_Scale(a);
}

/* 
 * @param a bit set
 * @return number of bit one
 */
int ScaleAnalyze::_NoteCount(t_Scale a)
{
	if (m_bsHash[a] == 0)
	{
		int k = 0;
		for (int i = 0; i < 12; i++)
		{
			if (a & (1 << i))
			{
				k++;
			}
		}
		m_bsHash[a] = k;
	}
	return m_bsHash[a];
}

void ScaleAnalyze::_PushBackNote(int note)
{
	while (note < 0)
	{
		note += 12;
	}
	note = note %12;
	if ((m_notePartSet.size() > 0) && ((1<<note) == m_notePartSet.back().back()))
	{
		m_notePartSet.push_back(m_notePartSet.back());
		m_notePartSet.back().push_back(1<< note);
	}
	else
	{
		int k = m_notePartSet.size();
		vector<t_Scale> tmpPS(k + 1, 0);
		tmpPS.back() = 1 << note;
		t_Scale precBs = 1<< note;
		for (vector<t_Scale>::reverse_iterator it = tmpPS.rbegin()+1;
				it != tmpPS.rend();
				it++)
		{
			k--;
			*it = m_notePartSet[k][k] | precBs;
			precBs = *it;
		}
		m_notePartSet.push_back(tmpPS);
	}
}

vector<KeyScale> ScaleAnalyze::AnaSplitPos(vector<MelodyNote> melodyNoteList)
{
	vector<vector<t_Scale> > xUnionPart; // g [i][j] means position with j splits
	vector<vector<int16_t> > splitPath; // p

	vector<int> stepK(12, 0);
	stepK[1] = 11;
	stepK[11] = 2;
	stepK[2] = 10;
	stepK[10] = 0;
	int precSplitNum = 0;
	int prec0UnionSum = 0;

	size_t curPos = 0;
	int maxSplit = -1;
	for (vector<MelodyNote>::iterator it = melodyNoteList.begin();
			it != melodyNoteList.end();
			it++)
	{
		//curPos++;
		_PushBackNote(it->note);
		//cout<< it->beginTime << "\t" << it->endTime << "\t" <<it->note << endl;

		//TODO max split is 10 ????
		m_score.push_back(vector<int16_t>(maxSplit + 2, 0));
		m_score[curPos][0] = _NoteCount(m_notePartSet[curPos][0]);

		xUnionPart.push_back(vector<t_Scale>(maxSplit + 2, 0));
		xUnionPart[curPos][0] = m_notePartSet[curPos][0]; 

		splitPath.push_back(vector<int16_t>(maxSplit + 2, -1));
		
		if ((precSplitNum == 0) && 
				(xUnionPart[curPos][0] == prec0UnionSum))
		{
			curPos++;
			continue;
		}
		prec0UnionSum = xUnionPart[curPos][0];

		for (size_t i = 0; i < curPos; i++)
		{
			size_t tSplitNum = maxSplit + 2;
			if (tSplitNum > i)
			{
				tSplitNum = i;
			}
			for (size_t s = 0; s < tSplitNum; s++)
			{
				size_t spNum = s + 1;

				int retUnionSet = 0xfff;
				int retScore = 20;
				int retPath = -1;
				if ((spNum < xUnionPart[i].size()) && xUnionPart[i][spNum]) // 不增加新的分割点
				{
					retUnionSet = _XUnion(xUnionPart[i][spNum], m_notePartSet[curPos][i +1], 0);
					retScore = _NoteCount(retUnionSet) + spNum;
					retPath = splitPath[i][spNum];
				}

				if ((m_notePartSet[curPos][i + 1] != retUnionSet) &&
						((spNum >=  xUnionPart[i].size()) || xUnionPart[i][spNum] != retUnionSet) ) 
				{ // 增加新的分割点，条件是判断剪枝
					for (int k = 1; k != 0; k = stepK[k])
					{
						if ((s < xUnionPart[i].size()) && (xUnionPart[i][s]))
						{
							int tmpUSet = _XUnion(xUnionPart[i][s], m_notePartSet[curPos][i + 1], k);
							int tmpScore = _NoteCount(tmpUSet) + spNum;
							if (tmpScore < retScore)
							{
								retScore = tmpScore;
								retUnionSet = tmpUSet;
								retPath = i;
								if ((retUnionSet == xUnionPart[i][s]) || (retUnionSet == m_notePartSet[curPos][i + 1]))
								{
									break; //剪枝
								}
							}// endif tmpScore
						} // endif xUnionPart
					}// endfor stepK
				}// endif 新分割点

				if ((spNum < m_score[curPos].size()) && ((m_score[curPos][spNum] == 0) || (m_score[curPos][spNum] > retScore)))
				{ 
					m_score[curPos][spNum] = retScore;
					xUnionPart[curPos][spNum] = retUnionSet;
					splitPath[curPos][spNum] = retPath;
				}

			}// endif s
			
		}// endif i

		int minScore = 1000000;
		int tmpSplitNum = 0;
		for (int s = m_score[curPos].size() - 1; s >=0; s--)
		{
			if ((m_score[curPos][s] > 0) && (minScore > m_score[curPos][s]))
			{
				minScore = m_score[curPos][s];
				tmpSplitNum = s;
			}
		}
		if (tmpSplitNum > maxSplit)
		{
			maxSplit = tmpSplitNum;
		}
		precSplitNum = tmpSplitNum;

		curPos++;
	}// endfor notelist
	return _GenScale(splitPath, melodyNoteList);
}

vector<KeyScale> ScaleAnalyze::_GenScale(vector<vector<int16_t> > splitPath, vector<MelodyNote> melodyNoteList)
{
	int minScore = 1 << 14;
	int splitNum = 0;

	int N = m_notePartSet.size() - 1;
	if (N < 0)
	{
		return vector<KeyScale>(0);
	}
	for (size_t s = 0; s < m_score[N].size(); s++)
	{
		if (( m_score[N][s] > 0) && (minScore > m_score[N][s]))
		{
			minScore = m_score[N][s];
			splitNum = s;
		}
	}

	int x = N;
	KeyScale ks;
	vector<KeyScale> rret;
	for (; splitNum >= 0; splitNum--)
	{
		ks.scale = m_notePartSet[x][splitPath[x][splitNum] + 1];
		ks.endTime = melodyNoteList[x].endTime;
		ks.beginTime = melodyNoteList[splitPath[x][splitNum] +1].beginTime;
		rret.push_back(ks);
		x = splitPath[x][splitNum];
	}

	vector<KeyScale> ret;
	for (vector<KeyScale>::reverse_iterator it = rret.rbegin();
			it != rret.rend();
			it ++)
	{
		ret.push_back(*it);
	}

	//generate root note
	t_Scale precScale = 0;
	size_t j = 0;
	for (vector<KeyScale>::iterator it = ret.begin()+1;
			it != ret.end();
			it++)
	{

		for (; 
				(j < melodyNoteList.size()) 
				&& (melodyNoteList[j].beginTime < it->beginTime); 
				j++);

		int maxTime = 0;
		int precTime = it->beginTime;
		int tmpRootNote = -1;
		for (int k = j-1; (k >= 0) && (melodyNoteList[k].note == melodyNoteList[j-1].note); k--)
		{

			int tmpTime = precTime - melodyNoteList[k].beginTime;
			if (maxTime < tmpTime)
			{
				maxTime = tmpTime;
				tmpRootNote = melodyNoteList[k].note;
			}
			precTime = melodyNoteList[k].beginTime;
		}

		for (size_t k = j;
				(k < melodyNoteList.size())
				&& (precScale & (1 << melodyNoteList[k].note) );
				k++)
		{
			int tmpTime = 100000000 - melodyNoteList[k].beginTime;
			if ((k +1) < melodyNoteList.size())
			{
				tmpTime = melodyNoteList[k+1].beginTime - melodyNoteList[k].beginTime;
			}
			if (maxTime < tmpTime)
			{
				maxTime = tmpTime;
				tmpRootNote = melodyNoteList[k].note;
			}
		}
		(it-1)->rootNote = tmpRootNote;
		precScale = it->scale;
	}
	ret.back().rootNote = melodyNoteList.back().note;
	
	return ret;
}

ScaleAnalyze::ScaleAnalyze()
{
	m_bsHash.resize(0x1000, 0);
	m_notePartSet.clear();
}
