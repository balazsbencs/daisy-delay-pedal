#pragma once
#include "../config/constants.h"
#include <cmath>

namespace pedal {

enum class LfoWave { Sine, Triangle };

class Lfo {
public:
    void Init(float rate_hz = 1.0f, LfoWave wave = LfoWave::Sine);
    void SetRate(float rate_hz);
    void SetWave(LfoWave wave) { wave_ = wave; }
    // Returns -1..+1
    float Process();

private:
    float    phase_     = 0.0f;
    float    phase_inc_ = 0.0f;
    LfoWave  wave_      = LfoWave::Sine;
};

} // namespace pedal
