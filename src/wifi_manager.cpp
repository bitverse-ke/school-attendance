#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

namespace wifiMgr {

static WiFiManager wm;
static String customDeviceId = DEVICE_ID_DEFAULT;
static String customBackendHost = BACKEND_HOST;
static bool connected = false;

static void saveConfigCallback() {
    Serial.println(F("[WIFI] Config saved → rebooting"));
    delay(1000);
    ESP.restart();
}

// WiFiManager custom params: Device ID + Backend host override
static void addCustomParams() {
    WiFiManagerParameter p_devid("devid", "Device ID", customDeviceId.c_str(), 32);
    WiFiManagerParameter p_host("host", "Backend host (optional)",
                                customBackendHost.c_str(), 64);
    wm.addParameter(&p_devid);
    wm.addParameter(&p_host);

    // After wm.process() returns, read them back
    customDeviceId = String(p_devid.getValue());
    if (customDeviceId.isEmpty()) customDeviceId = DEVICE_ID_DEFAULT;
    customBackendHost = String(p_host.getValue());
    if (customBackendHost.isEmpty()) customBackendHost = BACKEND_HOST;
}

bool begin() {
    wm.setAPCallback([](WiFiManager *wmPtr) {
        Serial.println(F("[WIFI] Captive portal active"));
        Serial.print(F("[WIFI] AP: Attendance-Setup  IP: 192.168.4.1\n"));
    });
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setConfigPortalTimeout(180);  // 3 min before falling back
    wm.setConnectTimeout(20);
    wm.setHostname(customDeviceId);

    addCustomParams();

    connected = wm.autoConnect("Attendance-Setup");
    if (connected) {
        Serial.printf("[WIFI] Connected: %s RSSI=%d IP=%s\n",
                      WiFi.SSID().c_str(), WiFi.RSSI(),
                      WiFi.localIP().toString().c_str());
    } else {
        Serial.println(F("[WIFI] Connection FAILED — running offline"));
    }
    return connected;
}

bool isConnected() {
    connected = (WiFi.status() == WL_CONNECTED);
    return connected;
}

void loop() {
    // WiFiManager has no per-loop handler for connected state.
    // Manual reconnect every WIFI_RETRY_MS.
    static unsigned long lastCheck = 0;
    if (!connected && millis() - lastCheck > WIFI_RETRY_MS) {
        lastCheck = millis();
        Serial.println(F("[WIFI] Reconnecting..."));
        WiFi.reconnect();
    }
    if (connected && millis() - lastCheck > 60000) {
        lastCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            connected = false;
            Serial.println(F("[WIFI] Lost connection"));
        }
    }
}

String deviceId()       { return customDeviceId; }
String backendHost()    { return customBackendHost; }

void resetSettings() {
    wm.resetSettings();
}

} // namespace