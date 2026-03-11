"""
Generate SVG schematics for the Daisy Seed Delay Pedal using pure Python.
Each sheet is a standalone SVG file written to ./export/.
"""

import os, math, textwrap

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "export")
os.makedirs(OUT, exist_ok=True)

# ─── SVG primitives ──────────────────────────────────────────────────────────

def svg_open(w, h, title=""):
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
        f'viewBox="0 0 {w} {h}" font-family="monospace" font-size="11">\n'
        f'  <title>{title}</title>\n'
        f'  <rect width="{w}" height="{h}" fill="white" stroke="none"/>\n'
    )

def svg_close():
    return "</svg>\n"

def line(x1, y1, x2, y2, color="#000", w=1.5):
    return f'  <line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="{w}"/>\n'

def rect(x, y, w, h, fill="white", stroke="#000", sw=1.5):
    return f'  <rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>\n'

def circle(cx, cy, r, fill="white", stroke="#000", sw=1.5):
    return f'  <circle cx="{cx}" cy="{cy}" r="{r}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>\n'

def text(x, y, s, anchor="middle", size=11, bold=False, color="#000"):
    weight = "bold" if bold else "normal"
    lines = s.split("\n")
    if len(lines) == 1:
        return (f'  <text x="{x}" y="{y}" text-anchor="{anchor}" '
                f'font-size="{size}" font-weight="{weight}" fill="{color}">{s}</text>\n')
    out = ""
    for i, ln in enumerate(lines):
        dy = 0 if i == 0 else "1.3em"
        out += (f'  <text x="{x}" y="{y}" text-anchor="{anchor}" '
                f'font-size="{size}" font-weight="{weight}" fill="{color}">'
                f'<tspan x="{x}" dy="{dy}">{ln}</tspan></text>\n')
        y += 14  # manual line advance for multi-line
    return out

def label(x, y, s, anchor="middle", size=10):
    return text(x, y, s, anchor=anchor, size=size)

def note(x, y, s, size=9):
    out = ""
    for i, ln in enumerate(s.split("\n")):
        out += text(x, y + i * 13, ln, anchor="start", size=size, color="#555")
    return out

# ─── Schematic component shapes ──────────────────────────────────────────────

def resistor(x1, y1, x2, y2):
    """Draw a resistor between two points (horizontal or vertical)."""
    horiz = abs(y2 - y1) < abs(x2 - x1)
    out = ""
    if horiz:
        mx, my = (x1 + x2) / 2, y1
        w = abs(x2 - x1) * 0.4
        out += line(x1, y1, mx - w, y1)
        out += rect(mx - w, y1 - 6, 2 * w, 12)
        out += line(mx + w, y1, x2, y2)
    else:
        mx, my = x1, (y1 + y2) / 2
        h = abs(y2 - y1) * 0.4
        out += line(x1, y1, x1, my - h)
        out += rect(x1 - 6, my - h, 12, 2 * h)
        out += line(x1, my + h, x2, y2)
    return out

def capacitor(x1, y1, x2, y2):
    """Draw a capacitor (two parallel plates)."""
    out = ""
    if abs(y2 - y1) >= abs(x2 - x1):  # vertical
        my = (y1 + y2) / 2
        out += line(x1, y1, x1, my - 4)
        out += line(x1 - 10, my - 4, x1 + 10, my - 4)
        out += line(x1 - 10, my + 4, x1 + 10, my + 4)
        out += line(x1, my + 4, x2, y2)
    else:  # horizontal
        mx = (x1 + x2) / 2
        out += line(x1, y1, mx - 4, y1)
        out += line(mx - 4, y1 - 10, mx - 4, y1 + 10)
        out += line(mx + 4, y1 - 10, mx + 4, y1 + 10)
        out += line(mx + 4, y1, x2, y2)
    return out

def diode(x1, y1, x2, y2):
    """Horizontal diode (anode left, cathode right)."""
    mx = (x1 + x2) / 2
    h = 10
    out = line(x1, y1, mx - h, y1)
    # Triangle
    out += f'  <polygon points="{mx-h},{y1-h} {mx-h},{y1+h} {mx+h},{y1}" fill="#000"/>\n'
    # Bar
    out += line(mx + h, y1 - h, mx + h, y1 + h, w=2)
    out += line(mx + h, y1, x2, y2)
    return out

