#pragma once
#include "config.h"
#include <stdint.h>
#include "FmVoice6.h"

class IRAM_ATTR DrumVoiceAllocator {
public:
    void init(FmVoice6* voices, int numVoices) {
        voicePool = voices;
        poolSize = (numVoices > MAX_VOICES) ? MAX_VOICES : numVoices;
    }

    int allocateVoice(uint8_t midiNote, uint8_t chokeId) {
        int activeForNote = 0;

        // Choke any voices in the same group
        if (chokeId > 0) {
            for (int i = 0; i < poolSize; ++i) {
                if (voicePool[i].isActive() && voicePool[i].getChokeGroup() == chokeId) {
                    voicePool[i].noteChoke();
                }
            }
        }

        // Count existing voices playing this note
        for (int i = 0; i < poolSize; ++i) {
            if (voicePool[i].isActive() && voicePool[i].getNote() == midiNote) {
                ++activeForNote;
            }
        }

        // Enforce per-note polyphony
        if (activeForNote >= MAX_VOICES_PER_NOTE) {
            // Find the least important voice for this note to steal
            float worstScore = -1.f;
            int worstIndex = -1;
            for (int i = 0; i < poolSize; ++i) {
                if (voicePool[i].isActive() && voicePool[i].getNote() == midiNote) {
                    float score = voicePool[i].getStealScore();
                    if (score > worstScore) {
                        worstScore = score;
                        worstIndex = i;
                    }
                }
            }
            if (worstIndex >= 0) {
                return worstIndex;
            } else {
                ESP_LOGW("Allocator", "Note %d dropped (max voices per note)", midiNote);
                return -1;
            }
        }

        // Find free voice
        for (int i = 0; i < poolSize; ++i) {
            if (!voicePool[i].isActive()) {
                return i;
            }
        }

        // Steal least important voice globally
        float worstScore = -1.f;
        int worstIndex = -1;
        for (int i = 0; i < poolSize; ++i) {
            float score = voicePool[i].getStealScore();
            if (score > worstScore) {
                worstScore = score;
                worstIndex = i;
            }
        }
        return worstIndex >= 0 ? worstIndex : 0;
    }

    void releaseNote(uint8_t midiNote) {
        for (int i = 0; i < poolSize; ++i) {
            if (voicePool[i].getNote() == midiNote) { 
                voicePool[i].noteOff();
            }
        }
    }

    int getActiveVoiceForNote(uint8_t note) {
        for (int i = 0; i < poolSize; ++i) {
            if (voicePool[i].isActive() && voicePool[i].getNote() == note) {
                return i;
            }
        }
        return -1;
    }

    uint8_t getNoteForVoice(int voiceIndex) {
        if (voiceIndex >= 0 && voiceIndex < poolSize) {
            return voicePool[voiceIndex].getNote();
        }
        return 255;
    }

private:
    FmVoice6* voicePool = nullptr;
    int poolSize = 0;
};
