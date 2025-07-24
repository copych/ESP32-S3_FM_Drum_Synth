#pragma once
#include "misc.h"

#pragma once
#include <math.h>

#ifndef PI_F
#define PI_F 3.14159265f
#endif

class IRAM_ATTR SvfFilter {
public:
    void init(float sampleRate) {
        sr_       = sampleRate;
        fc_       = 16000.0f;
        res_      = 0.0f;
        preDrive_ = 0.5f;
        drive_    = 0.0f;
        fcMax_    = sr_ / 2.75625f;
        low_ = band_ = high_ = 0.0f;
        morph_ = 0.33f; 
        updateParams();
    }

    void reset() {        
        low_ = band_ = high_ = 0.0f;
    }

    void setFreqHz(float f) {
        fc_ = fclamp(f, 1.0e-6f, fcMax_);
        updateParams();
    }

    void setResonance(float r) {
        res_ = fclamp(r, 0.0f, 1.0f);
        updateParams();
    }

    void setMorph(float m) {
        morph_ = fclamp(m, 0.0f, 1.0f);
    }

    void setDrive(float d) {
        // Map 0-1 input to exponential curve for more musical control
        preDrive_ = fclamp(d, 0.0f, 1.0f);
        // Calculate drive with compensation for resonance
        drive_ = preDrive_ * preDrive_ * (1.0f + 2.0f * res_);
        // Scale to maintain stability
        drive_ *= 0.3f;  // Empirical scaling factor
    }
 
    inline float getFreqHz() const      { return fc_; }
    inline float getResonance() const   { return res_; }
    inline float getDrive() const       { return preDrive_; }
    inline float getMorph() const       { return morph_; }

    void process(float in) {
        float notch = in - damp_ * band_;
        low_  += freq_ * band_;
        high_ = notch - low_;
        band_ = fclamp(band_, -1.5f, 1.5f);
        band_ = freq_ * high_ + band_ - drive_ * band_ * fabsf(band_);

        outLow_  = low_;
        outBand_ = band_;
        outHigh_ = high_;
    }

    inline float processMorph(float in) {
        // fast blend of LP → BP → HP
        process(in);
        float mixed;
        if (morph_ <= 0.5f) {
            float t = morph_ * 2.0f;
            mixed = outLow_ * (1.0f - t) + outBand_ * t;
        } else {
            float t = (morph_ - 0.5f) * 2.0f;
            mixed = outBand_ * (1.0f - t) + outHigh_ * t;
        }
        // optional final gain
        return mixed * 1.2f; // empirical gain
    }

    float getLow()  const { return outLow_; }
    float getBand() const { return outBand_; }
    float getHigh() const { return outHigh_; }

private:
    float sr_       = 44100.0f;
    float fc_       = 200.0f;
    float res_      = 0.5f;
    float freq_     = 0.25f;
    float damp_     = 0.0f;
    float preDrive_ = 0.5f;
    float drive_    = 0.25f;
    float fcMax_    = 16000.0f;
    float morph_    = 0.5f;

    // filter state
    float low_      = 0.0f;
    float band_     = 0.0f;
    float high_     = 0.0f;

    // outputs
    float outLow_   = 0.0f;
    float outBand_  = 0.0f;
    float outHigh_  = 0.0f;

    void updateParams() {
        freq_ = 2.0f * sinf(PI_F * fc_ / sr_);
        damp_ = fclamp(2.0f * (1.0f - powf(res_, 0.25f)),
                    0.0f,
                    fclamp(2.0f / freq_ - freq_ * 0.5f, 0.0f, 2.0f));
        // Update drive with new resonance value
        drive_ = preDrive_ * preDrive_ * (1.0f + 2.0f * res_) * 0.3f;
    }
};