/*
 * main.cpp — School-attendance edge device firmware
 *
 * Hardware: ESP32-WROOM-32 + R307 fingerprint sensor + SSD1306 OLED +
 *           DS3231 RTC + RGB LED + buzzer + LittleFS log
 *
 * Architecture: Hybrid (ESP32 edge + Firebase cloud).
 *   - R307 does matching locally on its own chip; templates never
 *     leave the sealed sensor. ESP32 only sees "matched ID N".
 *   - Verification events POST to Firebase Cloud Function for logging.
 *   - On WiFi outage, events queue to LittleFS and flush on reconnect.
 *   - OTA updates via ArduinoOTA when on WiFi.
 *
 * State machine: see state.h
 */

#include <Arduino.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "state.h"
#include "feedback.h"
#include "display_module.h"
#include "fingerprint_module.h"
#include "wifi_manager.h"
#include "ntp_sync.h"
#include "offline_queue.h"
#include "http_client.h"
#include "ota_module.h"

AppContext ctx;

// ─── Setup ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.printf("\n=== school-attendance %s hw=%s ===\n",
                  FIRMWARE_VERSION, HARDWARE_MODEL);

    // 30s hardware watchdog (resets ESP32 if main loop hangs)
    esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);

    feedback::begin();
    display::begin();
    display::showBoot(FIRMWARE_VERSION);

    if (!fp::begin()) {
        display::showError("Fingerprint sensor missing");
        // continue; we may still want WiFi config portal accessible
    }

    queue::begin();

    display::showConnectingWifi();
    bool wifiOk = wifiMgr::begin();
    ctx.wifiUp  = wifiOk;

    if (wifiOk) {
        ntp::begin();
        if (!ntp::sync()) {
            Serial.println(F("[MAIN] NTP sync failed; using RTC fallback"));
        }
        ctx.ntpUp = (ntp::epoch() > 1000000);
        http::begin();
        ota::begin(DEVICE_API_KEY);  // reuse device key as OTA password (prototype)
    } else {
        display::showOffline("WiFi unavailable");
    }

    ctx.queueDirty = (queue::pendingBytes() > 0);
    ctx.state      = State::IDLE;
    ctx.stateMs    = millis();

    Serial.printf("[MAIN] Boot complete. state=%s wifi=%d ntp=%d queue=%u bytes\n",
                  stateName(ctx.state), ctx.wifiUp, ctx.ntpUp,
                  queue::pendingBytes());
}

// ─── Serial command handler (admin laptop) ────────────────────────
static void handleSerialCommand() {
    if (!Serial.available()) return;
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) return;

    if (cmd == "STATUS") {
        Serial.printf("state=%s wifi=%d ntp=%d queue=%u fp=%s count=%u\n",
                      stateName(ctx.state), ctx.wifiUp, ctx.ntpUp,
                      queue::pendingBytes(), fp::model(), fp::templateCount());
    } else if (cmd == "TIME") {
        Serial.printf("epoch=%lu\n", ntp::epoch());
    } else if (cmd == "RESET") {
        ESP.restart();
    } else if (cmd.startsWith("ENROL ")) {
        uint16_t slot = cmd.substring(6).toInt();
        if (slot < 1 || slot > 999) {
            Serial.println(F("ENROL: slot must be 1..999"));
            return;
        }
        Serial.printf("Starting enrolment at slot %u\n", slot);
        display::showEnrolling("PLACE FINGER");
        int rc = fp::enroll(slot);
        if (rc == 0) {
            feedback::success();
            display::showEnrolling("DONE");
            Serial.printf("Enrolment OK at slot %u\n", slot);
            // Note: send to backend with slot number; admin dashboard
            // maps fingerprintId → studentId via admin enrolment UI.
        } else {
            feedback::failure();
            display::showError("ENROL FAILED");
            Serial.printf("Enrolment failed rc=%d\n", rc);
        }
        delay(2000);
        ctx.state = State::IDLE;
    } else if (cmd == "EMPTY") {
        Serial.println(F("Wiping fingerprint DB!"));
        fp::emptyDb();
    } else if (cmd.startsWith("DEL ")) {
        uint16_t slot = cmd.substring(4).toInt();
        if (fp::deleteId(slot)) Serial.printf("Deleted slot %u\n", slot);
        else Serial.println(F("Delete failed"));
    } else if (cmd == "WIFIRESET") {
        wifiMgr::resetSettings();
        ESP.restart();
    } else {
        Serial.println(F("Commands: STATUS, TIME, ENROL <1-999>, DEL <id>, EMPTY, RESET, WIFIRESET"));
    }
}

