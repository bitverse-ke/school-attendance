#include "feedback.h"
#include "config.h"

namespace feedback {

static bool inited = false;

void begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_RED,   OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE,  OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_RED,   HIGH);  // active-low LEDs common on dev boards;
    digitalWrite(LED_GREEN, HIGH);  // set HIGH = OFF, drive LOW to light
    digitalWrite(LED_BLUE,  HIGH);
    inited = true;
}

static void tone(uint16_t freq, uint16_t ms) {
#if ENABLE_BUZZER
    if (!inited) return;
    tone(BUZZER_PIN, freq, ms);
    delay(ms);
    noTone(BUZZER_PIN);
#endif
}

static void led(uint8_t pin, bool on) {
#if ENABLE_RGB_LED
    digitalWrite(pin, on ? LOW : HIGH);  // active-low
#endif
}

void success() {
    led(LED_GREEN, true);
    tone(2000, 80);
    delay(60);
    tone(2500, 80);
    led(LED_GREEN, false);
}

void failure() {
    led(LED_RED, true);
    tone(500, 400);
    led(LED_RED, false);
}

void processing() {
    led(LED_BLUE, true);
}

void idle() {
    led(LED_RED, false);
    led(LED_GREEN, false);
    led(LED_BLUE, false);
    noTone(BUZZER_PIN);
}

void networkErr() {
    // triple amber blink (red+green = amber on RGB)
    for (int i = 0; i < 3; i++) {
        led(LED_RED, true); led(LED_GREEN, true);
        delay(80);
        led(LED_RED, false); led(LED_GREEN, false);
        delay(120);
    }
}

} // namespace