def led(x1, y1, x2, y2):
    mx = (x1 + x2) / 2
    h = 10
    out = diode(x1, y1, x2, y2)
    # Emission arrows
    out += line(mx, y1 - h - 4, mx - 8, y1 - h - 16, color="#888")
    out += line(mx + 6, y1 - h - 4, mx - 2, y1 - h - 16, color="#888")
    return out

def inductor(x1, y1, x2, y2):
    """Simple inductor symbol (bumps)."""
    out = ""
    n = 4
    seg = (y2 - y1) / n
    out += line(x1, y1, x1, y1 + seg * 0.1)
    for i in range(n):
        cy = y1 + seg * (i + 0.5)
        out += f'  <path d="M {x1} {y1+seg*i} A {seg*0.4} {seg*0.4} 0 0 1 {x1} {y1+seg*(i+1)}" fill="none" stroke="#000" stroke-width="1.5"/>\n'
    out += line(x1, y2 - seg * 0.1, x2, y2)
    return out

def npn_bjt(cx, cy):
    """NPN BJT symbol centred at cx, cy."""
    out = ""
    # Circle
    out += circle(cx, cy, 20)
    # Base line
    out += line(cx - 20, cy, cx - 10, cy)
    # Vertical base bar
    out += line(cx - 10, cy - 15, cx - 10, cy + 15, w=2)
    # Collector
    out += line(cx - 10, cy - 10, cx + 15, cy - 20)
    out += line(cx + 15, cy - 20, cx + 20, cy - 20)
    # Emitter with arrow
    out += line(cx - 10, cy + 10, cx + 15, cy + 20)
    out += line(cx + 15, cy + 20, cx + 20, cy + 20)
    # Arrow on emitter
    out += f'  <polygon points="{cx+8},{cy+14} {cx+15},{cy+20} {cx+6},{cy+22}" fill="#000"/>\n'
    return out

def gnd(x, y):
    """Ground symbol."""
    out = line(x, y, x, y + 8)
    out += line(x - 12, y + 8, x + 12, y + 8, w=2)
    out += line(x - 8, y + 13, x + 8, y + 13, w=1.5)
    out += line(x - 4, y + 18, x + 4, y + 18, w=1.5)
    return out

def vdd(x, y, lbl="+3V3"):
    out = line(x, y, x, y - 8)
    out += line(x - 12, y - 8, x + 12, y - 8, w=2)
    out += text(x, y - 14, lbl, size=9)
    return out

def pot_symbol(x, y, lbl):
    """Potentiometer: resistor body with wiper arrow."""
    out = rect(x - 8, y - 25, 16, 50)
    # Wiper arrow
    out += f'  <polygon points="{x-18},{y} {x-8},{y-5} {x-8},{y+5}" fill="#000"/>\n'
    out += line(x - 18, y, x - 30, y)
    out += label(x, y + 35, lbl, size=9)
    return out

def switch_spst(x1, y1, x2, y2):
    mx = (x1 + x2) / 2
    out = line(x1, y1, mx - 12, y1)
    out += circle(mx - 12, y1, 3, fill="#000")
    out += circle(mx + 12, y1, 3, fill="#000")
    out += line(mx - 12, y1, mx + 12, y1 - 14)
    out += line(mx + 12, y1, x2, y2)
    return out

def opto_symbol(x, y):
    """6N138 optocoupler block."""
    out = rect(x - 30, y - 40, 60, 80)
    # LED inside
    out += diode(x - 30, y - 15, x, y - 15)
    # LED arrows (light emission)
    out += line(x - 15, y - 25, x - 8, y - 38, color="#666")
    out += line(x - 8, y - 25, x - 1, y - 38, color="#666")
    # Phototransistor inside (simplified)
    out += npn_bjt(x + 10, y + 15)
    out += label(x, y + 50, "6N138", size=9)
    return out

def relay_symbol(x, y):
    """SPDT relay: coil + switch contacts."""
    # Coil
    out = rect(x - 20, y - 15, 40, 30)
    out += label(x, y + 5, "coil", size=9)
    out += line(x - 20, y, x - 35, y)
    out += line(x + 20, y, x + 35, y)
    # Contact symbols
    out += label(x, y - 25, "COM", size=9)
    out += line(x, y - 15, x, y - 40)
    out += label(x - 30, y - 55, "NC", size=9)
    out += line(x - 20, y - 40, x, y - 40)
    out += line(x - 20, y - 40, x - 20, y - 65)
    out += label(x + 30, y - 55, "NO", size=9)
    out += line(x + 20, y - 40, x, y - 40)
    out += line(x + 20, y - 40, x + 20, y - 65)
    return out

