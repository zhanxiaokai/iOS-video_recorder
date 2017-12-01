/*****************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#define PI (float)3.14159265358979323846
#define L2SC (float)3.32192809488736218171
#include <iostream> 
using namespace std;

/*****************************************************************************/

#include "autotune.h"
using namespace std;

// DONE WITH FFT CODE


// The port numbers

#define AT_TUNE 0
#define AT_FIXED 1
#define AT_PULL 2
#define AT_A 3
#define AT_Bb 4
#define AT_B 5
#define AT_C 6
#define AT_Db 7
#define AT_D 8
#define AT_Eb 9
#define AT_E 10
#define AT_F 11
#define AT_Gb 12
#define AT_G 13
#define AT_Ab 14
#define AT_AMOUNT 15
#define AT_SMOOTH 16
#define AT_SHIFT 17
#define AT_SCWARP 18
#define AT_LFOAMP 19
#define AT_LFORATE 20
#define AT_LFOSHAPE 21
#define AT_LFOSYMM 22
#define AT_LFOQUANT 23
#define AT_FCORR 24
#define AT_FWARP 25
#define AT_MIX 26
#define AT_PITCH 27
#define AT_CONF 28
#define AT_INPUT1  29
#define AT_OUTPUT1 30
#define AT_LATENCY 31



