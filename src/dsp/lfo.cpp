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

float Lfo::Process() {
    float out = 0.0f;
    switch (wave_) {
        case LfoWave::Sine:
            out = sinf(phase_);
            break;
        case LfoWave::Triangle:
            out = (phase_ < 3.14159265f)
                ? (-1.0f + phase_ * (2.0f / 3.14159265f))
                : ( 3.0f - phase_ * (2.0f / 3.14159265f));
            break;
    }
    phase_ += phase_inc_;
    if (phase_ >= TWO_PI) phase_ -= TWO_PI;
    return out;
}

} // namespace pedal
