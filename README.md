# ESP32-S3 FM Drum Synth

A high-performance FM drum synthesizer built for the ESP32-S3.  
Supports multi-operator FM synthesis, realtime MIDI control, patch editing via OLED+encoder UI, and flexible drumkit management.

---

## ✨ Features

- ✅ **6-operator FM synthesis** with 17 extendable routing algorithms 
- ✅ **Custom envelope engine** with hold, fast-release, and block-aware processing
- ✅ **Stereo audio output** via I2S, using normalized float-to-int audio pipeline
- ✅ **Per-note drumkits** with full patch mapping to GM MIDI notes (128-note map)
- ✅ **Real-time MIDI control** (USB or serial), including velocity and retriggers
- ✅ **On-device GUI** via SH1106 OLED + rotary encoder + buttons
- ✅ **Waveform blending**: sine, triangle, square, saw, noise, and inverted variants
- ✅ **Nonlinear SVF filter with drive** per voice
- ✅ **Per-voice reverb send**, globally editable reverb parameters
- ✅ **Optimized DSP math**: fast sine, LUTs, `fast_fabsf`, saturators, etc.
- ✅ **Extremely low latency** voice allocator with note stealing and `MAX_VOICES_PER_NOTE` control
- ✅ **Drumkit save/load** via JSON on SD (or LittleFS, configurable)
- ✅ **Modular architecture**: patching, voice engine, allocator, GUI, and storage are decoupled

---

## 🧠 Architecture Overview

```text


Each FmVoice6:
    - 6x FmOperator
    - Flexible routing (17 algorithms)
    - Filter + envelope
    - Reverb send

Each FmOperator:
    - Multiple waveforms
    - Separated fmProcess(), amProcess(), outProcess()
    - Internal feedback
```

a demo recording [Demo MP3](https://github.com/copych/ESP32-S3_FM_Drum_Synth/raw/refs/heads/main/media/05-250801_1607.mp3)
