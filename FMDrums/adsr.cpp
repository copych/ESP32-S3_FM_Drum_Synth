/*
* AHDSR Envelope Generator
*/
#include "adsr.h"
#include <math.h>

static const char* TAG = "ADSR";

void Adsr::init(float sample_rate, int blockSize) {
    sample_rate_  = sample_rate / blockSize;
    attackShape_  = -1.f;
    attackTarget_ = 0.0f;
    attackTime_   = -1.f;
    decayTime_    = -1.f;
    releaseTime_  = -1.f;
    fastReleaseTime_  = -1.f;
    semiFastReleaseTime_  = -1.f;
    sus_level_    = 1.0f;
    x_            = 0.0f;
    gate_         = false;
    mode_         = ADSR_SEG_IDLE;

    setTime(ADSR_SEG_ATTACK, 0.0f);
    setTime(ADSR_SEG_HOLD, 0.05f);
    setTime(ADSR_SEG_DECAY, 0.05f);
    setTime(ADSR_SEG_RELEASE, 0.05f);
    setTime(ADSR_SEG_FAST_RELEASE, 0.0005f); // a few samples fade, trying to avoid clicks on polyphony overrun
    setTime(ADSR_SEG_SEMI_FAST_RELEASE, 0.02f); // for exclusive note groups voice stealing
}


void Adsr::retrigger(eEnd_t hardness) {
  gate_ = true;
  mode_ = ADSR_SEG_ATTACK;
  switch (hardness) {
    case END_NOW:
      x_ = 0.0f;
      D0_ = attackD0_;
      break;
    case END_FAST:
    case END_SEMI_FAST:
    case END_REGULAR:
    default:
      D0_ = attackD0_;
  }
}


void Adsr::end(eEnd_t hardness) {
  gate_ = false;
  target_ = -0.1f;
  switch (hardness) {
    case END_NOW:{
      mode_ = ADSR_SEG_IDLE;
      D0_ = attackD0_;
      x_ = 0.f;
      break;
    }
    case END_FAST:{
      mode_ = ADSR_SEG_FAST_RELEASE;
      D0_ = fastReleaseD0_;
      break;
    }
    case END_SEMI_FAST:{
      mode_ = ADSR_SEG_SEMI_FAST_RELEASE;
      D0_ = semiFastReleaseD0_;
      break;
    }
    case END_REGULAR:
    default:{
      mode_ = ADSR_SEG_RELEASE;
      D0_ = releaseD0_;
    }
  }
}


Adsr::eSegment_t Adsr::getCurrentSegment() {
  Adsr::eSegment_t ret = mode_;
  if (gate_ && (x_ == sus_level_)) {
    ret = ADSR_SEG_SUSTAIN;
  }
  return ret;
}

const char* Adsr::getCurrentSegmentStr() {
  Adsr::eSegment_t ret = mode_;
  if (gate_ && (x_ == sus_level_)) {
    ret = ADSR_SEG_SUSTAIN;
  }
  switch (ret) {
    case ADSR_SEG_ATTACK:
      return "ATTACK";
    case ADSR_SEG_HOLD:
      return "HOLD";
    case ADSR_SEG_DECAY:
      return "DECAY";
    case ADSR_SEG_SUSTAIN:
      return "SUSTAIN";
    case ADSR_SEG_RELEASE:
      return "RELEASE";
    case ADSR_SEG_FAST_RELEASE:
      return "FAST_RELEASE";
    case ADSR_SEG_SEMI_FAST_RELEASE:
      return "SEMI_FAST_RELEASE";
    case ADSR_SEG_IDLE:
      return "IDLE";
    default:
      return "UNKNOWN";
  }
}

