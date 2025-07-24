#pragma once
#include "config.h"
#ifdef ENABLE_GUI

#include "TextGUI.h"
#include "MenuItem.h"
#include "FmDrumSynth.h"
#include "GmDrums.h"
#include "FmOperator.h"
#include "DrumkitStorage.h"
#include "AlgoDiagrams.h"

extern TextGUI gui;
extern FmDrumSynth synth;

namespace MenuStructure {



MenuItem createAlgoChooserCustom(uint8_t& algoIndex) {
    return MenuItem::Custom(
        "",  // Empty title since we're drawing custom content
        [&](TextGUI& gui, U8G2& u8g2, int x, int y) {
            x = 0;
            y = 0;
            // Defensive check on algoIndex
            int index = algoIndex;
            if (index < 0 || index >= FmVoice6::NumAlgos) {
                index = 0;  // fallback safe index
            }

            const char* diagram = algoDiagrams[index];
            if (!diagram) {
                diagram = "(no diagram)";
            }  

            char buf[32];
            snprintf(buf, sizeof(buf), "%d", index);
            u8g2.drawStr( u8g2.getDisplayWidth() - u8g2.getUTF8Width(buf), y, buf);

            int lineY = y + 11;
            const char* ptr = diagram;
            while (ptr && *ptr) {
                const char* eol = strchr(ptr, '\n');
                int len = eol ? (eol - ptr) : strlen(ptr);
                char line[64];
                strncpy(line, ptr, len);
                line[len] = 0;
                u8g2.drawUTF8(x, lineY, line);
                lineY += 11;
                ptr = (eol && *eol == '\n') ? eol + 1 : eol;
            }

        },
        [&](TextGUI& gui, int action) -> bool {
            if (action == -1) {
                algoIndex = (algoIndex == 0) ? (FmVoice6::NumAlgos - 1) : (algoIndex - 1);
                return true;
            } 
            else if (action == +1) {
                algoIndex = (algoIndex + 1) % FmVoice6::NumAlgos;
                return true;
            } 
            else if (action == 0) {
                gui.updateParentItemTitle("Algorithm: " + String(algoIndex));
                return false;  // Pop submenu
            }

            return true;
        }
    );
}

MenuItem createAlgoEntry(FmDrumPatch& patch) {
    return MenuItem::Action(
        "Algorithm: " + String(patch.algoIndex),
        [&patch](TextGUI& gui) {
            gui.enterSubmenu(
                { createAlgoChooserCustom(patch.algoIndex) },
                "Select Algorithm"
            );
        }
    );
}

inline float intToFloatPercent(int v, int max_ = 100) {
    return static_cast<float>(v) / max_;
}

inline int floatToIntPercent(float f, int max_ = 100) {
    return static_cast<int>(f * max_ + 0.5f);
}

// Convert int [min..max] to float [minF..maxF]
inline float intToFloatRange(int v, int minInt, int maxInt, float minF, float maxF) {
    if (maxInt == minInt) return minF; // avoid div0
    float norm = float(v - minInt) / (maxInt - minInt);
    return minF + norm * (maxF - minF);
}

// Convert float [minF..maxF] to int [min..max] with rounding
inline int floatToIntRange(float f, int minInt, int maxInt, float minF, float maxF) {
    if (maxF == minF) return minInt; // avoid div0
    float norm = (f - minF) / (maxF - minF);
    int v = int(norm * (maxInt - minInt) + 0.5f) + minInt;
    // Clamp to range
    if (v < minInt) v = minInt;
    else if (v > maxInt) v = maxInt;
    return v;
}

inline std::vector<MenuItem> createOpEditor(FmOpParams& op) { 
    return {
        MenuItem::Value("Ratio",
            [&]() { return floatToIntRange(op.ratio, 0, 1000, 0.f, 10.f); },      // assuming ratio range 0..10
            [&](int v) { op.ratio = intToFloatRange(v, 0, 1000, 0.f, 10.f); },
            0, 1000, 1),

        MenuItem::Value("Detune",
            [&]() { return floatToIntRange(op.detune, -100, 100, -10.f, 10.f); },
            [&](int v) { op.detune = intToFloatRange(v, -100, 100, -10.f, 10.f); },
            -100, 100, 1),

        MenuItem::Value("Feedback",
            [&]() { return floatToIntRange(op.feedback, 0, 100, 0.f, 10.f); },    // assuming feedback max 10
            [&](int v) { op.feedback = intToFloatRange(v, 0, 100, 0.f, 10.f); },
            0, 100, 1),

        MenuItem::Value("Volume",
            [&]() { return floatToIntRange(op.volume, 0, 100, 0.f, 1.f); },
            [&](int v) { op.volume = intToFloatRange(v, 0, 100, 0.f, 1.f); },
            0, 100, 1),

        MenuItem::Option("Waveform",
            [&]() { return int(op.waveform); },
            [&](int v) { op.waveform = Waveform(v); },
            Waveform::optionNames())
    };
}


inline std::vector<MenuItem> createPatchEditor(FmDrumPatch& patch) {
    std::vector<MenuItem> items;

    items.push_back(createAlgoEntry(patch));

    items.push_back(MenuItem::Value("Base Freq",
        [&]() { return int(patch.baseFreq + 0.5f); },   // freq in Hz, integer
        [&](int v) { patch.baseFreq = float(v); },
        0, 10000, 1));

    items.push_back(MenuItem::Value("Volume",
        [&]() { return floatToIntRange(patch.volume, 0, 100, 0.f, 2.f); },
        [&](int v) { patch.volume = intToFloatRange(v, 0, 100, 0.f, 2.f); },
        0, 100, 1));
        
    items.push_back(MenuItem::Value("Pan",
        [&]() { return floatToIntRange(patch.pan, -100, 100, -1.f, 1.f); },
        [&](int v) { patch.pan = intToFloatRange(v, -100, 100, -1.f, 1.f); },
        -100, 100, 1));
        
    items.push_back(MenuItem::Value("Reverb Send Lvl",
        [&]() { return floatToIntRange(patch.reverbSend, 0, 100, 0.f, 1.f); },
        [&](int v) { patch.reverbSend = intToFloatRange(v, 0, 100, 0.f, 1.f); },
        0, 100, 1));

    items.push_back(MenuItem::Value("Velocity Mod",
        [&]() { return floatToIntRange(patch.velocityMod, 0, 100, 0.f, 1.f); },
        [&](int v) { patch.velocityMod = intToFloatRange(v, 0, 100, 0.f, 1.f); },
        0, 100, 1));

    items.push_back(MenuItem::Value("Attack ms",
        [&]() { return int(patch.attack * 1000 + 0.5f); },
        [&](int v) { patch.attack = v / 1000.0f; },
        0, 8000, 1));

    items.push_back(MenuItem::Value("Hold ms",
        [&]() { return int(patch.hold * 1000 + 0.5f); },
        [&](int v) { patch.hold = v / 1000.0f; },
        0, 8000, 1));

    items.push_back(MenuItem::Value("Decay ms",
        [&]() { return int(patch.decay * 1000 + 0.5f); },
        [&](int v) { patch.decay = v / 1000.0f; },
        0, 8000, 1));

    items.push_back(MenuItem::Value("Sustain %",
        [&]() { return floatToIntRange(patch.sustain, 0, 100, 0.f, 1.f); },
        [&](int v) { patch.sustain = intToFloatRange(v, 0, 100, 0.f, 1.f); },
        0, 100, 1));

    items.push_back(MenuItem::Value("Release ms",
        [&]() { return int(patch.release * 1000 + 0.5f); },
        [&](int v) { patch.release = v / 1000.0f; },
        0, 8000, 1));

    items.push_back(MenuItem::Toggle("Use Filter",
        [&]() { return patch.useFilter; },
        [&](bool v) {patch.useFilter = v; }
        ));
        
    items.push_back(MenuItem::Value("Filter Freq",
        [&]() { return int(patch.filterFreqHz + 0.5f); },
        [&](int v) { patch.filterFreqHz = float(v); },
        0, 16000, 1));

    items.push_back(MenuItem::Value("Resonance",
        [&]() { return floatToIntRange(patch.filterReso, 0, 100, 0.f, 1.f); },
        [&](int v) { patch.filterReso = intToFloatRange(v, 0, 100, 0.f, 1.f); },
        0, 100, 1));

    items.push_back(MenuItem::Value("Filter Morph",
        [&]() { return floatToIntRange(patch.filterMorph, 0, 100, 0.f, 1.f); },
        [&](int v) { patch.filterMorph = intToFloatRange(v, 0, 100, 0.f, 1.f); },
        0, 100, 1));

    char label[12];
    for (int i = 0; i < 6; ++i) {
        snprintf(label, sizeof(label), "Op %d", i);
        int opIndex = i;
        items.push_back(MenuItem::Submenu(String(label), [patchPtr = &patch, opIndex]() -> std::vector<MenuItem> {
            return createOpEditor(patchPtr->ops[opIndex]);
        }));

    }

    return items;
}


inline std::vector<MenuItem> createDrumkitEditor() {
    std::vector<MenuItem> items;
    FmDrumPatch* patchMap = synth.getPatchMap();
 
    for (int note = 0; note < 128; ++note) {
        const char* name = gmDrumNoteName(static_cast<GmDrumNote>(note));
        auto item = MenuItem::Action(
            String(note) + ": " + name,
            [note, patchMap](TextGUI& gui) {
                gui.enterSubmenu(
                    createPatchEditor(patchMap[note]),
                    String("Patch ") + String(note),
                    nullptr,
                    note
                );                
                ESP_LOGI("GUI", "Entering patch editor for note %d", note);
            } 
        );  
        items.push_back(item);
    }
    return items;
}

inline std::vector<MenuItem> createSystemMenu() {
    return {
        MenuItem::Submenu("Save Drumkit", []() {
            std::vector<MenuItem> items;
            auto kitNames = DrumkitStorage::listDrumkits(FS_USED, DRUMKIT_DIR);

            for (const auto& name : kitNames) {
                items.emplace_back(MenuItem::Action(name, [name](TextGUI& gui) {
                    char path[64];
                    snprintf(path, sizeof(path), DRUMKIT_DIR "/%s.json", name.c_str());
                    bool ok = DrumkitStorage::saveDrumkit(FS_USED, path, synth.getPatchMap());
                    gui.message(ok ? "Saved: " + name : "Save Failed");
                }));
            }

            items.emplace_back(MenuItem::Action("New Drumkit", [](TextGUI& gui) {
                String newName = DrumkitStorage::getNextDrumkitName(FS_USED, DRUMKIT_DIR);
                char path[64];
                snprintf(path, sizeof(path), DRUMKIT_DIR "/%s.json", newName.c_str());
                bool ok = DrumkitStorage::saveDrumkit(FS_USED, path, synth.getPatchMap());
                gui.message(ok ? "Saved: " + newName : "Save Failed");
            }));

            return items;
        }),

        MenuItem::Submenu("Load Drumkit", []() {
            std::vector<MenuItem> items;
            auto kitNames = DrumkitStorage::listDrumkits(FS_USED, DRUMKIT_DIR);

            for (const auto& name : kitNames) {
                items.emplace_back(MenuItem::Action(name, [name](TextGUI& gui) {
                    char path[64];
                    snprintf(path, sizeof(path), DRUMKIT_DIR "/%s.json", name.c_str());
                    bool ok = DrumkitStorage::loadDrumkit(FS_USED, path, synth.getPatchMap());
                    gui.message(ok ? "Loaded: " + name : "Load Failed");
                }));
            }

            return items;
        })
    };
}

inline std::vector<MenuItem> createRootMenu() {
    return {
        MenuItem::Submenu("Edit Drumkit", []() {
            return createDrumkitEditor();
        }),
        MenuItem::Submenu("System", []() {
            return createSystemMenu();
        })
    };
}

} // namespace MenuStructure

#endif // ENABLE_GUI
