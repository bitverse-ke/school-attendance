# Wiring — ESP32-WROOM-32 + R307 + peripherals

## Pin map (ESP32-DevKitC v4 layout)

| Function          | ESP32 GPIO | Connects to              | Notes                          |
|-------------------|------------|--------------------------|--------------------------------|
| Fingerprint RX    | GPIO 16    | R307 TX → divider → here | **Voltage divider required**   |
| Fingerprint TX    | GPIO 17    | R307 RX                  | 3.3V from ESP32 OK for R307 RX |
| OLED SDA          | GPIO 21    | OLED SDA                 | I2C bus 0                      |
| OLED SCL          | GPIO 22    | OLED SCL                 | I2C bus 0                      |
| DS3231 SDA        | GPIO 21    | RTC SDA                  | Shared I2C, addr 0x68          |
| DS3231 SCL        | GPIO 22    | RTC SCL                  | Shared I2C                     |
| Buzzer +          | GPIO 25    | Buzzer +                 | PWM-capable                    |
| LED Red           | GPIO 26    | RGB common-cathode R     | Active-low (HIGH = off)        |
| LED Green         | GPIO 27    | RGB G                    | Active-low                     |
| LED Blue          | GPIO 14    | RGB B                    | Active-low                     |

**Avoid these GPIOs:** 6–11 (flash), 0 (boot mode), 2 (on-board LED
on some clones), 12 (boot voltage select — must be LOW at boot).

## Power

```
                  ┌─────────────────┐
   5V PSU ────────┤ ESP32 5V (VIN)  │
                  │                 │
                  │  [AMS1117 3.3V] ├────── 3.3V rail ──── OLED, RTC
                  └─────────────────┘                          │
                                                             R307 VCC ← 5V!
```

**R307 wants 5V on its VCC** even though logic is 5V-tolerant. Pull
both 5V (VIN) and 3.3V from the ESP32 dev board's pins. Total draw
peaks at ~150mA (R307 capture burst) — 5V 1A PSU is sufficient.

## Voltage divider (mandatory)

```
R307 TX ─── 1kΩ ─┬── ESP32 GPIO 16 (RX)
                 │
                2kΩ
                 │
                GND
```

R307 outputs 5V on its TX line. ESP32 GPIO is **NOT** 5V-tolerant on
input — sending 5V into it will damage the chip over time. The
divider drops 5V → 3.3V. R307 RX is happy with 3.3V directly.

## I2C bus

Both OLED (0x3C) and DS3231 (0x68) on the same I2C bus. Default
ESP32 Wire pins (21/22) work. Pull-ups usually built into both
modules; if you see I2C errors, add 4.7kΩ pull-ups to 3.3V.

## RGB LED wiring

Common cathode: shared GND pin, three anodes via current-limit
resistors (220Ω each) to GPIO 26/27/14.

```
ESP32 GPIO 26 ── 220Ω ── LED R anode
ESP32 GPIO 27 ── 220Ω ── LED G anode
ESP32 GPIO 14 ── 220Ω ── LED B anode
LED cathode ──────────── GND
```

Code drives GPIO LOW to light, HIGH to off (active-low). See
`feedback.cpp`.

## Breadboard prototype (first pass)

![breadboard mental picture — not actual diagram]

- ESP32 DevKit on left half of full-size breadboard
- R307 to the right, ribbon cable to sensor mounted in enclosure lid
- OLED on top-left for visibility
- RTC + RGB LED in the middle
- Buzzer off to the side (it squeaks during testing)
- Power via USB cable (5V) from a wall adapter

## Outdoor enclosure (IP65)

For the first prototype, use a weatherproof junction box:

- 100×80×50mm ABS junction box (~$5)
- Drill hole for R307 sensor (15mm) on the lid
- Drill hole for OLED (cut square with Dremel)
- Two PG7 cable glands for: 5V power input, future PoE input
- Hot-glue around sensor and OLED edges for IP65 seal
- Mount to wall/turnstile with M4 screws through box base

## Verification

After wiring, before first flash:

```bash
# With ESP32 connected via USB and sensor powered:
# 1. Open serial monitor
pio device monitor

# 2. Should see on boot:
# === school-attendance 0.1.0 hw=esp32-wroom-r307-v1 ===
# [OLED] init ok
# [FP] Ready. cap=1000 sec=2 templates=0

# 3. If "Password verify FAILED":
#    - Check TX/RX are swapped (R307 TX → ESP RX, R307 RX → ESP TX)
#    - Check voltage divider on R307 TX line
#    - Try baud 9600 (some clones); edit FP_BAUD in config.h

# 4. If OLED blank:
#    - Try address 0x3D (some SSD1306 modules)
#    - Check I2C SDA/SCL aren't swapped

# 5. RTC: send "TIME" over serial, should reply with valid epoch
```

## Schematic (text-art)

```
                   ┌──────────────┐
   R307 ──┐        │  ESP32-WROOM │
   VCC ───┼─5V─    │              │
   GND ───┼─GND─   │              │
   TX ────┼─[DIV]─►RX  GPIO16    │
   RX ◄────┼─3.3V──TX  GPIO17    │
           │        │              │
           │        │  GPIO21 ◄──── OLED + RTC SDA
           │        │  GPIO22 ◄──── OLED + RTC SCL
           │        │  GPIO25 ────► Buzzer +
           │        │  GPIO26 ────► LED R
           │        │  GPIO27 ────► LED G
           │        │  GPIO14 ────► LED B
           │        │              │
           │        │  VIN  ◄───── 5V PSU
           │        │  GND  ◄───── PSU GND
           │        └──────────────┘
           │
        [1kΩ]
           └──┐
              ┌┴┐ 1kΩ
              └┬┘
           ────┤ ├─── to ESP RX (GPIO 16)
              ┌┴┐ 2kΩ
              └┬┘
           ────┴──── GND
```

When we move to PCB, replace the divider with a proper level
shifter (e.g. TXS0108E) or run R307 from 3.3V (some clones work;
genuine R307 wants 5V).
