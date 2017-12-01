#include "mel_chord_ana.h"
#include "scale_analyze.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <string.h>

using namespace std;
MelChordAna::MelChordAna(int sampleRate)
{
	m_sampleRate = sampleRate;
	m_supportFlag = 0;
}

MelChordAna::~MelChordAna()
{
}

void MelChordAna::_InitByOldMelFile(string filename)
{
	if (filename != "")
	{
		int32_t diffMax;
		stringstream sso;
		ifstream is(filename.c_str(), ios::binary);
		is.read(reinterpret_cast<char *>(&diffMax), 4);
		int32_t secureDiffMax = 285714 ^ ( ~diffMax);

		int k = 0;
		while (!is.eof())
		{
			int32_t ti1, ti2, ti3, ti4;
			ti4 = -1;
			is.read(reinterpret_cast<char *>(&ti1), 4);
			ti1 ^= secureDiffMax;
			is.read(reinterpret_cast<char *>(&ti2), 4);
			ti2 ^= secureDiffMax;
			is.read(reinterpret_cast<char *>(&ti3), 4);
			ti3 ^= secureDiffMax;
			is.read(reinterpret_cast<char *>(&ti4), 4);
			if (ti4 == -1)
			{
				break;
			}
			ti4 ^= secureDiffMax;
			sso<< ti1 << "\t" << ti2 << "\t" << ti3 << "\t" << ti4 << endl;
			k++;
		}
		stringstream sso1;
		sso1 << "1\t"<< k << endl;
		sso1 << sso.str();
		_AnalyzeMelString(sso1.str());

	}
}

void MelChordAna::InitByMelFile(string filename, int tag)
{
	_Init(tag);
	if (filename != "")
	{
		size_t pos = filename.rfind(".mel");
		if ((pos != string::npos) && ((pos + 4) == filename.length()))
		{
			_InitByOldMelFile(filename);
			return ;
		}
        pos = filename.rfind(".melp");
        if ((pos != string::npos) && ((pos + 5) == filename.length()))
        {
            std::ifstream t;
            long long length;
            t.open(filename.c_str(), ios::in | ios::binary);      // open input file
            t.seekg(0, std::ios::end);    // go to the end
            length = t.tellg();           // report location (this is the length)
            t.seekg(0, std::ios::beg);    // go back to the beginning
            char *buffer = new char[length + 10];    // allocate memory for a buffer of appropriate dimension
            memset(buffer, 0, (size_t)(length + 10));
            t.read(buffer, (streamsize)length);       // read the whole file into the buffer
            t.close();
			char cVersion[8] = {0};
			memcpy(cVersion, buffer, 4);
			int iVersion = 0;
			iVersion = atoi(cVersion);
			if ((iVersion > _MELP_VERSION) || (iVersion == 0))
			{
				return;
			}
            for(int i = 0; i < length; i++) {
               buffer[i] ^= 285714;
            }
            string s(buffer + 4);
            
            _AnalyzeMelString(s);
            return;
        }
	}
}

void MelChordAna::InitByMelFileMock(string filename, int tag)
{
	_Init(tag);
	if (filename != "")
	{
		size_t pos = filename.rfind(".mel");
		if ((pos != string::npos) && ((pos + 4) == filename.length()))
		{
			cout<< "HERE for Old mel" << endl;
			_InitByOldMelFile(filename);
			return ;
		}
		FILE * fin = fopen(filename.c_str(), "r" );
		char x[102400] = {0};
		fread(x, sizeof(char) , 102400, fin );
		fclose(fin);
		string s(x);
		_AnalyzeMelString(s);
	}
}

void MelChordAna::_Init(int tag)
{
	m_tag = tag;
	m_sampleCount = -1;
	m_chordIdx = 0;
	m_keyIdx = 0;
	m_songFrameIdx = 0;
	m_pitchDiff = 0;
}

