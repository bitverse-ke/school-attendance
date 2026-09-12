/*
 * fingerprint_module.h — R307 sensor wrapper
 *
 * R307 stores 1000 templates on-chip and does matching locally — ESP32
 * only sees "matched ID N with score S" or "no match". Templates are
 * NEVER returned to ESP32 (and therefore never to WiFi) by default.
 * This is the security model that makes R307 safe for a school: the
 * fingerprint image and template never leaves the sealed sensor.
 */
#ifndef FINGERPRINT_MODULE_H
#define FINGERPRINT_MODULE_H

#include <Arduino.h>

namespace fp {
    bool        begin();              // init UART, verify sensor comms
    bool        isReady();            // comms still OK?
    uint16_t    templateCount();      // # stored templates
    const char* model();              // sensor model string

    // Capture pipeline
    bool        waitForFinger(uint32_t timeoutMs);
    bool        capture();            // image + char-store-1 conversion

    // Match: returns ID (1..N) or 0 if no match. Score on success.
    int16_t     identify(uint16_t& outScore);

    // Enrol: takes 2 captures, creates model, stores at idSlot.
    // Returns 0 on success, negative on error.
    int16_t     enroll(uint16_t idSlot);

    bool        deleteId(uint16_t id);
    bool        emptyDb();
}

#endif