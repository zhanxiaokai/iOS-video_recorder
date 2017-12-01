#ifndef MEL_TYPES_H
#define MEL_TYPES_H
#include <iostream>
using namespace std;

typedef  int16_t t_Scale;
class MelodyNote
{
public:
	int beginTime;
	int endTime;
	int16_t note;
	int exhibitionPos;
	int beginTimeMs;
	int endTimeMs;
	int16_t note_org;

	static int TransNote(int note)
	{
		return (note + 3) % 12;
	}
};

class KeyScale
{
public:
	int beginTime;
	int endTime;
	t_Scale scale;
	int rootNote; // 0 ~ 11 default -1
	KeyScale()
		:beginTime(0),endTime(0),scale(0),rootNote(-1)
	{}
	static t_Scale TransScale(t_Scale scale)
	{
		int bitDiff = 3; // C - A
		return ShiftScale(scale, bitDiff);
	}
	static t_Scale ShiftScale(t_Scale scale, int bitDiff)
	{
		int t1 = scale & 0xfff;
		t1 = t1 << bitDiff;
		int tmpBit = t1 >> 12;
		t1 |= tmpBit; 
		scale = t1 & 0xfff;
		return scale;
	}
};

class SongFrame
{
public:
	int beginTime;
	int endTime;
	t_Scale code;

};

#endif
