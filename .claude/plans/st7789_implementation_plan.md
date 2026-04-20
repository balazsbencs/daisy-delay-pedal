# ST7789 Display Implementation Plan

Replace the 128×64 monochrome SSD1306 OLED (I2C) with a 240×320 ST7789 IPS color
display (SPI). The audio engine, all DSP modes, parameters, MIDI, and preset logic
are untouched.

---

## Constraints

| Resource | Current | Budget |
|----------|---------|--------|
| FLASH | 121 / 128 KB (92.88%) | ~7 KB free — critical |
| SDRAM | 6.3 / 64 MB | 57 MB free — comfortable |
| CPU (main loop) | 1 ms tick | 33 ms display budget at 30 fps |
| QSPI flash | presets only | 7+ MB available for assets |

FLASH is the primary constraint. Every phase must be measured before proceeding.

---

## Color Strategy

Two compile-time modes, selected by a single `#define`:

### Mode A — Monochrome (default, ship first)
- Frame buffer: `uint16_t` RGB565, but only `0xFFFF` (white) and `0x0000` (black) used.
- Visual result: high-contrast, crisp — same language as the OLED but larger.
- Zero FLASH cost for color data.
- Enables direct visual comparison with the OLED layout during development.

### Mode B — Mode accent colors (`#define DISPLAY_MODE_COLORS`)
- Add a 10-element `uint16_t mode_colors[10]` table (20 bytes in FLASH).
- Mode name, active bar, and status text rendered in the mode's accent color.
- Everything else stays white-on-black.
- Gives each of the 10 modes a distinct identity at negligible cost.

```cpp
// src/display/display_colors.h
#ifdef DISPLAY_MODE_COLORS
static constexpr uint16_t kModeColors[10] = {
    0x07FF,  // Digital  — cyan
    0xF800,  // Duck     — red
    0x07E0,  // Swell    — green
    0xFFE0,  // Trem     — yellow
    0xF81F,  // DBucket  — magenta
    0xFD20,  // Tape     — orange
    0x001F,  // Dual     — blue
    0x7FFF,  // Pattern  — light cyan
    0xFF00,  // Filter   — yellow-green
    0x8010,  // Lofi     — purple
};
#else
static constexpr uint16_t kModeColors[10] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
};
#endif
```

**Recommendation:** Ship Mode A. Enable Mode B once FLASH budget is confirmed safe.
Full color backgrounds, gradients, or PNG assets are out of scope unless moved to QSPI.

---

## Phase 0 — Cleanup & Baseline (prerequisite)

**Goal:** Free FLASH and unblock the SPI pin before writing any display code.

### 0a. Remove legacy ADC pot initialization
`pin_map.h` defines `POT_TIME` through `POT_MOD_DEP` as ADC channels 0–6.
These pins are no longer connected (hardware moved to encoders). Find the `hw.adc`
initialization in `main.cpp` and remove the ADC channel configuration entirely.

This frees D18 (PA7, ADC channel 3) for use as SPI1_MOSI.

### 0b. Remove I2C OLED driver code
- Remove the `OledDisplay<SSD130xI2c128x64Driver>` instance and all its includes.
- Remove the I2C init block from `main.cpp` hardware setup.
- Delete `src/display/display_manager.cpp` and `display_manager.h` (will be rewritten).
- Keep `display_layout.h` as a reference for porting the layout.

This frees D11 (PB8 = I2C1_SCL) and D12 (PB9 = I2C1_SDA) as general GPIOs.

### 0c. Measure FLASH
Build and record the new FLASH usage. Expected: ~5–8 KB freed. Document the baseline
before any display code is added.

---

## Phase 1 — ST7789 SPI Driver

**New file:** `src/display/st7789_driver.h` + `src/display/st7789_driver.cpp`

### Pin constants (add to `src/config/pin_map.h`)
```cpp
// ST7789 SPI display
constexpr daisy::Pin DISP_SCK = daisy::seed::D22;  // PA5  SPI1_SCK
constexpr daisy::Pin DISP_SDA = daisy::seed::D18;  // PA7  SPI1_MOSI
constexpr daisy::Pin DISP_CS  = daisy::seed::D13;  // PB6
constexpr daisy::Pin DISP_DC  = daisy::seed::D14;  // PB7
constexpr daisy::Pin DISP_RES = daisy::seed::D26;  // PD11
constexpr daisy::Pin DISP_BLK = daisy::seed::D24;  // PA1  (or hardwire to 3V3)
```

### Driver interface
```cpp
class St7789Driver {
public:
    void Init();
    // Blocking fill — use only during startup/test
    void FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    // DMA transfer of full frame buffer — non-blocking, calls cb when done
    using DoneCallback = void(*)(void*);
    void StartDmaTransfer(const uint16_t* buf, uint32_t len,
                          DoneCallback cb, void* ctx);
    bool IsBusy() const;
private:
    void WriteCmd(uint8_t cmd);
    void WriteData(const uint8_t* data, size_t len);
    void SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    daisy::SpiHandle spi_;
    daisy::GPIO      pin_cs_, pin_dc_, pin_res_, pin_blk_;
};
```

### ST7789 init sequence
The init sequence (SWRESET → SLPOUT → COLMOD 16-bit → MADCTL → display on)
is ~15 command bytes total. Store in a `static const uint8_t` table in FLASH.
Key settings: COLMOD=0x55 (RGB565), MADCTL=0x00 (portrait, RGB order).

### DMA pattern
```
St7789Driver::StartDmaTransfer():
  1. Assert CS low
  2. Assert DC high (data mode)
  3. spi_.DmaTransmit(buf, len, nullptr, DmaDoneISR, this)

DmaDoneISR (SPI DMA complete interrupt):
  1. Deassert CS high
  2. Call user callback
  3. Clear busy flag
```

