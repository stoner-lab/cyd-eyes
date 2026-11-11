#pragma once

#include <Arduino.h>

// Pins come from the CYD pinout (witnessmenow/ESP32-Cheap-Yellow-Display).
// The RGB LED is active LOW, the LDR sits on an input-only GPIO.
namespace board {

const int PIN_LED_R = 4;
const int PIN_LED_G = 16;
const int PIN_LED_B = 17;
const int PIN_LDR = 34;
const int PIN_BTN = 0;  // BOOT button, used as the touch substitute until
                        // the XPT2046 is wired up and calibrated.

inline void ledWrite(bool r, bool g, bool b) {
  digitalWrite(PIN_LED_R, r ? LOW : HIGH);
  digitalWrite(PIN_LED_G, g ? LOW : HIGH);
  digitalWrite(PIN_LED_B, b ? LOW : HIGH);
}

inline void begin() {
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  ledWrite(false, false, false);

  pinMode(PIN_BTN, INPUT_PULLUP);
  analogReadResolution(12);
}

inline int ldr() { return analogRead(PIN_LDR); }

}  // namespace board