def section_header(x, y, title, w=700):
    out = rect(x, y - 18, w, 24, fill="#e8e8f0", stroke="#666")
    out += text(x + 10, y, title, anchor="start", size=12, bold=True)
    return out

def component_box(x, y, w, h, name, value):
    out = rect(x, y, w, h, fill="#f9f9ff")
    out += text(x + w/2, y + h/2 - 4, name, size=10, bold=True)
    out += text(x + w/2, y + h/2 + 10, value, size=9)
    return out

# ─── Sheet 1: POWER ──────────────────────────────────────────────────────────

def draw_power():
    W, H = 820, 420
    out = svg_open(W, H, "Power Supply")
    out += section_header(10, 28, "1 · Power Supply  (9V DC → 5V LDO → Daisy VIN)")

    # J1
    out += component_box(30, 60, 80, 50, "J1", "2.1mm\nBarrel 9V")
    out += line(110, 85, 160, 85)
    out += text(135, 78, "+9V", size=9)

    # D1
    out += diode(160, 85, 240, 85)
    out += text(200, 72, "D1  1N4001", size=9)

    # C1 to GND
    out += line(240, 85, 270, 85)
    out += line(270, 85, 270, 110)
    out += capacitor(270, 110, 270, 160)
    out += text(285, 137, "C1  100µF/16V", size=9)
    out += gnd(270, 160)

    # LDO
    out += line(270, 85, 340, 85)
    out += component_box(340, 60, 140, 50, "U2", "MCP1700-5002\n5V LDO (TO-92)")
    out += line(480, 85, 530, 85)

    # C2 input bypass
    out += line(350, 85, 350, 110)
    out += capacitor(350, 110, 350, 155)
    out += text(365, 135, "C2  100nF", size=9)
    out += gnd(350, 155)

    # C3 output bypass
    out += line(530, 85, 530, 110)
    out += capacitor(530, 110, 530, 155)
    out += text(545, 135, "C3  100nF", size=9)
    out += gnd(530, 155)

    # C4 output bulk
    out += line(530, 85, 590, 85)
    out += line(590, 85, 590, 110)
    out += capacitor(590, 110, 590, 155)
    out += text(605, 135, "C4  100µF/10V", size=9)
    out += gnd(590, 155)

    # +5V output label
    out += line(590, 85, 660, 85)
    out += rect(660, 68, 130, 34, fill="#d4edda")
    out += text(725, 80, "+5V", bold=True, size=13)
    out += text(725, 96, "→ Daisy VIN", size=9)

    # GND bus
    out += line(110, 85, 110, 210)
    out += line(110, 210, 680, 210)
    out += gnd(680, 210)
    out += text(400, 225, "GND", size=10, bold=True)

    # Note
    out += note(30, 265,
        "Note: Daisy Seed 3V3 is internally regulated by a second LDO on the module.\n"
        "Pull-up resistors for pots and GPIO take 3V3 directly from Daisy pin 38.\n"
        "D1 provides reverse-polarity protection on the 9V input rail.")

    out += svg_close()
    path = f"{OUT}/1_power.svg"
    open(path, "w").write(out)
    print(f"  wrote {path}")


# ─── Sheet 2: POTS ───────────────────────────────────────────────────────────

POTS = [
    ("RV1", "TIME",     "ADC0", "R1",  "C5"),
    ("RV2", "REPEATS",  "ADC1", "R2",  "C6"),
    ("RV3", "MIX",      "ADC2", "R3",  "C7"),
    ("RV4", "FILTER",   "ADC3", "R4",  "C8"),
    ("RV5", "GRIT",     "ADC4", "R5",  "C9"),
    ("RV6", "MOD SPD",  "ADC5", "R6",  "C10"),
    ("RV7", "MOD DEP",  "ADC6", "R7",  "C11"),
]

