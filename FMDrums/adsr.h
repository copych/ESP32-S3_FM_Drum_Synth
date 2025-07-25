#pragma once
#include <Arduino.h>

/** Distinct stages that the phase of the envelope can be located in.
- IDLE   = located at phase location 0, and not currently running
- HOLD    =     Third segment of envelope where phase remains constant 
- ATTACK  = First segment of envelope where phase moves from 0 to 1
- DECAY   = Second segment of envelope where phase moves from 1 to SUSTAIN value
- RELEASE =     Fourth segment of envelop where phase moves from SUSTAIN to 0
*/

// note that ADSR_SEG_SUSTAIN value is not used in this implementation and the GetCurrentSegment() will return ADSR_SEG_DECAY eventhough ADSR_SEG_SUSTAIN would be correct
// if you need it

/** adsr envelope module
Original author(s) : Paul Batchelor
Ported from Soundpipe by Ben Sergentanis, May 2020
Remake by Steffan DIedrichsen, May 2021
Modified by Copych, Jan 2024-May 2025
Added fast and semi-fast releases
Added HOLD phase
*/


class IRAM_ATTR Adsr
{
  public:
enum eSegment_t { ADSR_SEG_IDLE, ADSR_SEG_ATTACK, ADSR_SEG_HOLD, ADSR_SEG_DECAY, ADSR_SEG_SUSTAIN, ADSR_SEG_RELEASE, ADSR_SEG_SEMI_FAST_RELEASE, ADSR_SEG_FAST_RELEASE };
enum eEnd_t { END_REGULAR, END_SEMI_FAST, END_FAST, END_NOW };
    Adsr() {}
    ~Adsr() {}

    /** Initializes the Adsr module.
        \param sample_rate - The sample rate of the audio engine being run. 
    */
    void init(float sample_rate, int blockSize = 1);
	
    /**
     \function Retrigger forces the envelope back to attack phase
        \param hard  resets the history to zero, results in a click.
     */
    void retrigger(eEnd_t hardness);
	
    /**
     \function End forces the envelope to idle phase
        \param hard resets the history to zero, results in a click.
     */
    void end(eEnd_t hardness);
	
    /** Processes one sample through the filter and returns one sample.
    */
    float process();

	
    /** Sets time
        Set time per segment in seconds
    */
    void setTime(int seg, float time);
    void setAttackTime(float timeInS, float shape = 0.0f);
    void setHoldTime(float timeInS);
    void setDecayTime(float timeInS);
    void setReleaseTime(float timeInS);
    void setFastReleaseTime(float timeInS);
    void setSemiFastReleaseTime(float timeInS);

  private:
    void setTimeConstant(float timeInS, float& time, float& coeff);

  public:
    /** Sustain level
        \param sus_level - sets sustain level, 0...1.0
    */
    inline void setSustainLevel(float sus_level)
    {
        sus_level = (sus_level <= 0.f) ? -0.001f // forces envelope into idle
                                       : (sus_level > 1.f) ? 1.f : sus_level;
        sus_level_ = sus_level;
    }
    /** get the current envelope segment
        \return the segment of the envelope that the phase is currently located in.
    */
    eSegment_t getCurrentSegment() ;
    /** Tells whether envelope is active
        \return true if the envelope is currently in any stage apart from idle.
    */
    const char* getCurrentSegmentStr() ;
    /** Tells whether envelope is active
        \return true if the envelope is currently in any stage apart from idle.
    */
    inline bool isRunning() const { return mode_ != ADSR_SEG_IDLE; }
    /** Tells whether envelope is active
        \return true if the envelope is currently in any stage apart from idle.
    */
    inline bool isIdle() const { return mode_ == ADSR_SEG_IDLE; }

    inline float getVal() const { return x_; }
    inline float getTarget() const { return target_; }

    inline float getAttackTime() const { return attackTime_; }
    inline float getHoldTime() const { return holdTime_; }
    inline float getDecayTime() const { return decayTime_; }
    inline float getSustainLevel() const { return sus_level_; }
    inline float getReleaseTime() const { return releaseTime_; }
    inline float getFastReleaseTime() const { return fastReleaseTime_; }

  private:
    float   sus_level_{0.f};
    volatile float   x_{0.f};
    volatile float   target_{0.f};
    volatile float 	D0_{0.f};
    float   attackShape_{-1.f};
    float   attackTarget_{0.0f};
    float   attackTime_{-1.0f};
    float   decayTime_{-1.0f};
    float   releaseTime_{-1.0f};
    float   fastReleaseTime_{-1.0f};
    float   semiFastReleaseTime_{-1.0f};
    float   attackD0_{0.f};
    float   decayD0_{0.f};
    float   releaseD0_{0.f};
    float   fastReleaseD0_{0.f};
    float   semiFastReleaseD0_{0.f};
    float   holdTime_ = 0.0f;   // seconds
    uint32_t holdSamples_ = 0; // how many samples to hold
    uint32_t holdCounter_ = 0; // current counter
    int     sample_rate_;
    volatile eSegment_t mode_{ADSR_SEG_IDLE};
    bool    gate_{false};
};
