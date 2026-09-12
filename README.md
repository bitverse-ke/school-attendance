# School Attendance — Edge Firmware

Fingerprint-based attendance for ~600 students. **Hybrid architecture:**
R307 fingerprint sensors at gates/classrooms (ESP32 edge), Firebase
backend for admin UI, reporting, and audit logs.

**Project status:** Prototype / outdoor build, firmware 0.1.0.

---

## What this is

A small wall-mounted or turnstile-mounted box:

```
   ┌──────────────────────────────┐
   │  ┌────────┐  ┌────────────┐  │
   │  │ OLED   │  │ R307 sensor│  │  ← student places finger here
   │  │ 0.96"  │  │ (sealed)   │  │
   │  └────────┘  └────────────┘  │
   │  ESP32-WROOM-32              │
   │  DS3231 RTC + RGB LED + buzz │
   └──────────────────────────────┘
            │
            │ WiFi (or PoE later)
            ▼
   Firebase Cloud Functions → Firestore → Angular admin UI
```

## Hardware BOM (per device)

| Component                    | Spec                         | Cost (KES) |
|------------------------------|------------------------------|------------|
| ESP32-WROOM-32 DevKit        | 38-pin, USB-C, on-board LED  | 2,000–2,500|
| R307 fingerprint sensor      | UART, 1000-template capacity | 1,200–1,500|
| SSD1306 OLED 0.96"           | I2C, 128×64                  | 600–800    |
| DS3231 RTC module            | I2C, ±2 ppm                  | 400–600    |
| RGB LED (common cathode)     | 5mm                          | 50         |
| Active buzzer                | 3.3V, 5mA                    | 50         |
| Voltage divider (1kΩ+2kΩ)    | For R307 TX → ESP32 RX       | 20         |
| IP65 junction box            | 100×80×50mm                  | 800–1,200  |
| Cable glands (PG7)           | 2 pcs (sensor, power)        | 100        |
| 5V 2A PSU (outdoor rated)    | Or PoE module for PCB rev    | 1,500      |
| **Total**                    |                              | **~6,800** |

For 3 devices: **~KSh 20,000** in hardware. Vs commercial ZKTeco gate
reader at ~$250 each (KSh 32,000+ per gate just for hardware).

## Wiring

See [`docs/WIRING.md`](docs/WIRING.md). One voltage divider required —
R307 TX is 5V logic, ESP32 RX is 3.3V.

## Build & flash

```bash
# Install PlatformIO CLI (or use VSCode extension)
pip install platformio

# Copy secrets template and fill in your device API key
cp src/secrets.h.template src/secrets.h
$EDITOR src/secrets.h   # set DEVICE_API_KEY, DEVICE_HMAC_KEY

# Build
pio run

# Flash over USB
pio run -t upload

# Monitor serial (115200)
pio device monitor
```

## First-boot configuration

1. Power on the ESP32 — it starts an AP called **Attendance-Setup**.
2. Connect phone/laptop to that AP.
3. Captive portal opens (or browse to `192.168.4.1`).
4. Enter:
   - School WiFi SSID + password
   - Device ID (e.g. `gate-north-01`)
   - Backend host (default OK for prototype)
5. Device reboots, connects to WiFi, syncs NTP, registers with backend.

## Daily operation

- Idle: OLED shows time + "Place finger"
- Scan: blue LED, "Scanning…", then green flash + ID display on match
- No match: red flash + "NO MATCH — see admin"
- Offline: amber blink pattern; events queue locally

## Serial admin commands

Connect USB, set 115200 baud. Useful while developing:

```
STATUS         → state, WiFi, queue depth, sensor count
TIME           → current epoch
ENROL 247      → enrol new student into slot 247
DEL 247        → delete slot 247
EMPTY          → wipe fingerprint DB (careful!)
RESET          → reboot ESP32
WIFIRESET      → clear WiFi creds (portal on next boot)
```

## Project structure