def draw_pots():
    W, H = 860, 480
    out = svg_open(W, H, "Potentiometers")
    out += section_header(10, 28, "2 · Potentiometers  (7× 10kΩ linear, Alpha RV09)")

    COL = 115
    for i, (rv, name, adc, r, c) in enumerate(POTS):
        x = 20 + i * COL

        # +3V3
        out += vdd(x + 8, 70, "+3V3")

        # Pot symbol (as a box)
        out += rect(x, 80, 50, 100, fill="#f0f0ff")
        out += text(x + 25, 118, rv, size=9, bold=True)
        out += text(x + 25, 132, name, size=9)
        out += text(x + 25, 146, "10kΩ lin", size=8)

        # Pin 1 (+3V3) top
        out += line(x + 25, 70, x + 25, 80)
        # Pin 3 (GND) bottom
        out += line(x + 25, 180, x + 25, 200)
        out += gnd(x + 25, 200)

        # Wiper (pin 2) → series resistor → ADC
        out += line(x, 130, x - 20, 130)
        out += resistor(x - 20, 130, x - 20, 200)
        out += text(x - 7, 167, r, size=8)
        out += text(x - 7, 179, "1kΩ", size=8)

        # Junction + AA cap + ADC label
        out += circle(x - 20, 200, 3, fill="#000")
        out += capacitor(x - 20, 200, x - 20, 250)
        out += text(x - 7, 227, c, size=8)
        out += text(x - 7, 239, "100nF", size=8)
        out += gnd(x - 20, 250)

        # ADC output arrow
        out += line(x - 20, 200, x - 50, 200)
        out += text(x - 52, 197, f"→ {adc}", anchor="end", size=9)

    out += note(20, 340,
        "Each pot: pin 1 = +3V3, pin 2 = wiper, pin 3 = GND.\n"
        "1kΩ series resistor protects Daisy ADC pin from short-circuit.\n"
        "100nF anti-alias / ESD cap from ADC node to GND.")

    out += svg_close()
    path = f"{OUT}/2_pots.svg"
    open(path, "w").write(out)
    print(f"  wrote {path}")


# ─── Sheet 3: CONTROLS ───────────────────────────────────────────────────────

def draw_controls():
    W, H = 820, 540
    out = svg_open(W, H, "Controls")
    out += section_header(10, 28, "3 · Controls  (Encoder EC11 + 2× Footswitch)")

    # ── Encoder section ──
    out += text(200, 65, "Rotary Encoder EC11  (D0/D1/D2)", anchor="middle", size=11, bold=True)

    ENC = [
        ("A",  "R8",  "C12", "ENC_A",  "D0"),
        ("B",  "R9",  "C13", "ENC_B",  "D1"),
        ("SW", "R10", "C14", "ENC_SW", "D2"),
    ]
    for i, (pin, r, c, sig, daisy) in enumerate(ENC):
        x = 80 + i * 200
        # +3V3
        out += vdd(x, 90)
        # Pull-up resistor
        out += resistor(x, 90, x, 160)
        out += text(x + 14, 127, f"{r} 10kΩ", size=9)
        # Junction node
        out += circle(x, 160, 3, fill="#000")
        # Debounce cap to GND
        out += capacitor(x, 160, x, 220)
        out += text(x + 14, 193, f"{c} 100nF", size=9)
        out += gnd(x, 220)
        # Switch to GND
        out += switch_spst(x, 160, x, 290)
        out += text(x + 14, 230, f"EC11 pin {pin}", size=9)
        out += gnd(x, 290)
        # Signal output
        out += line(x, 160, x - 55, 160)
        out += text(x - 58, 157, f"→ {sig} ({daisy})", anchor="end", size=9)

    # EC11 body
    out += rect(20, 295, 340, 50, fill="#e8e8ff")
    out += text(190, 315, "SW1 · EC11 Rotary Encoder", anchor="middle", size=10, bold=True)
    out += text(190, 332, "Alps or compatible — 20mm height, with push switch", anchor="middle", size=9)

    # ── Footswitch section ──
    out += text(600, 65, "Footswitches", anchor="middle", size=11, bold=True)

    FS = [
        ("SW2", "R11", "C15", "SW_BYPASS", "D3", "Bypass"),
        ("SW3", "R12", "C16", "SW_TAP",    "D4", "Tap Tempo"),
    ]
    for i, (sw, r, c, sig, daisy, lbl) in enumerate(FS):
        x = 500 + i * 210
        out += vdd(x, 90)
        out += resistor(x, 90, x, 160)
        out += text(x + 14, 127, f"{r} 10kΩ", size=9)
        out += circle(x, 160, 3, fill="#000")
        out += capacitor(x, 160, x, 220)
        out += text(x + 14, 193, f"{c} 100nF", size=9)
        out += gnd(x, 220)
        out += switch_spst(x, 160, x, 290)
        out += text(x + 14, 230, f"{sw}\nSPST momentary", size=9)
        out += gnd(x, 290)
        out += line(x, 160, x - 55, 160)
        out += text(x - 58, 157, f"→ {sig} ({daisy})", anchor="end", size=9)
        out += text(x, 305, lbl, anchor="middle", size=10, bold=True)

    out += note(20, 380,
        "All digital inputs use internal Daisy pull-ups (DaisySeed::ConfigureAdcChannel not needed).\n"
        "External 10kΩ pull-ups and 100nF debounce caps added for robustness in guitar pedal environment.\n"
        "Footswitches are standard SPST momentary types (e.g. Gorva Design or Boss-style).")

    out += svg_close()
    path = f"{OUT}/3_controls.svg"
    open(path, "w").write(out)
    print(f"  wrote {path}")


