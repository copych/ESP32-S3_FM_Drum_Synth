#pragma once
#include <Arduino.h>
#include <math.h>

/** adsr envelope module
Original author(s) : Paul Batchelor
Ported from Soundpipe by Ben Sergentanis, May 2020
Remake by Steffan DIedrichsen, May 2021

Hardly modded by Copych, Jan 2024-Aug 2025
Added fast and semi-fast releases
Added HOLD phase
Unified D0 calculation
Fixed setting targets for different starting/ending points
Made epsylon neibourhood configurable via static const
Made it header-only
*/

class IRAM_ATTR Adsr {
public:
    enum eSegment_t {
        ADSR_SEG_IDLE, ADSR_SEG_ATTACK, ADSR_SEG_HOLD,
        ADSR_SEG_DECAY, ADSR_SEG_SUSTAIN, ADSR_SEG_RELEASE,
        ADSR_SEG_SEMI_FAST_RELEASE, ADSR_SEG_FAST_RELEASE
    };

    enum eEnd_t { END_REGULAR, END_SEMI_FAST, END_FAST, END_NOW };


    Adsr() {}
    ~Adsr() {}

    void init(float sample_rate, int blockSize = 1) {
        sample_rate_ = sample_rate / blockSize; 
        attackTarget_ = 1.0f + epsylon;
        attackTime_ = decayTime_ = releaseTime_ = -1.0f;
        fastReleaseTime_ = semiFastReleaseTime_ = -1.0f;
        sus_level_ = 1.0f;
        x_ = 0.0f;
        gate_ = false;
        mode_ = ADSR_SEG_IDLE;

        setTime(ADSR_SEG_ATTACK, 0.0f);
        setTime(ADSR_SEG_HOLD, 0.05f);
        setTime(ADSR_SEG_DECAY, 0.05f);
        setTime(ADSR_SEG_RELEASE, 0.05f);
        setTime(ADSR_SEG_FAST_RELEASE, 0.0005f); // just a few samples to manage polyphony overrun
        setTime(ADSR_SEG_SEMI_FAST_RELEASE, 0.02f); // note stealing, e.g. for exclusive note groups (open/close hats etc)
    }

    void retrigger(eEnd_t hardness) {
        gate_ = true;
        mode_ = ADSR_SEG_ATTACK;
        if (hardness == END_NOW) x_ = 0.0f;
        D0_ = attackD0_;
    }

    void end(eEnd_t hardness) {
        gate_ = false;
        target_ = -epsylon;
        switch (hardness) {
            case END_NOW:
                mode_ = ADSR_SEG_IDLE; D0_ = attackD0_; x_ = 0.f; break;
            case END_FAST:
                mode_ = ADSR_SEG_FAST_RELEASE; D0_ = fastReleaseD0_; break;
            case END_SEMI_FAST:
                mode_ = ADSR_SEG_SEMI_FAST_RELEASE; D0_ = semiFastReleaseD0_; break;
            case END_REGULAR:
            default:
                mode_ = ADSR_SEG_RELEASE; D0_ = releaseD0_; break;
        }
    }

    eSegment_t getCurrentSegment() {
        if (gate_ && (x_ == sus_level_)) return ADSR_SEG_SUSTAIN;
        return mode_;
    }

    const char* getCurrentSegmentStr() {
        eSegment_t seg = getCurrentSegment();
        switch (seg) {
            case ADSR_SEG_ATTACK: return "ATTACK";
            case ADSR_SEG_HOLD: return "HOLD";
            case ADSR_SEG_DECAY: return "DECAY";
            case ADSR_SEG_SUSTAIN: return "SUSTAIN";
            case ADSR_SEG_RELEASE: return "RELEASE";
            case ADSR_SEG_FAST_RELEASE: return "FAST_RELEASE";
            case ADSR_SEG_SEMI_FAST_RELEASE: return "SEMI_FAST_RELEASE";
            case ADSR_SEG_IDLE: return "IDLE";
            default: return "UNKNOWN";
        }
    }

    float getPenalty() const  {
        eSegment_t seg;
        if (gate_ && (x_ == sus_level_)) seg = ADSR_SEG_SUSTAIN;
        switch (seg) {
            case ADSR_SEG_ATTACK: return 0.0f;
            case ADSR_SEG_HOLD: return 0.0f;
            case ADSR_SEG_DECAY: return x_ * 0.2f;
            case ADSR_SEG_SUSTAIN: return x_ * 0.5f;
            case ADSR_SEG_RELEASE: return x_ * 0.4f;
            case ADSR_SEG_FAST_RELEASE: return x_ * 0.7f;
            case ADSR_SEG_SEMI_FAST_RELEASE: return x_ * 0.9f;
            case ADSR_SEG_IDLE: return x_ ;
            default: return x_ * 0.5f;
        }
    }

    inline bool isRunning() const { return mode_ != ADSR_SEG_IDLE; }
    inline bool isIdle() const { return mode_ == ADSR_SEG_IDLE; }
    inline float getVal() const { return x_; }
    inline float getTarget() const { return target_; }

