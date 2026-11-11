#include "game2048.h"

namespace {

// Board geometry: 4 cells of 48 px with 6 px gaps, shifted right and down to
// leave a clear margin for the exit button. Cells of a fingertip size beat two
// extra pixels of board.
const int16_t BOARD_X = 62;
const int16_t BOARD_Y = 26;
const int16_t CELL = 48;
const int16_t GAP = 6;
const int16_t STEP = CELL + GAP;

const int16_t EXIT_X = 2;
const int16_t EXIT_Y = 2;
const int16_t EXIT_W = 60;
const int16_t EXIT_H = 46;

// Hit area is deliberately larger than the drawn button and includes the very
// corner: presses against the bezel clamp to coordinate 0 and would otherwise
// fall outside.
const int16_t EXIT_HIT_W = 70;
const int16_t EXIT_HIT_H = 54;

const uint16_t COLOUR_EMPTY = 0x18E3;  // dark slate
const uint16_t COLOUR_TEXT_DARK = 0x0000;
const uint16_t COLOUR_TEXT_LIGHT = 0xFFFF;

// Indexed by log2(value): 2, 4, 8, ... 32768.
const uint16_t TILE_COLOURS[15] = {
    0x05FF, 0x04DF, 0x03BF, 0x029F, 0x019F, 0x00DF, 0x0D3F, 0x0C1F,
    0x0AFF, 0x09DF, 0x7FE0, 0x7C00, 0xF800, 0xF000, 0xE000};

const uint16_t rngSeed = 0;

}  // namespace

void Game2048::begin(TFT_eSPI *tft) { _tft = tft; }

bool Game2048::inExitButton(int16_t x, int16_t y) const {
  return x >= 0 && x <= EXIT_HIT_W && y >= 0 && y <= EXIT_HIT_H;
}

void Game2048::newGame() {
  for (uint8_t i = 0; i < 16; i++) _board[i] = 0;
  _score = 0;
  _started = true;
  spawn();
  spawn();
  draw();
}

uint16_t Game2048::colourFor(uint16_t value) const {
  if (value < 2) return COLOUR_EMPTY;
  uint8_t exp = 0;
  while ((uint16_t)(1u << exp) < value && exp < 15) exp++;
  return TILE_COLOURS[exp - 1];
}

void Game2048::spawn() {
  uint8_t freeCells[16];
  uint8_t n = 0;
  for (uint8_t i = 0; i < 16; i++) {
    if (_board[i] == 0) freeCells[n++] = i;
  }
  if (n == 0) return;
  const uint8_t slot = freeCells[random(n)];
  _board[slot] = (random(10) == 0) ? 4 : 2;
}

// Slides one line toward index 0 (or toward the far end when reverse is set),
// merging equal neighbours once each, and adds the gained value to the score.
void Game2048::slideLine(uint16_t *line, bool reverse) {
  uint16_t src[4];
  for (uint8_t i = 0; i < 4; i++) src[i] = reverse ? line[3 - i] : line[i];

  uint16_t packed[4] = {0, 0, 0, 0};
  uint8_t n = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (src[i] != 0) packed[n++] = src[i];
  }

  uint32_t gained = 0;
  for (uint8_t i = 0; i + 1 < 4; i++) {
    if (packed[i] != 0 && packed[i] == packed[i + 1]) {
      packed[i] *= 2;
      gained += packed[i];
      packed[i + 1] = 0;
    }
  }

  uint16_t merged[4] = {0, 0, 0, 0};
  n = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (packed[i] != 0) merged[n++] = packed[i];
  }

  for (uint8_t i = 0; i < 4; i++) {
    line[i] = reverse ? merged[3 - i] : merged[i];
  }
  _score += gained;
}

bool Game2048::move(MoveDir dir) {
  if (!_started) return false;

  uint16_t before[16];
  memcpy(before, _board, sizeof(before));

  uint16_t line[4];
  for (uint8_t i = 0; i < 4; i++) {
    // Gather one row or one column into a scratch line.
    for (uint8_t j = 0; j < 4; j++) {
      if (dir == MOVE_LEFT || dir == MOVE_RIGHT) {
        line[j] = _board[i * 4 + j];
      } else {
        line[j] = _board[j * 4 + i];
      }
    }

    slideLine(line, (dir == MOVE_RIGHT || dir == MOVE_DOWN));

    for (uint8_t j = 0; j < 4; j++) {
      if (dir == MOVE_LEFT || dir == MOVE_RIGHT) {
        _board[i * 4 + j] = line[j];
      } else {
        _board[j * 4 + i] = line[j];
      }
    }
  }

  if (memcmp(before, _board, sizeof(before)) == 0) {
    _score = _score;  // unchanged board: no spawn, no redraw
    return false;
  }

  spawn();
  if (_score > _best) _best = _score;
  draw();
  return true;
}

bool Game2048::over() const {
  for (uint8_t i = 0; i < 16; i++) {
    if (_board[i] == 0) return false;
  }
  for (uint8_t r = 0; r < 4; r++) {
    for (uint8_t c = 0; c < 4; c++) {
      const uint16_t v = _board[r * 4 + c];
      if (c < 3 && _board[r * 4 + c + 1] == v) return false;
      if (r < 3 && _board[(r + 1) * 4 + c] == v) return false;
    }
  }
  return true;
}

void Game2048::drawCell(uint8_t index) {
  const uint8_t row = index / 4;
  const uint8_t col = index % 4;
  const int16_t x = BOARD_X + col * STEP;
  const int16_t y = BOARD_Y + row * STEP;
  const uint16_t value = _board[index];

  _tft->fillRect(x, y, CELL, CELL, colourFor(value));
  if (value == 0) return;

  _tft->setTextDatum(MC_DATUM);
  char text[6];
  itoa(value, text, 10);
  const uint8_t len = strlen(text);
  const uint8_t font = (len <= 3) ? 4 : 2;
  _tft->setTextColor((value <= 4) ? COLOUR_TEXT_DARK : COLOUR_TEXT_LIGHT);
  _tft->drawString(text, x + CELL / 2, y + CELL / 2, font);
}

void Game2048::draw() {
  if (_tft == nullptr) return;

  _tft->fillScreen(0x0000);
  _tft->fillRect(BOARD_X - GAP, BOARD_Y - GAP, 4 * STEP + GAP, 4 * STEP + GAP,
                 COLOUR_EMPTY);

  for (uint8_t i = 0; i < 16; i++) drawCell(i);
  drawHud();
}

void Game2048::drawHud() {
  if (_tft == nullptr) return;

  // Exit button: a filled square with a left arrow glyph.
  _tft->fillRect(EXIT_X, EXIT_Y, EXIT_W, EXIT_H, 0x18E3);
  _tft->setTextDatum(MC_DATUM);
  _tft->setTextColor(0x07FF);
  _tft->drawString("<", EXIT_X + EXIT_W / 2, EXIT_Y + EXIT_H / 2, 4);

  _tft->setTextDatum(TR_DATUM);
  _tft->setTextColor(0x07FF);
  char text[24];
  snprintf(text, sizeof(text), "%lu  best %lu", (unsigned long)_score,
           (unsigned long)_best);
  _tft->drawString(text, 316, 8, 2);
}