void MelChordAna::_AnalyzeMelString(string contents)
{
	int sec;
	int lines;

	contents = _TrimContents(contents);

	if (contents != "")
	{
		stringstream ss(contents);

		while (!ss.eof())
		{

			sec = -1;
			ss >> sec;
			if (sec == -1)
			{
				break;
			}
			ss >> lines;
			vector<int> tmpTime;
			tmpTime.resize(2);
			KeyScale tmpKeyScale;
			char tmpBuf[10240];
			switch (sec)
			{
				case 0: // key
					for (int i = 0; i< lines; i++)
					{
						ss	>> tmpKeyScale.beginTime
							>> tmpKeyScale.endTime
							>> tmpKeyScale.scale
							>> tmpKeyScale.rootNote;
						tmpKeyScale.beginTime = (tmpKeyScale.beginTime / 1000.0) * m_sampleRate;
						tmpKeyScale.endTime = (tmpKeyScale.endTime / 1000.0) * m_sampleRate;
						tmpKeyScale.scale = tmpKeyScale.TransScale(tmpKeyScale.scale);
						tmpKeyScale.rootNote = MelodyNote::TransNote(tmpKeyScale.rootNote);
						m_keyScale.push_back(tmpKeyScale);
					}
					break;
				case 1: // melody
					//ss >> m_melMaxPos;
					m_melMaxPos = 0;
					for (int i = 0; i< lines; i++)
					{
						MelodyNote tmpMN;
						ss  >> tmpMN.beginTimeMs
							>> tmpMN.endTimeMs
							>> tmpMN.note_org
							>> tmpMN.exhibitionPos;
						if (tmpMN.exhibitionPos > m_melMaxPos)
						{
							m_melMaxPos = tmpMN.exhibitionPos;
						}
						tmpMN.beginTime = (tmpMN.beginTimeMs / 1000.0) * m_sampleRate;
						tmpMN.endTime = (tmpMN.endTimeMs / 1000.0) * m_sampleRate;
						tmpMN.note = MelodyNote::TransNote(tmpMN.note_org);
						m_melNote.push_back(tmpMN);
					}
					m_melMaxPos += 1;
					break;
				case 2: // chord
					for (int i = 0; i< lines; i++)
					{
						KeyScale tmpChord;
						ss	>> tmpChord.beginTime
							>> tmpChord.endTime
							>> tmpChord.scale
							>> tmpChord.rootNote;
						tmpChord.rootNote = MelodyNote::TransNote(tmpChord.rootNote);
						tmpChord.beginTime = (tmpChord.beginTime / 1000.0) * m_sampleRate;
						tmpChord.endTime = (tmpChord.endTime / 1000.0) * m_sampleRate;
						tmpChord.scale = tmpChord.TransScale(tmpChord.scale);
						m_chordScale.push_back(tmpChord);
					}
					break;
				case 3: //song frame
					for (int i = 0; i < lines; i++)
					{
						SongFrame sf;
						ss	>> sf.beginTime
							>> sf.endTime
							>> sf.code;
						sf.beginTime = (sf.beginTime / 1000.0) * m_sampleRate;
						sf.endTime = (sf.endTime / 1000.0) * m_sampleRate;
						m_songFrame.push_back(sf);
					}
					break;
				default:
					for (int i = 0; i < lines; i++)
					{
						memset(tmpBuf, 0, sizeof(char) * 10240);
						ss.getline(tmpBuf, 10230);
						if (strlen(tmpBuf) == 0)
						{
							i--;
							continue;
						}
					}
					break;

			} // switch
			
		} // while

		if (m_keyScale.size() == 0)
		{
			_ComputeKeyScale();
		}
		_FormatTime();

		_ComputeChordKeyScale();
		if (m_keyScale.size() > 0)
		{
			m_supportFlag |= MelChordAna::SupportKey;
		}
		if (m_melNote.size() > 0)
		{
			m_supportFlag |= MelChordAna::SupportMelody;
		}
		if (m_chordScale.size() > 0)
		{
			m_supportFlag |= MelChordAna::SupportChord;
		}
		if (m_songFrame.size() > 0)
		{
			m_supportFlag |= MelChordAna::SupportSongFrame;
		}
	}
	else
	{
		return;
	}
}

