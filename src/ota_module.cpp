#include "ota_module.h"
#include "config.h"
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <Update.h>

namespace ota {

#if ENABLE_OTA
static bool started = false;

bool begin(const char* password) {
    if (WiFi.status() != WL_CONNECTED) return false;

    ArduinoOTA.setHostname(WiFi.getHostname());
    ArduinoOTA.setPassword(password);

    ArduinoOTA.onStart([]() {
        Serial.println(F("[OTA] Start"));
    });
    ArduinoOTA.onEnd([]() {
        Serial.println(F("[OTA] Done"));
    });
    ArduinoOTA.onProgress([](unsigned int p, unsigned int total) {
        Serial.printf("[OTA] %u/%u\n", p, total);
    });
    ArduinoOTA.onError([](ota_error_t err) {
        Serial.printf("[OTA] Error %u\n", err);
    });

    ArduinoOTA.begin();
    started = true;
    Serial.println(F("[OTA] Ready"));
    return true;
}

void loop() {
    if (started) ArduinoOTA.handle();
}
#else
bool begin(const char*) { return false; }
void loop() {}
#endif

} // namespace