void AutoTune::Init(int sampleRate, int channels, string melfilename)
{

	int ti;

	//Autotalent* membvars = malloc(sizeof(Autotalent));

	m_aref = 440;

	m_fs = sampleRate;
	m_channels = channels;

	if (sampleRate>=88200) {
		m_cbsize = 4096;
	}
	else {
		m_cbsize = 2048;
	}
	m_corrsize = m_cbsize / 2 + 1;

	m_pmax = 1/(float)70;  // max and min periods (ms)
	m_pmin = 1/(float)700; // eventually may want to bring these out as sliders

	m_nmax = (sampleRate * m_pmax);
	if (m_nmax > m_corrsize) {
		m_nmax = m_corrsize;
	}
	m_nmin = (uint64_t)(sampleRate * m_pmin);

	m_cbi = new float[m_cbsize];
	memset(m_cbi, 0, sizeof(float) * m_cbsize);
	m_cbf = new float[m_cbsize];
	memset(m_cbf, 0, sizeof(float) * m_cbsize);
	m_cbo = new float[m_cbsize];
	memset(m_cbo, 0, sizeof(float) * m_cbsize);

	m_cbiwr = 0;
	m_cbiwrol = 0;
	m_cbord = 0;

	m_lfophase = 0;

	m_flamb = -(0.8517*sqrt(atan(0.06583*sampleRate))-0.1916);

	// Standard raised cosine window, max height at N/2
	m_hannwindow = new float[m_cbsize];
	for (ti=0; ti<m_cbsize; ti++) {
		m_hannwindow[ti] = -0.5*cos(2*PI*ti/m_cbsize) + 0.5;
	}

	// Generate a window with a single raised cosine from N/4 to 3N/4
	m_cbwindow = new float[m_cbsize];
	memset(m_cbwindow, 0, sizeof(float) * m_cbsize);
	for (ti=0; ti<(m_cbsize / 2); ti++) {
		m_cbwindow[ti+m_cbsize/4] = -0.5*cos(4*PI*ti/(m_cbsize - 1)) + 0.5;
	}

	m_noverlap = 4;
	
	m_cbsizeDivOverlap = m_cbsize / m_noverlap ; 

	m_fmembvars = new FftRoutine(m_cbsize);

	m_ffttime = new float[m_cbsize];
	m_fftfreqre = new float[m_corrsize];
	m_fftfreqim = new float[m_corrsize];


	// ---- Calculate autocorrelation of window ----
    LOG_N = (int)log2((double)m_cbsize);
    tempSplitComplex.realp = new float[m_cbsize];
    //            memcpy(tempSplitComplex.realp, m_ffttime, N/2);
    tempSplitComplex.imagp = new float[m_cbsize];
    fftsetup = vDSP_create_fftsetup(LOG_N, kFFTRadix2);
    
    mag = new float[m_cbsize];
    magpower = new float[m_cbsize];
    memset(magpower, 0, m_cbsize * sizeof(float));
    
	m_acwinv = new float[m_cbsize];
	for (ti=0; ti<m_cbsize; ti++) {

		m_ffttime[ti] = m_cbwindow[ti];
	}
	m_fmembvars->fft_forward(m_cbwindow, m_fftfreqre, m_fftfreqim);
	for (ti=0; ti<m_corrsize; ti++) {
		m_fftfreqre[ti] = (m_fftfreqre[ti])*(m_fftfreqre[ti]) + (m_fftfreqim[ti])*(m_fftfreqim[ti]);
		m_fftfreqim[ti] = 0;
	}
	m_fmembvars->fft_inverse(m_fftfreqre, m_fftfreqim, m_ffttime);
	for (ti=1; ti<m_cbsize; ti++) {
		m_acwinv[ti] = m_ffttime[ti]/m_ffttime[0];
		if (m_acwinv[ti] > 0.000001) {
			m_acwinv[ti] = (float)1/m_acwinv[ti];
		}
		else {
			m_acwinv[ti] = 0;
		}
	}
	m_acwinv[0] = 1;
	// ---- END Calculate autocorrelation of window ----


	m_lrshift = 0;
	m_ptarget = 0;
	m_sptarget = 0;

	m_vthresh = 0.7;  //  The voiced confidence (unbiased peak) threshold level

	// Pitch shifter initialization
	m_phprdd = 0.01; // Default period
	m_inphinc = (float)1/(m_phprdd * sampleRate);
	m_phincfact = 1;
	m_phasein = 0;
	m_phaseout = 0;
	m_frag = new float[m_cbsize];
	m_fragsize = 0;

    m_ti4 = 0;


	m_pfTune = 440.0;
	m_pfFixed = 0.0;
	m_pfPull = 0.0;
	m_pfBb = -1;
	m_pfAmount = 1.0;
	m_pfSmooth = 0.0;
	if (melfilename == "" )
	{
		m_pfSmooth = 0.0;
	}
	m_pfSmooth *= 0.8;
	m_pfShift = 0.0;
	m_pfScwarp = 0.0;
	m_pfLfoamp = 0.0;
	m_pfLforate = 5.0;
	m_pfLfoshape = 0.0;
	m_pfLfosymm = 0.0;
	m_pfFwarp = 0.0;
	m_pfMix = 1.0;
	m_pfPitch = 0.0;
	m_pfConf = 0.0;
	m_pfLatency = m_cbsize - 1;
	

	m_conf = 0;
	m_inpitch = 0;
	m_totalSampleCount = 0;

	m_scaleCounter = new MelChordAna(sampleRate * m_channels);
	m_scaleCounter->InitByMelFile(melfilename, 0);
	//m_scaleCounter->InitByMelFileMock(melfilename);
	m_scaleIdx = -1;
	
}