void MelChordAna::_ComputeChordKeyScale()
{
	m_chordKeyScale = m_keyScale;
	if (m_chordScale.size() == 0)
	{
		int scaleTempl = 0xab5; //A 大调
		for (vector<KeyScale>::iterator it = m_chordKeyScale.begin();
				it != m_chordKeyScale.end();
				it++)
		{
			for (int k = 0; k < 12; k++)
			{
				int tmpScale = KeyScale::ShiftScale(scaleTempl, k); 
				if ((tmpScale | it->scale) == it->scale)
				{

					if (k != it->rootNote)
					{
						it->rootNote = k; //TODO 未更新m_keyScale  的 rootNote
					}
					break;
				}
			}
			int tmpScale = KeyScale::ShiftScale(scaleTempl, it->rootNote);
			if ((tmpScale | it->scale) == tmpScale)
			{
				//tmpScale 是超集
				if (((tmpScale ^ it->scale) & (1<< it->scale)) == 0)
				{
					//差集不包含根音
					it->scale = tmpScale;
					break;
				}
			}
		}
	}
	else
	{
		int j = m_chordScale.size() -1;
		for (int i = m_chordKeyScale.size() -1 ; i >= 0; i--)
		{
			int noteStat[12] = {0};
			int tn = 0;
			for (; (j >= 0) && (m_chordScale[j].beginTime > m_chordKeyScale[i].beginTime ); j--)
			{
				for (int it = 0; it < 12; it++)
				{
					if (m_chordScale[j].scale & (1<<it))
					{
						noteStat[it]++;
						tn ++;
					}
				}

			}
			tn = tn / 20;
			for (int k = 0; k < 12; k++)
			{
				if (noteStat[k] > tn)
				{
					m_chordKeyScale[i].scale |= 1 << k;
				}
			}
		}
	}
}

int MelChordAna::GetToneScoreTempl(vector<MelodyNote>& ret)
{
	ret = m_melNote;
	return m_melMaxPos;
}
void MelChordAna::_ComputeKeyScale()
{
	ScaleAnalyze sa;
	m_keyScale = sa.AnaSplitPos(m_melNote);

}

string MelChordAna::_TrimContents(string contents)
{
	string::size_type tmpPos = contents.find_first_not_of(" \t\n");
	if (tmpPos == string::npos)
	{
		contents = "";
	}
	else
	{
		string::size_type tmpPos2 = contents.find_last_not_of(" \t\n");
		if (tmpPos2 != string::npos)
		{
			contents =  contents.substr(tmpPos, tmpPos2 - tmpPos+1);
		}
		else
		{
			contents =  contents.substr(tmpPos);
		}
	}
	return contents;
}

void MelChordAna::_FormatTime()
{
	if (m_chordScale.size() > 0)
	{
		m_chordScale[0].beginTime = 0;
	}
		int tmpTime = 0;
		for (vector<KeyScale>::reverse_iterator it = m_keyScale.rbegin();
				it != m_keyScale.rend();
				it++)
		{
			if (tmpTime)
			{
				it->endTime = tmpTime;
			}
			tmpTime = it->beginTime;
		}
		if (m_keyScale.size() > 0)
		{
			m_keyScale[0].beginTime = 0;
		}
}

void MelChordAna::_UpdateSampleCount(int sampleCount)
{
	if (sampleCount < m_sampleCount)
	{
		m_chordIdx = 0;
		m_keyIdx = 0;
		//m_melIdx = 0;
		m_songFrameIdx = 0;
	}

	m_sampleCount = sampleCount;
}

int MelChordAna::GetChordIdx(int sampleCount)
{
	_UpdateSampleCount(sampleCount);
	for (;
			(m_chordIdx < int(m_chordScale.size()))
			&& (m_chordScale[m_chordIdx].beginTime <= sampleCount);
			m_chordIdx++);
	return m_chordIdx - 1;
}

