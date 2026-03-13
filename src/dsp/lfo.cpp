#include "lfo.h"

namespace pedal {

static constexpr float TWO_PI = 6.28318530717958647692f;

void Lfo::Init(float rate_hz, LfoWave wave) {
    phase_ = 0.0f;
    wave_  = wave;
    SetRate(rate_hz);
}

void Lfo::SetRate(float rate_hz) {
    phase_inc_ = TWO_PI * rate_hz * INV_SAMPLE_RATE;
}

static float lfo_compute(float phase, LfoWave wave) {
    switch (wave) {
        case LfoWave::Sine:
            return sinf(phase);
        case LfoWave::Triangle:
            return (phase < 3.14159265f)
                ? (-1.0f + phase * (2.0f / 3.14159265f))
                : ( 3.0f - phase * (2.0f / 3.14159265f));
    }
    return 0.0f;
}

float Lfo::Process() {
    const float out = lfo_compute(phase_, wave_);
    phase_ += phase_inc_;
    if (phase_ >= TWO_PI) phase_ -= TWO_PI;
    return out;
}

float Lfo::PrepareBlock() {
    const float out = lfo_compute(phase_, wave_);
    phase_ += phase_inc_ * static_cast<float>(BLOCK_SIZE);
    if (phase_ >= TWO_PI) phase_ -= TWO_PI;
    return out;
}

} // namespace pedal
