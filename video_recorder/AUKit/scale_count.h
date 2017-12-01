#ifndef SCALE_COUNT_H
#define SCALE_COUNT_H

#include <string>
#include <vector>
#include <set>
#include <stdlib.h>
//#include <inttypes.h>

class ScaleCount
{

public:
	typedef enum
	{
		BEGIN_TIME	=	0,
		END_TIME	=	1,
		NOTE		=	2,
		POSITION	=	3,
	} MelType;

	typedef enum
	{
		N_A			=	0,
		N_Bb		=	1,
		N_B			=	2,
		N_C			=	3,
		N_Db		=	4,
		N_D			=	5,
		N_Eb		=	6,
		N_E			=	7,
		N_F			=	8,
		N_Gb		=	9,
		N_G			=	10,
		N_Ab		=	11,
	} NoteType;

public:
	ScaleCount(int sampleRate);

	~ScaleCount();

	void InitByMelFile(std::string filename);
	
	void InitByMelFileMock(std::string filename);

	void _AnalyzePartScale();

	int GetScaleIdx(int sampleCount);

	std::set<NoteType> GetScaleSet(int idx);




private:

	std::vector<std::vector<int> > m_mel;

	std::vector<std::vector<int> > m_scale;
	
	int32_t m_diffMax;

	std::vector<int> m_scalePartIdxList;

	std::vector<std::set<NoteType> > m_scaleSetList; 

	int m_scaleIdx;
	int m_sampleRate;
	int m_sampleCount;

};

#endif
