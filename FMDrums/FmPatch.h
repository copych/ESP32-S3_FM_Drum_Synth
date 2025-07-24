#pragma once

#include "config.h"
#include "FmOperator.h"


// Single operator parameter set
struct FmOpParams {
    float ratio = 0.f;
    float detune = 0.f;
    float feedback = 0.f;
    float volume = 0.8f;
    Waveform waveform = Waveform::Sine;
};

// Single FM drum patch
struct FmDrumPatch {
    char* name;
    uint8_t algoIndex;
    float baseFreq = 440.0f;
    FmOpParams ops[6];
    float volume = 1.0f;
    float pan = 0.0f; // [-1 center +1]
    float reverbSend = 0.1f;
    float attack = 0.01f;
    float hold   = 0.01f;
    float decay  = 0.2f;
    float sustain = 0.0f;
    float release = 0.1f;
    float velocityMod = 0.5f;
    float filterFreqHz = 16000.0f;
    float filterReso = 0.0f;
    float filterMorph = 0.33f;
    uint8_t useFilter = 0;
};

extern FmDrumPatch fmDrumPatches[];   
extern int numFmDrumPatches;