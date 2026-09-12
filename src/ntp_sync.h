/*
 * ntp_sync.h — NTP + DS3231 RTC fallback
 *
 * NTP syncs every NTP_SYNC_MS when WiFi is up. RTC keeps time across
 * power cycles. ESP32 internal RTC drifts ~5 min/week; DS3231 <2 ppm.
 */
#ifndef NTP_SYNC_H
#define NTP_SYNC_H

#include <Arduino.h>

namespace ntp {
    bool begin();
    bool sync();                 // force NTP sync (returns true on success)
    bool syncIfStale();          // sync only if older than NTP_SYNC_MS
    unsigned long epoch();       // current unix time (UTC)
    bool rtcPresent();           // DS3231 detected
}

#endif