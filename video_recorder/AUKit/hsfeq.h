#ifndef HSFEQ_H
#define HSFEQ_H

#include <stdlib.h>

class HSFEQ
{
private:
		double m_a0 ;
		double m_a1 ;
		double m_a2 ;
		double m_a3 ;
		double m_a4 ;
		int16_t m_x1 ;
		int16_t m_x2 ;
		int16_t m_y1 ;
		int16_t m_y2 ;

public:
	HSFEQ(double dbGain, double freq, double srate);
	int16_t BiQuad(int16_t inSample);
};

#endif 

