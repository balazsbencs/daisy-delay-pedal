#include "swell_delay.h"
#include "../dsp/delay_line_sdram.h"
#include "../config/constants.h"

namespace pedal {

static float DSY_SDRAM_BSS swell_buf[MAX_DELAY_SAMPLES];
static DelayLineSdram       swell_line;

void SwellDelay::Init() {
    swell_line.Init(swell_buf, MAX_DELAY_SAMPLES);
    follower_.Init(5.0f, 80.0f);
    dc_.Init();
    state_                = SwellState::Idle;
    env_gain_             = 0.0f;
    prev_above_threshold_ = false;
}

void SwellDelay::Reset() {
    swell_line.Reset();
    dc_.Init();
    state_                = SwellState::Idle;
    env_gain_             = 0.0f;
    prev_above_threshold_ = false;
}

StereoFrame SwellDelay::Process(float input, const ParamSet& params) {
    const float delay_samps = params.time * SAMPLE_RATE;
    swell_line.SetDelay(delay_samps);

    // mod_spd controls attack rate (samples to full), mod_dep controls decay rate
    // Higher mod_spd = faster attack; use it as a per-sample increment coefficient
    const float attack_rate = params.mod_spd * INV_SAMPLE_RATE;  // fraction/sample
    const float decay_rate  = params.mod_dep * INV_SAMPLE_RATE;

    // Detect rising edge: envelope crosses threshold upward
    const float level       = follower_.Process(input);
    const bool  now_above   = level > TRIGGER_THRESHOLD;
    const bool  rising_edge = now_above && !prev_above_threshold_;
    prev_above_threshold_   = now_above;

    if (rising_edge) {
        state_ = SwellState::Attack;
    }

    // Advance AD state machine
    switch (state_) {
        case SwellState::Idle:
            // env_gain_ stays at 0; decay down if somehow above
            if (env_gain_ > 0.0f) {
                env_gain_ -= decay_rate;
                if (env_gain_ < 0.0f) env_gain_ = 0.0f;
            }
            break;

        case SwellState::Attack:
            env_gain_ += attack_rate;
            if (env_gain_ >= 1.0f) {
                env_gain_ = 1.0f;
                state_    = SwellState::Decay;
            }
            break;

        case SwellState::Decay:
            env_gain_ -= decay_rate;
            if (env_gain_ <= 0.0f) {
                env_gain_ = 0.0f;
                state_    = SwellState::Idle;
            }
            break;
    }

    float wet = swell_line.Read() * env_gain_;

    const float feedback = wet * params.repeats;
    swell_line.Write(input + feedback);

    wet = dc_.Process(wet);

    return StereoFrame{wet, wet};
}

} // namespace pedal
