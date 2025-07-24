#pragma once
#include <Arduino.h>
#include <FS.h>
#include <vector>
#include <ArduinoJson.h>
#include "FmPatch.h"


namespace DrumkitStorage {

bool saveSinglePatch(fs::FS& fs, const char* path, const FmDrumPatch& patch);
bool loadSinglePatch(fs::FS& fs, const char* path, FmDrumPatch& outPatch);

bool saveDrumkit(fs::FS& fs, const char* path, FmDrumPatch patches[128]);
bool loadDrumkit(fs::FS& fs, const char* path, FmDrumPatch patches[128]);

std::vector<String> listDrumkits(fs::FS& fs, const char* dir);
String getNextDrumkitName(fs::FS& fs, const char* dir);

} // namespace DrumkitStorage
