#pragma once

#include <math.h>
#include <string>
#include <stdexcept>
#include <vector>
#include "misc.h"

#define MOD_RANGE 16.0f

class IRAM_ATTR Waveform {
public:
    enum Enum {
        Sine, Cosine, Triangle, Square, Saw,
        NegSine, NegCosine, NegTriangle, NegSquare, NegSaw
    };

    Waveform() : value(Sine) {}
    Waveform(Enum v) : value(v) {}
    explicit Waveform(int v) {
        if (v < 0 || v > NegSaw) throw std::out_of_range("Invalid Waveform value");
        value = static_cast<Enum>(v);
    }

    operator Enum() const { return value; }

    const std::string name() const {
        static const std::string names[] = {
            "Sine", "Cosine", "Triangle", "Square", "Saw",
            "Negative Sine", "Negative Cosine", "Negative Triangle", "Negative Square", "Negative Saw"
        };
        return names[value];
    }

    const std::string shortName() const {
        static const std::string names[] = {
            "sin", "cos", "tri", "sqr", "saw",
            "-sin", "-cos", "-tri", "-sqr", "-saw"
        };
        return names[value];
    }

    static const std::vector<String>& optionNames() {
        static const std::vector<String> names = {
            "sin", "cos", "tri", "sqr", "saw",
            "-sin", "-cos", "-tri", "-sqr", "-saw"
        };
        return names;
    }

    static size_t numOptions() { return 10; }

    bool operator==(Waveform other) const { return value == other.value; }
    bool operator!=(Waveform other) const { return value != other.value; }

    Enum value;
};

// === Waveform generators ===
inline float __attribute__((always_inline)) IRAM_ATTR wf_sine(float t)     { return SIN_FUNC_NORM(t); }
inline float __attribute__((always_inline)) IRAM_ATTR wf_cos(float t)      { return SIN_FUNC_NORM(t + 0.25f); }
inline float __attribute__((always_inline)) IRAM_ATTR wf_triangle(float t) { return 4.0f * fast_fabsf(t - 0.5f) - 1.0f; }
inline float __attribute__((always_inline)) IRAM_ATTR wf_square(float t)   { return (t < 0.5f) ? 1.0f : -1.0f; }
inline float __attribute__((always_inline)) IRAM_ATTR wf_saw(float t)      { return t * 2.0f - 1.0f; }
inline float __attribute__((always_inline)) IRAM_ATTR wf_negsine(float t)     { return -SIN_FUNC_NORM(t); }
inline float __attribute__((always_inline)) IRAM_ATTR wf_negcos(float t)      { return -SIN_FUNC_NORM(t + 0.25f); }
inline float __attribute__((always_inline)) IRAM_ATTR wf_negtriangle(float t) { return 1.0f - 4.0f * fast_fabsf(t - 0.5f); }
inline float __attribute__((always_inline)) IRAM_ATTR wf_negsquare(float t)   { return (t < 0.5f) ? -1.0f : 1.0f; }
inline float __attribute__((always_inline)) IRAM_ATTR wf_negsaw(float t)      { return 1.0f - t * 2.0f; }

class IRAM_ATTR FmOperator {
public:
    FmOperator() {
        setWaveform(Waveform::Sine);
        setVolume(0.8f);
    }

    inline float& getSampleRate()   { return sampleRate_; }
    inline float& getFrequency()    { return baseFreq_; }
    inline float& getRatio()        { return ratio_; }
    inline float& getDetune()       { return detune_; }
    inline float& getFeedback()     { return fb_; }
    inline float& getFeedbackMod()  { return fbMod_; }
    inline float& getVolume()       { return volume_; }
    inline Waveform& getWaveform()  { return waveform_; }

    inline float getSampleRate() const  { return sampleRate_; }
    inline float getFrequency() const   { return baseFreq_; }
    inline float getRatio() const       { return ratio_; }
    inline float getDetune() const      { return detune_; }
    inline float getFeedback() const    { return fb_; }
    inline float getFeedbackMod() const { return fbMod_; }
    inline float getVolume() const      { return volume_; }
    inline Waveform getWaveform() const { return waveform_; }

