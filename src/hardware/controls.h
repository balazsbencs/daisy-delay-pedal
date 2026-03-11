#pragma once
#include "daisy_seed.h"
#include "../params/param_set.h"
#include "../config/delay_mode_id.h"
#include "../config/constants.h"

namespace pedal {

struct ControlState {
    float pot_normalized[NUM_POTS]; // smoothed 0..1
    int   encoder_increment;        // -1, 0, +1 since last poll
    bool  encoder_pressed;
    bool  bypass_pressed;           // rising edge
    bool  tap_pressed;              // rising edge
    bool  bypass_held;              // currently held
};

class Controls {
public:
    void Init(daisy::DaisySeed& hw);
    // Call each main loop iteration
    void Poll();
    const ControlState& state() const { return state_; }

private:
    daisy::Encoder    encoder_;
    daisy::Switch     sw_bypass_;
    daisy::Switch     sw_tap_;
    daisy::DaisySeed* hw_ = nullptr;
    ControlState      state_{};
    float             smoothed_[NUM_POTS]{};
};

} // namespace pedal
