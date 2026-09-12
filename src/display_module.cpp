#include "display_module.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace display {

static Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);
static unsigned long lastActivityMs = 0;
static bool sleeping = false;

// Forward decl so screens below can use it
static const char* formatTime(unsigned long epoch);

void wake() {
    if (sleeping) {
        oled.ssd1306_command(SSD1306_DISPLAYON);
        sleeping = false;
    }
    lastActivityMs = millis();
}

void tick() {
    if (!sleeping && (millis() - lastActivityMs) > IDLE_BACKLIGHT_MS) {
        oled.ssd1306_command(SSD1306_DISPLAYOFF);
        sleeping = true;
    }
}

void begin() {
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println(F("[OLED] init FAILED — check I2C wiring"));
        return;
    }
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.display();
    wake();
}

// Accepts either const char* or F()-string via Print's overloads
template <typename T>
static void drawHeader(T title) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.print(F("Attendance "));
    oled.print(FIRMWARE_VERSION);
    oled.drawLine(0, 9, OLED_W, 9, SSD1306_WHITE);
    oled.setCursor(0, 12);
    oled.print(title);
}

void showBoot(const char* fwVersion) {
    wake();
    drawHeader(F("Booting"));
    oled.setCursor(0, 26);
    oled.print(F("FW: ")); oled.println(fwVersion);
    oled.setCursor(0, 38);
    oled.print(F("HW: ")); oled.println(HARDWARE_MODEL);
    oled.display();
}

void showConnectingWifi() {
    wake();
    drawHeader(F("WiFi"));
    oled.setCursor(0, 28);
    oled.println(F("Connecting..."));
    oled.setCursor(0, 42);
    oled.println(F("(portal if no creds)"));
    oled.display();
}

void showOffline(const char* reason) {
    wake();
    drawHeader(F("Offline mode"));
    oled.setCursor(0, 28);
    oled.println(reason);
    oled.setCursor(0, 44);
    oled.println(F("Logs queued locally"));
    oled.display();
}

void showIdle(unsigned long epoch) {
    wake();
    drawHeader(F("Ready - place finger"));
    oled.setTextSize(2);
    oled.setCursor(20, 30);
    oled.print(formatTime(epoch));
    oled.display();
}

void showScanning() {
    wake();
    drawHeader(F("Scanning..."));
    oled.setTextSize(2);
    oled.setCursor(36, 28);
    oled.print(F("..."));
    oled.display();
}

void showMatch(uint16_t studentId, const char* name, uint16_t score) {
    wake();
    drawHeader(F("MATCH"));
    oled.setTextSize(1);
    oled.setCursor(0, 24);
    oled.print(F("ID:")); oled.print(studentId);
    oled.setCursor(70, 24);
    oled.print(F("sc:")); oled.print(score);
    oled.setTextSize(2);
    oled.setCursor(0, 38);
    oled.print(name);
    oled.display();
}

void showNoMatch() {
    wake();
    drawHeader(F("NO MATCH"));
    oled.setTextSize(1);
    oled.setCursor(0, 28);
    oled.println(F("Finger not recognised."));
    oled.setCursor(0, 44);
    oled.println(F("See admin to enrol."));
    oled.display();
}

void showEnrolling(const char* step) {
    wake();
    drawHeader(F("ENROL NEW"));
    oled.setTextSize(2);
    oled.setCursor(0, 30);
    oled.print(step);
    oled.display();
}

void showError(const char* msg) {
    wake();
    drawHeader(F("ERROR"));
    oled.setCursor(0, 26);
    oled.println(msg);
    oled.display();
}

// Tiny inline formatter to avoid pulling Time lib here
static char buf[9];
static const char* formatTime(unsigned long epoch) {
    if (epoch < 1000000) {
        buf[0] = '-'; buf[1] = '-'; buf[2] = ':';
        buf[3] = '-'; buf[4] = '-'; buf[5] = 0;
        return buf;
    }
    unsigned long h = (epoch % 86400L) / 3600;
    unsigned long m = (epoch % 3600) / 60;
    unsigned long s = epoch % 60;
    buf[0] = '0' + (h / 10); buf[1] = '0' + (h % 10);
    buf[2] = ':';
    buf[3] = '0' + (m / 10); buf[4] = '0' + (m % 10);
    buf[5] = ':';
    buf[6] = '0' + (s / 10); buf[7] = '0' + (s % 10);
    buf[8] = 0;
    return buf;
}

} // namespace