    void setSampleRate(float sr) {
        sampleRate_ = sr;
        updatePhaseInc();
    }

    void setFrequency(float hz) {
        baseFreq_ = hz;
        updatePhaseInc();
    }

    void setRatio(float r) {
        ratio_ = r;
        updatePhaseInc();
    }

    void setDetune(float d) {
        detune_ = d;
        updatePhaseInc();
    }

    void setFeedback(float f) {
        fb_ = f;
        feedback_ = (fb_ == 0) ? 0.0f : (1.0f / powf(2.0f, (7.0f - fb_)));
        fbMult_ = fbMod_ * feedback_;
    }

    void setFeedbackMod(float m) {
        fbMod_ = m;
        fbMult_ = fbMod_ * feedback_;
    }

    void setWaveform(Waveform wf = Waveform::Sine) {
        waveform_ = wf;
    }

    void setVolume(float v) {
        volume_ = v;
        outLevel_ = volume_;
        fmLevel_  = 0.1f * powf(161.0f, volume_) - 0.1f;
        amLevel_  = volume_ * 0.5f;
        amOffset_  = 1.0f - 0.5f * amLevel_;
    }

    void reset() {
        phase_ = 0.f;
        lastOut_ = 0.f;
    }

    inline float __attribute__((always_inline)) IRAM_ATTR     renderWaveform(Waveform::Enum wf, float t) {
        switch (wf) {
            case Waveform::Sine:        return wf_sine(t);
            case Waveform::Cosine:      return wf_cos(t);
            case Waveform::Triangle:    return wf_triangle(t);
            case Waveform::Square:      return wf_square(t);
            case Waveform::Saw:         return wf_saw(t);
            case Waveform::NegSine:     return wf_negsine(t);
            case Waveform::NegCosine:   return wf_negcos(t);
            case Waveform::NegTriangle: return wf_negtriangle(t);
            case Waveform::NegSquare:   return wf_negsquare(t);
            case Waveform::NegSaw:      return wf_negsaw(t);
            default:                   return wf_sine(t);
        }
    }

    inline float __attribute__((always_inline)) IRAM_ATTR fmProcess(float modIn, float env) {
        float t = advance(modIn);
        float s = renderWaveform(waveform_.value, t);
        lastOut_ = s;
        return fmLevel_ * s * env;
    }

    inline float __attribute__((always_inline)) IRAM_ATTR amProcess(float modIn) {
        float t = advance(modIn);
        float s = renderWaveform(waveform_.value, t);
        lastOut_ = s;
        return amOffset_ + amLevel_ * s; 
    }

    inline float __attribute__((always_inline)) IRAM_ATTR outProcess(float modIn, float env) {
        float t = advance(modIn);
        float s = renderWaveform(waveform_.value, t);
        lastOut_ = s;
        return outLevel_ * s * env;
    }


private:
    inline float __attribute__((always_inline)) IRAM_ATTR wrap01(float x) const {
        return x - fast_floorf(x);
    }

    inline float __attribute__((always_inline)) IRAM_ATTR advance(float modIn) {
        phase_ += phaseInc_;
        if (phase_ >= 1.0f) phase_ -= 1.0f;
        return wrap01(phase_ + modIn * MOD_RANGE + fbMult_ * lastOut_);
    }


    inline void __attribute__((always_inline)) IRAM_ATTR updatePhaseInc() {
        float f = baseFreq_ * ratio_ + detune_;
        phaseInc_ = f * DIV_SAMPLE_RATE;
    }

    // Internal state
    float sampleRate_ = 44100.f;
    float baseFreq_   = 440.f;
    float ratio_      = 1.f;
    float detune_     = 0.f;
    float fb_         = 0.f;
    float fbMod_      = 1.f;
    float feedback_   = 0.f;
    float fbMult_     = 0.f;
    float volume_     = 0.8f;

    float phase_      = 0.f;
    float phaseInc_   = 0.f;
    float lastOut_    = 0.f;

    float outLevel_   = 0.8f;
    float fmLevel_    = 0.0f;
    float amLevel_    = 0.0f;
    float amOffset_   = 1.0f;
    Waveform waveform_;
};