### Phase 1 validation
`#ifdef DISPLAY_TEST` block in `main.cpp`: fill screen with alternating red/blue bands.
Remove before Phase 2.

---

## Phase 2 — Frame Buffer & Rendering Primitives

**New file:** `src/display/display_renderer.h` + `src/display/display_renderer.cpp`

### Frame buffer allocation (SDRAM)
```cpp
// display_renderer.cpp  — file scope, SDRAM
DSY_SDRAM_BSS static uint16_t kFrameBuf[320 * 240];  // 150 KB
```

240×320 × 2 bytes = 153,600 bytes. External SDRAM — no SRAM or stack impact.

### Rendering primitives (all write to `kFrameBuf`)

| Function | Description |
|----------|-------------|
| `Clear(color)` | Fill entire buffer |
| `FillRect(x,y,w,h,color)` | Filled rectangle |
| `HLine(x,y,len,color)` | Horizontal line (bar drawing) |
| `VLine(x,y,len,color)` | Vertical separator |
| `DrawChar(x,y,ch,fg,bg,font)` | Single character from 1-bit font |
| `DrawText(x,y,str,fg,bg,font)` | String wrapper |

### Font strategy
Port the existing `Font_11x18` and `Font_6x8` bitmap tables directly — they are
1-bit packed arrays. `DrawChar` expands each bit to `fg` or `bg` into `kFrameBuf`.
Font data is identical; zero additional FLASH cost.

Do **not** add new fonts until FLASH budget is re-evaluated after Phase 3.

---

## Phase 3 — Display Manager Rewrite

**Rewrite:** `src/display/display_manager.h` + `src/display/display_manager.cpp`

Keep the same public interface so `main.cpp` call sites change minimally:

```cpp
class DisplayManager {
public:
    void Init();
    void Update(const DisplayState& state);  // called every 33 ms
    bool IsBusy() const;
private:
    void Render(const DisplayState& state);  // writes frame buffer
    void Flush();                            // starts DMA transfer
    St7789Driver driver_;
    bool         dma_busy_ = false;
};
```

### Layout mapping (240×320, portrait)

```
┌────────────────────────────────┐  y=0
│  MODE NAME   │  BYPASS  │ PRE  │  h=40  (Font_11x18, accent color in Mode B)
├────────────────────────────────┤  y=40
│  TIME  ████████░░░░░░░  2340ms │
│  REP   ██████░░░░░░░░░  0.72   │  7 rows × ~30px each
│  MIX   ████░░░░░░░░░░░  0.45   │
│  FLT   ████████████░░░  0.83   │
│  GRIT  ██░░░░░░░░░░░░░  0.18   │
│  MSPD  ██████░░░░░░░░░  4.2Hz  │
│  MDEP  ████░░░░░░░░░░░  0.40   │
├────────────────────────────────┤  y=250
│  MIDI CLK  │  SHF  │  LOAD P3 │  h=30  (Font_6x8, status row)
└────────────────────────────────┘  y=280
```

The extra horizontal space (240 vs 128 px) is used for longer bar fills and numeric
value text to the right of each bar — no layout restructuring needed.

### DMA handoff in main loop
```cpp
// main.cpp — inside 33 ms display update block
if (!display_manager.IsBusy()) {
    display_manager.Update(display_state);  // render + kick DMA
}
// returns immediately; DMA runs in background
```

---

## Phase 4 — Color Mode (optional)

Add `#define DISPLAY_MODE_COLORS` to `Makefile`.
Enable only after Phase 3 FLASH measurement confirms headroom.

Color is applied in `Render()` by selecting `kModeColors[state.mode_id]` as the
foreground for the mode name row and bar fills. 20 bytes of FLASH, no new code paths.

---

## Phase 5 — FLASH Audit & QSPI Fallback

After Phase 3:
```bash
arm-none-eabi-size build/delay.elf
```

**< 125 KB:** Safe. Enable Mode B, optionally add a larger font.

**> 126 KB:** Move font bitmap tables to QSPI:
1. Store font arrays at a fixed QSPI offset (e.g., 0x4000, above preset storage at 0x0000).
2. Copy to QSPI at first boot or via a flash tool.
3. Replace font pointer in `DrawChar` with `hw.qspi.Read()`.
4. QSPI reads ~100 ns — negligible at 33 ms update rate.

---

## File Change Summary

| File | Action |
|------|--------|
| `src/config/pin_map.h` | Add `DISP_*` constants; remove legacy ADC pot channels |
| `src/display/display_manager.h/cpp` | Rewrite |
| `src/display/st7789_driver.h/cpp` | New |
| `src/display/display_renderer.h/cpp` | New |
| `src/display/display_colors.h` | New |
| `src/display/display_layout.h` | Update coordinate constants for 240×320 |
| `src/main.cpp` | Remove I2C/OLED init; guard DMA busy on display update |
| `Makefile` | Swap SSD1306 objects for new display objects |

---

## Risk Register

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| FLASH overflow | High | Phase 0 cleanup first; measure after every phase |
| D18 ADC conflict (POT_FILTER, ch.3) | Medium | Remove ADC init in Phase 0 before touching SPI |
| DMA transfer too slow | Low | At 50 MHz, 240×320 = ~12 ms; well under 33 ms budget |
| SPI1 clock polarity mismatch | Low | ST7789 uses CPOL=1 CPHA=1 (SPI mode 3); set in config |
| DMA re-entrancy | Low | Guard with `dma_busy_` flag; never start two transfers |

---

## Build Verification Checkpoints

After each phase:
```bash
make -j4 2>&1 | grep -E "text|data|bss|FLASH"
arm-none-eabi-size build/delay.elf
```

Target: keep `.text + .rodata` under 126 KB throughout.
