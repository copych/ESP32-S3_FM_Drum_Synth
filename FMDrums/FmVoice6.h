#pragma once
#include "FmOperator.h"
#include "FmPatch.h"
#include "svf_morph.h"
#include "Adsr.h"
#include <array> 

struct FmDrumPatch; 

#define NUM_ALGOS 18

class IRAM_ATTR FmVoice6 {
public:
    FmVoice6() {
        setSampleRate(SAMPLE_RATE);
        for (auto& op : ops) op.setSampleRate(sampleRate_);
        filter.init(sampleRate_);
        env.init(sampleRate_);
    }
    static constexpr int NumOps = 6;
    static constexpr int NumAlgos = NUM_ALGOS;
    inline uint8_t& getAlgorithm()  { return algo_; }
    inline FmOperator& getOp(int i) { return ops[i]; }
    inline const FmOperator& getOp(int i) const { return ops[i]; }
    inline float& getFrequency() { return baseFreq_; }
    inline float& getVeloMod() { return veloMod_; }
    inline float& getVolume() { return volume_; }
    inline float& getPan() { return pan_; }
    inline float& getPanL() { return panL_; }
    inline float& getPanR() { return panR_; }
    inline float& getReverbSend() { return reverbSend_; }
    inline Adsr& getEnv() { return env; }
    inline SvfFilter& getFilter() { return filter; }
    inline bool isFilterActive() const { return useFilter_; }
    void setSampleRate(float sr) {
        sampleRate_ = sr;
        env.init(sr);
        for (auto& op : ops) op.setSampleRate(sr);
    }

    void setFilterActive(bool active) {
        useFilter_ = active ? true : false;
    }

    void setOperatorParams(int i, float ratio, float detune, float feedback = 0.f, float vol = 0.8f, Waveform wf = Waveform::Sine) {
        if (i >= 0 && i < NumOps) {
            ops[i].setRatio(ratio);
            ops[i].setDetune(detune);
            ops[i].setFeedback(feedback);
            ops[i].setVolume(vol);
            ops[i].setWaveform(wf);
        }
    }

    void setVeloMod(float mod) {
        veloMod_ = fclamp(mod, 0.0f, 1.0f);
        veloMult_ = velocity_ * veloMod_;
    }

    void setAhdsr(float a, float h, float d, float s, float r) {
        env.setAttackTime(a);
        env.setHoldTime(h);
        env.setDecayTime(d);
        env.setSustainLevel(s);
        env.setReleaseTime(r);
    }

    void setFrequency(float hz) {
        baseFreq_ = hz;
        for (auto& op : ops)
            op.setFrequency(hz);
    }

    void setAlgorithm(uint8_t id) {
        algo_ = (id < NumAlgos) ? id : 0;
    }

    void setVolume(float v) {
        volume_ = v;
        velocityVol_ = v * velocity_;
    }

    inline void setPan(float pan) {
        panR_ = sin_lut(0.125f + 0.125f * pan);  // pan ∈ [-1..1] → [0.0 .. 0.25]
        panL_ = sin_lut(0.125f - 0.125f * pan);  // pan ∈ [1..-1] → [0.0 .. 0.25]
    }

    void setReverbSend(float rev) {
        reverbSend_ = fclamp(rev, 0.f, 1.f);
    }

    void reset() {
        for (auto& op : ops) op.reset();
        filter.reset();
    }

    void noteOn(float hz, float vel = 1.f) {
        reset();
     //   if (hz > 0) setFrequency(hz); else setFrequency(baseFreq_);
        velocity_ = vel;
        velocityVol_ = vel * volume_;
        veloMult_ = velocity_ * veloMod_;
        env.retrigger(Adsr::END_NOW);
    }

    void noteOff() {
        env.end(Adsr::END_REGULAR);
    }

