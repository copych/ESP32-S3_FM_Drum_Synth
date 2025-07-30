#pragma once

#include <Arduino.h>
#include <FS.h>
#include <vector>
#include <ArduinoJson.h>
#include "FmPatch.h"
#include "esp_log.h"

namespace DrumkitStorage {

 

inline void serializePatch(JsonObject obj, const FmDrumPatch& patch) {
    obj["name"] = patch.name;
    obj["alg"] = patch.algoIndex;
    obj["freq"] = patch.baseFreq;
    obj["veloMod"] = patch.velocityMod;
    obj["vol"] = patch.volume;
    obj["pan"] = patch.pan;
    obj["rvb"] = patch.reverbSend;

    obj["atk"] = patch.attack;
    obj["hold"] = patch.hold;
    obj["dec"] = patch.decay;
    obj["sus"] = patch.sustain;
    obj["rel"] = patch.release;
    obj["flt"] = patch.useFilter;

    obj["filterFreq"] = patch.filterFreqHz;
    obj["filterReso"] = patch.filterReso;
    obj["filterMorph"] = patch.filterMorph;

    JsonArray ops = obj.createNestedArray("ops");
    for (int i = 0; i < 6; ++i) {
        JsonObject op = ops.createNestedObject();
        op["ratio"] = patch.ops[i].ratio;
        op["detune"] = patch.ops[i].detune;
        op["fb"] = patch.ops[i].feedback;
        op["vol"] = patch.ops[i].volume;
        op["wave"] = static_cast<int>(patch.ops[i].waveform.value);
    }
}

inline void deserializePatch(JsonObject obj, FmDrumPatch& patch) {
    const char* jsonName = obj["name"] | "";
    strncpy(patch.name, jsonName, sizeof(patch.name) - 1);
    patch.name[sizeof(patch.name) - 1] = '\0';

    patch.algoIndex = obj["alg"] | 0;
    patch.baseFreq = obj["freq"] | 440.0f;
    patch.velocityMod = obj["veloMod"] | 0.5f;
    patch.volume = obj["vol"] | 1.0f;
    patch.pan = obj["pan"] | 0.0f;
    patch.reverbSend = obj["rvb"] | 0.1f;

    patch.attack  = obj["atk"] | 0.01f;
    patch.hold    = obj["hold"] | 0.01f;
    patch.decay   = obj["dec"] | 0.2f;
    patch.sustain = obj["sus"] | 0.0f;
    patch.release = obj["rel"] | 0.1f;

    patch.useFilter = obj["flt"] | 0;

    patch.filterFreqHz = obj["filterFreq"] | 16000.0f;
    patch.filterReso = obj["filterReso"] | 0.5f;
    patch.filterMorph = obj["filterMorph"] | 0.0f;

    JsonArray ops = obj["ops"];
    for (int i = 0; i < 6; ++i) {
        JsonObject op = ops[i];
        patch.ops[i].ratio    = op["ratio"] | 1.0f;
        patch.ops[i].detune   = op["detune"] | 0.0f;
        patch.ops[i].feedback = op["fb"] | 0.0f;
        patch.ops[i].volume   = op["vol"] | 0.8f;
        patch.ops[i].waveform = Waveform(op["wave"] | 0);
    }
}

inline bool saveSinglePatch(fs::FS& fs, const char* path, const FmDrumPatch& patch) {
    DynamicJsonDocument doc(1024);
    serializePatch(doc.to<JsonObject>(), patch);
    File f = fs.open(path, FILE_WRITE);
    if (!f) return false;
    serializeJsonPretty(doc, f);
    f.close();
    return true;
}

inline bool loadSinglePatch(fs::FS& fs, const char* path, FmDrumPatch& outPatch) {
    File f = fs.open(path, FILE_READ);
    if (!f) return false;
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return false;
    deserializePatch(doc.as<JsonObject>(), outPatch);
    return true;
}

inline bool saveDrumkit(fs::FS& fs, const char* path, FmDrumPatch patches[128], const FxReverb& reverb) {
    DynamicJsonDocument doc(65536);
    JsonObject root = doc.to<JsonObject>();

    // Add reverb params
    root["reverbTime"] = reverb.getTime();
    root["reverbLevel"] = reverb.getLevel();
    root["reverbDamp"] = reverb.getDamping();
    root["reverbPreDelay"] = reverb.getPreDelayTime();

    JsonArray patchArray = root.createNestedArray("patches");
    for (int i = 0; i < 128; ++i) {
        JsonObject obj = patchArray.createNestedObject();
        serializePatch(obj, patches[i]);
    }

    File f = fs.open(path, FILE_WRITE);
    if (!f) return false;
    serializeJsonPretty(doc, f);
    f.close();
    return true;
}

inline bool loadDrumkit(fs::FS& fs, const char* path, FmDrumPatch patches[128], FxReverb& reverb) {
    File f = fs.open(path, FILE_READ);
    if (!f) return false;

    DynamicJsonDocument doc(65536);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return false;

    JsonObject root = doc.as<JsonObject>();

    if (!root.containsKey("patches")) return false;
    JsonArray arr = root["patches"];
    if (arr.size() < 128) return false;

    for (int i = 0; i < 128; ++i)
        deserializePatch(arr[i], patches[i]);

    // Load reverb params
    if (root.containsKey("reverbTime"))     reverb.setTime(root["reverbTime"]);
    if (root.containsKey("reverbLevel"))    reverb.setLevel(root["reverbLevel"]);
    if (root.containsKey("reverbDamp"))     reverb.setDamping(root["reverbDamp"]);
    if (root.containsKey("reverbPreDelay")) reverb.setPreDelayTime(root["reverbPreDelay"]);

    return true;
}


inline std::vector<String> listDrumkits(fs::FS& fs, const char* dir) {
    std::vector<String> kits;
    File root = fs.open(dir);
    if (!root || !root.isDirectory()) {
        ESP_LOGW("DrumkitStorage", "Directory '%s' missing or not a directory", dir);
        return kits;
    }

    File file;
    while ((file = root.openNextFile())) {
        String name = file.name();
        ESP_LOGI("DrumkitStorage", "Found file: %s", name.c_str());

        String lower = name;
        lower.toLowerCase();

        if (lower.endsWith(".json")) {
            if (name.startsWith(dir)) {
                name.remove(0, String(dir).length());
            }
            name.remove(name.length() - 5); // remove ".json"
            kits.push_back(name);
            ESP_LOGI("DrumkitStorage", "Drumkit added: %s", name.c_str());
        }
    }
    return kits;
}

inline String getNextDrumkitName(fs::FS& fs, const char* dir) {
    auto kits = listDrumkits(fs, dir);
    int maxIndex = 0;
    for (auto& name : kits) {
        String lower = name;
        lower.toLowerCase();
        if (lower.startsWith("drumkit")) {
            int num = lower.substring(7).toInt();
            if (num > maxIndex) maxIndex = num;
        }
    }
    return "Drumkit" + String(maxIndex + 1);
}

} // namespace DrumkitStorage
