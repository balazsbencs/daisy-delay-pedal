#pragma once
#include <cmath>

namespace pedal {

class Saturation {
public:
    void Init() { drive_ = 1.0f; }
    void SetDrive(float drive) { drive_ = 1.0f + drive * 15.0f; } // 1..16
    float Process(float sample) const;

private:
    float drive_ = 1.0f;
};

inline float Saturation::Process(float sample) const {
    float driven = sample * drive_;
    // Soft clip via cubic approximation: (3x - x^3) / 2, clamped
    if (driven > 1.0f)  driven = 1.0f;
    if (driven < -1.0f) driven = -1.0f;
    return driven * (3.0f - driven * driven) * 0.5f;
}

} // namespace pedal
