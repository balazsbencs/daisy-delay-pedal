#pragma once
#include "daisy_seed.h"

namespace pedal {
namespace pins {
// ADC pots (index = ADC channel)
constexpr int POT_TIME    = 0;
constexpr int POT_REPEATS = 1;
constexpr int POT_MIX     = 2;
constexpr int POT_FILTER  = 3;
constexpr int POT_GRIT    = 4;
constexpr int POT_MOD_SPD = 5;
constexpr int POT_MOD_DEP = 6;
// GPIO
constexpr daisy::Pin ENC_A      = daisy::seed::D0;
constexpr daisy::Pin ENC_B      = daisy::seed::D1;
constexpr daisy::Pin ENC_SW     = daisy::seed::D2;
constexpr daisy::Pin SW_BYPASS  = daisy::seed::D3;
constexpr daisy::Pin SW_TAP     = daisy::seed::D4;
constexpr daisy::Pin RELAY      = daisy::seed::D5;
constexpr daisy::Pin LED_BYPASS = daisy::seed::D6;
} // namespace pins
} // namespace pedal