// Called every time we get a new chunk of audio
void AutoTune::runAutoTune(int16_t * inputSamples, int16_t * outputSamples,
		uint32_t sampleCount) {
	int16_t* pfInput;
	int16_t* pfOutput;
	uint64_t lSampleIndex;

	int N;
	int Nf;

	int64_t ti;
	int64_t ti2;
	int64_t ti3;
	int64_t ti4;
	int64_t ti5;
	float tf;
	float tf2;

	// Variables for cubic spline interpolator
	float indd;
	int ind0;
	int ind1;
	int ind2;
	int ind3;
	float vald;
	float val0;
	float val1;
	float val2;
	float val3;

	int lowersnap;
	int uppersnap;
	float lfoval;

	float pperiod;
	float inpitch;
	float conf;
	float outpitch;
	int scaleIdx;
	
	assert(sampleCount % m_channels == 0);
	pfInput = inputSamples;
	pfOutput = outputSamples;
	memset(pfOutput, 0, sizeof(int16_t) * sampleCount);


	////////////////
	m_totalSampleCount += sampleCount;
	scaleIdx = m_scaleCounter->GetKeyIdx(m_totalSampleCount);
	if (scaleIdx != m_scaleIdx)
	{
		m_scaleIdx = scaleIdx;
		SetScale();
	}

	/////////////////////////////


	tf = pow((float)2,m_pfFwarp/2)*(1+m_flamb)/(1-m_flamb);

	N = m_cbsize;
	Nf = m_corrsize;

	pperiod = m_pmax;
	inpitch = m_inpitch;
	conf = m_conf;
	outpitch = m_outpitch;


	/*******************
	 *  MAIN DSP LOOP  *
	 *******************/
	for (lSampleIndex = 0; lSampleIndex < sampleCount; lSampleIndex+=m_channels)  {

		// load data into circular buffer
		tf = ((float) *(pfInput+lSampleIndex)) / 32768.0; //TODO converter
		
		ti4 = m_cbiwr;
		m_cbi[ti4] = tf;

		m_cbf[ti4] = tf;

		// Input write pointer logic
		m_cbiwr++;
		if (m_cbiwr >= N) {
			m_cbiwr = 0;
		}

		m_cbiwrol++;
		if (m_cbiwrol >= m_cbsizeDivOverlap)
		{
			m_cbiwrol = 0;
		}


		// ********************
		// * Low-rate section *
		// ********************

		// Every N/noverlap samples, run pitch estimation / manipulation code
		//if ((m_cbiwr)%(N/m_noverlap) == 0) {
		if (m_cbiwrol == 0) { // 优化取模运算

			// ---- Obtain autocovariance ----

			// Window and fill FFT buffer
			ti2 = m_cbiwr;
			ti3 = ti2;
			for (ti=0; ti<N; ti++) { //TODO vDSP
				//m_ffttime[ti] = (float)(m_cbi[(ti2-ti+N)%N]*m_cbwindow[ti]);
				m_ffttime[ti] = (float)(m_cbi[ti3]*m_cbwindow[ti]); // 优化取模运算
				if (ti3 == 0)
				{
					ti3 = N -1;
				}
				else
				{
					ti3--;
				}
			}

			// Calculate FFT


            vDSP_ctoz((DSPComplex*)m_ffttime, 2, &tempSplitComplex, 1, N/2);

            vDSP_fft_zrip(fftsetup, &tempSplitComplex, 1, LOG_N, kFFTDirection_Forward);
            //remove DC
            tempSplitComplex.realp[0] = 0;

            float zxk = tempSplitComplex.imagp[0];
            vDSP_zvabs(&tempSplitComplex, 1, mag, 2, N/2);
            vDSP_vsq(mag, 2, magpower, 2, N/2);
            vDSP_ctoz((DSPComplex*)magpower, 2, &tempSplitComplex, 1, N/2);
            tempSplitComplex.imagp[0] = zxk * zxk;
            vDSP_fft_zrip(fftsetup, &tempSplitComplex, 1, LOG_N, kFFTDirection_Inverse);
            vDSP_ztoc(&tempSplitComplex, 1, (DSPComplex*)m_ffttime, 2, N/2);

			// Normalize
			tf = (float)1/m_ffttime[0];
			for (ti=1; ti<N; ti++) { //TODO vDSP
				m_ffttime[ti] = m_ffttime[ti] * tf;
			}
			m_ffttime[0] = 1;

			//  ---- END Obtain autocovariance ----


			//  ---- Calculate pitch and confidence ----

			// Calculate pitch period
			//   Pitch period is determined by the location of the max (biased)
			//     peak within a given range
			//   Confidence is determined by the corresponding unbiased height
			tf2 = 0;
			pperiod = m_pmin;

			int64_t tmpMin = m_nmin + 1;

			for (; tmpMin < m_nmax; tmpMin++)
			{
				if (m_ffttime[tmpMin] > m_ffttime[tmpMin -1])
				{
					break;
				}
			}

			int64_t tmpMax = m_nmax -1;
			for (; tmpMax > tmpMin; tmpMax--)
			{
				if (m_ffttime[tmpMax] <= m_ffttime[tmpMax -1])
				{
					break;
				}
			}
            
            float partMax = 0;
            ti4 = 0;
            float ttf2 = 0;
            int64_t tti4 = 0;
            for (ti = tmpMin; ti < tmpMax; ti++) //TODO vDSP_maxvi
            {
                if (m_ffttime[ti] > tf2)
                {
                    tf2 = m_ffttime[ti];
                    ti4 = ti;//TODO useless code
                    ttf2 = tf2;
                }
                if (m_ffttime[ti] > partMax)
                {
                    partMax = m_ffttime[ti];
                    tti4 = ti;
                }
                if (m_ffttime[ti] <= 0)
                {
                    if (tf2 != 0)
                    {
                        if ((partMax / tf2) > 0.5)
                        {

                            if (round(m_ti4 *5.0/ tti4) == 5)
                            {
                                ttf2 = partMax;
                                ti4 = tti4;

                            }
                        }

                    }
                    partMax = 0;
                }
            }
            if ((tf2 != 0) && ((partMax / tf2) > 0.5))
            {
                if (round(m_ti4 *5.0/ tti4) == 5)
                {
                    ttf2 = partMax;
                    ti4 = tti4;
                }
            }

            tf2 = ttf2;
            m_ti4 = ti4;

			if (tf2>0) {
				conf = tf2*m_acwinv[ti4];
				if (ti4>0 && ti4<Nf) {
					// Find the center of mass in the vicinity of the detected peak
					tf = m_ffttime[ti4-1]*(ti4-1);
					tf = tf + m_ffttime[ti4]*(ti4);
					tf = tf + m_ffttime[ti4+1]*(ti4+1);
					tf = tf/(m_ffttime[ti4-1] + m_ffttime[ti4] + m_ffttime[ti4+1]);
					pperiod = tf/m_fs;
				}
				else {
					pperiod = (float)ti4/m_fs;
				}
			}

			// Convert to semitones
			tf = (float) -12*log10((float)m_pfTune*pperiod)*L2SC;
			if (conf>=m_vthresh) {
				inpitch = tf;
				m_inpitch = tf; // update pitch only if voiced
			}
			m_conf = conf;

			m_pfPitch = inpitch;
			m_pfConf = conf;

			//  ---- END Calculate pitch and confidence ----


			//  ---- Modify pitch in all kinds of ways! ----

			outpitch = inpitch;

			// Pull to fixed pitch
			outpitch = (1-m_pfPull)*outpitch + m_pfPull*m_pfFixed;

			// -- Convert from semitones to scale notes --
			ti = (int)(outpitch/12 + 32) - 32; // octave
			tf = outpitch - ti*12; // semitone in octave
			ti2 = (int)tf; //lower pitch , mark by wgt
			ti3 = ti2 + 1; //higher pitch, mark by wgt
			// a little bit of pitch correction logic, since it's a convenient place for it
			ti4 = ti2 % 12;
			ti5 = ti3 % 12;
			if (m_iNotes[ti4]<0 || m_iNotes[ti5]<0) { // if between 2 notes that are more than a semitone apart
				lowersnap = 1;
				uppersnap = 1;
			}
			else {
				lowersnap = 0;
				uppersnap = 0;
				if (m_iNotes[ti4]==1) { // if specified by user
					lowersnap = 1;
				}
				if (m_iNotes[ti5]==1) { // if specified by user
					uppersnap = 1;
				}
			}
			// (back to the semitone->scale conversion)
			// finding next higher pitch in scale
			while (m_iNotes[ti5]<0) { // 优化取模
				//ti3 = ti3 + 1;
				ti3++;
				ti5++;
				if (ti5 == 12)
				{
					ti5 = 0;
				}
			}
			// finding next lower pitch in scale
			while (m_iNotes[ti4]<0) { // 优化取模
				//ti2 = ti2 - 1;
				ti2 --;
				if (ti4 == 0)
				{
					ti4 = 11;
				}
				else
				{
					ti4 --;
				}
			}
			tf = (tf-ti2)/(ti3-ti2) + m_iPitch2Note[ti4];
			if (ti2<0) {
				tf = tf - m_numNotes;
			}
			outpitch = tf + m_numNotes*ti;
			// -- Done converting to scale notes --

			// The actual pitch correction
			ti = (int)(outpitch+128) - 128;
			tf = outpitch - ti - 0.5;
			ti2 = ti3-ti2;
			if (ti2>2) { // if more than 2 semitones apart, put a 2-semitone-like transition halfway between
				tf2 = (float)ti2/2;
			}
			else {
				tf2 = (float)1;
			}
			if (m_pfSmooth<0.001) {
				tf2 = tf*tf2/0.001;
			}
			else {
				tf2 = tf*tf2/m_pfSmooth;
			}
			if (tf2<-0.5) tf2 = -0.5;
			if (tf2>0.5) tf2 = 0.5;
			tf2 = 0.5*sin(PI*tf2) + 0.5; // jumping between notes using horizontally-scaled sine segment
			tf2 = tf2 + ti;
			if ( (tf<0.5 && lowersnap) || (tf>=0.5 && uppersnap) ) {
				outpitch = m_pfAmount*tf2 + ((float)1-m_pfAmount)*outpitch;
			}

			// Add in pitch shift
			outpitch = outpitch + m_pfShift;

			// LFO logic
			tf = m_pfLforate*N/(m_noverlap*m_fs);
			if (tf>1) tf=1;
			m_lfophase = m_lfophase + tf;
			if (m_lfophase>1) m_lfophase = m_lfophase-1;
			lfoval = m_lfophase;
			tf = (m_pfLfosymm + 1)/2;
			if (tf<=0 || tf>=1) {
				if (tf<=0) lfoval = 1-lfoval;
			}
			else {
				if (lfoval<=tf) {
					lfoval = lfoval/tf;
				}
				else {
					lfoval = 1 - (lfoval-tf)/(1-tf);
				}
			}
				// linear combination of cos and line
			lfoval = (0.5 - 0.5*cos(lfoval*PI))*m_pfLfoshape + lfoval*(1-m_pfLfoshape);
			lfoval = m_pfLfoamp*(lfoval*2 - 1);

			// Convert back from scale notes to semitones
			outpitch = outpitch + m_iScwarp; // output scale rotate implemented here
			ti = (int)(outpitch/m_numNotes + 32) - 32;
			tf = outpitch - ti*m_numNotes;
			ti2 = (int)tf;
			ti3 = ti2 + 1;
			outpitch = m_iNote2Pitch[ti3%m_numNotes] - m_iNote2Pitch[ti2];
			if (ti3>=m_numNotes) {
				outpitch = outpitch + 12;
			}
			outpitch = outpitch*(tf - ti2) + m_iNote2Pitch[ti2];
			outpitch = outpitch + 12*ti;
			outpitch = outpitch - (m_iNote2Pitch[m_iScwarp] - m_iNote2Pitch[0]); //more scale rotation here

			// add in unquantized LFO
			outpitch = outpitch + lfoval*2;


			if (outpitch<-36) outpitch = -48;
			if (outpitch>24) outpitch = 24;

			m_outpitch = outpitch;

			//  ---- END Modify pitch in all kinds of ways! ----

			// Compute variables for pitch shifter that depend on pitch
			m_inphinc = m_pfTune*pow(2,inpitch/12)/m_fs;
			m_outphinc = m_pfTune*pow(2,outpitch/12)/m_fs;
			m_phincfact = m_outphinc/m_inphinc;
		}
		// ************************
		// * END Low-Rate Section *
		// ************************



		// *****************
		// * Pitch Shifter *
		// *****************

		// Pitch shifter (kind of like a pitch-synchronous version of Fairbanks' technique)
		//   Note: pitch estimate is naturally N/2 samples old
		m_phasein = m_phasein + m_inphinc;
		m_phaseout = m_phaseout + m_outphinc;

		//   When input phase resets, take a snippet from N/2 samples in the past
		if (m_phasein >= 1) {
			m_phasein = m_phasein - 1;
			ti2 = m_cbiwr - N/2;
			ti3 = N/2 -1;
			ti4 = ti3 + ti2 + 1;
			ti = ti3 + 1;
			if (ti == ti4)
			{
				memcpy(m_frag, m_cbf, sizeof(float) * N);
			}
			else if (ti < ti4)
			{
				memcpy(m_frag + ti , m_cbf + ti4, sizeof(float) * (N - ti4)) ;
				memcpy(m_frag + ti - ti4 + N, m_cbf, sizeof(float) * (ti4 - ti));
				memcpy(m_frag, m_cbf + ti4 - ti, sizeof(float) * ti);
			}
			else 
			{
				memcpy(m_frag + ti , m_cbf + ti4, sizeof(float) * (N - ti)) ;
				memcpy(m_frag , m_cbf + ti4 - ti + N, sizeof(float) * (ti - ti4));
				memcpy(m_frag + ti - ti4, m_cbf, sizeof(float) * ti4);
			}
		}

		//   When output phase resets, put a snippet N/2 samples in the future
		if (m_phaseout >= 1) {
			m_fragsize = m_fragsize*2;
			if (m_fragsize > N) {
				m_fragsize = N;
			}
			m_phaseout = m_phaseout - 1;
			ti3 = (int64_t)(((float)m_fragsize) / m_phincfact);
			if (ti3>=N/2) {
				ti3 = N/2 - 1;
			}
			int64_t ti6 = N/2;
			float tf7 = float(N)/ti3;
			float tf8 = (-ti3/2) * tf7 + ti6;
			ti2 = (m_cbord + N/2 + N - ti3/2) % N;
			for (ti=-ti3/2; ti<(ti3/2); ti++) { 
				int tid0; 
				int tid1; 
				int tid2; 
				int tid3; 
				tf = m_hannwindow[int64_t(tf8)];
				tf8 += tf7;
				// 3rd degree polynomial interpolator - based on eqns from Hal Chamberlin's book
				indd = m_phincfact*ti;
				ind1 = (int)indd;
				ind2 = ind1+1;
				ind3 = ind1+2;
				ind0 = ind1-1;
				if (ind0 < 0)
				{
					tid0 = ind0 + N;
				}
				else
				{
					tid0 = ind0;
				}
				tid1 = tid0 + 1;
				tid2 = tid0 + 2;
				tid3 = tid0 + 3;

				if (tid1 >= N)
				{
					tid1 -= N;
					tid2 -= N;
					tid3 -= N;
				}
				else if (tid2 >= N)
				{
					tid2 -= N;
					tid3 -= N;
				}
				else if (tid3 >= N)
				{
					tid3 -= N;
				}

				val0 = m_frag[tid0]; //优化取模
				val1 = m_frag[tid1];
				val2 = m_frag[tid2];
				val3 = m_frag[tid3];
				vald = 0;
				float tfid0 = indd - ind0;
				float tfid1 = indd - ind1;
				float tfid2 = indd - ind2;
				float tfid3 = indd - ind3;

				float tfidm0 = (float)0.166666666667 * tfid1 * tfid2;
				float tfidm1 = (float)0.5 * tfid0 * tfid3;
				
				vald = vald - val0 * tfidm0 * tfid3;
				vald = vald + val1 * tfid2 * tfidm1;
				vald = vald - val2 * tfid1 * tfidm1;
				vald = vald + val3 * tfid0 * tfidm0;
				m_cbo[ti2] += vald*tf;
				ti2++;
				if (ti2 >= N)
				{
					ti2 = 0;
				}
			}
			m_fragsize = 0;
		}
		m_fragsize++;

		//   Get output signal from buffer
		tf = m_cbo[m_cbord]; // read buffer

		m_cbo[m_cbord] = 0; // erase for next cycle
		m_cbord++; // increment read pointer
		if (m_cbord >= N) {
			m_cbord = 0;
		}

		// *********************
		// * END Pitch Shifter *
		// *********************

		ti4 = (m_cbiwr + 2)%N;

		// Write audio to output of plugin
		// Mix (blend between original (delayed) =0 and processed =1)
		//int16_t tmpOutSample =  int16_t( (m_pfMix*tf + (1-m_pfMix)*m_cbi[ti4]) * 32767.0 );
		double tmpFOutSample =  m_pfMix*tf + (1-m_pfMix)*m_cbi[ti4];
		//TODO 增加高通滤波器后再做clip
		tmpFOutSample *= 0.9;
		if (tmpFOutSample > 0.99)
		{
			tmpFOutSample = 0.99;
		}
        if (tmpFOutSample < -0.99)
        {
            tmpFOutSample = -0.99;
        }
		int16_t tmpOutSample = tmpFOutSample * 32767.0;  //TODO converter 
		*(pfOutput++) = tmpOutSample;
		if (m_channels == 2)
		{
			*(pfOutput++) = tmpOutSample;
		}

	}

	// Tell the host the algorithm latency
	m_pfLatency = (double)(N-1);
}

