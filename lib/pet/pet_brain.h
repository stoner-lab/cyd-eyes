#pragma once

#include "eye_presets.h"
#include "pet_state.h"

// One place that turns needs into a face. Keeping this pure (no drawing, no IO)
// is what lets the eye engine and the pet engine stay unaware of each other.
enum LedMood : uint8_t {
  LED_OFF,
  LED_NEUTRAL,
  LED_HAPPY,
  LED_HUNGRY,
  LED_SLEEPY,
  LED_SAD,
  LED_ANGRY
};

inline EyeEmotion emotionFor(const PetSnapshot &s) {
  if (s.asleep) return EM_ASLEEP;
  if (s.sleepiness > 80.0f) return EM_SLEEPY;
  if (s.hunger > 78.0f) return EM_HUNGRY;
  if (s.joy > 82.0f) return EM_HAPPY;
  if (s.joy < 22.0f) return EM_SAD;
  if (s.hunger > 60.0f) return EM_BORED;
  if (s.joy > 62.0f && s.hunger < 35.0f) return EM_PLAYFUL;
  if (s.sleepiness > 55.0f) return EM_SLEEPY;
  if (s.joy < 45.0f) return EM_BORED;
  return EM_NORMAL;
}

inline LedMood ledFor(EyeEmotion e) {
  switch (e) {
    case EM_HAPPY:
    case EM_PLAYFUL:
      return LED_HAPPY;
    case EM_HUNGRY:
      return LED_HUNGRY;
    case EM_SLEEPY:
    case EM_ASLEEP:
      return LED_SLEEPY;
    case EM_SAD:
      return LED_SAD;
    case EM_ANGRY:
      return LED_ANGRY;
    case EM_NORMAL:
      return LED_NEUTRAL;
    default:
      return LED_OFF;
  }
}