```
school-attendance/
├── platformio.ini             # deps, board, build flags
├── src/
│   ├── main.cpp               # FSM + setup/loop
│   ├── config.h               # pins, constants, feature flags
│   ├── state.h                # FSM enum + context
│   ├── feedback.h/.cpp        # buzzer + RGB LED
│   ├── display_module.h/.cpp  # OLED UI
│   ├── fingerprint_module.*   # R307 driver wrapper
│   ├── wifi_manager.*         # WiFiManager + captive portal
│   ├── ntp_sync.*             # NTP + DS3231 RTC fallback
│   ├── offline_queue.*        # LittleFS event queue
│   ├── http_client.*          # Firebase Cloud Functions client
│   ├── ota_module.*           # ArduinoOTA
│   └── secrets.h.template     # API keys (git-ignored when copied)
├── docs/
│   ├── WIRING.md
│   ├── FIREBASE_API.md
│   └── OUTDOOR_BUILD.md
└── README.md (this file)
```

## Architectural decisions

### Why R307 over ZKTeco FK series

- **Cheaper** (~$15 vs $80+) — important for 600-student school budget
- **Matching done on-chip** — fingerprint template NEVER leaves the sealed
  sensor. This is a hard requirement for Kenya DPA 2019 compliance
  (biometric data of minors = sensitive personal data).
- **1000-template capacity** — fits 600 students + 100% buffer

### Why ESP32 over Raspberry Pi

- **Cost** (~$5 vs $40+) — at 3 gates this matters
- **Power** (0.5W vs 3W+) — feasible for solar/PoE
- **Boot time** (2s vs 25s) — school opens 7am, no waiting
- **GPIO out of the box** — no HATs required

The trade-off: less RAM (320KB), no OS-level process isolation. For this
scope (one process, simple I/O) it's a perfect fit. If we later add
local ML or on-device face recognition, we'd revisit.

### Why Firebase over a self-hosted backend

- **Time-to-prototype** — Cloud Functions + Firestore + Auth + Hosting is
  one project, no infra setup
- **Real-time** — Firestore listeners give the admin UI live attendance feed
- **Free tier covers initial dev**, Blaze plan at this scale is ~$10/mo

Trade-off: vendor lock-in. Mitigation: events are pushed as JSON; if we
ever migrate, the Cloud Function becomes a thin adapter.

### Why HTTP for device↔cloud, not MQTT

- **Simpler** for a prototype — one HTTPS endpoint per event type
- **Easier to debug** — `curl` + Cloud Functions logs are sufficient
- **Better Firebase integration** — Cloud Functions are HTTPS-triggered
- **Trade-off:** higher per-message overhead. For 600 students × 4
  scans/day × 250 bytes = 600KB/day. Negligible.

If we later need sub-second latency across many devices or pub/sub
patterns, MQTT via a self-hosted broker is the upgrade path.

## Security notes

- **API keys** in `secrets.h` are checked into nothing — file is
  git-ignored. Provisioning script generates keys at first boot.
- **HTTPS** with `setInsecure()` for prototype (saves cert management).
  TODO: bundle Google CA fingerprint for pin-and-skip.
- **Templates stay on R307** — never serialised to ESP32 flash.
- **Offline queue** is LittleFS, not RAM — survives reboot.
- **Audit log** in Firestore captures every verification event with
  device ID, score, and timestamp.

## Kenya DPA 2019 compliance

This device handles biometric data of minors (sensitive personal data).
Non-negotiable items:

- ODPC registration of data controller (the school)
- DPIA before go-live
- Written parental consent (admin UI flow)
- Auto-delete templates 30 days after student leaves
- Privacy notice displayed during enrolment

The firmware supports this — only template IDs (numbers 1–1000) leave
the sensor; actual fingerprint images/templates never do. The school
must still complete the ODPC paperwork.

## Roadmap

- [x] Fingerprint capture + matching
- [x] OLED UI for staff
- [x] Offline queue + flush on reconnect
- [x] NTP + RTC fallback
- [x] Heartbeat + remote status
- [x] OTA firmware updates
- [ ] Custom PCB with PoE (Silvertel AG9900) — *future rev*
- [ ] Web enrolment push from admin UI (Cloud Function → device)
- [ ] Name cache in NVS so OLED shows student name, not just ID
- [ ] Multi-gate pairing (gate-north, gate-south, classroom-A)
- [ ] Solar + battery variant for off-grid schools

## License

TBD — internal Bitverse Limited project.