void AutoTune::SeekFromStart(int frameNum)
{
	m_totalSampleCount = frameNum * m_channels;
}

void AutoTune::PitchShift(int pitch)
{
    m_scaleCounter->PitchShift(pitch);
    SetScale();
}

void AutoTune::SetScale()
{
	int ti2;
	int ti;
	for (ti = 0; ti < 12; ti++)
	{
		m_iNotes[ti] = -1;
	}
	int scalePartSet = m_scaleCounter->GetKeyScale();
    for (int i = 0; i < 12; i++)
	{
        if ((1 << i) & scalePartSet) {
            m_iNotes[i] = 1;
        }
	}

	// Some logic for the semitone->scale and scale->semitone conversion
	// If no notes are selected as being in the scale, instead snap to all notes
	ti2 = 0;
	for (ti=0; ti<12; ti++) {
		if (m_iNotes[ti]>=0) {
			m_iPitch2Note[ti] = ti2;
			m_iNote2Pitch[ti2] = ti;
			ti2 = ti2 + 1;
		}
		else {
			m_iPitch2Note[ti] = -1;
		}
	}
	m_numNotes = ti2;
	while (ti2<12) {
		m_iNote2Pitch[ti2] = -1;
		ti2 = ti2 + 1;
	}
	m_iScwarp = m_pfScwarp;
	m_iScwarp = (m_iScwarp + m_numNotes*5)%m_numNotes;

}



