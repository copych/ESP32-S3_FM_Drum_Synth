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

        t1 = micros();

        for (int v = 0; v < MAX_VOICES; ++v) {
            if (likely(voices[v].isActive())) {
                voices[v].process(); // write into internal buffer
            }
        }

        t2 = micros();

        for (int i = 0; i < DMA_BUFFER_LEN; ++i) {
            float mixL = 0.f;
            float mixR = 0.f;

            for (int v = 0; v < MAX_VOICES; ++v) {
                if (likely(voices[v].isActive())) {
                    float s = 0.25f * voices[v].getBlockBuffer()[i];
                    mixL += s * voices[v].getPanL();
                    mixR += s * voices[v].getPanR();

                    float sendAmt = voices[v].getReverbSend();
                    sendL[i] += s * sendAmt;
                    sendR[i] += s * sendAmt;
                }
            }

            outL[i] = mixL;
            outR[i] = mixR;

        }

        t3 = micros();

        reverb.processBlock(sendL, sendR);

        t4 = micros();

        for (int i = 0; i < DMA_BUFFER_LEN; ++i) {
            outL[i] += sendL[i];
            outR[i] += sendR[i];
        }

        
  //      if (decimator++ >= 1024) {
  //          decimator = 0;
  //          ESP_LOGI("Synth", "Render times: voice %d, pan-n-mix %d, reverb %d", t2-t1, t3-t2, t4-t3);
  //      }
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
    uint32_t decimator = 0;
    size_t  t1 = 0, t2 = 0, t3 = 0, t4 = 0; 
};
