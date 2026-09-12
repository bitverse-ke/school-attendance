/*
 * feedback.h — Buzzer and RGB LED for pass/fail/wait feedback
 */
#ifndef FEEDBACK_H
#define FEEDBACK_H

#include <Arduino.h>

namespace feedback {
    void begin();
    void success();   // green flash + 2 short beeps
    void failure();   // red flash + 1 long beep
    void processing();// blue solid while sensor working
    void idle();      // LEDs off
    void networkErr();// amber blink pattern
}

#endif
