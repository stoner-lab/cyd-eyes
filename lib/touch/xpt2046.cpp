#include "xpt2046.h"

namespace {

const int PIN_CLK = 25;
const int PIN_MOSI = 32;
const int PIN_MISO = 39;
const int PIN_CS = 33;
const int PIN_IRQ = 36;

// Command bytes: 12-bit conversion, single ended.
const uint8_t CMD_X = 0x90;
const uint8_t CMD_Y = 0xD0;

// Measured over 131 touch samples on this panel: rawX ran 374..1686 and rawY
// ran 334..1650. The earlier guess (Y 440..1500) clipped the top and bottom of
// the screen, which is why the edges felt dead.
const int RAW_X_MIN = 360;
const int RAW_X_MAX = 1700;
const int RAW_Y_MIN = 320;
const int RAW_Y_MAX = 1670;

// A real touch gives three consecutive samples within a few counts of each
// other; a floating pen-detect line does not.
const int STABLE_SPREAD = 300;

// Orientation of the panel relative to the 320x240 landscape screen.
const bool INVERT_X = false;
const bool INVERT_Y = false;

const int SCREEN_W = 320;
const int SCREEN_H = 240;

const int SETTLE_US = 2;

}  // namespace

void XPT2046::begin() {
  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_MISO, INPUT);
  pinMode(PIN_IRQ, INPUT);

  digitalWrite(PIN_CS, HIGH);
  digitalWrite(PIN_CLK, LOW);
  digitalWrite(PIN_MOSI, LOW);
}

bool XPT2046::pressed() { return digitalRead(PIN_IRQ) == LOW; }

uint16_t XPT2046::transfer(uint8_t cmd) {
  digitalWrite(PIN_CS, LOW);
  delayMicroseconds(SETTLE_US);

  for (int8_t bit = 7; bit >= 0; bit--) {
    digitalWrite(PIN_MOSI, (cmd >> bit) & 1);
    delayMicroseconds(SETTLE_US);
    digitalWrite(PIN_CLK, HIGH);
    delayMicroseconds(SETTLE_US);
    digitalWrite(PIN_CLK, LOW);
  }

  uint16_t value = 0;
  for (uint8_t bit = 0; bit < 12; bit++) {
    digitalWrite(PIN_CLK, HIGH);
    delayMicroseconds(SETTLE_US);
    value = (uint16_t)((value << 1) | (digitalRead(PIN_MISO) ? 1 : 0));
    digitalWrite(PIN_CLK, LOW);
    delayMicroseconds(SETTLE_US);
  }

  digitalWrite(PIN_CS, HIGH);
  return value;
}

// Median of three, rejected when the three disagree: real contact is steady,
// an unconnected pen-detect line is not.
bool XPT2046::sampleStable(uint8_t cmd, uint16_t &out) {
  uint16_t a = transfer(cmd);
  uint16_t b = transfer(cmd);
  uint16_t c = transfer(cmd);

  uint16_t lo = min(a, min(b, c));
  uint16_t hi = max(a, max(b, c));
  if ((int)(hi - lo) > STABLE_SPREAD) return false;

  out = (uint16_t)max(lo, (uint16_t)min(max(a, b), max(min(a, b), c)));
  return true;
}

bool XPT2046::read(uint16_t &rawX, uint16_t &rawY) {
  if (!pressed()) return false;

  uint16_t a = 0, b = 0;
  if (!sampleStable(CMD_X, a) || !sampleStable(CMD_Y, b)) return false;

  // Any stable contact counts, anywhere on the glass: clamping happens in the
  // mapping below, so a press past the calibrated edge still registers.
  if (a < 20 || a > 4070 || b < 20 || b > 4070) return false;

  rawX = a;
  rawY = b;

  int32_t nx = a;
  int32_t ny = b;
  if (INVERT_X) nx = RAW_X_MIN + RAW_X_MAX - nx;
  if (INVERT_Y) ny = RAW_Y_MIN + RAW_Y_MAX - ny;

  _sx = (int16_t)constrain(map(nx, RAW_X_MIN, RAW_X_MAX, 0, SCREEN_W - 1), 0,
                           SCREEN_W - 1);
  _sy = (int16_t)constrain(map(ny, RAW_Y_MIN, RAW_Y_MAX, 0, SCREEN_H - 1), 0,
                           SCREEN_H - 1);
  return true;
}
