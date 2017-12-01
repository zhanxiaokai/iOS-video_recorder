#include <iostream>
#include <math.h>
#include "hsfeq.h"
using namespace std;
HSFEQ::HSFEQ(double dbGain, double freq, double srate)
{
	double A, omega, sn, cs, beta;

	A = pow(10, dbGain /40);
	omega = 2 * M_PI * freq /srate;
	sn = sin(omega);
	cs = cos(omega);
	beta = sqrt(A + A);

	double a0, a1, a2, b0, b1, b2;
	b0 = A * ((A + 1) + (A - 1) * cs + beta * sn);
	b1 = -2 * A * ((A - 1) + (A + 1) * cs);
	b2 = A * ((A + 1) + (A - 1) * cs - beta * sn);
	a0 = (A + 1) - (A - 1) * cs + beta * sn;
	a1 = 2 * ((A - 1) - (A + 1) * cs);
	a2 = (A + 1) - (A - 1) * cs - beta * sn;
	m_a0 = b0 /a0;
	m_a1 = b1 /a0;
	m_a2 = b2 /a0;
	m_a3 = a1 /a0;
	m_a4 = a2 /a0;
	m_x1 = 0; 
	m_x2 = 0;
	m_y1 = 0;
	m_y2 = 0;
}

int16_t HSFEQ::BiQuad(int16_t inSample)
{
   int result;

   // compute result 
   result = m_a0 * inSample + m_a1 * m_x1 + m_a2 * m_x2 -
       m_a3 * m_y1 - m_a4 * m_y2;

   // shift x1 to x2, inSample to x1 
   m_x2 = m_x1;
   m_x1 = inSample;

   // shift y1 to y2, result to y1 
   m_y2 = m_y1;
   m_y1 = result;
   if (result > 32767)
   {
	   result = 32767;
   }
   if (result < -32768)
   {
	   result = -32768;
   }

   return (int16_t)result;
}

