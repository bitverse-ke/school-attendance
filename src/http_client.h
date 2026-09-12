/*
 * http_client.h — Firebase Cloud Functions HTTPS client
 *
 * Endpoints (defined in docs/FIREBASE_API.md):
 *   POST /deviceEnroll      — local enrolment → sync to backend
 *   POST /deviceEvent       — verification event
 *   POST /deviceHeartbeat   — periodic liveness ping
 *   POST /devicePullTemplates — fetch recently added templates
 *
 * Auth: per-device API key in X-Device-Key header. Backend validates
 *       against the device registry in Firestore.
 */
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <Arduino.h>

namespace http {
    bool begin();

    // Returns true if request succeeded (HTTP 2xx).
    bool sendEnroll(uint16_t fingerprintId, const char* studentId, const char* name);
    bool sendVerify(uint16_t fingerprintId, uint16_t score, unsigned long epoch);
    bool sendHeartbeat(unsigned long uptimeSec, int rssi, unsigned long epoch);
    bool pullTemplates(unsigned long sinceEpoch, String& outJsonList);

    // Lightweight check for UI: returns true if last request succeeded.
    bool lastRequestOk();

    // Convenience: try to drain the offline queue, sending each event.
    // Stops on first failure (likely WiFi/HTTP issue).
    // Returns count of events successfully sent.
    size_t flushQueue();
}

#endif