int MelChordAna::GetKeyIdx(int sampleCount)
{
	_UpdateSampleCount(sampleCount);

	for (; 
			(m_keyIdx < int(m_keyScale.size())) 
			&& (m_keyScale[m_keyIdx].beginTime <= sampleCount);
			m_keyIdx++);
	return m_keyIdx - 1;

}

int MelChordAna::GetKeyScale()
{
	if (m_keyIdx <= 0)
	{
		return 0xfff;
	}
	return m_keyScale[m_keyIdx - 1].scale;

}


bool MelChordAna::IsNoteInChordKey(int note)
{
	if (m_keyIdx <= 0)
	{
		return false;
	}
	return bool((1 << note) & m_chordKeyScale[m_keyIdx - 1].scale);
}

bool MelChordAna::IsNoteInChord(int note)
{
	if (m_chordIdx <= 0)
	{
		return false;
	}
	if ((1 << note) & m_chordScale[m_chordIdx - 1].scale)
	{
		return true;
	}
	return false;
}

bool MelChordAna::IsInChorus(int sampleCount)
{
	if (m_songFrame.size() == 0)
	{
		return true;
	}
	_UpdateSampleCount(sampleCount);
	for (;
			(m_songFrameIdx < int(m_songFrame.size()))
			&& (m_songFrame[m_songFrameIdx].beginTime <= sampleCount);
			m_songFrameIdx++);
	if (m_songFrameIdx <= 0)
	{
		return false;
	}
	int code = m_songFrame[m_songFrameIdx -1].code ;
	switch (code)
	{
		case 0x5: case 0x11: case 0x21: 
			return true;
			break;
	}
	return false;
}

int MelChordAna::GetSupportFlag()
{
	return m_supportFlag;
}

void MelChordAna::PitchShift(int pitchDiff)
{

	pitchDiff = pitchDiff - m_pitchDiff;
	while (pitchDiff < 0)
	{
		pitchDiff += 12;
	}
	pitchDiff %= 12;
	if (pitchDiff == 0)
	{
		return ;
	}
	m_pitchDiff = pitchDiff;
	for (vector<MelodyNote>::iterator it = m_melNote.begin();
			it != m_melNote.end();
			it++)
	{
		it->note = (it->note + pitchDiff) % 12;
		it->note_org = (it->note_org + pitchDiff) % 12;
	}
	for (vector<KeyScale>::iterator it = m_keyScale.begin();
			it != m_keyScale.end();
			it++)
	{
		it->scale = KeyScale::ShiftScale(it->scale, pitchDiff);
		it->rootNote = (it->rootNote + pitchDiff) % 12;
	}
	for (vector<KeyScale>::iterator it = m_chordKeyScale.begin();
			it != m_chordKeyScale.end();
			it++)
	{
		it->scale = KeyScale::ShiftScale(it->scale, pitchDiff);
		it->rootNote = (it->rootNote + pitchDiff) % 12;
	}
	for (vector<KeyScale>::iterator it = m_chordScale.begin();
			it != m_chordScale.end();
			it++)
	{
		it->scale = KeyScale::ShiftScale(it->scale, pitchDiff);
		it->rootNote = (it->rootNote + pitchDiff) % 12;
	}
}

void MelChordAna::PrintChord()
{
	cout<< "BM: " << m_chordScale[m_chordIdx -1].scale << "\t" ;
}

bool MelChordAna::IsNoteInKey(int note)
{
	if (m_keyIdx <= 0)
	{
		return true;
	}
	return (1<<note) & m_keyScale[m_keyIdx - 1].scale;
}

int MelChordAna::_NoteDistance(int a, int b)
{
	if (a == b)
	{
		return 100;
	}
	int c = a - b;
	c += 24;
	c %= 12;
	c = 12 - c;
	return c;
}