# ─── Sheet 4: AUDIO I/O + RELAY BYPASS ───────────────────────────────────────

def draw_io():
    W, H = 840, 520
    out = svg_open(W, H, "Audio I/O and Relay Bypass")
    out += section_header(10, 28, "4 · Audio I/O + True-Bypass Relay  (G5V-1 SPDT)")

    # Input jack
    out += component_box(20, 60, 80, 50, "J2", "Input\n1/4\" TS")
    out += line(100, 85, 160, 85)

    # Relay contacts
    out += relay_symbol(360, 155)
    # COM wire from input
    out += line(160, 85, 360, 85)
    out += line(360, 85, 360, 115)  # COM into relay

    # NC path (bypass) → output jack
    out += line(340, 90, 200, 90)
    out += line(200, 90, 200, 280)
    out += line(200, 280, 650, 280)
    out += text(420, 272, "NC (bypass path)", size=9, color="#555")

    # NO path → Daisy IN_L
    out += line(380, 90, 540, 90)
    out += text(460, 82, "NO (effect active path)", size=9, color="#555")
    out += rect(540, 70, 140, 34, fill="#d4edda")
    out += text(610, 82, "AUDIO_IN_L", size=9, bold=True)
    out += text(610, 96, "→ Daisy IN_L", size=9)

    # Output jack
    out += component_box(650, 262, 80, 50, "J3", "Output\n1/4\" TS")

    # Daisy OUT_L → output
    out += rect(540, 262, 100, 34, fill="#d4edda")
    out += text(590, 274, "AUDIO_OUT_L", size=8, bold=True)
    out += text(590, 288, "← Daisy OUT_L", size=8)
    out += line(640, 279, 650, 279)

    # ── Relay driver ──
    out += text(200, 335, "Relay Driver", anchor="middle", size=11, bold=True)

    # +5V at top of coil
    out += vdd(280, 350, "+5V")
    out += line(280, 350, 280, 375)
    # Relay coil box
    out += component_box(245, 375, 70, 50, "RLY1", "G5V-1\n5V coil")
    # Flyback diode across coil
    out += diode(330, 390, 330, 350)  # vertical
    out += text(345, 372, "D2  1N4148\nflyback", size=9)
    out += line(330, 390, 315, 390)
    out += line(330, 350, 280, 350)

    # NPN transistor
    out += npn_bjt(280, 475)
    out += line(280, 425, 280, 455)   # coil bottom → collector
    # Base resistor
    out += resistor(160, 475, 258, 475)
    out += text(210, 468, "R13  1kΩ", size=9)
    out += rect(20, 462, 140, 26, fill="#d4edda")
    out += text(90, 470, "RELAY_CTL", size=9, bold=True)
    out += text(90, 483, "← Daisy D5", size=9)
    # Emitter → GND
    out += line(280, 495, 280, 510)
    out += gnd(280, 510)

    # ── Bypass LED ──
    out += text(540, 335, "Bypass LED", anchor="middle", size=11, bold=True)
    out += rect(455, 348, 130, 26, fill="#d4edda")
    out += text(520, 356, "LED_BYPASS", size=9, bold=True)
    out += text(520, 369, "← Daisy D6", size=9)
    out += line(520, 374, 520, 390)
    out += resistor(520, 390, 520, 440)
    out += text(535, 417, "R14  470Ω", size=9)
    out += led(520, 440, 520, 490)
    out += text(538, 468, "LED1  Red 3mm", size=9)
    out += gnd(520, 490)

    out += note(20, 420,
        "Relay: COM = input tip.  NC = output tip (bypass, relay off).  NO = output tip (effect, relay on).\n"
        "When RELAY_CTL HIGH: Q1 saturates, coil energised, NO selected → effect signal routed to output.\n"
        "D2 clamps inductive spike when Q1 turns off.  Q1 base current ≈ 3mA (Vcc=3.3V, R13=1kΩ).")

    out += svg_close()
    path = f"{OUT}/4_io.svg"
    open(path, "w").write(out)
    print(f"  wrote {path}")


