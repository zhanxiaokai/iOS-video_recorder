#ifndef PITCH_BEND_H
#define PITCH_BEND_H
#include <inttypes.h>
#include <string.h> 
#include "mel_chord_ana.h"
class PitchBend
{
public:
	PitchBend();
	~PitchBend();

	float Bend(float * frag, int samplePos);
	void ComputePitch(double inpitch, double inphinc, int samplePos);
	void SetScale(double inpitch);
	void Init(std::string melfilename, int cbsize, int sampleRate, int channels, float tag, float volumeRate, float extraPitch, bool isOnlyChorus);
	void SetOnlyChorus(bool isOnlyChorus);
	void PitchShift(int pitchDiff);
	bool IsSupportSongFrame();
	bool IsValid();
    bool IsSupportMelScore();

private:
	int m_channels;
	double m_phincfact; // factor determining output phase increment
	double m_outphinc; // input phase increment
	int m_iMaxPhaseOut;
	int m_iPhaseOut;
	int m_fs;
	float m_outpitch;
	int m_scaleIdx;
	float m_volumeRate;
	int m_fadeInRate;
	int m_pfScwarp ;


	float m_tag;
	float m_extraPitch;

	bool m_onlyChorus;

	int m_cbsize; // size of circular buffer
	int64_t m_fragsize; // size of fragment in samples
	int m_iNotes[12];
	int m_iPitch2Note[12];
	int m_iNote2Pitch[12];
	unsigned int  m_numNotes;
	int m_iScwarp;
	int m_endSampleNum;
	float m_pfTune ;
	int m_cbord;
	float m_pfSmooth ;
	float* m_cbo; // circular output buffer
	float* m_hannwindow; // length-N hann

	typedef struct tHashFactor
	{
		float f0;
		float f1;
		float f2;
		float f3;
		bool hit;
	} HashFactor;
	HashFactor * m_hashFactor;
	MelChordAna * m_scaleCounter;

	float _ComputeVald(float fHashKey, float * frag, int tid0);

};

#endif
