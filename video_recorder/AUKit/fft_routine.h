#ifndef FFT_ROUTINE_H
#define FFT_ROUTINE_H

#include <stdlib.h>
#include "mayer_fft.h"
//#include <stdio.h>

// Variables for FFT routine
class FftRoutine
{
private :
	int m_nfft;
	float * m_fft_data;

public:
	// Constructor for FFT routine
	FftRoutine(int nfft)
	{
		m_nfft = nfft;

		m_fft_data = new float[nfft];

	}

	// Destructor for FFT routine
	~FftRoutine()
	{
		delete [] m_fft_data;

	}

	// Perform forward FFT of real data
	// Accepts:
	//   membvars - pointer to struct of FFT variables
	//   input - pointer to an array of (real) input values, size nfft
	//   output_re - pointer to an array of the real part of the output,
	//     size nfft/2 + 1
	//   output_im - pointer to an array of the imaginary part of the output,
	//     size nfft/2 + 1
	void fft_forward(float* input, float* output_re, float* output_im)
	{
		int ti;
		int nfft;
		int hnfft;

		nfft = m_nfft;
		hnfft = nfft/2;

		for (ti=0; ti<nfft; ti++) {
			m_fft_data[ti] = input[ti];
		}

		MayerFft::mayer_realfft(nfft, m_fft_data);

		output_im[0] = 0;
		for (ti=0; ti<hnfft; ti++) {
			output_re[ti] = m_fft_data[ti];
			output_im[ti+1] = m_fft_data[nfft-1-ti];
		}
		output_re[hnfft] = m_fft_data[hnfft];
		output_im[hnfft] = 0;
	}

	// Perform inverse FFT, returning real data
	// Accepts:
	//   membvars - pointer to struct of FFT variables
	//   input_re - pointer to an array of the real part of the output,
	//     size nfft/2 + 1
	//   input_im - pointer to an array of the imaginary part of the output,
	//     size nfft/2 + 1
	//   output - pointer to an array of (real) input values, size nfft
	void fft_inverse(float* input_re, float* input_im, float* output)
	{
		int ti;
		int nfft;
		int hnfft;

		nfft = m_nfft;
		hnfft = nfft/2;

		for (ti=0; ti<hnfft; ti++) {
			m_fft_data[ti] = input_re[ti];
			m_fft_data[nfft-1-ti] = input_im[ti+1];
		}
		m_fft_data[hnfft] = input_re[hnfft];

		MayerFft::mayer_realifft(nfft, m_fft_data);

		for (ti=0; ti<nfft; ti++) {
			output[ti] = m_fft_data[ti];
		}
	}
};

#endif
