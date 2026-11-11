#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

// Three settings that used to require editing a constant and reflashing:
// backlight level, idle timeout before the screen switches off, and eye colour.
// Values live in NVS so they survive a power cut.
class Settings {
 public:
  void begin(TFT_eSPI *tft);
  void draw();
  int hit(int16_t x, int16_t y) const;  // 0..8 button, -1 nothing
  void apply(int id);                   // changes one value and redraws

  int fullDuty() const;
  int dimDuty() const;
  uint32_t offMs() const;
  uint16_t eyeColour() const;

 private:
  void save();

  TFT_eSPI *_tft = nullptr;
  uint8_t _brightness = 0;  // 0 = 100%, 1 = 40%, 2 = 20%
  uint8_t _timeout = 1;     // 0 = 1 min, 1 = 3 min, 2 = 10 min
  uint8_t _colour = 0;      // 0 cyan, 1 green, 2 amber, 3 white
};
