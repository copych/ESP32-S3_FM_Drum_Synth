/*
* FxReverb - Reverberation audio effect
* based on a freeverb conception
* stereo processing of a mono input signal
* has a pre-delay setting 0..MAX_PREDELAY_MS
* has damping setting 0..1
* 
* May 2025
* Author: Evgeny Aslovskiy AKA Copych
* License: MIT
*/

#pragma once
#include "config.h"

#ifdef BOARD_HAS_PSRAM 
  #define REV_MULTIPLIER 1.8f
  #define MALLOC_CAP MALLOC_CAP_SPIRAM
#else
  #define REV_MULTIPLIER 0.35f
  #define MALLOC_CAP MALLOC_CAP_INTERNAL
#endif

constexpr int DRAM_ATTR NUM_COMBS = 4;
constexpr int DRAM_ATTR NUM_ALLPASSES = 3;
constexpr int DRAM_ATTR MAX_PREDELAY_MS = 100;
constexpr float DRAM_ATTR ATTENUATOR = 0.2f / NUM_COMBS;

const DRAM_ATTR float comb_lengths[NUM_COMBS] = {3604.0f, 3112.0f, 4044.0f, 4492.0f};
const DRAM_ATTR float comb_gains[NUM_COMBS]   = {0.805f, 0.827f, 0.783f, 0.764f};
const DRAM_ATTR float comb_damping_coef[NUM_COMBS] = {0.83f, 0.9f, 1.0f, 0.8f}; 

const DRAM_ATTR float allpass_lengths[NUM_ALLPASSES] = {500.0f, 168.0f, 48.0f};
const DRAM_ATTR float allpass_gains[NUM_ALLPASSES]   = {0.707f, 0.707f, 0.707f};

class IRAM_ATTR FxReverb {
public:
  FxReverb() {}

  inline void init() {
    for (int ch = 0; ch < 2; ++ch) {
      for (int i = 0; i < NUM_COMBS; ++i) {
        int size = int((comb_lengths[i] + ch * 17) * REV_MULTIPLIER); // small offset for stereo
        combBuf[ch][i] = (float*)heap_caps_aligned_alloc(4, sizeof(float) * size, MALLOC_CAP);
        combSize[ch][i] = size;
        combPtr[ch][i] = 0;
        combStore[ch][i] = 0.0f;

        if (!combBuf[ch][i]) {
          ESP_LOGE("Reverb", "No memory for combBuf[%d][%d]", ch, i);
        } else {
          memset(combBuf[ch][i], 0, sizeof(float) * size);
        }
      }

      for (int i = 0; i < NUM_ALLPASSES; ++i) {
        int size = int((allpass_lengths[i] + ch * 11) * REV_MULTIPLIER); // offset for stereo
        allpassBuf[ch][i] = (float*)heap_caps_aligned_alloc(4, sizeof(float) * size, MALLOC_CAP);
        allpassSize[ch][i] = size;
        allpassPtr[ch][i] = 0;

        if (!allpassBuf[ch][i]) {
          ESP_LOGE("Reverb", "No memory for allpassBuf[%d][%d]", ch, i);
        } else {
          memset(allpassBuf[ch][i], 0, sizeof(float) * size);
        }
      }
    }
    int size = int((MAX_PREDELAY_MS / 1000.0f) * SAMPLE_RATE);
    predelayBuf = (float*)heap_caps_aligned_alloc(4, sizeof(float) * size, MALLOC_CAP);
    if (!predelayBuf) {
      ESP_LOGE("Reverb", "Failed to allocate predelayBuf");
    } else {
      memset(predelayBuf, 0, sizeof(float) * size);
      predelaySize = size;
      predelayPtr = 0;
    }
    setLevel(0.5f);
    setTime(0.8f);
    setPreDelayTime(10.0f);
    setDamping(0.6f);
  }

  inline float getTime() const {  return rev_time;  }
  inline float getLevel() const { return rev_level; }
  inline float getPreDelayTime() const { return (float)delaySamples * 1000.0f * DIV_SAMPLE_RATE; }
  inline float getDamping() const { return globalDamping; }
  

  inline void setPreDelayTime(float ms) {
    delaySamples = int(ms * 0.001f * (float)SAMPLE_RATE);
    if (delaySamples >= predelaySize) delaySamples = predelaySize - 1;
    if (delaySamples < 0) delaySamples = 0;
    predelayReadOffset = (predelayPtr - delaySamples + predelaySize) % predelaySize;

    ESP_LOGD("Reverb", "Pre-delay set to %.1f ms (%d samples)", ms, delaySamples);
  }

