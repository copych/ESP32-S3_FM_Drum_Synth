#pragma once
#include "config.h"
#include "FmVoice6.h"
#include "DrumVoiceAllocator.h"
#include "FmPatch.h"
#include "i2s_in_out.h"
#include "fx_reverb.h"

extern float sendL[DMA_BUFFER_LEN];
extern float sendR[DMA_BUFFER_LEN];

class IRAM_ATTR FmDrumSynth {
public:
    void init() {
        allocator.init(voices, MAX_VOICES);

        for (int i = 0; i < MAX_VOICES; ++i)
            voices[i].setSampleRate(SAMPLE_RATE);

        for (int i = 0; i < 128; ++i) {
            int patchIndex = (i - 36 + numFmDrumPatches) % numFmDrumPatches;
            patchMap[i] = fmDrumPatches[patchIndex];
        }
        reverb.init();
    }


    void handleNoteOn(uint8_t midiNote, uint8_t velocity) {
        int idx = allocator.allocateVoice(midiNote);
        if (idx < 0 || idx >= MAX_VOICES) {
            ESP_LOGE("Synth", "Invalid voice %d", idx);
            return;
        }
        voices[idx].reset();
        voices[idx].applyPatch(patchMap[midiNote]);
        voices[idx].noteOn(-1.0f, velocity / 127.f);
        ESP_LOGI("Synth", "Note %d on, voice %d", midiNote, idx);
    }


    void handleNoteOff(uint8_t midiNote) {
        int idx = allocator.getActiveVoiceForNote(midiNote);
        if (idx >= 0) {
            voices[idx].noteOff();
            allocator.releaseNote(midiNote);
        }
    }

    void renderAudioBlock(float* outL, float* outR) {
        memset(outL, 0, DMA_BUFFER_LEN * sizeof(float));
        memset(outR, 0, DMA_BUFFER_LEN * sizeof(float));

        memset(sendL, 0, DMA_BUFFER_LEN * sizeof(float));
        memset(sendR, 0, DMA_BUFFER_LEN * sizeof(float));

        for (int i = 0; i < DMA_BUFFER_LEN; ++i) {
            float mixL = 0.f;
            float mixR = 0.f;

            for (int v = 0; v < MAX_VOICES; ++v) {
                if (likely(voices[v].isActive())) {
                    float s = 0.25f * voices[v].process();
                    mixL += s * voices[v].getPanL();
                    mixR += s * voices[v].getPanR();
                    float sendAmt = voices[v].getReverbSend();
                    sendL[i] += s * sendAmt;
                    sendR[i] += s * sendAmt;
                }
            }

            outL[i] = mixL ;
            outR[i] = mixR ;
        }

        reverb.processBlock(sendL, sendR);

        for (int i = 0; i < DMA_BUFFER_LEN; ++i) {
            outL[i] += sendL[i];
            outR[i] += sendR[i];
        }
    }

    // Accessors
    FmDrumPatch* getPatchMap() { return patchMap; }
    FmVoice6* getVoices() { return voices; }
    DrumVoiceAllocator& getAllocator() { return allocator; }
    inline FxReverb& getReverb() { return reverb; }

private:
    FmVoice6 voices[MAX_VOICES];
    DrumVoiceAllocator allocator;
    FmDrumPatch patchMap[128];
    FxReverb reverb;
};