// ─── Main loop ─────────────────────────────────────────────────────
static unsigned long lastFingerPoll = 0;
static unsigned long lastHeartbeat  = 0;
static unsigned long lastNtpCheck   = 0;

void loop() {
    esp_task_wdt_reset();
    handleSerialCommand();
    wifiMgr::loop();
    display::tick();
    ota::loop();

    // Periodic: re-check NTP
    if (ctx.wifiUp && millis() - lastNtpCheck > NTP_SYNC_MS) {
        lastNtpCheck = millis();
        ntp::syncIfStale();
        ctx.ntpUp = (ntp::epoch() > 1000000);
    }

    switch (ctx.state) {

        case State::IDLE: {
            display::showIdle(ntp::epoch());

            // Heartbeat
            if (ctx.wifiUp && ctx.ntpUp &&
                millis() - lastHeartbeat > HEARTBEAT_MS) {
                lastHeartbeat = millis();
                if (!http::sendHeartbeat(millis() / 1000,
                                         WiFi.RSSI(),
                                         ntp::epoch())) {
                    Serial.println(F("[MAIN] heartbeat failed; queueing"));
                }
            }

            // Periodic queue flush
            if (ctx.wifiUp && ctx.queueDirty) {
                ctx.state     = State::FLUSHING_QUEUE;
                ctx.stateMs   = millis();
                break;
            }

            // Poll sensor for finger (every ~200ms)
            if (millis() - lastFingerPoll > 200) {
                lastFingerPoll = millis();
                if (fp::waitForFinger(150)) {
                    feedback::processing();
                    ctx.state   = State::CAPTURING;
                    ctx.stateMs = millis();
                }
            }

            // Idle return-to-zero guard (in case stateMs never updates)
            if (millis() - ctx.stateMs > NO_FINGER_TIMEOUT_MS) {
                ctx.stateMs = millis();
            }
            break;
        }

        case State::CAPTURING:
            if (!fp::capture()) {
                feedback::failure();
                display::showError("Capture failed");
                delay(1500);
                feedback::idle();
                ctx.state   = State::IDLE;
                ctx.stateMs = millis();
                break;
            }
            ctx.state   = State::MATCHING;
            ctx.stateMs = millis();
            break;

        case State::MATCHING: {
            uint16_t score = 0;
            int16_t id = fp::identify(score);
            if (id == 0) {
                feedback::failure();
                display::showNoMatch();
                delay(2000);
                feedback::idle();
                ctx.state   = State::IDLE;
                ctx.stateMs = millis();
            } else {
                ctx.lastMatchId    = id;
                ctx.lastMatchScore = score;
                snprintf(ctx.lastMatchName, sizeof(ctx.lastMatchName),
                         "ID:%u", id);
                display::showMatch(id, ctx.lastMatchName, score);
                feedback::success();
                if (ntp::epoch() > 1000000) {
                    http::sendVerify(id, score, ntp::epoch());
                }
                ctx.state   = State::SHOWING_RESULT;
                ctx.stateMs = millis();
            }
            break;
        }

        case State::SHOWING_RESULT:
            if (millis() - ctx.stateMs > 2500) {
                feedback::idle();
                ctx.state   = State::IDLE;
                ctx.stateMs = millis();
            }
            break;

        case State::FLUSHING_QUEUE: {
            feedback::processing();
            size_t sent = http::flushQueue();
            feedback::idle();
            Serial.printf("[MAIN] Flushed %u queued events\n", sent);
            if (queue::pendingBytes() == 0) ctx.queueDirty = false;
            ctx.state   = State::IDLE;
            ctx.stateMs = millis();
            break;
        }

        case State::ERROR_RECOVERY:
            if (millis() - ctx.stateMs > 10000) {
                if (fp::begin()) {
                    ctx.errorCount = 0;
                    ctx.state      = State::IDLE;
                    ctx.stateMs    = millis();
                } else if (++ctx.errorCount > 5) {
                    display::showError("FP DEAD - power cycle");
                    while (true) { esp_task_wdt_reset(); delay(1000); }
                } else {
                    ctx.stateMs = millis();
                }
            }
            break;

        default:
            ctx.state   = State::IDLE;
            ctx.stateMs = millis();
            break;
    }

    delay(10);  // yield to other tasks
}
