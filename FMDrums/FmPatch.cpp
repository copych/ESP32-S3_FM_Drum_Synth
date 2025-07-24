#include "FmPatch.h"
#include "FmOperator.h"

// Common drum patch definitions
FmDrumPatch fmDrumPatches[] = {
    {
        "Sub Kick", 0, 55.0f, // name , algoId, baseFreq
        {
            {0.67f, 0.f, 1.5f, 0.8f, Waveform::Sine},   // op0 - carrier
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {1.0f, 0.f, 3.0f, 0.8f, Waveform::Square}    // op5 - modulator
        },
        1.0f, // volume
        0.0f, // pan
        0.1f, // reverbSend
        0.001f, 0.01f, 0.3f, 0.0f, 0.2f // AHDSR
        , 0.5f // modulation by velocity
        , 16000.0f, 0.0f, 0.33f, 0 // filter freq, resonance, morph, active
    },
    {
        "Noise Clap", 1, 650.0f,
        {
            {1.0f, 0.f, 0.1f, 0.8f, Waveform::Sine},   // op0 - carrier
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {1.0f, 0.f, 4.6f, 0.8f, Waveform::Sine},   // op4
            {0.0f, 0.f, 4.8f, 0.8f, Waveform::Sine}    // op5 - noise
        },
        1.0f,
        0.0f, // pan
        0.1f, // reverbSend
        0.001f, 0.005f, 0.12f, 0.0f, 0.05f
        , 0.5f
        , 16000.0f, 0.0f, 0.5f, 0 // filter freq, resonance, morph, active
    },
    {
        "Closed Hat", 2, 3200.0f,
        {
            {1.0f, 0.f, 0.0f, 0.8f, Waveform::Sine},   // op0 - carrier
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {2.0f, 0.f, 4.5f, 0.8f, Waveform::Square}  // op5 - modulator
        },
        1.0f,
        0.0f, // pan
        0.1f, // reverbSend
        0.001f, 0.005f, 0.12f, 0.0f, 0.06f
        , 0.5f
        , 16000.0f, 0.0f, 0.5f, 0 // filter freq, resonance, morph, active
    },
    {
        "Deep Tom", 3, 180.0f,
        {
            {0.8f, 0.f, 0.0f, 0.8f, Waveform::Sine},   // op0 - carrier
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {1.0f, 0.f, 0.2f, 0.8f, Waveform::Sine},
            {1.5f, 0.f, 1.2f, 0.8f, Waveform::Sine},
            {2.0f, 0.f, 1.5f, 0.8f, Waveform::Sine}
        },
        1.0f,
        0.0f, // pan
        0.1f, // reverbSend
        0.002f, 0.01f, 0.4f, 0.0f, 0.3f
        , 0.5f
        , 16000.0f, 0.0f, 0.5f, 0 // filter freq, resonance, morph, active
    },
    {
        "Snare Body", 4, 210.0f,
        {
            {1.0f, 0.f, 0.0f, 0.8f, Waveform::Sine},   // op0 - carrier
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {1.0f, 0.f, 0.5f, 0.8f, Waveform::Sine},
            {1.5f, 0.f, 1.8f, 0.8f, Waveform::Sine},
            {3.0f, 0.f, 4.0f, 0.8f, Waveform::Sine}
        },
        1.0f,
        0.0f, // pan
        0.1f, // reverbSend
        0.002f, 0.01f, 0.25f, 0.0f, 0.25f
        , 0.5f
        , 16000.0f, 0.0f, 0.5f, 0 // filter freq, resonance, morph, active
    },
    {
        "Snare Noise", 5, 200.0f,
        {
            {1.0f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {1.0f, 0.f, 1.0f, 0.8f, Waveform::Sine},
            {1.0f, 0.f, 1.5f, 0.8f, Waveform::Sine},
            {1.0f, 0.f, 2.0f, 0.8f, Waveform::Sine},
            {1.0f, 0.f, 2.5f, 0.8f, Waveform::Sine},
            {0.0f, 0.f, 4.5f, 0.8f, Waveform::Sine} // noise
        },
        1.0f,
        0.0f, // pan
        0.1f, // reverbSend
        0.005f, 0.01f, 0.35f, 0.0f, 0.3f
        , 0.5f
        , 16000.0f, 0.0f, 0.5f, 0 // filter freq, resonance, morph, active
    },
    {
        "Metal Stack", 6, 1200.0f,
        {
            {1.0f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {1.5f, 0.f, 2.5f, 0.8f, Waveform::Sine},
            {2.0f, 0.f, 2.5f, 0.8f, Waveform::Sine},
            {2.5f, 0.f, 2.5f, 0.8f, Waveform::Sine},
            {3.0f, 0.f, 2.5f, 0.8f, Waveform::Sine},
            {4.0f, 0.f, 3.0f, 0.8f, Waveform::Sine}
        },
        1.0f,
        0.0f, // pan
        0.1f, // reverbSend
        0.005f, 0.01f, 0.6f, 0.0f, 0.6f
        , 0.5f
        , 16000.0f, 0.0f, 0.5f, 0 // filter freq, resonance, morph, active
    },
    {
        "Noise Bell", 7, 800.0f,
        {
            {1.0f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {2.0f, 0.f, 1.0f, 0.8f, Waveform::Sine},
            {2.0f, 0.f, 2.0f, 0.8f, Waveform::Sine},
            {3.0f, 0.f, 2.0f, 0.8f, Waveform::Sine}
        },
        1.0f,
        0.0f, // pan
        0.1f, // reverbSend
        0.002f, 0.01f, 0.6f, 0.0f, 0.3f
        , 0.5f
        , 16000.0f, 0.0f, 0.5f, 0 // filter freq, resonance, morph, active
    },
    {
        "Chime", 8, 1000.0f,
        {
            {1.0f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {3.0f, 0.f, 2.0f, 0.8f, Waveform::Sine},
            {4.0f, 0.f, 2.0f, 0.8f, Waveform::Sine},
            {5.0f, 0.f, 3.0f, 0.8f, Waveform::Sine}
        },
        1.0f,
        0.0f, // pan
        0.1f, // reverbSend
        0.002f, 0.01f, 1.0f, 0.0f, 0.9f
        , 0.5f
        , 16000.0f, 0.0f, 0.5f, 0 // filter freq, resonance, morph, active
    },
    {
        "Tight Clap", 9, 600.0f,
        {
            {1.0f, 0.f, 0.1f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {2.0f, 0.f, 1.8f, 0.8f, Waveform::Sine},
            {2.2f, 0.f, 2.0f, 0.8f, Waveform::Sine},
            {0.0f, 0.f, 4.5f, 0.8f, Waveform::Sine}
        },
        1.0f,
        0.0f, // pan
        0.1f, // reverbSend
        0.001f, 0.005f, 0.15f, 0.0f, 0.12f
        , 0.5f
        , 16000.0f, 0.0f, 0.5f, 0 // filter freq, resonance, morph, active
    },
    {
        "Tick Click", 10, 1500.0f,
        {
            {1.0f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {2.0f, 0.f, 1.0f, 0.8f, Waveform::Sine},
            {0.f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {3.0f, 0.f, 2.5f, 0.8f, Waveform::Sine},
            {4.0f, 0.f, 2.5f, 0.8f, Waveform::Sine},
            {0.0f, 0.f, 3.0f, 0.8f, Waveform::Sine}
        },
        1.0f,
        0.0f, // pan
        0.1f, // reverbSend
        0.001f, 0.005f, 0.1f, 0.0f, 0.08f
        , 0.5f
        , 16000.0f, 0.0f, 0.5f, 0 // filter freq, resonance, morph, active
    },
    {
        "Glass FX", 11, 1700.0f,
        {
            {1.0f, 0.f, 0.0f, 0.8f, Waveform::Sine},
            {0.5f, 0.f, 0.5f, 0.8f, Waveform::Sine},
            {1.2f, 0.f, 0.5f, 0.8f, Waveform::Sine},
            {2.0f, 0.f, 2.0f, 0.8f, Waveform::Sine},
            {2.5f, 0.f, 3.0f, 0.8f, Waveform::Sine},
            {0.0f, 0.f, 4.5f, 0.8f, Waveform::Sine}
        },
        1.0f,
        0.0f, // pan
        0.1f, // reverbSend
        0.005f, 0.01f, 0.4f, 0.0f, 0.3f
        , 0.5f
        , 16000.0f, 0.0f, 0.5f, 0 // filter freq, resonance, morph, active
    },
    {
        "Rail bell", 10, 2000.0f,  
        { 
            {1.047f,  0.f, 0.0f, 0.8f, Waveform::Square},  // ops[0] (carrier)
            {1.481f,  0.f, 0.0f, 0.8f, Waveform::Square},  // ops[1] (mod)
            {1.109f,  0.f, 0.0f, 0.8f, Waveform::Square},  // ops[2] (carrier)
            {2.490f,  0.f, 0.0f, 0.8f, Waveform::Square},  // ops[3] (mod)
            {1.397f,  0.f, 0.0f, 0.8f, Waveform::Square},  // ops[4] (carrier)
            {2.490f,  0.f, 0.0f, 0.8f, Waveform::Square}  // ops[5] (mod)
        },
        1.0f,
        0.0f, // pan
        0.1f, // reverbSend
        0.01f, 0.0f, 6.3f, 0.0f, 6.1f
        , 0.5f
        , 16000.0f, 0.0f, 0.5f, 0 // filter freq, resonance, morph, active
    }
};


int numFmDrumPatches = sizeof(fmDrumPatches) / sizeof(FmDrumPatch);


