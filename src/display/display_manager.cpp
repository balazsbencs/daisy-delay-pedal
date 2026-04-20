#include "display_manager.h"
#include "display_renderer.h"
#include "display_colors.h"
#include "display_layout.h"

namespace pedal {

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void DisplayManager::Init() {
    driver_.Init();
    DisplayRenderer::Clear(kColorBlack);
    driver_.FillScreen(kColorBlack);  // push black frame synchronously at boot
}

// ---------------------------------------------------------------------------
// Update (rate-limited entry point)
// ---------------------------------------------------------------------------

void DisplayManager::Update(DelayModeId      mode,
                             const ParamSet&  params,
                             const Bypass&    bypass,
                             const TempoSync& tempo,
                             int              preset_slot,
                             bool             shift_layer_active,
                             PresetUiEvent    preset_event,
                             uint32_t         now_ms) {
    (void)now_ms;  // rate limiting removed — DMA itself is the natural throttle
    if (driver_.IsBusy()) {
        return;
    }
    Render(mode, params, bypass, tempo, preset_slot, shift_layer_active, preset_event);
    driver_.StartDmaTransfer(DisplayRenderer::FrameBuffer(),
                             DisplayRenderer::FrameBufferBytes(),
                             nullptr, nullptr);
}

// ---------------------------------------------------------------------------
// Render — writes the full 240×320 frame into the SDRAM buffer
// ---------------------------------------------------------------------------

void DisplayManager::Render(DelayModeId      mode,
                             const ParamSet&  params,
                             const Bypass&    bypass,
                             const TempoSync& tempo,
                             int              preset_slot,
                             bool             shift_layer_active,
                             PresetUiEvent    preset_event) {
    const uint16_t accent = kModeColors[static_cast<int>(mode)];

    DisplayRenderer::Clear(kColorBlack);

    // --- Header: inverted when shift is active ---
    const uint16_t hdr_bg   = shift_layer_active ? accent      : kColorBlack;
    const uint16_t hdr_fg   = shift_layer_active ? kColorBlack : accent;
    const uint16_t hdr_aux  = shift_layer_active ? kColorBlack : kColorWhite;
    if (shift_layer_active) {
        DisplayRenderer::FillRect(0, 0, layout::SCREEN_W, layout::HEADER_H, accent);
    }

    DisplayRenderer::DrawText(layout::MODE_X, layout::MODE_Y,
                              ModeName(mode), hdr_fg, hdr_bg, Font_11x18);

    DisplayRenderer::DrawText(layout::BYPASS_X, layout::BYPASS_Y,
                              bypass.IsActive() ? "ON" : "BY",
                              hdr_aux, hdr_bg, Font_7x10);

    char slot_label[3] = {'P', static_cast<char>('1' + (preset_slot % 8)), 0};
    DisplayRenderer::DrawText(layout::SLOT_X, layout::SLOT_Y,
                              slot_label, hdr_aux, hdr_bg, Font_6x8);

    // --- Separators ---
    DisplayRenderer::HLine(0, layout::SEP1_Y, layout::SCREEN_W, kColorWhite);
    DisplayRenderer::HLine(0, layout::SEP2_Y, layout::SCREEN_W, kColorWhite);

    // --- 7 parameter rows ---
    static const char* kLabels[7] = {
        "TIME", "REPEATS", "MIX", "FILTER", "GRIT", "MOD SPEED", "MOD DEPTH"
    };
    const float kBarVals[7] = {
        params.time    / 2.5f,
        params.repeats / 0.98f,
        params.mix,
        params.filter,
        params.grit,
        params.mod_spd / 10.0f,
        params.mod_dep,
    };

    for (int i = 0; i < 7; ++i) {
        const uint16_t row_y = static_cast<uint16_t>(
            layout::PARAM_AREA_Y + static_cast<uint16_t>(i) * layout::PARAM_ROW_H);

        DisplayRenderer::DrawText(layout::LABEL_X,
                                  row_y + layout::LABEL_OFFSET_Y,
                                  kLabels[i], kColorWhite, kColorBlack, Font_6x8);

        DisplayRenderer::DrawBar(layout::BAR_X,
                                 row_y + layout::BAR_OFFSET_Y,
                                 layout::BAR_W,
                                 layout::BAR_H,
                                 kBarVals[i], accent);
    }

    // --- Status row: tempo source ---
    if (tempo.HasMidiClock()) {
        DisplayRenderer::DrawText(layout::TEMPO_X, layout::TEMPO_Y,
                                  "MIDI", kColorWhite, kColorBlack, Font_6x8);
    } else if (tempo.HasTap()) {
        DisplayRenderer::DrawText(layout::TEMPO_X, layout::TEMPO_Y,
                                  "TAP", kColorWhite, kColorBlack, Font_6x8);
    }

    // --- Status row: preset event ---
    if (preset_event == PresetUiEvent::Loaded) {
        DisplayRenderer::DrawText(layout::PRESET_EVENT_X, layout::PRESET_EVENT_Y,
                                  "LOAD", kColorWhite, kColorBlack, Font_6x8);
    } else if (preset_event == PresetUiEvent::Saved) {
        DisplayRenderer::DrawText(layout::PRESET_EVENT_X, layout::PRESET_EVENT_Y,
                                  "SAVE", kColorWhite, kColorBlack, Font_6x8);
    }
}

// ---------------------------------------------------------------------------
// ModeName — full names now that we have 240 px width
// ---------------------------------------------------------------------------

const char* DisplayManager::ModeName(DelayModeId id) {
    switch (id) {
        case DelayModeId::Duck:    return "Duck";
        case DelayModeId::Swell:   return "Swell";
        case DelayModeId::Trem:    return "Trem";
        case DelayModeId::Digital: return "Digital";
        case DelayModeId::Dbucket: return "BBucket";
        case DelayModeId::Tape:    return "Tape";
        case DelayModeId::Dual:    return "Dual";
        case DelayModeId::Pattern: return "Pattern";
        case DelayModeId::Filter:  return "Filter";
        case DelayModeId::Lofi:    return "LoFi";
        default:                   return "?";
    }
}

} // namespace pedal
