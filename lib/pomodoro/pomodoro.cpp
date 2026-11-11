#include "pomodoro.h"

namespace {
const uint32_t WORK_MS = 25UL * 60UL * 1000UL;
const uint32_t BREAK_MS = 5UL * 60UL * 1000UL;

const uint16_t COLOUR_WORK = 0x07FF;   // cyan
const uint16_t COLOUR_BREAK = 0x07E0;  // green
const uint16_t COLOUR_PAUSED = 0x7BEF; // grey
}  // namespace

void Pomodoro::begin(TFT_eSPI *tft) {
  _tft = tft;
  _phaseMs = WORK_MS;
  _phase = 0;
  _running = false;
  _accumMs = 0;
  _sessions = 0;
}

void Pomodoro::startPhase(uint32_t now, uint8_t phase) {
  _phase = phase;
  _phaseMs = (phase == 0) ? WORK_MS : BREAK_MS;
  _accumMs = 0;
  _startedAt = now;
  _lastShown = 0xFFFFFFFF;
}

uint32_t Pomodoro::elapsed(uint32_t now) const {
  uint32_t total = _accumMs;
  if (_running) total += now - _startedAt;
  return total;
}

uint32_t Pomodoro::remaining(uint32_t now) const {
  const uint32_t done = elapsed(now);
  return (done >= _phaseMs) ? 0 : _phaseMs - done;
}

void Pomodoro::toggle(uint32_t now) {
  if (_running) {
    _accumMs = elapsed(now);
    _running = false;
  } else {
    _startedAt = now;
    _running = true;
  }
  _lastShown = 0xFFFFFFFF;
  draw();
}

void Pomodoro::reset() {
  _running = false;
  _accumMs = 0;
  _sessions = 0;
  _phase = 0;
  _phaseMs = WORK_MS;
  _lastShown = 0xFFFFFFFF;
  draw();
}

void Pomodoro::tick(uint32_t now) {
  if (_running && remaining(now) == 0) {
    // Work finished: count it and roll into the break, and the other way round.
    if (_phase == 0) _sessions++;
    startPhase(now, (_phase == 0) ? 1 : 0);
    draw();
    return;
  }

  const uint32_t secs = remaining(now) / 1000;
  if (secs != _lastShown) {
    _lastShown = secs;
    draw();
  }
}

void Pomodoro::draw() {
  if (_tft == nullptr) return;

  const uint32_t total = _phaseMs / 1000;
  uint32_t left = _lastShown;
  if (left == 0xFFFFFFFF) left = total;

  const uint16_t accent = !_running ? COLOUR_PAUSED
                                    : (_phase == 0 ? COLOUR_WORK : COLOUR_BREAK);

  _tft->fillScreen(TFT_BLACK);

  _tft->setTextDatum(TL_DATUM);
  _tft->setTextColor(accent, TFT_BLACK);
  _tft->drawString("POMODORO", 8, 8, 2);

  char buf[24];
  snprintf(buf, sizeof(buf), "DONE %u", (unsigned)_sessions);
  _tft->setTextDatum(TR_DATUM);
  _tft->drawString(buf, 312, 8, 2);

  // Big digits: font 7 is digits and colon only, which is exactly what is needed.
  snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)(left / 60), (unsigned)(left % 60));
  _tft->setTextDatum(MC_DATUM);
  _tft->drawString(buf, 160, 108, 7);

  const char *phase = (_phase == 0) ? "WORK" : "BREAK";
  if (!_running) phase = "PAUSED";
  _tft->setTextColor(accent, TFT_BLACK);
  _tft->drawString(phase, 160, 160, 4);

  _tft->setTextColor(0x7BEF, TFT_BLACK);
  _tft->drawString("tap: start/stop   down: reset   up: exit", 160, 214, 2);
}
