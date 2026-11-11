#include "settings.h"

#include <Preferences.h>

namespace {

const int16_t ROW_Y[3] = {44, 92, 140};
const int16_t ROW_H = 36;
const int16_t BTN_X = 104;
const int16_t BTN_W = 52;
const int16_t BTN_GAP = 4;

const char *BRIGHT_LABELS[3] = {"100%", "40%", "20%"};
const char *TIMEOUT_LABELS[3] = {"1m", "3m", "10m"};
const char *COLOUR_LABELS[4] = {"cyan", "green", "amber", "white"};

const uint16_t ACCENT = 0x07FF;
const uint16_t OUTLINE = 0x4208;
const uint16_t TEXT_DIM = 0x8C71;

// Duty values for LEDC, matching the three brightness steps.
const int FULL_DUTY[3] = {255, 102, 51};
const int DIM_DUTY[3] = {60, 24, 12};
const uint32_t OFF_MS[3] = {60000UL, 180000UL, 600000UL};
const uint16_t EYE_COLOURS[4] = {0x07FF, 0x07E0, 0xFD20, 0xFFFF};

int buttonCount(uint8_t row) { return (row == 2) ? 4 : 3; }

int16_t buttonX(uint8_t row, uint8_t index) {
  (void)row;
  return BTN_X + index * (BTN_W + BTN_GAP);
}

void drawRow(TFT_eSPI *tft, uint8_t row, const char *title, const char **labels,
             uint8_t count, uint8_t selected) {
  tft->setTextDatum(ML_DATUM);
  tft->setTextColor(TEXT_DIM, TFT_BLACK);
  tft->drawString(title, 8, ROW_Y[row] + ROW_H / 2, 2);

  for (uint8_t i = 0; i < count; i++) {
    const int16_t x = buttonX(row, i);
    const bool on = (i == selected);
    if (on) {
      tft->fillRect(x, ROW_Y[row], BTN_W, ROW_H, ACCENT);
      tft->setTextColor(TFT_BLACK, ACCENT);
    } else {
      tft->drawRect(x, ROW_Y[row], BTN_W, ROW_H, OUTLINE);
      tft->setTextColor(TEXT_DIM, TFT_BLACK);
    }
    tft->setTextDatum(MC_DATUM);
    tft->drawString(labels[i], x + BTN_W / 2, ROW_Y[row] + ROW_H / 2, 2);
  }
}

}  // namespace

void Settings::begin(TFT_eSPI *tft) {
  _tft = tft;
  Preferences prefs;
  // Opened read-write so the namespace is created on first boot; opening it
  // read-only on a fresh board logs an NVS error.
  prefs.begin("cfg", false);
  _brightness = prefs.getUChar("bl", 0);
  _timeout = prefs.getUChar("off", 1);
  _colour = prefs.getUChar("col", 0);
  prefs.end();
  if (_brightness > 2) _brightness = 0;
  if (_timeout > 2) _timeout = 1;
  if (_colour > 3) _colour = 0;
}

void Settings::save() {
  Preferences prefs;
  prefs.begin("cfg", false);
  prefs.putUChar("bl", _brightness);
  prefs.putUChar("off", _timeout);
  prefs.putUChar("col", _colour);
  prefs.end();
}

int Settings::fullDuty() const { return FULL_DUTY[_brightness]; }
int Settings::dimDuty() const { return DIM_DUTY[_brightness]; }
uint32_t Settings::offMs() const { return OFF_MS[_timeout]; }
uint16_t Settings::eyeColour() const { return EYE_COLOURS[_colour]; }

int Settings::hit(int16_t x, int16_t y) const {
  for (uint8_t row = 0; row < 3; row++) {
    if (y < ROW_Y[row] || y > ROW_Y[row] + ROW_H) continue;
    const uint8_t count = (uint8_t)buttonCount(row);
    for (uint8_t i = 0; i < count; i++) {
      const int16_t bx = buttonX(row, i);
      if (x >= bx && x <= bx + BTN_W) return row * 4 + i;
    }
  }
  return -1;
}

void Settings::apply(int id) {
  if (id < 0) return;
  const uint8_t row = (uint8_t)(id / 4);
  const uint8_t index = (uint8_t)(id % 4);

  if (row == 0 && index < 3) {
    _brightness = index;
  } else if (row == 1 && index < 3) {
    _timeout = index;
  } else if (row == 2 && index < 4) {
    _colour = index;
  } else {
    return;
  }
  save();
  draw();
}

void Settings::draw() {
  TFT_eSPI *tft = _tft;
  if (tft == nullptr) return;

  tft->fillScreen(TFT_BLACK);
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(ACCENT, TFT_BLACK);
  tft->drawString("SETTINGS", 8, 8, 2);

  drawRow(tft, 0, "Brightness", BRIGHT_LABELS, 3, _brightness);
  drawRow(tft, 1, "Screen off", TIMEOUT_LABELS, 3, _timeout);
  drawRow(tft, 2, "Eye colour", COLOUR_LABELS, 4, _colour);

  tft->setTextDatum(MC_DATUM);
  tft->setTextColor(TEXT_DIM, TFT_BLACK);
  tft->drawString("swipe right to close", 160, 212, 2);
}
