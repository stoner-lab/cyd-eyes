#pragma once

#include <Arduino.h>

// Minimal bit-banged XPT2046 driver.
//
// The CYD wires the touch panel on its own four wires (25/32/33/36/39) while the
// display owns HSPI and the SD card owns VSPI, so neither hardware SPI port is
// available. Bit-banging at this speed is fine: touch is polled at 50 Hz and the
// chip only needs 2.5 MHz.
class XPT2046 {
 public:
  void begin();
  bool pressed();

  // Reads both axes. Returns false when no pen is down.
  bool read(uint16_t &rawX, uint16_t &rawY);

  // Last mapped position in screen pixels (320x240 landscape).
  int16_t screenX() const { return _sx; }
  int16_t screenY() const { return _sy; }

 private:
  uint16_t transfer(uint8_t cmd);
  bool sampleStable(uint8_t cmd, uint16_t &out);

  int16_t _sx = 0;
  int16_t _sy = 0;
};
