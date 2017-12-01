#ifndef AUTO_TUNE_H
#define AUTO_TUNE_H

#include "fft_routine.h"
#include "scale_count.h"
#include <string>
#include <inttypes.h>
#include <Accelerate/Accelerate.h>
#include "mel_chord_ana.h"

class AutoTune
{
private:
  float m_pfTune ;
  float m_pfFixed;
  float m_pfPull ;
  int m_pfA ;
  int m_pfBb ;
  int m_pfB ;
  int m_pfC ;
  int m_pfDb ;
  int m_pfD ;
  int m_pfEb ;
  int m_pfE ;
  int m_pfF ;
  int m_pfGb ;
  int m_pfG ;
  int m_pfAb ;
  float m_pfAmount ;
  float m_pfSmooth ;
  float m_pfShift ;
  int m_pfScwarp ;
  float m_pfLfoamp ;
  float m_pfLforate ;
  float m_pfLfoshape ;
  float m_pfLfosymm ;
  float m_pfFwarp ;
  float m_pfMix ;
  float m_pfPitch ;
  float m_pfConf ;
  double m_pfLatency ;

  FftRoutine* m_fmembvars; // TODO fft_vars, member variables for fft routine
  OpaqueFFTSetup *fftsetup;
  int m_fs; // Sample rate
  int m_channels;

  int m_cbsize; // size of circular buffer
  int m_cbsizeDivOverlap;
  int m_corrsize; // m_cbsize/2 + 1
  int m_cbiwr;
  int m_cbiwrol;
  int m_cbord;
  float* m_cbi; // circular input buffer
  float* m_cbf; // circular formant correction buffer
  float* m_cbo; // circular output buffer
  
  float* m_cbwindow; // hann of length N/2, zeros for the rest
  float* m_acwinv; // inverse of autocorrelation of window
  float* m_hannwindow; // length-N hann
  int m_noverlap;

  float* m_ffttime;
  float* m_fftfreqre;
  float* m_fftfreqim;
  int LOG_N;

  // VARIABLES FOR LOW-RATE SECTION
  float m_aref; // A tuning reference (Hz)
  float m_inpitch; // Input pitch (semitones)
  float m_conf; // Confidence of pitch period estimate (between 0 and 1)
  float m_outpitch; // Output pitch (semitones)
  float m_vthresh; // Voiced speech threshold

  float m_pmax; // Maximum allowable pitch period (seconds)
  float m_pmin; // Minimum allowable pitch period (seconds)
  int64_t m_ti4;

  int64_t m_nmax; // Maximum period index for pitch prd est
  int64_t m_nmin; // Minimum period index for pitch prd est

  float m_lfophase;

  // VARIABLES FOR PITCH SHIFTER
  float m_phprdd; // default (unvoiced) phase period
  double m_inphinc; // input phase increment
  double m_outphinc; // input phase increment
  double m_phincfact; // factor determining output phase increment
  double m_phasein;
  double m_phaseout;
  float* m_frag; // windowed fragment of speech
  int64_t m_fragsize; // size of fragment in samples

  float m_lrshift; // Shift prescribed by low-rate section
  int m_ptarget; // Pitch target, between 0 and 11
  float m_sptarget; // Smoothed pitch target

  // VARIABLES FOR FORMANT CORRECTOR
  float m_flamb;

  unsigned int  m_numNotes;
  int m_iScwarp;

  int m_iNotes[12];
  int m_iPitch2Note[12];
  int m_iNote2Pitch[12];

  uint32_t m_totalSampleCount;

  MelChordAna * m_scaleCounter;
  int m_scaleIdx;
    
  float *mag;
  float *magpower;
  DSPSplitComplex tempSplitComplex;

public:
    void Init(int sampleRate, int channels, std::string melfilename);
//WARNING sampleCount必须是偶数！！！！！
    void runAutoTune(int16_t * inputSamples, int16_t * outputSamples, uint32_t sampleCount);
    void PitchShift(int pitch);
    void SetScale();
    void SeekFromStart(int frameNum);
    ~AutoTune();

};


#endif
