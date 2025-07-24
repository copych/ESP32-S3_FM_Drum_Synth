#pragma once

#include <functional>
#include <esp_timer.h>
#include <driver/pcnt.h>

class IRAM_ATTR MuxEncoder {
public:
    enum encMode {
        MODE_HALF_STEP,
        MODE_FULL_STEP,
        MODE_DOUBLE_STEP,
        MODE_QUAD_STEP
    };

    static constexpr int8_t stepIncrement[2][16] = {
        // MODE_HALF_STEP
        {0, 1, -1, 0,  -1, 0, 0, 1,  1, 0, 0, -1,  0, -1, 1, 0},
        // MODE_FULL_STEP
        {0, 0,  0, 0,  -1, 0, 0, 1,  1, 0, 0, -1,  0,  0, 0, 0}
    };

    void bind(uint8_t id, uint8_t* a, uint8_t* b, std::function<void(int, int)> cb,
              encMode mode = MODE_FULL_STEP,
              gpio_num_t pcnt_gpio = GPIO_NUM_NC,
              pcnt_unit_t pcnt_unit = PCNT_UNIT_0) {
        _id = id;
        _a = a;
        _b = b;
        _callbackFunc = cb;
        _mode = mode;

        if (pcnt_gpio >= 0) {
            _usePcnt = true;
            _pcntUnit = pcnt_unit;

            pcnt_config_t pcnt_config = {
                .pulse_gpio_num = pcnt_gpio,
                .ctrl_gpio_num = PCNT_PIN_NOT_USED,
                .lctrl_mode = PCNT_MODE_KEEP,
                .hctrl_mode = PCNT_MODE_KEEP,
                .pos_mode = PCNT_COUNT_INC,
                .neg_mode = PCNT_COUNT_INC,
                .counter_h_lim = 10000,
                .counter_l_lim = -10000,
                .unit = pcnt_unit,
                .channel = PCNT_CHANNEL_0
            };

            pcnt_unit_config(&pcnt_config);
            pcnt_set_filter_value(pcnt_unit, 100); 
            pcnt_filter_enable(pcnt_unit);
            pcnt_counter_pause(pcnt_unit);
            pcnt_counter_clear(pcnt_unit);
            pcnt_counter_resume(pcnt_unit);
        }

        for (int i = 0; i < HISTORY_SIZE; ++i) {
            speedHistory[i].timeUs = 0;
            speedHistory[i].count = 0;
        }
    }

    void process() {
        if (!_a || !_b || !_callbackFunc) return;

        unsigned int clk = (*_a == LOW);
        unsigned int dt = (*_b == LOW);

        newState = (clk | (dt << 1));
        if (newState != oldState) {
            int stateMux = newState | (oldState << 2);
            oldState = newState;

            int delta = 0;
            if (_mode == MODE_HALF_STEP || _mode == MODE_FULL_STEP) {
                delta = stepIncrement[_mode][stateMux];
            } else {
                delta = stepIncrement[MODE_HALF_STEP][stateMux];
                _accumulator += delta;

                if (newState == 0) {
                    int needed = (_mode == MODE_DOUBLE_STEP) ? 2 : 4;
                    if (abs(_accumulator) >= needed) {
                        delta = (_accumulator > 0) ? 1 : -1;
                        _accumulator = 0;
                    } else {
                        _accumulator = 0;
                        delta = 0;
                    }
                } else {
                    delta = 0;
                }
            }

            if (delta != 0) {
                updateSpeedHistory();
                _callbackFunc(_id, delta);
            }
        }
    }

    bool isHiSpeed() const {
        int total = 0;
        int64_t now = esp_timer_get_time();
        for (int i = 0; i < HISTORY_SIZE; ++i) {
            if ((now - speedHistory[i].timeUs) < SPEED_WINDOW_US)
                total += abs(speedHistory[i].count);
        }
        return total >= SPEED_THRESHOLD;
    }

private:
    struct SpeedSample {
        int64_t timeUs;
        int count;
    };

    void updateSpeedHistory() {
        int16_t pcntVal = 0;
        int delta = 1;

        if (_usePcnt) {
            pcnt_get_counter_value(_pcntUnit, &pcntVal);
            delta = pcntVal - _pcntLastValue;
            _pcntLastValue = pcntVal;
        }

        speedHistory[historyIndex] = { esp_timer_get_time(), delta };
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    }

    uint8_t _id;
    uint8_t* _a = nullptr;
    uint8_t* _b = nullptr;
    encMode _mode = MODE_FULL_STEP;
    std::function<void(int, int)> _callbackFunc;

    int oldState = 0;
    int newState = 0;
    int _accumulator = 0;

    // PCNT support
    bool _usePcnt = false;
    pcnt_unit_t _pcntUnit = PCNT_UNIT_0;
    int16_t _pcntLastValue = 0;

    // Speed detection
    static constexpr int HISTORY_SIZE = 16;
    static constexpr int SPEED_THRESHOLD = 5;
    static constexpr int64_t SPEED_WINDOW_US = 50000;  // 50ms window

    SpeedSample speedHistory[HISTORY_SIZE];
    int historyIndex = 0;
};
