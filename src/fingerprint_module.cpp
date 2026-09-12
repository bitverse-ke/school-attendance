#include "fingerprint_module.h"
#include "config.h"
#include <HardwareSerial.h>
#include <Adafruit_Fingerprint.h>

namespace fp {

static HardwareSerial   fpSerial(2);  // UART2
static Adafruit_Fingerprint finger(&fpSerial);

static char modelBuf[24] = "?";
static bool ready = false;

bool begin() {
    fpSerial.begin(FP_BAUD, SERIAL_8N1, FP_RX_PIN, FP_TX_PIN);
    delay(100);  // R307 needs settle time after power-on

    finger.begin(FP_BAUD);
    if (finger.verifyPassword() != FINGERPRINT_OK) {
        Serial.println(F("[FP] Password verify FAILED — wiring?"));
        ready = false;
        return false;
    }

    finger.getModel();  // populates finger.model (Model struct)
    // R307 Model struct: { status_reg, system_id, capacity, security_level,
    //                     device_addr, packet_len, baud_rate }
    uint16_t cap = finger.model ? finger.model->capacity : 0;
    uint16_t sec = finger.model ? finger.model->security_level : 0;
    snprintf(modelBuf, sizeof(modelBuf), "cap=%u sec=%u", cap, sec);
    Serial.printf("[FP] Ready. %s templates=%u\n", modelBuf, templateCount());
    ready = true;
    return true;
}

bool isReady() {
    if (!ready) return false;
    return finger.verifyPassword() == FINGERPRINT_OK;
}

const char* model() { return modelBuf; }

uint16_t templateCount() {
    if (!ready) return 0;
    finger.getTemplateCount();
    return finger.templateCount;
}

bool waitForFinger(uint32_t timeoutMs) {
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (finger.getImage() == FINGERPRINT_OK) return true;
        delay(50);
    }
    return false;
}

bool capture() {
    // Sensor already has image (from waitForFinger). Convert to char-store-1.
    if (finger.image2Tz(1) != FINGERPRINT_OK) {
        Serial.println(F("[FP] image2Tz FAILED — image too messy"));
        return false;
    }
    return true;
}

int16_t identify(uint16_t& outScore) {
    if (finger.fingerSearch() != FINGERPRINT_OK) {
        return 0;  // no match
    }
    outScore = finger.confidence;
    return finger.fingerID;
}

int16_t enroll(uint16_t idSlot) {
    // Two-pass enrolment: capture, remove finger, capture again, create model
    Serial.println(F("[FP] ENROL: place finger #1"));
    while (finger.getImage() != FINGERPRINT_OK) delay(50);
    if (finger.image2Tz(1) != FINGERPRINT_OK) return -1;

    Serial.println(F("[FP] ENROL: remove finger"));
    while (finger.getImage() != FINGERPRINT_NOFINGER) delay(50);
    delay(500);

    Serial.println(F("[FP] ENROL: place finger #2"));
    while (finger.getImage() != FINGERPRINT_OK) delay(50);
    if (finger.image2Tz(2) != FINGERPRINT_OK) return -2;

    if (finger.createModel() != FINGERPRINT_OK) return -3;
    if (finger.storeModel(idSlot) != FINGERPRINT_OK) return -4;

    Serial.printf("[FP] ENROL: stored at slot %u\n", idSlot);
    return 0;
}

bool deleteId(uint16_t id) {
    return finger.deleteModel(id) == FINGERPRINT_OK;
}

bool emptyDb() {
    return finger.emptyDatabase() == FINGERPRINT_OK;
}

} // namespace