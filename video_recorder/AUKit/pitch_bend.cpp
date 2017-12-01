#include <algorithm>
#include "pitch_bend.h"
#include <math.h>
#include <stdio.h>
#include <vector>
#include <iostream>

using namespace std;
#define PI (float)3.14159265358979323846
#define L2SC (float)3.32192809488736218171
float PitchBend::Bend(float * frag, int samplePos)
{
    float tf;
    float indd;
    int ind0;
    int ind1;
    float * valp;
    int64_t ti;
    int64_t ti2;
    int64_t ti3;
    int64_t ti4;
    int64_t ti6;
    float tf7, tf8;
    int N = m_cbsize;
    int tid0;
    float fHashKey;
    int iHashKey;
    m_iPhaseOut++;
    
    //   When output phase resets, put a snippet N/2 samples in the future
    if (m_iPhaseOut >= m_iMaxPhaseOut) {
        m_fragsize = m_fragsize*2;
        if (m_fragsize > N) {
            m_fragsize = N;
        }
        m_iPhaseOut = m_iPhaseOut - m_iMaxPhaseOut;
        ti3 = (int64_t)(((float)m_fragsize) / m_phincfact);
        ti6 = N/2;
        if (ti3>=ti6) {
            ti3 = ti6 - 1;
        }
        ti4 = ti3/2;
        
        tf7 = float(N)/ti3;
        tf8 = (-ti4) * tf7 + ti6;
        ti2 = (m_cbord + ti6 + N - ti4) % N;
        for (ti=-ti4; ti<(ti4); ti++) {
            // 3rd degree polynomial interpolator - based on eqns from Hal Chamberlin's book
            indd = m_phincfact*ti;
            ind1 = (int)indd;
            ind0 = ind1-1;
            fHashKey = indd - ind0;
            iHashKey = (int)(fHashKey * 100 );
            
            if (ind0 < 0)
            {
                tid0 = ind0 + N;
            }
            else
            {
                tid0 = ind0;
            }
            
            HashFactor * th = m_hashFactor + iHashKey;
            if (!th->hit)
            {
                float tfid0 = fHashKey;
                float tfid1 = fHashKey-1 ;
                float tfid2 = fHashKey - 2;
                float tfid3 = fHashKey - 3;
                
                float tfidm0 = (float)0.166666666667 * tfid1 * tfid2;
                float tfidm1 = (float)0.5 * tfid0 * tfid3;
                
                th->f0 = tfidm0 * tfid3;
                th->f1 = tfidm1 * tfid2;
                th->f2 = tfidm1 * tfid1;
                th->f3 = tfidm0 * tfid0;
                th->hit = true;
            }
            
            valp = frag+tid0; //优化取模
            
            m_cbo[ti2] += m_hannwindow[int(tf8)] * (
                                                    - (*valp) * th->f0
                                                    + (*(valp+1)) * th->f1
                                                    - (*(valp+2)) * th->f2
                                                    + (*(valp+3)) * th->f3);
            tf8 += tf7;
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
    
    
    // Write audio to output of plugin
    // Mix (blend between original (delayed) =0 and processed =1)
    double tmpFOutSample = tf;
    //TODO 增加高通滤波器后再做clip
    
    if (tmpFOutSample > 0.99)
    {
        tmpFOutSample = 0.99;
    }
    if (tmpFOutSample < -0.99)
    {
        tmpFOutSample = -0.99;
    }
    tmpFOutSample *= m_volumeRate;
    return tmpFOutSample;
}

float PitchBend::_ComputeVald(float fHashKey, float * frag, int tid0)
{
    float tfid0 = fHashKey +1;
    float tfid1 = fHashKey;
    float tfid2 = fHashKey - 1;
    float tfid3 = fHashKey - 2;
    
    float tfidm0 = (float)0.166666666667 * tfid1 * tfid2;
    float tfidm1 = (float)0.5 * tfid0 * tfid3;
    
    float factor0 = tfidm0 * tfid3;
    float factor1 = tfidm1 * tfid2;
    float factor2 = tfidm1 * tfid1;
    float factor3 = tfidm0 * tfid0;
    
    float val0 = frag[tid0]; //优化取模
    float val1 = frag[tid0 + 1];
    float val2 = frag[tid0 + 2];
    float val3 = frag[tid0 + 3];
    
    return -val0 * factor0 + val1 * factor1 - val2 * factor2 + val3 * factor3;
}

void PitchBend::ComputePitch(double inpitch, double inphinc, int samplePos)
{
    //  ---- Modify pitch in all kinds of ways! ----
    
    double outpitch = inpitch;
    if (!m_onlyChorus  || (m_scaleCounter->IsInChorus(samplePos)))
    {
        outpitch = inpitch + m_tag;
        
        int tip = round(outpitch);
        //outpitch = round(outpitch); //TODO 原始音高变化
        outpitch = outpitch;
        tip += 48;
        tip %=12;
        
        m_scaleCounter->GetKeyIdx(samplePos);
        m_scaleCounter->GetChordIdx(samplePos);
        int i;
        if (m_tag >= 0)
        {
            for (i = 0; i < 3; i++)
            {
                if (m_scaleCounter->IsNoteInChord((tip + i)%12))
                {
                    outpitch += i;
                    break;
                }
            }
            if (i == 3)
            {
                for (i = 0; i < 3; i++)
                {
                    if (m_scaleCounter->IsNoteInChordKey((tip + i) % 12))
                    {
                        outpitch += i;
                        break;
                    }
                }
            }
            if (i == 3)
            {
                outpitch = round(inpitch);
            }
        }
        else
        {
            for (i = 2; i >=0; i--)
            {
                if (m_scaleCounter->IsNoteInChord((tip +i ) % 12))
                {
                    outpitch += i;
                    break;
                }
            }
            if (i < 0)
            {
                for (i = 2; i >=0; i--)
                {
                    if (m_scaleCounter->IsNoteInChordKey((tip + i) % 12))
                    {
                        outpitch += i;
                        break;
                    }
                }
            }
            if (i < 0)
            {
                outpitch = round(inpitch);
            }
        }
    }
    
    outpitch += m_extraPitch;
    if (fabs(m_outpitch - outpitch) < 2)
    {
        m_outpitch = outpitch;
        //m_outpitch += (outpitch - m_outpitch) * 0.5; //TODO 平滑
    }
    else
    {
        m_outpitch = outpitch;
    }
    
    // Compute variables for pitch shifter that depend on pitch
    // 一个sample占多少周期
    double outphinc = m_outphinc;
    m_outphinc = m_pfTune*pow(2,m_outpitch/12)/m_fs;
    m_phincfact = m_outphinc/inphinc;
    m_iMaxPhaseOut = int(1/m_outphinc);
    if (m_outphinc > 0.00001)
    {
        m_iPhaseOut = int(m_iPhaseOut * outphinc / m_outphinc + 0.5) ;
    }
}

void PitchBend::SetOnlyChorus(bool isOnlyChorus)
{
    m_onlyChorus = isOnlyChorus;
}

bool PitchBend::IsSupportSongFrame()
{
    return (m_scaleCounter->GetSupportFlag() & MelChordAna::SupportSongFrame);
}

bool PitchBend::IsValid()
{
    return (m_scaleCounter->GetSupportFlag() & MelChordAna::SupportKey);
}

bool PitchBend::IsSupportMelScore()
{
    return (m_scaleCounter->GetSupportFlag() & MelChordAna::SupportMelody);
}

void PitchBend::PitchShift(int pitchDiff)
{
    m_scaleCounter->PitchShift(pitchDiff);
}

void PitchBend::Init(string melfilename, int cbsize, int sampleRate, int channels, float tag, float volumeRate, float extraPitch, bool isOnlyChorus)
{
    m_channels = channels;
    m_fs = sampleRate;
    m_phincfact = 1;
    m_scaleIdx = -1;
    m_outphinc = 0;
    m_pfScwarp = 0.0; 
    m_volumeRate = volumeRate * 0.9;
    
    m_iPhaseOut = 0;
    m_iMaxPhaseOut = INT_MAX;
    
    m_cbsize = cbsize;
    m_fragsize = 0;
    m_numNotes = 0;
    m_iScwarp = 0;
    m_endSampleNum = 0;
    m_pfTune = 440.0;
    m_cbord = 0;
    m_pfSmooth = 0.0;
    m_outpitch = 0;
    if (melfilename != "" )
    {
        m_pfSmooth *= 0.8;
    }
    m_tag = tag;
    m_extraPitch = extraPitch;
    m_onlyChorus = isOnlyChorus;
    
    m_cbo = new float[m_cbsize];
    memset(m_cbo, 0, sizeof(float) * m_cbsize);
    
    m_scaleCounter = new MelChordAna(sampleRate * m_channels);
    m_scaleCounter->InitByMelFile(melfilename, m_tag);
    
    m_hannwindow = new float[m_cbsize];
    for (int ti=0; ti<m_cbsize; ti++) {
        m_hannwindow[ti] = -0.5*cos(2*PI*ti/m_cbsize) + 0.5;
    }
    
    m_hashFactor = new HashFactor[210];
    memset(m_hashFactor, 0, sizeof(HashFactor) * 210);
}

PitchBend::PitchBend()
{
    m_cbo = NULL;
    m_scaleCounter = NULL;
    m_hannwindow = NULL;
    m_hashFactor = NULL;
}
PitchBend::~PitchBend()
{
    if (m_cbo != NULL)
    {
        delete[] m_cbo;
    }
    m_cbo = NULL;
    
    if (m_scaleCounter != NULL)
    {
        delete m_scaleCounter;
    }
    m_scaleCounter = NULL;
    
    if (m_hannwindow != NULL)
    {
        delete[] m_hannwindow;
    }
    m_hannwindow = NULL;
    
    if (m_hashFactor != NULL)
    {
        delete[] m_hashFactor;
    }
    m_hashFactor = NULL;
}


