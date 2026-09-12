/*
 * config.h — Pin map, constants, feature flags
 *
 * Outdoor prototype. Verify each pin against YOUR specific dev board
 * (ESP32-DevKitC, NodeMCU-32S, Wemos LOLIN D32 all differ slightly).
 */
#ifndef CONFIG_H
#define CONFIG_H

// ─── Board identifier ───────────────────────────────────────────────
#define DEVICE_ID_DEFAULT "gate-north-01"   // Override via WiFiManager
#define FIRMWARE_VERSION   "0.1.0"
#define HARDWARE_MODEL     "esp32-wroom-r307-v1"

// ─── Fingerprint sensor (R307, UART2) ───────────────────────────────
// R307 runs at 5V logic; its TX → ESP32 RX MUST go through a voltage
// divider (1kΩ + 2kΩ) to drop 5V → 3.3V. ESP32 TX → R307 RX is fine
// directly (R307 reads 3.3V as logic-high).
#define FP_RX_PIN  16   // ESP32 RX2 ← R307 TX (after divider)
#define FP_TX_PIN  17   // ESP32 TX2 → R307 RX
#define FP_BAUD    57600

// ─── OLED display (SSD1306 128x64, I2C0) ───────────────────────────
#define OLED_SDA    21
#define OLED_SCL    22
#define OLED_ADDR   0x3C
#define OLED_W      128
#define OLED_H      64

// ─── RTC DS3231 (I2C0, shares bus with OLED) ────────────────────────
#define RTC_ADDR    0x68

// ─── Feedback (buzzer + RGB LED) ────────────────────────────────────
#define BUZZER_PIN  25
#define LED_RED     26
#define LED_GREEN   27
#define LED_BLUE    14

// ─── Offline log queue (LittleFS, not SD for prototype) ─────────────
// We may add an SD card later for >10k event buffering. LittleFS
// comfortably holds ~50k events in the 1.5MB SPIFFS partition.
#define LOG_FILE          "/events.log"
#define LOG_MAX_BYTES     1048576   // 1MB rolling buffer
#define LOG_FLUSH_BATCH   20        // events to send per network burst

// ─── Timing ─────────────────────────────────────────────────────────
#define IDLE_BACKLIGHT_MS 15000     // OLED auto-off after N seconds idle
#define NO_FINGER_TIMEOUT_MS 30000  // return to idle if no finger placed
#define WIFI_RETRY_MS     30000
#define NTP_SYNC_MS       21600000 // 6h
#define HEARTBEAT_MS      300000   // 5min to backend
#define WATCHDOG_TIMEOUT_S 30      // brownout + hang protection

// ─── Backend (Firebase Cloud Functions HTTPS endpoint) ──────────────
// Fill in secrets.h.template → secrets.h with real values
#define BACKEND_HOST  "us-central1-YOUR-PROJECT.cloudfunctions.net"
#define BACKEND_PORT  443
#define API_ENROLL    "/deviceEnroll"
#define API_VERIFY    "/deviceEvent"
#define API_HEARTBEAT "/deviceHeartbeat"
#define API_PULL_TPL  "/devicePullTemplates"

// ─── NTP ────────────────────────────────────────────────────────────
#define NTP_SERVER_1  "pool.ntp.org"
#define NTP_SERVER_2  "ke.pool.ntp.org"   // Kenya NTP pool
#define NTP_TZ_OFFSET_SEC 3 * 3600         // EAT (UTC+3, no DST in Kenya)

// ─── Feature flags ──────────────────────────────────────────────────
#define ENABLE_OTA          1
#define ENABLE_BUZZER       1
#define ENABLE_RGB_LED      1
#define ENABLE_OFFLINE_LOG  1
#define ENABLE_RTC          1

#endif // CONFIG_H
