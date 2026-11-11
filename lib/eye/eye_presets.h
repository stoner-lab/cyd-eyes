#pragma once

#include <stdint.h>

// Expressions are parameter sets drawn per frame, not pre-rendered frames.
enum EyeEmotion : uint8_t {
  EM_NORMAL = 0,
  EM_HAPPY,
  EM_PLAYFUL,
  EM_HUNGRY,
  EM_SLEEPY,
  EM_ASLEEP,
  EM_SAD,
  EM_ANGRY,
  EM_SURPRISED,
  EM_DAZED,
  EM_CONTENT,
  EM_COUNT
};

struct EyePreset {
  float openness;   // vertical squeeze, 1.0 = fully open
  float rx;         // half width in px
  float ry;         // half height in px
  float pupil;      // pupil radius in px
  float dy;         // eye centre offset from the base line
  float spacing;    // eye centre distance from screen centre
  float browTilt;   // degrees, positive lowers the inner end
  float browLift;   // px above the top edge of the eye
  float browLen;    // px
  float browCurve;  // px of arch, positive bows upward
  float lidDrop;    // 0..1 share of the eye hidden by the upper lid
  float closedArc;  // >1 draws a closed arc instead of an eye body
  const char *thought;
};

extern const EyePreset EYE_PRESETS[EM_COUNT];