    float __attribute__((always_inline)) process() {
        float envVal = env.process();
        float raw ;

        switch(algo_) {
            case 0:
                raw = algo0_2c(*this, envVal);
                break;
            case 1:
                raw = algo1_3c(*this, envVal);
                break;
            case 2:
                raw = algo2_1m_1c(*this, envVal);
                break;
            case 3:
                raw = algo3_2m_2c(*this, envVal);
                break;
            case 4:
                raw = algo4_3ms_1c(*this, envVal);
                break;
            case 5:
                raw = algo5_4ms_1c(*this, envVal);
                break;
            case 6:
                raw = algo6_2m_1m_1c(*this, envVal);
                break;
            case 7:
                raw = algo7_3m_1m_2c(*this, envVal);
                break;
            case 8:
                raw = algo8_2m_1m_1c(*this, envVal);
                break;
            case 9:
                raw = algo9_2m_2m_2c(*this, envVal);
                break;
            case 10:
                raw = algo10_2m_3c(*this, envVal);
                break;
            case 11:
                raw = algo11_3m_3c(*this, envVal);
                break;
            case 12:
                raw = algo12_2m_4c(*this, envVal);
                break;
            case 13:
                raw = algo13_1m_5c(*this, envVal);
                break;
            case 14:
                raw = algo14_2m_1amp_1c(*this, envVal);
                break;
            case 15:
                raw = algo15_2m_2amp_2c(*this, envVal);
                break;
            case 16:
                raw = algo16_2m_2amp_1c(*this, envVal);
                break;
            case 17:
                raw = algo17_4m_1amp_1c(*this, envVal);
                break;
            default:
                raw = 0.0f;  
                break;
        }

        if (unlikely(useFilter_)) {
            return velocityVol_ * envVal * filter.processMorph(raw);
        } 
        return velocityVol_ * envVal * raw;
    }
 
    bool isActive() const {
        return env.isRunning();
    }

    void applyPatch(const FmDrumPatch& p) {
        setAhdsr(p.attack, p.hold, p.decay, p.sustain, p.release);
        setAlgorithm(p.algoIndex);
        setFrequency(p.baseFreq);
        setVolume(p.volume);
        setPan(p.pan);
        setReverbSend(p.reverbSend);
        filter.setFreqHz(p.filterFreqHz);
        filter.setResonance(p.filterReso);
        filter.setMorph(p.filterMorph);
        setFilterActive(p.useFilter!=0);
        for (int i = 0; i < 6; ++i) {
            setOperatorParams(i, p.ops[i].ratio, p.ops[i].detune, p.ops[i].feedback, p.ops[i].volume, p.ops[i].waveform);
        }
    }

    FmDrumPatch toPatch(const char* name = "Perc") const {
        FmDrumPatch p;
        strcpy(p.name, name);
        p.volume = volume_;
        p.pan = pan_;
        p.reverbSend = reverbSend_;
        p.attack  = env.getAttackTime();
        p.hold    = env.getHoldTime();
        p.decay   = env.getDecayTime();
        p.sustain = env.getSustainLevel();
        p.release = env.getReleaseTime();
        p.filterFreqHz = filter.getFreqHz();
        p.filterReso = filter.getResonance();
        p.filterMorph = filter.getMorph();
        p.useFilter = useFilter_ ? 1 : 0;
        p.algoIndex = algo_;
        for (int i = 0; i < 6; ++i) {
            p.ops[i].ratio    = ops[i].getRatio();
            p.ops[i].detune   = ops[i].getDetune();
            p.ops[i].feedback = ops[i].getFeedback();
            p.ops[i].volume   = ops[i].getVolume();
            p.ops[i].waveform = ops[i].getWaveform();
        }
        return p;
    }

private:
    float sampleRate_ = 44100.f;
    float baseFreq_ = 60.f;
    float velocity_ = 1.f;
    float veloMod_ = 0.5f;
    float veloMult_ = velocity_ * veloMod_;
    float volume_ = 1.0f;
    float velocityVol_ = 1.0f;
    bool useFilter_ = true;
    uint8_t algo_ = 0;
    float pan_ = 0.f;
    float panL_ = ONE_DIV_SQRT2;
    float panR_ = ONE_DIV_SQRT2;
    float reverbSend_ = 0.1f;

    std::array<FmOperator, NumOps> ops;
    Adsr env;
    SvfFilter filter;

    using AlgoFn = float(*)(FmVoice6&, float);

