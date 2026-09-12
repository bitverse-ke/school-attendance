#include "http_client.h"
#include "config.h"
#include "secrets.h"
#include "offline_queue.h"
#include "wifi_manager.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace http {

static WiFiClientSecure  client;
static HTTPClient        https;
static bool              lastOk = false;

bool begin() {
    // For prototype we skip cert verification. TODO: pin Google CA cert.
    // Get it via: openssl s_client -connect us-central1-YOUR.cloudfunctions.net:443 \
    //     -servername YOUR 2>/dev/null | openssl x509 -fingerprint -sha256 -noout
    client.setInsecure();
    lastOk = false;
    return true;
}

static bool postJson(const char* path, const String& body, String& response) {
    if (WiFi.status() != WL_CONNECTED) return false;
    if (wifiMgr::deviceId().isEmpty()) return false;

    String host = wifiMgr::backendHost();
    String url  = String("https://") + host + path;

    https.begin(client, url);
    https.addHeader("Content-Type", "application/json");
    https.addHeader("X-Device-Id", wifiMgr::deviceId());
    https.addHeader("X-Device-Key", DEVICE_API_KEY);
    https.setTimeout(8000);

    int code = https.POST(body);
    lastOk = (code >= 200 && code < 300);
    if (lastOk) {
        response = https.getString();
    } else {
        Serial.printf("[HTTP] %s → %d\n", path, code);
    }
    https.end();
    return lastOk;
}

bool sendEnroll(uint16_t fingerprintId, const char* studentId, const char* name) {
    StaticJsonDocument<256> doc;
    doc["type"]          = "enroll";
    doc["fingerprintId"] = fingerprintId;
    doc["studentId"]     = studentId;
    doc["name"]          = name;
    doc["epoch"]         = (uint32_t)(millis() / 1000);  // placeholder; backend resolves via its clock
    String body; serializeJson(doc, body);

    String resp;
    bool ok = postJson(API_ENROLL, body, resp);
    if (!ok) queue::enqueue(body);
    return ok;
}

bool sendVerify(uint16_t fingerprintId, uint16_t score, unsigned long epoch) {
    StaticJsonDocument<256> doc;
    doc["type"]          = "verify";
    doc["fingerprintId"] = fingerprintId;
    doc["score"]         = score;
    doc["epoch"]         = (uint32_t)epoch;
    doc["deviceId"]      = wifiMgr::deviceId();
    String body; serializeJson(doc, body);

    String resp;
    bool ok = postJson(API_VERIFY, body, resp);
    if (!ok) queue::enqueue(body);
    return ok;
}

bool sendHeartbeat(unsigned long uptimeSec, int rssi, unsigned long epoch) {
    StaticJsonDocument<256> doc;
    doc["type"]      = "heartbeat";
    doc["uptimeSec"] = (uint32_t)uptimeSec;
    doc["rssi"]      = rssi;
    doc["epoch"]     = (uint32_t)epoch;
    doc["deviceId"]  = wifiMgr::deviceId();
    String body; serializeJson(doc, body);

    String resp;
    return postJson(API_HEARTBEAT, body, resp);
}

bool pullTemplates(unsigned long sinceEpoch, String& outJsonList) {
    StaticJsonDocument<128> doc;
    doc["sinceEpoch"] = (uint32_t)sinceEpoch;
    doc["deviceId"]   = wifiMgr::deviceId();
    String body; serializeJson(doc, body);

    String resp;
    if (!postJson(API_PULL_TPL, body, resp)) return false;
    outJsonList = resp;
    return true;
}

bool lastRequestOk() { return lastOk; }

size_t flushQueue() {
    size_t sent = 0;
    queue::rewind();
    String line;
    while (queue::tryNext(line)) {
        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, line)) {
            queue::commit();   // skip garbage
            continue;
        }
        const char* type = doc["type"] | "";
        String resp;
        bool ok = false;
        if      (strcmp(type, "verify")  == 0) ok = postJson(API_VERIFY,  line, resp);
        else if (strcmp(type, "enroll")  == 0) ok = postJson(API_ENROLL,  line, resp);
        else if (strcmp(type, "heartbeat")==0) ok = postJson(API_HEARTBEAT,line, resp);

        if (!ok) break;        // bail; retry next loop

        queue::commit();
        sent++;
        if (sent >= LOG_FLUSH_BATCH) break;  // throttle
    }
    return sent;
}

} // namespace