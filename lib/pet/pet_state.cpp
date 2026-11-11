#include "pet_state.h"

namespace {
// Rates per minute. Slow enough that a check every few hours matters,
// fast enough that the face changes during a single session.
const float HUNGER_PER_MIN = 0.9f;
const float JOY_PER_MIN = 0.6f;
const float SLEEP_PER_MIN = 0.7f;

// Asleep the pet still gets hungry, but recovers rest.
const float HUNGER_ASLEEP_PER_MIN = 0.35f;
const float JOY_ASLEEP_PER_MIN = 0.1f;
const float SLEEP_RECOVER_PER_MIN = 1.6f;

const uint32_t SAVE_INTERVAL_MS = 5UL * 60UL * 1000UL;
}  // namespace

void PetState::begin() {
  _prefs.begin("pet", false);
  // First boot has no keys yet; reading a missing float logs an NVS error.
  if (_prefs.isKey("hunger")) _hunger = _prefs.getFloat("hunger", _hunger);
  if (_prefs.isKey("joy")) _joy = _prefs.getFloat("joy", _joy);
  if (_prefs.isKey("sleep")) _sleepiness = _prefs.getFloat("sleep", _sleepiness);
  if (_prefs.isKey("asleep")) _asleep = _prefs.getBool("asleep", false);
  _loaded = true;
  _lastTick = millis();
  _lastSave = millis();
  Serial.printf("pet loaded: hunger %.1f joy %.1f sleep %.1f asleep %d\n", _hunger,
                _joy, _sleepiness, _asleep);
}

PetSnapshot PetState::snapshot() const {
  PetSnapshot s;
  s.hunger = _hunger;
  s.joy = _joy;
  s.sleepiness = _sleepiness;
  s.asleep = _asleep;
  return s;
}

void PetState::tick(uint32_t nowMs, bool dark) {
  if (!_loaded) return;
  if (_lastTick == 0) {
    _lastTick = nowMs;
    return;
  }

  uint32_t dtMs = nowMs - _lastTick;
  _lastTick = nowMs;
  float minutes = (float)dtMs / 60000.0f;

  if (_asleep) {
    _hunger += HUNGER_ASLEEP_PER_MIN * minutes;
    _joy -= JOY_ASLEEP_PER_MIN * minutes;
    _sleepiness -= SLEEP_RECOVER_PER_MIN * minutes;
    if (_sleepiness <= 5.0f && !dark) {
      _asleep = false;
      _sleepiness = 5.0f;
    }
  } else {
    _hunger += HUNGER_PER_MIN * minutes;
    _joy -= JOY_PER_MIN * minutes;
    _sleepiness += SLEEP_PER_MIN * minutes * (dark ? 2.0f : 1.0f);
    if (dark && _sleepiness > 62.0f) _asleep = true;
    if (_sleepiness > 96.0f) _asleep = true;
  }

  _hunger = constrain(_hunger, 0.0f, 100.0f);
  _joy = constrain(_joy, 0.0f, 100.0f);
  _sleepiness = constrain(_sleepiness, 0.0f, 100.0f);

  if (nowMs - _lastSave > SAVE_INTERVAL_MS) {
    _lastSave = nowMs;
    save();
  }
}

void PetState::feed() {
  _hunger = constrain(_hunger - 30.0f, 0.0f, 100.0f);
  _joy = constrain(_joy + 6.0f, 0.0f, 100.0f);
}

void PetState::petting() {
  _joy = constrain(_joy + 9.0f, 0.0f, 100.0f);
  _sleepiness = constrain(_sleepiness - 3.0f, 0.0f, 100.0f);
}

void PetState::wake() {
  _asleep = false;
  _sleepiness = constrain(_sleepiness - 20.0f, 0.0f, 100.0f);
}

void PetState::save() {
  if (!_loaded) return;
  _prefs.putFloat("hunger", _hunger);
  _prefs.putFloat("joy", _joy);
  _prefs.putFloat("sleep", _sleepiness);
  _prefs.putBool("asleep", _asleep);
}
