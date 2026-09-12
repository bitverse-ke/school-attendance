#include "ntp_sync.h"
#include "config.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <RTClib.h>

namespace ntp {

static WiFiUDP   ntpUDP;
static NTPClient ntpClient(ntpUDP, NTP_SERVER_1, NTP_TZ_OFFSET_SEC, 60000);
static RTC_DS3231 rtc;
static bool rtcOk = false;
static unsigned long lastSyncMs = 0;

bool begin() {
    Wire.beginTransmission(RTC_ADDR);
    bool ack = (Wire.endTransmission() == 0);
    if (!ack) {
        Serial.println(F("[NTP] DS3231 not found on I2C"));
        rtcOk = false;
    } else {
        rtc.begin();
        rtcOk = true;
        if (rtc.lostPower()) {
            Serial.println(F("[NTP] RTC lost power, will re-sync from NTP"));
        }
    }
    return rtcOk;
}

bool sync() {
    if (WiFi.status() != WL_CONNECTED) return false;
    if (!ntpClient.forceUpdate()) return false;

    unsigned long e = ntpClient.getEpochTime();
    if (rtcOk) rtc.adjust(DateTime(e));

    lastSyncMs = millis();
    Serial.printf("[NTP] Synced: %lu\n", e);
    return true;
}

bool syncIfStale() {
    if (millis() - lastSyncMs > NTP_SYNC_MS) return sync();
    return false;
}

unsigned long epoch() {
    if (rtcOk) {
        DateTime now = rtc.now();
        return now.unixtime();
    }
    return ntpClient.getEpochTime();
}

bool rtcPresent() { return rtcOk; }

} // namespace