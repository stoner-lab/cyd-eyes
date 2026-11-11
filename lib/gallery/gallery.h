#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

// Photo viewer reading /cyd/*.jpg from the microSD slot.
//
// The CYD wires the card on VSPI (5/18/19/23) while the display owns HSPI and
// the touch panel is bit-banged, so the card gets a bus of its own.
class Gallery {
 public:
  bool begin();
  bool ready() const { return _ready; }
  uint16_t count();
  // Draws a random image centred on the screen. Returns false when the card is
  // missing or the folder holds no pictures.
  bool showRandom(TFT_eSPI *tft);

 private:
  bool _ready = false;
};
