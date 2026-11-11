#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "eye_presets.h"

// Draws two eyes from a parameter set, with blink and look direction.
// Each eye is composed in a sprite and pushed in one blit: drawing straight to
// the panel made every frame show the clear-then-redraw pass as black streaks.
class EyeRenderer {
 public:
  void begin(TFT_eSPI *tft, int16_t cx, int16_t cy);
  void setEmotion(EyeEmotion emotion, bool immediate = false);
  void look(float x, float y);  // both in -1.0 .. 1.0
  void blinkNow();
  void update(uint32_t nowMs);

  // Drawn inside the right eye sprite, so it shares that blit and cannot be
  // erased between frames. nullptr hides it.
  void setOverlay(const char *text) { _overlay = text; }

  // 0..1 progress of a yawn. With no mouth on screen this only squeezes the
  // eyes, which is the part of a yawn that still reads at this size.
  void setYawn(float amount) { _yawn = constrain(amount, 0.0f, 1.0f); }

  // Set from the settings screen.
  void setEyeColour(uint16_t colour);

  EyeEmotion emotion() const { return _emotion; }

 private:
  void renderEye(int16_t cx, float openFactor, uint8_t side, uint32_t nowMs);
  static void fillRoundRect(TFT_eSprite &spr, int16_t x, int16_t y, int16_t w,
                            int16_t h, int16_t r, uint16_t colour);
  static void fillCircle(TFT_eSprite &spr, int16_t cx, int16_t cy, int16_t r,
                         uint16_t colour);
  static void drawBrow(TFT_eSprite &spr, int16_t cx, float yTop, float len,
                       float tilt, float curve, uint8_t side);
  static void drawClosedArc(TFT_eSprite &spr, int16_t cx, int16_t cy, float halfWidth,
                            float arc);

  TFT_eSPI *_tft = nullptr;
  TFT_eSprite *_spr[2] = {nullptr, nullptr};
  int16_t _cx = 160;
  int16_t _cy = 120;

  EyePreset _cur{};
  EyePreset _target{};
  EyeEmotion _emotion = EM_NORMAL;
  const char *_overlay = nullptr;
  float _yawn = 0.0f;

  float _lookX = 0.0f, _lookY = 0.0f;
  float _lookTargetX = 0.0f, _lookTargetY = 0.0f;

  bool _blinking = false;
  uint32_t _blinkStart = 0;
  uint32_t _nextBlink = 0;
  uint32_t _lastBlinkEnd = 0;
};
