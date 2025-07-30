#pragma once
#include <math.h>
#include "misc.h"

#ifndef PI_F
#define PI_F 3.14159265f
#endif

class IRAM_ATTR SvfFilter {
public:
    void init(float sampleRate) {
        sr_ = sampleRate;
        fc_ = 16000.0f;
        res_ = 0.0f;
        preDrive_ = 0.5f;
        drive_ = 0.0f;
        fcMax_ = sr_ / 2.75625f;
        morph_ = 0.33f;
        reset();
        updateParams();
    }

    void reset() {
        low_ = band_ = high_ = 0.0f;
    }

    void setFreqHz(float f) {
        fc_ = fclamp(f, 1.0f, fcMax_);
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
        preDrive_ = fclamp(d, 0.0f, 1.0f);
        drive_ = preDrive_ * preDrive_ * (1.0f + 2.0f * res_) * 0.3f;
    }

    inline float getFreqHz()    const { return fc_; }
    inline float getResonance() const { return res_; }
    inline float getDrive()     const { return preDrive_; }
    inline float getMorph()     const { return morph_; }

    inline void process(float in) {
        float notch = in - damp_ * band_;

        // First update band with limited feedback
        float hp = notch - low_;
        float bp = fclamp(band_ + freq_ * hp, -bandLimit_, bandLimit_);

        // Nonlinear drive
        bp -= drive_ * bp * fabsf(bp);

        // Update states
        float lp = low_ + freq_ * bp;
        low_ = lp;
        band_ = bp;
        high_ = hp;

        outLow_  = lp;
        outBand_ = bp;
        outHigh_ = hp;
    }

    inline float processMorph(float in) {
        process(in);
        float mixed;
        if (morph_ <= 0.5f) {
            float t = morph_ * 2.0f;
            mixed = outLow_ * (1.0f - t) + outBand_ * t;
        } else {
            float t = (morph_ - 0.5f) * 2.0f;
            mixed = outBand_ * (1.0f - t) + outHigh_ * t;
        }
        return mixed * 1.2f; // Empirical output gain
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
    float bandLimit_ = 1.5f; // soft safety

    // Filter state
    float low_  = 0.0f;
    float band_ = 0.0f;
    float high_ = 0.0f;

    // Outputs
    float outLow_  = 0.0f;
    float outBand_ = 0.0f;
    float outHigh_ = 0.0f;

    void updateParams() {
        // Frequency to omega
        float omega = PI_F * fc_ * DIV_SAMPLE_RATE;
        freq_ = 2.0f * fast_sin(omega);

        // Improved damping equation
        damp_ = fclamp(
            2.0f * (1.0f - powf(res_, 0.25f)),
            0.0f,
            fclamp(2.0f / freq_ - 0.5f * freq_, 0.0f, 2.0f)
        );

        // Band limiting to avoid runaway
        bandLimit_ = 1.5f + 2.0f * res_;  // slightly adaptive

        // Update drive again
        drive_ = preDrive_ * preDrive_ * (1.0f + 2.0f * res_) * 0.3f;
    }
};