    inline float getAttackTime() const { return attackTime_; }
    inline float getHoldTime() const { return holdTime_; }
    inline float getDecayTime() const { return decayTime_; }
    inline float getSustainLevel() const { return sus_level_; }
    inline float getReleaseTime() const { return releaseTime_; }
    inline float getFastReleaseTime() const { return fastReleaseTime_; }

    void setTime(int seg, float time) {
        switch (seg) {
            case ADSR_SEG_ATTACK: setAttackTime(time); break;
            case ADSR_SEG_HOLD: setHoldTime(time); break;
            case ADSR_SEG_DECAY: setTimeConstant(time, decayTime_, decayD0_); break;
            case ADSR_SEG_RELEASE: setTimeConstant(time, releaseTime_, releaseD0_); break;
            case ADSR_SEG_FAST_RELEASE: setTimeConstant(time, fastReleaseTime_, fastReleaseD0_); break;
            case ADSR_SEG_SEMI_FAST_RELEASE: setTimeConstant(time, semiFastReleaseTime_, semiFastReleaseD0_); break;
            default: break;
        }
    }

    void setHoldTime(float timeInS) {
        holdTime_ = timeInS;
        holdSamples_ = timeInS > 0.0f ? (uint32_t)(timeInS * sample_rate_) : 0;
        holdCounter_ = holdSamples_;
    }

    void setAttackTime(float timeInS) { setTimeConstant(timeInS, attackTime_, attackD0_); }
    void setDecayTime(float timeInS) { setTimeConstant(timeInS, decayTime_, decayD0_); }
    void setReleaseTime(float timeInS) { setTimeConstant(timeInS, releaseTime_, releaseD0_); }
    void setFastReleaseTime(float timeInS) { setTimeConstant(timeInS, fastReleaseTime_, fastReleaseD0_); }
    void setSemiFastReleaseTime(float timeInS) { setTimeConstant(timeInS, semiFastReleaseTime_, semiFastReleaseD0_); }

    inline void setSustainLevel(float sus_level) {
        sus_level = (sus_level <= 0.f) ? -0.001f : (sus_level > 1.f) ? 1.f : sus_level;
        sus_level_ = sus_level;
    }

    float __attribute__((hot, always_inline)) IRAM_ATTR process() {
        float out = 0.0f;
        switch (mode_) {
            case ADSR_SEG_IDLE:
                out = 0.0f; break;

            case ADSR_SEG_ATTACK:
                x_ += D0_ * (attackTarget_ - x_);
                out = x_;
                if (out >= 1.f) {
                    x_ = out = 1.f;
                    if (holdSamples_ > 0) {
                        mode_ = ADSR_SEG_HOLD;
                        holdCounter_ = holdSamples_;
                    } else {
                        mode_ = ADSR_SEG_DECAY;
                    target_ = sus_level_ - (x_ - sus_level_) * epsylon;
                        D0_ = decayD0_;
                    }
                }
                break;

            case ADSR_SEG_HOLD:
                out = x_;
                if (holdCounter_ > 0) --holdCounter_;
                else {
                    mode_ = ADSR_SEG_DECAY;
                    target_ = sus_level_ - (x_ - sus_level_) * epsylon;
                    D0_ = decayD0_;
                }
                break;

            case ADSR_SEG_DECAY:
            case ADSR_SEG_RELEASE:
            case ADSR_SEG_FAST_RELEASE:
            case ADSR_SEG_SEMI_FAST_RELEASE:
                x_ += D0_ * (target_ - x_);
                out = x_;
                if (out < 0.0f) {
                    mode_ = ADSR_SEG_IDLE;
                    x_ = out = 0.f;
                    target_ = -epsylon;
                    D0_ = attackD0_;
                }
                break;

            default: break;
        }
        return out;
    }

private:
    static constexpr float epsylon = 0.01f;
    static constexpr float epsylon2 = epsylon / (1.0f + epsylon);
    void setTimeConstant(float timeInS, float& time, float& coeff) {
        if (timeInS != time) {
            time = timeInS;
            coeff = (time > 0.f) ? 1.0f - powf(epsylon2, 1.0f / (sample_rate_ * time)) : 1.f;
        }
    }

    float sus_level_{0.f}, x_{0.f}, target_{0.f}, D0_{0.f};
    float attackTarget_{1.0f}, attackTime_{-1.0f};
    float decayTime_{-1.0f}, releaseTime_{-1.0f}, fastReleaseTime_{-1.0f}, semiFastReleaseTime_{-1.0f};
    float attackD0_{0.f}, decayD0_{0.f}, releaseD0_{0.f}, fastReleaseD0_{0.f}, semiFastReleaseD0_{0.f};
    float holdTime_ = 0.0f;
    uint32_t holdSamples_ = 0, holdCounter_ = 0;
    int sample_rate_;
    volatile eSegment_t mode_{ADSR_SEG_IDLE};
    bool gate_{false};
};