    // --- algorithm implementations ---
    inline float __attribute__((always_inline)) IRAM_ATTR algo0_2c(FmVoice6& v, float e) {
        // [0]→[out]→
        // [5]↗ 
        return ONE_DIV_SQRT2 * (v.ops[0].outProcess(0.f, e) + v.ops[5].outProcess(0.0f, e));
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo1_3c(FmVoice6& v, float e) {
        // [4]↘
        // [0]→[out]→
        // [5]↗ 
        return ONE_DIV_SQRT3 * (v.ops[0].outProcess(0.f, e) + v.ops[5].outProcess(0.0f, e) + v.ops[4].outProcess(0.0f, e));
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo2_1m_1c(FmVoice6& v, float e) {
        // [5]→[0]→[out]→
        float m = v.ops[5].fmProcess(0.0f, e);
        return v.ops[0].outProcess(m, e);
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo3_2m_2c(FmVoice6& v, float e) {
        // [5]→[0]→[out]→
        // [4]→[3]↗
        float m5 = v.ops[5].fmProcess(0.0f, e);
        float m4 = v.ops[4].fmProcess(0.0f, e);
        return  ONE_DIV_SQRT2 * (v.ops[0].outProcess(m5, e) + v.ops[3].outProcess(m4, e));
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo4_3ms_1c(FmVoice6& v, float e) {
        // [5]→[4]→[3]→[0]→[out]→
        float m1 = v.ops[5].fmProcess(0.f, e);
        float m2 = v.ops[4].fmProcess(m1, e);
        float m3 = v.ops[3].fmProcess(m2, e);
        return v.ops[0].outProcess(m3, e);
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo5_4ms_1c(FmVoice6& v, float e) {
        // [5]→[4]→[3]→[0]→[out]→
        //             [2]↗
        float m5 = v.ops[5].fmProcess(0.f, e);
        float m4 = v.ops[4].fmProcess(m5, e);
        float m3 = v.ops[3].fmProcess(m4, e);
        float m2 = v.ops[2].fmProcess(0.f, e);
        return ONE_DIV_SQRT2 * (v.ops[0].outProcess(m3, e) + v.ops[2].outProcess(m2, e));
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo6_2m_1m_1c(FmVoice6& v, float e) {
        // [5]↘
        // [4]→[0]→[out]→
        // [3]↗
        float m5 = v.ops[5].fmProcess(0.f, e);
        float m4 = v.ops[4].fmProcess(0.f, e);
        float m3 = v.ops[3].fmProcess(0.f, e);
        return v.ops[0].outProcess(m3 + m4 + m5, e);
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo7_3m_1m_2c(FmVoice6& v, float e) {
        // [5]→[4]→[3]→[2]→[out]→
        //         [1]→[0]↗
        float m5 = v.ops[5].fmProcess(0.f, e);
        float m4 = v.ops[4].fmProcess(m5, e);
        float m3 = v.ops[3].fmProcess(m4, e);
        float m1 = v.ops[1].fmProcess(0.f, e);
        return ONE_DIV_SQRT2 * (v.ops[0].outProcess(m1, e) + v.ops[2].outProcess(m3, e));
    }


    inline float __attribute__((always_inline)) IRAM_ATTR algo8_2m_1m_1c(FmVoice6& v, float e) {
        // [5]→[3]→[0]→[out]→
        // [4]↗
        float m5 = v.ops[5].fmProcess(0.f, e);
        float m4 = v.ops[4].fmProcess(0.f, e);
        float m3 = v.ops[3].fmProcess(m5 + m4, e);
        return v.ops[0].outProcess(m3, e);
    }  

    inline float __attribute__((always_inline)) IRAM_ATTR algo9_2m_2m_2c(FmVoice6& v, float e) {
        // [2]→[1]→[0]→[out]→
        // [5]→[4]→[3]↗
        float m1 = v.ops[2].fmProcess(0.f, e);
        float m2 = v.ops[1].fmProcess(m1, e);
        float m3 = v.ops[5].fmProcess(0.f, e);
        float m4 = v.ops[4].fmProcess(m3, e);
        return ONE_DIV_SQRT2 * (v.ops[0].outProcess(m2, e ) + v.ops[3].outProcess(m4, e)) ;
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo10_2m_3c(FmVoice6& v, float e) {
        //     [2]↘
        // [4]→[1]→[out]→
        // [5]→[0]↗
        float m4 = v.ops[4].fmProcess(0.f, e);
        float m5 = v.ops[5].fmProcess(0.f, e);
        return ONE_DIV_SQRT3 * (v.ops[0].outProcess(m5, e) + v.ops[1].outProcess(m4, e) + v.ops[2].outProcess(0.f, e));
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo11_3m_3c(FmVoice6& v, float e) {
        // [1]→[0]↘
        // [3]→[2]→[out]→
        // [5]→[4]↗
        float m1 = v.ops[1].fmProcess(0.f, e);
        float m3 = v.ops[3].fmProcess(0.f, e);
        float m5 = v.ops[5].fmProcess(0.f, e);
        return ONE_DIV_SQRT3 * (v.ops[0].outProcess(m1, e) + v.ops[2].outProcess(m3, e) + v.ops[4].outProcess(m5, e));
    }


    inline float __attribute__((always_inline)) IRAM_ATTR algo12_2m_4c(FmVoice6& v, float e) {
        // [5]→[0]↘
        // [4]→[1]→[out]→
        //     [2]↗
        //     [3]↗
        float m5 = v.ops[1].fmProcess(0.f, e);
        float m4 = v.ops[4].fmProcess(0.f, e);
        return ONE_DIV_SQRT5 * (v.ops[0].outProcess(m5, e) + v.ops[1].outProcess(m4, e) + v.ops[2].outProcess(0.f, e) + v.ops[3].outProcess(0.f, e) );
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo13_1m_5c(FmVoice6& v, float e) {
        // [5]→[0]↘
        //    ↘[1]→[out]→
        //     [2]↗
        //     [3]↗
        //     [4]↗
        float m5 = v.ops[1].fmProcess(0.f, e);
        return ONE_DIV_SQRT5 * (v.ops[0].outProcess(m5, e) + v.ops[1].outProcess(m5, e) + v.ops[2].outProcess(0.f, e) + v.ops[3].outProcess(0.f, e) + v.ops[4].outProcess(0.f, e));
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo14_2m_1amp_1c(FmVoice6& v, float e) {
        // [3(amp)]↘
        //   [5]→[0]→[out]→
        float amp3 = v.ops[3].amProcess(0.f); // positive amplitude 0..1
        float m5 = v.ops[5].fmProcess(0.f, e);
        return  v.ops[0].outProcess(m5, e * amp3);
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo15_2m_2amp_2c(FmVoice6& v, float e) {
        // [3(amp)]↘
        //     [5]→[0]→[out]→
        //     [4]→[1]↗
        // [2(amp)]↗
        float amp3 = v.ops[3].amProcess(0.f) ; 
        float amp2 = v.ops[2].amProcess(0.f) ; 
        float m5 = v.ops[5].fmProcess(0.f, e);
        float m4 = v.ops[4].fmProcess(0.f, e);
        return ONE_DIV_SQRT2 * (v.ops[0].outProcess(m5, e * amp3) + v.ops[1].outProcess(m4, e * amp2));
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo16_2m_2amp_1c(FmVoice6& v, float e) {
        //       [3(amp)]↘
        //         [5]→[0]→[out]→
        //         [4]↗
        // [2(amp)]↗
        float amp3 =  v.ops[3].amProcess(0.f) ; // positive amplitude 0..1
        float amp2 =  v.ops[2].amProcess(0.f) ; // positive amplitude 0..1
        float m5 = v.ops[5].fmProcess(0.f, e);
        float m4 = v.ops[4].fmProcess(0.f, amp2);
        return amp3 * v.ops[0].outProcess(m5 + m4, e)  ;
    }

    inline float __attribute__((always_inline)) IRAM_ATTR algo17_4m_1amp_1c(FmVoice6& v, float e) {
        //       [1]↘
        //   [5]→[4]→[0]→[out]→
        //     [2(amp)]↗
        // [3]↗
        float m3 = v.ops[3].fmProcess(0.f, e);
        float amp2 = v.ops[2].amProcess(m3); // positive amplitude 0..1
        float m5 = v.ops[5].fmProcess(0.f, e);
        float m4 = v.ops[4].fmProcess(m5, e);
        float m1 = v.ops[1].fmProcess(0.f, e);
        return  v.ops[0].outProcess(m1 + m4, e * amp2);
    }

};
