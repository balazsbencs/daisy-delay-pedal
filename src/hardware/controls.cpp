#include "controls.h"
#include "../config/pin_map.h"

namespace pedal {

void Controls::Init(daisy::DaisySeed& hw) {
    hw_ = &hw;

    // ADC
    daisy::AdcChannelConfig adc_cfg[NUM_POTS];
    adc_cfg[0].InitSingle(daisy::seed::A0);
    adc_cfg[1].InitSingle(daisy::seed::A1);
    adc_cfg[2].InitSingle(daisy::seed::A2);
    adc_cfg[3].InitSingle(daisy::seed::A3);
    adc_cfg[4].InitSingle(daisy::seed::A4);
    adc_cfg[5].InitSingle(daisy::seed::A5);
    adc_cfg[6].InitSingle(daisy::seed::A6);
    hw.adc.Init(adc_cfg, NUM_POTS);
    hw.adc.Start();

    encoder_.Init(pins::ENC_A, pins::ENC_B, pins::ENC_SW);
    sw_bypass_.Init(pins::SW_BYPASS);
    sw_tap_.Init(pins::SW_TAP);

    for (int i = 0; i < NUM_POTS; ++i) {
        smoothed_[i] = hw_->adc.GetFloat(i);
    }
}

void Controls::Poll() {
    // Debounce switches
    encoder_.Debounce();
    sw_bypass_.Debounce();
    sw_tap_.Debounce();

    // Smooth pots with one-pole LP
    for (int i = 0; i < NUM_POTS; ++i) {
        float raw = hw_->adc.GetFloat(i);
        // Invert if pots read backwards (common with hardware wiring)
        smoothed_[i] += POT_SMOOTH * (raw - smoothed_[i]);
        state_.pot_normalized[i] = smoothed_[i];
    }

    state_.encoder_increment = encoder_.Increment();
    state_.encoder_pressed   = encoder_.FallingEdge();
    state_.bypass_pressed    = sw_bypass_.RisingEdge();
    state_.tap_pressed       = sw_tap_.RisingEdge();
    state_.bypass_held       = sw_bypass_.Pressed();
}

} // namespace pedal
