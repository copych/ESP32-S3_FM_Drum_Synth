#pragma once
#include "config.h"
#include <stdint.h>
#include "FmVoice6.h" 

struct ActiveVoice {
    uint8_t note = 255;   // 255 = inactive
    uint32_t startTime = 0;
};

class IRAM_ATTR DrumVoiceAllocator {
public:
    void init(FmVoice6* voices, int numVoices) {
        voicePool = voices;
        poolSize = (numVoices > MAX_VOICES) ? MAX_VOICES : numVoices;
        for (int i = 0; i < poolSize; ++i)
            activeVoices[i].note = 255;
    }

    int allocateVoice(uint8_t midiNote, uint8_t chokeId) {
        uint32_t now = millis();

        int activeForNote = 0;

        // Manage choke groups
        if (chokeId > 0) {
            for (int i = 0; i < poolSize; ++i) {
                if (voicePool[i].isActive() && voicePool[i].getChokeGroup() == chokeId) {
                    voicePool[i].noteChoke();
                    activeVoices[i].note = 255;
                }
            }
        }

        // Count voices already playing this note
        for (int i = 0; i < poolSize; ++i) {
            if (activeVoices[i].note == midiNote)
                ++activeForNote;   
        }

        // Limit voices per note
        if (activeForNote >= MAX_VOICES_PER_NOTE) {
            int oldest = -1;
            uint32_t oldestTime = UINT32_MAX;
            for (int i = 0; i < poolSize; ++i) {
                if (activeVoices[i].note == midiNote && activeVoices[i].startTime < oldestTime) {
                    oldest = i;
                    oldestTime = activeVoices[i].startTime;
                }
            }
            if (oldest >= 0) {
                activeVoices[oldest].startTime = now;
                return oldest;
            } else {
                // Enforce MAX_VOICES_PER_NOTE
                ESP_LOGW("Allocator", "Note %d dropped (max voices per note)", midiNote);
                return -1;
            }
        }

        // Try to find a free voice
        for (int i = 0; i < poolSize; ++i) {
            if (!voicePool[i].isActive()) {
                activeVoices[i].note = midiNote;
                activeVoices[i].startTime = now;
                return i;
            }
        }

        // Steal oldest voice globally
        int oldest = 0;
        uint32_t oldestTime = activeVoices[0].startTime;
        for (int i = 1; i < poolSize; ++i) {
            if (activeVoices[i].startTime < oldestTime) {
                oldest = i;
                oldestTime = activeVoices[i].startTime;
            }
        }

        activeVoices[oldest].note = midiNote;
        activeVoices[oldest].startTime = now;
        return oldest;
    }

    void releaseNote(uint8_t midiNote) {
        for (int i = 0; i < poolSize; ++i) {
            if (activeVoices[i].note == midiNote && !voicePool[i].isActive()) {
                activeVoices[i].note = 255;
            }
        }
    }

    int getActiveVoiceForNote(uint8_t note) {
        for (int i = 0; i < poolSize; ++i)
            if (activeVoices[i].note == note)
                return i;
        return -1;
    }

    uint8_t getNoteForVoice(int voiceIndex) {
        if (voiceIndex >= 0 && voiceIndex < poolSize)
            return activeVoices[voiceIndex].note;
        return 255;
    }

private:
    FmVoice6* voicePool = nullptr;
    int poolSize = 0;
    ActiveVoice activeVoices[MAX_VOICES];
};