# ─── Sheet 5: MIDI ───────────────────────────────────────────────────────────

def draw_midi():
    W, H = 820, 480
    out = svg_open(W, H, "MIDI Input")
    out += section_header(10, 28, "5 · MIDI Input  (TRS Type-A, 6N138 Opto-isolation)")

    # J4 TRS jack
    out += component_box(20, 60, 80, 60, "J4", "TRS 3.5mm\nMIDI\nType-A")
    out += line(100, 75, 140, 75)   # tip
    out += line(100, 95, 140, 95)   # ring
    out += line(100, 115, 140, 115) # sleeve

    out += text(120, 68, "tip", size=9)
    out += text(120, 88, "ring", size=9)
    out += text(120, 108, "sleeve", size=9)

    # Tip: R15 → opto anode
    out += resistor(140, 75, 220, 75)
    out += text(180, 66, "R15  220Ω", size=9)

    # Ring: R16 → D3 → opto cathode
    out += resistor(140, 95, 220, 95)
    out += text(180, 86, "R16  220Ω", size=9)
    out += diode(220, 95, 290, 95)
    out += text(255, 86, "D3  1N4148", size=9)

    # Sleeve → GND
    out += line(140, 115, 160, 115)
    out += gnd(160, 115)

    # 6N138 opto block
    out += rect(290, 50, 160, 130, fill="#f9f9ff")
    out += text(370, 68, "U3  6N138", anchor="middle", size=10, bold=True)
    out += text(370, 82, "Optocoupler DIP-8", anchor="middle", size=9)

    # LED inside opto (anode pin 2, cathode pin 3)
    out += line(220, 75, 290, 75)   # tip → anode
    out += line(290, 95, 290, 95)
    out += diode(305, 85, 380, 85)
    out += text(342, 75, "LED", size=8, color="#555")

    # Cathode connects to ring path
    out += line(290, 95, 305, 95)
    out += line(305, 95, 305, 85)   # join to LED cathode

    # Phototransistor output (pin 6)
    out += npn_bjt(420, 130)
    # Vcc pin 8 → +3V3
    out += line(370, 50, 370, 30)
    out += vdd(370, 30)
    # C17 bypass
    out += capacitor(420, 30, 480, 30)
    out += gnd(480, 30)
    out += text(450, 22, "C17  100nF", size=9)
    # GND pin 5
    out += line(370, 180, 370, 200)
    out += gnd(370, 200)

    # Pull-up R17
    out += line(440, 130, 510, 130)
    out += circle(510, 130, 3, fill="#000")
    out += line(510, 130, 510, 60)
    out += resistor(510, 60, 510, 30)
    out += text(525, 47, "R17  4.7kΩ", size=9)
    out += vdd(510, 30)

    # Output → MIDI_RX
    out += line(510, 130, 600, 130)
    out += rect(600, 115, 180, 32, fill="#d4edda")
    out += text(690, 127, "MIDI_RX", bold=True, size=10)
    out += text(690, 141, "→ Daisy D14 (UART1 RX)", size=9)

    # USB MIDI note box
    out += rect(20, 260, 770, 60, fill="#fff9e6", stroke="#cca")
    out += text(405, 278, "USB MIDI", anchor="middle", size=11, bold=True)
    out += text(405, 295, "Connect Daisy USB port directly to host. No extra components needed.", anchor="middle", size=10)
    out += text(405, 312, "libDaisy MidiUsbHandler handles enumeration and parsing natively.", anchor="middle", size=9)

    out += note(20, 345,
        "TRS Type-A (MIDI 1.0 spec): tip = DIN-5 pin 5 (current sink), ring = DIN-5 pin 4 (current source).\n"
        "6N138 provides full galvanic isolation between MIDI source and Daisy ground.\n"
        "R17 pull-up inverts the open-collector output: idle line = HIGH (MIDI mark state).")

    out += svg_close()
    path = f"{OUT}/5_midi.svg"
    open(path, "w").write(out)
    print(f"  wrote {path}")


# ─── Main ─────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    print("Rendering schematics ...")
    draw_power()
    draw_pots()
    draw_controls()
    draw_io()
    draw_midi()
    print(f"Done. SVGs written to: {OUT}/")
