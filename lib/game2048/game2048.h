#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

enum MoveDir : uint8_t { MOVE_LEFT, MOVE_RIGHT, MOVE_UP, MOVE_DOWN };

// Minimal 2048 for a 320x240 panel: 32 bytes of board, one score counter, no
// animation. Every redraw is a full board repaint (~11 ms), which is far cheaper
// than keeping sprites or tracking dirty cells.
class Game2048 {
 public:
  void begin(TFT_eSPI *tft);

  void newGame();
  bool move(MoveDir dir);  // true when the board changed
  bool over() const;
  bool started() const { return _started; }

  uint32_t score() const { return _score; }
  uint32_t best() const { return _best; }

  void draw();
  void drawHud();
  bool inExitButton(int16_t x, int16_t y) const;

 private:
  void slideLine(uint16_t *line, bool reverse);
  void spawn();
  void drawCell(uint8_t index);
  uint16_t colourFor(uint16_t value) const;

  TFT_eSPI *_tft = nullptr;
  uint16_t _board[16] = {0};
  uint32_t _score = 0;
  uint32_t _best = 0;
  bool _started = false;
};
