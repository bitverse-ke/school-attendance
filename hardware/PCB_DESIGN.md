# PCB Design — Production v1 (PoE + Outdoor IP65)

This is the design specification for the **production PCB revision** that
replaces the breadboard prototype. It documents board dimensions, layer
stack, placement, BOM, and routing guidance so a hardware engineer can
finish layout in KiCad GUI, or so a PCB fab house can quote it.

The KiCad schematic starter lives at `hardware/kicad/`. Open it, add the
footprints from the BOM below, and place them per the **Component
Placement Strategy** section.

---

## Board spec at a glance

| Parameter | Value |
|-----------|-------|
| Dimensions | 80 mm × 60 mm (rectangle) |
| Layers | **4-layer** (sig / GND / power / sig) |
| PCB thickness | 1.6 mm |
| Copper weight | 1 oz outer / 0.5 oz inner |
| Surface finish | ENIG (lead-free, outdoor-safe) |
| Solder mask | matte black (UV-resistant) |
| Silkscreen | white |
| Min track / gap | 0.2 mm / 0.2 mm (8 mil) |
| Min via | 0.4 mm drill / 0.6 mm pad |
| Mounting holes | 4× M3, corner positions |
| Operating temp | −20°C to +70°C |
| Estimated fab cost | KSh 2,500–4,500 per board (5-board batch at JLCPCB / PCBWay) |

## Layer stack

```
Layer 1 (top):   signal + components (most stuff on top)
Layer 2 (inner): GND plane (continuous, no cuts)
Layer 3 (inner): power planes (3V3 + 5V split)
Layer 4 (bottom): signal + a few passives
```

Why 4 layers and not 2:
- GND plane underneath the ESP32 antenna region gives clean RF
- Power plane simplifies 3V3 distribution to many pins
- Lower EMI for outdoor (school has WiFi interference from routers)

## Component placement strategy

Top side (component side):

```
┌────────────────────────────────────────────────────┐
│  [USB-C]   [PoE header]                            │
│                                                    │
│  [AMS1117-3.3]    [Decoupling caps]                │
│                                                    │
│  [ESP32-WROOM-32]   ← antenna faces LEFT edge     │
│  with keep-out zone (15mm × 15mm copper-free)     │
│                                                    │
│  [R307 header] (4-pin)     [OLED header] (4-pin)  │
│  with 1kΩ + 2kΩ divider near header              │
│                                                    │
│  [DS3231 header] (4-pin)                           │
│                                                    │
│  [3× LED]   [Buzzer]   [BOOT btn]   [RESET btn]   │
│                                                    │
│   ●      ●        ●        ●    ← 4× M3 mounting  │
└────────────────────────────────────────────────────┘
```

Placement rules:

1. **ESP32 antenna keep-out** — first 15 mm on the side opposite the antenna must be copper-free. Check your ESP32 module datasheet for exact orientation.
2. **R307 cable** — sensor cable should be ≤ 30 cm (UART distance). Header near edge of board so cable routes straight out.
3. **OLED + RTC close to ESP32** — I²C bus is short; keep pull-ups within 5 cm of SDA/SCL pins.
4. **Decoupling caps** — one 100 nF ceramic within 5 mm of every IC VCC pin.
5. **Voltage divider near R307 header** — keep the 1kΩ + 2kΩ within 1 cm of the connector to minimise noise pickup on the 5V line.

Bottom side:
- Just a few passives (filter caps for power, decoupling)
- All LEDs, buttons, ESP32 on top for easier access during assembly

## Power section

Power input options (both populated, jumper-selectable):

**Option A: USB-C (5V from any USB source)**
- USB-C receptacle: HRO TYPE-C-31-M-12 (Digi-Key 2059-USB-C-Receptacle-ND)
- 5V rail → AMS1117-3.3 → 3.3V rail
- AMS1117 footprint: SOT-223
- Input cap: 10 µF tantalum
- Output cap: 10 µF tantalum
- Decoupling: 100 nF ceramic near AMS1117 VOUT

**Option B: PoE (802.3af, ~48V from PoE switch)**
- Silvertel AG9900 PoE module (Digi-Ke...[truncated]y 1568-1015-ND) on bottom side, with thermal pad to GND plane
- Output: isolated 5V / 2A → feeds the AMS1117 like USB-C
- Optional: silkscreen note indicating PoE polarity doesn't matter (PD module handles)

## BOM (production revision)