/********************
 *  THE DESTRUCTOR! *
 ********************/
AutoTune::~AutoTune()
{
	delete m_fmembvars;
	if (m_cbi != NULL)
	{
		//free(m_cbi);
		delete[] m_cbi;
		m_cbi = NULL;
	}
	if (m_cbf != NULL)
	{
		//free(m_cbf);
		delete[] m_cbf;
		m_cbf = NULL;
	}
	if (m_cbo != NULL)
	{
		//free(m_cbo);
		delete[] m_cbo;
		m_cbo = NULL;
	}
	if (m_cbwindow != NULL)
	{
		//free(m_cbwindow);
		delete[] m_cbwindow;
		m_cbwindow = NULL;
	}
	if (m_hannwindow != NULL)
	{
		//free(m_hannwindow);
		delete[] m_hannwindow;
		m_hannwindow = NULL;
	}
	if (m_acwinv != NULL)
	{
		//free(m_acwinv);
		delete[] m_acwinv;
		m_acwinv = NULL;
	}
	if (m_frag != NULL)
	{
		//free(m_frag);
		delete[] m_frag;
		m_frag = NULL;
	}
	if (m_ffttime != NULL)
	{
		//free(m_ffttime);
		delete[] m_ffttime;
		m_ffttime = NULL;
	}
	if (m_fftfreqre != NULL)
	{
		//free(m_fftfreqre);
		delete[] m_fftfreqre;
		m_fftfreqre = NULL;
	}
	if (m_fftfreqim != NULL)
	{
		//free(m_fftfreqim);
		delete[] m_fftfreqim;
		m_fftfreqim = NULL;
	}

	if (m_scaleCounter != NULL)
	{
		delete m_scaleCounter;
	}
    
    if (fftsetup) {
        vDSP_destroy_fftsetup(fftsetup);
    }
    
    if (mag != NULL)
    {
        //free(m_fftfreqim);
        delete[] mag;
        mag = NULL;
    }
    if (magpower != NULL)
    {
        //free(m_fftfreqim);
        delete[] magpower;
        magpower = NULL;
    }
    if (tempSplitComplex.realp) {
        delete[] tempSplitComplex.realp;
        tempSplitComplex.realp = NULL;
    }
    if (tempSplitComplex.imagp) {
        delete[] tempSplitComplex.imagp;
        tempSplitComplex.imagp = NULL;
    }
	
}

