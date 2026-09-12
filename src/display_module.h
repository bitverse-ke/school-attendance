/*
 * display_module.h — SSD1306 OLED UI
 */
#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <Arduino.h>
#include "state.h"

namespace display {
    void begin();
    void showBoot(const char* fwVersion);
    void showConnectingWifi();
    void showOffline(const char* reason);
    void showIdle(unsigned long epoch);
    void showScanning();
    void showMatch(uint16_t studentId, const char* name, uint16_t score);
    void showNoMatch();
    void showEnrolling(const char* step); // "PLACE FINGER" -> "PLACE AGAIN" -> "DONE"
    void showError(const char* msg);
    void tick(); // call in loop; handles auto-sleep
}

#endif
