#include "tone_filter.h"

namespace pedal {

void ToneFilter::Init() {
    lp_state_ = 0.0f;
    hp_state_ = 0.0f;
    SetKnob(0.5f);
}

void ToneFilter::SetKnob(float knob) {
    // LP cutoff: knob 0..0.5 maps 200Hz..8000Hz
    // HP cutoff: knob 0.5..1.0 maps 200Hz..3000Hz
    // Mix: 0 = full LP, 0.5 = flat (LP+HP cancel), 1 = full HP
    float lp_cutoff, hp_cutoff;
    if (knob <= 0.5f) {
        float t = knob * 2.0f; // 0..1
        lp_cutoff = 200.0f + t * 7800.0f; // 200..8000 Hz
        hp_cutoff = 20.0f;
        lp_mix_ = 1.0f;
        hp_mix_ = 0.0f;
    } else {
        float t = (knob - 0.5f) * 2.0f; // 0..1
        lp_cutoff = 20000.0f;
        hp_cutoff = 20.0f + t * t * 2980.0f; // 20..3000 Hz
        lp_mix_ = 1.0f - t;
        hp_mix_ = t;
    }
    lp_coef_ = 1.0f - expf(-2.0f * 3.14159265f * lp_cutoff * INV_SAMPLE_RATE);
    hp_coef_ = 1.0f - expf(-2.0f * 3.14159265f * hp_cutoff * INV_SAMPLE_RATE);
}

float ToneFilter::Process(float sample) {
    lp_state_ += lp_coef_ * (sample    - lp_state_);
    hp_state_ += hp_coef_ * (lp_state_ - hp_state_);
    float hp_out = lp_state_ - hp_state_;
    return lp_mix_ * lp_state_ + hp_mix_ * hp_out;
}

} // namespace pedal
