#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

// 25/5 timer. Uses millis(), so no RTC and no network are needed; the count keeps
// running while the screen is dimmed or dark.
class Pomodoro {
 public:
  void begin(TFT_eSPI *tft);

  void toggle(uint32_t now);  // start or pause
  void reset();
  void tick(uint32_t now);    // phase changes and redraws on the second

  bool running() const { return _running; }
  void draw();

 private:
  uint32_t elapsed(uint32_t now) const;
  uint32_t remaining(uint32_t now) const;
  void startPhase(uint32_t now, uint8_t phase);

  TFT_eSPI *_tft = nullptr;
  uint32_t _phaseMs = 0;
  uint32_t _accumMs = 0;    // elapsed before the current run
  uint32_t _startedAt = 0;
  uint32_t _lastShown = 0xFFFFFFFF;
  bool _running = false;
  uint8_t _phase = 0;       // 0 = work, 1 = break
  uint8_t _sessions = 0;
};
