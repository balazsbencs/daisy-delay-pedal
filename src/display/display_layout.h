#pragma once
#include <cstdint>

namespace pedal {
namespace layout {

// Physical screen dimensions (ST7789 240x320 portrait).
constexpr uint16_t SCREEN_W = 240;
constexpr uint16_t SCREEN_H = 320;

// ── Header (y=0..39, h=40) ───────────────────────────────────────────────
constexpr uint16_t HEADER_H = 40;
constexpr uint16_t MODE_X   = 4;
constexpr uint16_t MODE_Y   = 11;  // vertically centred (Font_11x18 is 18 px)
constexpr uint16_t BYPASS_X = 154;
constexpr uint16_t BYPASS_Y = 15;  // Font_7x10
constexpr uint16_t SLOT_X   = 208;
constexpr uint16_t SLOT_Y   = 16;  // Font_6x8

// ── Separator lines ──────────────────────────────────────────────────────
constexpr uint16_t SEP1_Y = 39;   // below header (last header pixel row)
constexpr uint16_t SEP2_Y = 250;  // above status row

// ── Parameter rows (y=40..249, 7 rows × ~30 px) ─────────────────────────
// Row i top edge: PARAM_AREA_Y + i * PARAM_ROW_H
constexpr uint16_t PARAM_AREA_Y  = 40;
constexpr uint16_t PARAM_ROW_H   = 30;  // 7 × 30 = 210 px; last row ends at 250

// Within each row (relative y offset):
constexpr uint16_t LABEL_OFFSET_Y = 10;  // Font_6x8 label (8 px tall), centred in 30 px
constexpr uint16_t BAR_OFFSET_Y   = 10;  // bar top aligned with label top
constexpr uint16_t BAR_H          = 14;  // bar height including 1-pixel border
constexpr uint16_t LABEL_X        = 4;
constexpr uint16_t BAR_X          = 80;
constexpr uint16_t BAR_W          = 160; // bar ends at x=210, balanced right margin

// ── Status row (y=251..280) ──────────────────────────────────────────────
constexpr uint16_t TEMPO_X        = 4;
constexpr uint16_t TEMPO_Y        = 261;
constexpr uint16_t SHIFT_X        = 100;
constexpr uint16_t SHIFT_Y        = 261;
constexpr uint16_t PRESET_EVENT_X = 174;
constexpr uint16_t PRESET_EVENT_Y = 261;

} // namespace layout
} // namespace pedal