void Adsr::setTime(int seg, float time) {
  switch (seg) {
    case ADSR_SEG_ATTACK:
      {
        setAttackTime(time, 0.0f);  
        break;
      }
    case ADSR_SEG_HOLD:
      {
        setHoldTime(time);
        break;
      }
    case ADSR_SEG_DECAY:
      {
        setTimeConstant(time, decayTime_, decayD0_);
      }
      break;
    case ADSR_SEG_RELEASE:
      {
        setTimeConstant(time, releaseTime_, releaseD0_);
      }
      break;
    case ADSR_SEG_SEMI_FAST_RELEASE:
      {
        setTimeConstant(time, semiFastReleaseTime_, semiFastReleaseD0_);
      }
      break;
    case ADSR_SEG_FAST_RELEASE:
      {
        setTimeConstant(time, fastReleaseTime_, fastReleaseD0_);
      }
      break;

    default: return;
  }
}


void Adsr::setAttackTime(float timeInS, float shape) {
  if ((timeInS != attackTime_) || (shape != attackShape_)) {
    attackTime_ = timeInS;
    attackShape_ = shape;
    float x = shape;
    float target = 9.f * powf(x, 10.f) + 0.3f * x + 1.01f;
    attackTarget_ = target;
    float logTarget = logf(1.f - (1.f / target));  // -1 for decay
    if (timeInS > 0.f) {
      attackD0_ = 1.f - expf(logTarget / (timeInS * sample_rate_));
    } else
      attackD0_ = 1.f;  // instant change
  }
}

void Adsr::setHoldTime(float timeInS) {
  holdTime_ = timeInS;
  if (holdTime_ > 0.0f) {
      holdSamples_ = (uint32_t)(holdTime_ * sample_rate_);
      holdCounter_ = holdSamples_;
  } else {
      holdSamples_ = 0;
      holdCounter_ = 0;
  }
}

void Adsr::setDecayTime(float timeInS) {
  setTimeConstant(timeInS, decayTime_, decayD0_);
}

void Adsr::setReleaseTime(float timeInS) {
  setTimeConstant(timeInS, releaseTime_, releaseD0_);
}

void Adsr::setFastReleaseTime(float timeInS) {
  setTimeConstant(timeInS, fastReleaseTime_, fastReleaseD0_);
}

void Adsr::setSemiFastReleaseTime(float timeInS) {
  setTimeConstant(timeInS, semiFastReleaseTime_, semiFastReleaseD0_);
}

void Adsr::setTimeConstant(float timeInS, float& time, float& coeff) {
  if (timeInS != time) {
    time = timeInS;
    if (time > 0.f) {
      coeff = 1.f - expf(-1.0f / (0.2f * time * sample_rate_));
    } else
      coeff = 1.f;  // instant change
  }
}


float __attribute__((hot, always_inline)) IRAM_ATTR Adsr::process() {
  float out = 0.0f;
  switch (mode_) {
    case ADSR_SEG_IDLE:
      out = 0.0f;
      break;
    case ADSR_SEG_ATTACK:
      x_ += (float)D0_ * ((float)attackTarget_ - (float)x_);
      out = x_;
      if (out >= 1.f) {
        x_ = out = 1.f;
        if (holdSamples_ > 0) {
            mode_ = ADSR_SEG_HOLD;
            holdCounter_ = holdSamples_;
        } else {
            mode_ = ADSR_SEG_DECAY;
            target_ = sus_level_;
            D0_ = decayD0_;
        }
      }
      break;
    case ADSR_SEG_HOLD:
      out = x_;
      if (holdCounter_ > 0) {
          holdCounter_--;
      } else {
          mode_ = ADSR_SEG_DECAY;
          target_ = sus_level_ - (x_ - sus_level_) * 0.1f;
          D0_ = decayD0_;
      }
      break;
    case ADSR_SEG_DECAY:
    case ADSR_SEG_RELEASE:
    case ADSR_SEG_FAST_RELEASE:
    case ADSR_SEG_SEMI_FAST_RELEASE:
      x_ += (float)D0_ * ((float)target_ - (float)x_);
      out = x_;
      if (out < 0.0f) {
        mode_ = ADSR_SEG_IDLE;
        x_ = out = 0.f;
        target_ = -0.1f;
        D0_ = attackD0_;
      }
      break;
    default: 
      break;
  }

  return out;
}
