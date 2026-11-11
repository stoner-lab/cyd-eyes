#pragma once

#include <Arduino.h>
#include <Preferences.h>

// Needs only three numbers. A fourth stat was cut on purpose: with no speaker
// the pet cannot signal hygiene, so the player would never understand it.
struct PetSnapshot {
  float hunger;      // 0 fed .. 100 starving
  float joy;         // 0 miserable .. 100 delighted
  float sleepiness;  // 0 awake .. 100 asleep
  bool asleep;
};

class PetState {
 public:
  void begin();
  void tick(uint32_t nowMs, bool dark);
  void feed();
  void petting();
  void wake();
  void save();

  PetSnapshot snapshot() const;
  bool asleep() const { return _asleep; }

 private:
  Preferences _prefs;
  float _hunger = 25.0f;
  float _joy = 65.0f;
  float _sleepiness = 15.0f;
  bool _asleep = false;

  uint32_t _lastTick = 0;
  uint32_t _lastSave = 0;
  bool _loaded = false;
};