  inline void setTime(float value) {
    rev_time = 0.998f * value + 0.001f;
    for (int ch = 0; ch < 2; ++ch) {
      for (int i = 0; i < NUM_COMBS; ++i)
        combLim[ch][i] = int(rev_time * combSize[ch][i]);
      for (int i = 0; i < NUM_ALLPASSES; ++i)
        allpassLim[ch][i] = int(rev_time * allpassSize[ch][i]);
    }
  }

  inline void setLevel(float value) {     rev_level = value;   }
  
  inline void setDamping(float d) {
    globalDamping = d < 0.0f ? 0.0f : (d > 1.0f ? 1.0f : d);
    for (int i = 0; i < NUM_COMBS; ++i) {
      comb_dampings[i] = globalDamping * comb_damping_coef[i] ;
    }
    ESP_LOGI("Reverb", "Global damping set to %.2f", globalDamping);
  }
  
  inline void  __attribute__((hot,always_inline)) IRAM_ATTR processBlock(float* signal_l, float* signal_r) {
    for (int n = 0; n < DMA_BUFFER_LEN; ++n) {
      float inSample = 0.5f * (signal_l[n] + signal_r[n]);

      // Pre-delay
      predelayBuf[predelayPtr] = inSample;
      float delayed = predelayBuf[predelayReadOffset];
      predelayPtr = (predelayPtr + 1) % predelaySize;
      predelayReadOffset = (predelayReadOffset + 1) % predelaySize;

      float wetL = processChannel(0, delayed);
      float wetR = processChannel(1, delayed);

      signal_l[n] = rev_level * wetL;
      signal_r[n] = rev_level * wetR;
    }
  }

  inline void  __attribute__((hot,always_inline)) IRAM_ATTR process(float* signal_l, float* signal_r) {
    // Store current input sample into predelay buffer
    float inSample = 0.5f * (*signal_l + *signal_r);
    predelayBuf[predelayPtr] = inSample;

    // Read from pre-delay buffer
    float delayedSample = predelayBuf[predelayReadOffset];

    // Advance pointers
    predelayPtr = (predelayPtr + 1) % predelaySize;
    predelayReadOffset = (predelayReadOffset + 1) % predelaySize;

    // Use delayedSample for reverb input
    float wetL = processChannel(0, delayedSample);
    float wetR = processChannel(1, delayedSample);


    *signal_l = rev_level * wetL;
    *signal_r = rev_level * wetR;
  }

private:

  float comb_dampings[NUM_COMBS] = {0.2f, 0.25f, 0.3f, 0.22f}; 
  float* predelayBuf = nullptr;
  int predelaySize = 0;
  int predelayPtr = 0;
  int delaySamples = 0;
  int predelayReadOffset = 0;
  float globalDamping = 0.25f;

  float rev_time = 0.5f;
  float rev_level = 0.5f;

  float* combBuf[2][NUM_COMBS] = {};
  int combSize[2][NUM_COMBS] = {};
  int combPtr[2][NUM_COMBS] = {};
  int combLim[2][NUM_COMBS] = {};
  float combStore[2][NUM_COMBS] = {};  // for damping

  float* allpassBuf[2][NUM_ALLPASSES] = {};
  int allpassSize[2][NUM_ALLPASSES] = {};
  int allpassPtr[2][NUM_ALLPASSES] = {};
  int allpassLim[2][NUM_ALLPASSES] = {};

  inline float  __attribute__((hot,always_inline)) IRAM_ATTR processChannel(int ch, float input) {
    float sum = 0.0f;

    for (int i = 0; i < NUM_COMBS; ++i)
        sum += doComb(ch, i, input);

    float out = sum * ATTENUATOR;

    for (int i = 0; i < NUM_ALLPASSES; ++i)
        out = doAllpass(ch, i, out);

    return out;
  }

  inline float  __attribute__((hot,always_inline)) IRAM_ATTR doComb(int ch, int idx, float in) {
    int& p = combPtr[ch][idx];
    float g = comb_gains[idx];
    float& store = combStore[ch][idx];
    float* buf = combBuf[ch][idx];

    float out = buf[p];
    store = store * (1.0f - comb_dampings[idx]) + out * comb_dampings[idx];

    buf[p] = in + store * g;
    p = (p + 1 >= combLim[ch][idx]) ? 0 : p + 1;
    return out;
  }


  inline float  __attribute__((hot,always_inline)) IRAM_ATTR doAllpass(int ch, int idx, float in) {
    int& p = allpassPtr[ch][idx];
    float g = allpass_gains[idx];
    float* buf = allpassBuf[ch][idx];
    float out = buf[p];
    float v = out * g + in;
    buf[p] = v;
    out = out - g * in;
    p = (p + 1 >= allpassLim[ch][idx]) ? 0 : p + 1;
    return out;
  }
};
