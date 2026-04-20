# ST7789 2" IPS Display — Daisy Seed Wiring Guide

## Display Pinout

| Display Pin | Function | Notes |
|-------------|----------|-------|
| GND | Ground | |
| VCC | Power | 3.3 V |
| SCL | SPI Clock (SCLK) | Labeled SCL, not I2C |
| SDA | SPI Data In (MOSI) | Labeled SDA, not I2C |
| RES | Reset | Active low |
| DC | Data / Command select | High = data, Low = command |
| CS | Chip Select | Active low |
| BLK | Backlight enable | High = on |

> **Note:** Despite the I2C-looking labels (SCL/SDA), this is a 4-wire SPI interface.
> MISO is not present — the ST7789 is write-only for display purposes.

---

## Daisy Seed Connections

| Display Pin | Daisy Seed Pin | STM32 Port | Function |
|-------------|----------------|------------|----------|
| GND | GND (any) | — | Ground |
| VCC | 3V3 OUT (Pin 21) | — | 3.3 V supply |
| SCL | **D22** | PA5 | SPI1_SCK |
| SDA | **D18** | PA7 | SPI1_MOSI |
| RES | **D26** | PD11 | GPIO output |
| DC | **D14** | PB7 | GPIO output |
| CS | **D13** | PB6 | GPIO output |
| BLK | **D24** or 3V3 | PA1 | GPIO output (or hardwire to 3V3 for always-on backlight) |

### Daisy Seed Pin Reference Diagram

```
Daisy Seed (top view, USB to the right)
───────────────────────────────────────────────────────
Pin 1  3V3  ●  ● GND  Pin 40
Pin 2  D0   ●  ● D30  Pin 39   ← Encoder 3B (PB15)
Pin 3  D1   ●  ● D29  Pin 38   ← Encoder 3A (PB14)
Pin 4  D2   ●  ● D28  Pin 37   ← Encoder 2B (PA2)
Pin 5  D3   ●  ● D27  Pin 36   ← Encoder 2A (PG9)
Pin 6  D4   ●  ● D26  Pin 35   ← RES  ★
Pin 7  D5   ●  ● D25  Pin 34
Pin 8  D6   ●  ● D24  Pin 33   ← BLK  ★
Pin 9  D7   ●  ● D23  Pin 32
Pin 10 D8   ●  ● D22  Pin 31   ← SCL  ★  SPI1_SCK
Pin 11 D9   ●  ● D21  Pin 30
Pin 12 D10  ●  ● D20  Pin 29
Pin 13 D11  ●  ● D19  Pin 28
Pin 14 D12  ●  ● D18  Pin 27   ← SDA  ★  SPI1_MOSI
Pin 15 D13  ●  ● ...            ← CS  ★
Pin 16 D14  ●  ● ...            ← DC  ★
       ...
Pin 20 GND  ●  ● ...
Pin 21 3V3  ●  ● ...            ← VCC ★
```
*(★ = display connections)*

---

## Voltage & Electrical Notes

- The ST7789 and Daisy Seed both operate at **3.3 V** — no level shifting required.
- Total current draw of the display (backlight on): ~20–60 mA depending on brightness.
  The Daisy Seed 3V3 rail (~300 mA LDO capacity) handles this comfortably.
- If BLK is connected to a GPIO (D24), set it HIGH after init. For always-on backlight,
  wire directly to 3V3 and leave D24 unconnected.
- Use short wires (< 10 cm) for SPI lines — 50 MHz signals are sensitive to length.
- A 100 nF decoupling capacitor between VCC and GND close to the display is good practice.

---

## SPI Configuration (firmware reference)

```cpp
SpiHandle::Config spi_cfg;
spi_cfg.periph         = SpiHandle::Config::Peripheral::SPI_1;
spi_cfg.mode           = SpiHandle::Config::Mode::MASTER;
spi_cfg.direction      = SpiHandle::Config::Direction::TWO_LINES_TX_ONLY;
spi_cfg.datasize       = 8;
spi_cfg.clock_polarity = SpiHandle::Config::ClockPolarity::HIGH;
spi_cfg.clock_phase    = SpiHandle::Config::ClockPhase::TWO_EDGE;
spi_cfg.nss            = SpiHandle::Config::NSS::SOFT;
spi_cfg.baud_prescaler = SpiHandle::Config::BaudPrescaler::PS_4;  // ~50 MHz
spi_cfg.pin_config.sclk = daisy::seed::D22;  // PA5  SPI1_SCK
spi_cfg.pin_config.mosi = daisy::seed::D18;  // PA7  SPI1_MOSI
spi_cfg.pin_config.miso = {DSY_GPIOX, 0};   // not used
spi_cfg.pin_config.nss  = {DSY_GPIOX, 0};   // software CS
```

> **Important:** D18 (PA7) is the legacy `POT_FILTER` ADC channel (ADC1 ch.3).
> Remove the ADC initialization for pot channels before using D18 as SPI MOSI.
> See implementation plan Phase 0.
