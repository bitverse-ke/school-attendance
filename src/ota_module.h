/*
 * ota_module.h — ArduinoOTA wrapper for remote firmware updates
 */
#ifndef OTA_MODULE_H
#define OTA_MODULE_H

#include <Arduino.h>

namespace ota {
    bool begin(const char* password);
    void loop();
}

#endif