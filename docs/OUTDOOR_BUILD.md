# Outdoor build notes

## Why outdoor matters

- Power cuts: brown-outs, surges, voltage dips on PoE
- WiFi: distance from AP, walls, weather (rain attenuates 2.4 GHz)
- Dust, rain, humidity: IP65 enclosure mandatory
- Sun: direct heat inside sealed box → component derating
- Vandalism: mounted at student height, must be robust

## Enclosure spec

- **IP65 ABS junction box**, 100×80×50mm minimum
- Hinged door or 4-screw lid for access
- Clear window for OLED (cut with Dremel, hot-glue acrylic sheet)
- 15mm hole for R307 sensor (slightly recessed — protects sensor)
- 2× PG7 cable glands: power input + future expansion
- Mounting flanges on the box, not the lid
- Wall-mount at ~1.4m height (chest height for adult, accessible for kids)

## Thermal

ESP32 + R307 together dissipate ~1W. In a sealed black box in direct
Nairobi sun, internal temp can hit 60°C+.

- **Paint box white** or use light-grey ABS
- **Ventilation slots** at top + bottom with Gore-Tex membrane (Goretex
  Vent PMF100643) — IP65 + breathability
- **No direct sun mounting** — shade under eave if possible

R307 spec: 0–40°C operating. DS3231: 0–70°C. ESP32: -40–85°C. R307 is
the thermal bottleneck. Keep box internal temp below 40°C.

## Power

### Phase 1 (breadboard + dev board): USB power

- 5V 2A USB power adapter, weatherproof outlet cover
- ESP32 dev kit draws from USB
- Battery backup optional: 18650 + TP4056 + boost converter

### Phase 2 (custom PCB + PoE): IEEE 802.3af

When we design the PCB, integrate:

- **Silvertel AG9900** PoE module — 802.3af, 5W output, isolated
- Or **TI TPS23753** if we need more power (PD controller)
- 12V → 5V buck converter on PCB
- 5V → 3.3V LDO (or use ESP32 module's onboard regulator)

Benefits:
- Single CAT5/CAT6 cable for power + data
- Switch-side UPS covers power outages
- No mains voltage at the gate (safer for school)
- No need for outdoor 240V outlet

Cable run: CAT6 from school network closet, max 100m per PoE spec.
Use outdoor-rated UV-stable cable (e.g. CommScope OC-TS-50).

### Phase 3 (off-grid / no network): solar

For rural schools without reliable grid:

- 20W mono solar panel
- 12V 7Ah SLA battery (~$2,000)
- 12V → 5V buck converter (3A)
- 4G LTE modem (Huawei E3372 or similar) for backhaul
- Estimated runtime: 5 days autonomy

Defer this until Phase 1 + 2 are proven.

## WiFi reliability

Outdoor mounting often means distance from AP. Tips:

- **Place AP within 30m line-of-sight** of each gate
- **Use 2.4 GHz** (better wall penetration than 5 GHz)
- **External antenna** on the ESP32 (U.FL connector on some modules)
- **Mesh nodes** for large school compounds (TP-Link Deco or similar)
- **Outdoor AP** (Ubiquiti UAP-AC-M) for campus deployment

If WiFi is too unreliable, fall back to LoRa for last-mile
(only viable for small payloads, doesn't fit Cloud Function POSTs).
Better alternative: 4G LTE modem per device with cellular failover.

## Mounting

```
         ┌──────────────┐
         │ Enclosure    │
         │ ┌──────────┐ │
         │ │  OLED    │ │   ← visible at adult eye level
         │ └──────────┘ │
         │ ┌──────────┐ │
         │ │  R307    │ │   ← student places finger here
         │ │ sensor   │ │   (slightly recessed for protection)
         │ └──────────┘ │
         │              │
         │   [PCB v1]   │   ← inside, accessible via door
         │              │
         └──────┬───────┘
                │ M4 screws × 4
                │
           ─────┴─────   wall / turnstile post
```

Mount height: ~1.4m (chest height of adult, easy for grade-4+).

## Maintenance

- **Firmware OTA** via ArduinoOTA or HTTPS — no physical access needed
- **Sensor cleaning** — R307 sensor surface gets dusty/grubby. Wipe
  monthly with isopropyl alcohol + lint-free cloth.
- **Battery check** (if applicable) — every 6 months
- **Seal inspection** — every 12 months, re-seal if silicone cracking

## Cost evolution

| Phase | Hardware | Dev effort | Notes                  |
|-------|----------|-----------:|------------------------|
| 1     | ~KSh 7k  | 1 week     | Breadboard + dev kit   |
| 2     | ~KSh 4k  | 3 weeks    | Custom PCB + PoE       |
| 3     | ~KSh 30k | 2 weeks    | Solar + 4G for off-grid|

For 3 gates: KSh 21k (Phase 1) → KSh 12k (Phase 2) — Phase 2 saves
money at scale because per-unit cost drops.