| Ref | Part | Value | Footprint | Qty | Source | Unit (KSh) |
|-----|------|-------|-----------|-----|--------|------------|
| J1  | USB-C receptacle | — | USB-C-2-16P | 1 | LCSC C165948 | 180 |
| J2  | 4-pin 2.54 mm header | R307 | PinHeader_1x04_P2.54mm | 1 | LCSC C124414 | 30 |
| J3  | 4-pin 2.54 mm header | OLED | PinHeader_1x04_P2.54mm | 1 | LCSC C124414 | 30 |
| J4  | 4-pin 2.54 mm header | RTC  | PinHeader_1x04_P2.54mm | 1 | LCSC C124414 | 30 |
| U1  | AMS1117-3.3 | 3.3V LDO | SOT-223-3 | 1 | LCSC C6186 | 50 |
| U2  | ESP32-WROOM-32E | 4 MB flash | MODULE_ESP32-WROOM-32E | 1 | Mouser 356-ESP32WROOM32E | 1,400 |
| U3  | DS3231SN | RTC | SOIC-16W | 1 | LCSC C7512 | 350 |
| U4  | Silvertel AG9900 | PoE PD | MODULE_AG9900 | 1 (opt) | Digi-Key 1568-1015-ND | 2,800 |
| BZ1 | Piezo buzzer | 3 kHz / 85 dB | Buzzer_12x9.5RM7 | 1 | LCSC C92899 | 100 |
| Q1  | 2N2222A NPN | 600 mA | TO-92 | 1 | LCSC C282776 | 30 |
| D1–D3 | LED (R, G, B) | 20 mA | LED_0603 | 3 | LCSC C72043 | 20 ea |
| R1, R8 | 1 kΩ ±5% | 0.25 W | R_0603 | 2 | LCSC C25804 | 5 |
| R2  | 2 kΩ ±5% | 0.25 W | R_0603 | 1 | LCSC C25900 | 5 |
| R3, R4 | 4.7 kΩ ±5% | 0.25 W | R_0603 (I²C pull-ups) | 2 | LCSC C25905 | 5 |
| R5–R7 | 330 Ω ±5% | 0.25 W | R_0603 (LED current limit) | 3 | LCSC C25817 | 5 |
| R9–R11 | 10 kΩ ±5% | 0.25 W | R_0603 (BOOT/RESET pull) | 2 | LCSC C25804 | 5 |
| C1, C2 | 10 µF tantalum | 16 V | C_Tantalum_A | 2 | LCSC C7171 | 80 |
| C3, C4, C5 | 100 nF ceramic | 50 V | C_0603 | 5 | LCSC C165948 | 15 |
| SW1, SW2 | Tactile switch | 6×6 mm | SW_SPST_TL3342 | 2 | LCSC C525775 | 30 |
|     | 4× M3 brass standoff | 8 mm | — | 4 | AliExpress | 50 ea |

**Single-board cost: ~KSh 5,200** (5-board batch at JLCPCB: KSh 800 PCB + assembly + components)

## Manufacturing notes

1. **Gerbers export** — in KiCad: `File → Fabrication Outputs → Gerbers (.gtb)`. Use these settings:
   - Layers: all (top, bottom, both inners, edge cuts, silkscreen, mask)
   - Format: RS-274X
   - Precision: 4.5 (enough for 0.2 mm features)
   - Generate drill file (`.drl`) separately

2. **Assembly** — JLCPCB SMT service can populate the small parts (R, C, LED, Q1) for ~KSh 800. Hand-solder the ESP32 module, USB-C, and PoE module afterwards (or buy a populated service).

3. **Test points** — add test pads on the back for:
   - +5V rail
   - +3.3V rail
   - GND
   - I²C SDA, SCL
   This makes bring-up debugging much faster.

4. **Conformal coating** — for outdoor IP65 enclosure, apply acrylic or urethane conformal coating after assembly. HumiSeal 1B73 or similar. Protects against humidity and salt spray (if coastal).

## Enclosure (IP65)

Don't fab the PCB without planning the enclosure:

- **Material**: ABS or polycarbonate, UV-stabilised
- **Size**: 100 × 80 × 40 mm (room for PCB + cable glands)
- **Cable glands**: 2× PG7 (for USB + sensor cable)
- **Display cutout**: 25 × 15 mm window for OLED (covered by clear acrylic lens)
- **Mount**: wall-mount tabs or pole-mount U-bolts
- **Suggested source**: 3D print (PETG) for prototype; injection-mold for production

3D-print model placeholder: `hardware/enclosure/school-attendance_enclosure.step` (TODO).

## What's NOT in this revision (deferred to v2)

- GSM/LTE module for sites without WiFi
- Battery backup + charger (for short power cuts)
- Tamper switch (security)
- Solar panel input (for remote/off-grid sites)
- Camera module (for facial recognition backup)
- Anti-vandal enclosure (metal, not plastic)

These are documented in `docs/OUTDOOR_BUILD.md` as future considerations.
