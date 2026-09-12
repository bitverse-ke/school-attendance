/*
 * wifi_manager.h — WiFiManager-based connection with captive portal
 *
 * First boot: opens "Attendance-Setup" AP + portal at 192.168.4.1
 * School admin enters WiFi creds + Device ID + optional backend override.
 * Subsequent boots: auto-connect, 30s timeout → captive portal as fallback.
 */
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

namespace wifiMgr {
    bool begin();
    bool isConnected();
    void loop();                // call in main loop
    String deviceId();
    String backendHost();       // may override BACKEND_HOST via portal
    void resetSettings();       // force portal on next boot
}

#endif