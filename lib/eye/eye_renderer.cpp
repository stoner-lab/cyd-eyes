#include "eye_renderer.h"

#include <math.h>

namespace {
uint16_t g_eyeColour = 0x07FF;  // replaced at runtime by the settings screen
const uint16_t PUPIL_COLOUR = 0x0000;  // black
const uint16_t GLINT_COLOUR = 0xFFFF;  // catchlight, the detail that makes it cute
const float LERP = 0.18f;
const float LOOK_LERP = 0.55f;  // gaze has to feel attached to the finger
const uint32_t BLINK_MS = 170;

// The eye body never exceeds rx 54 plus a 12 px gaze shift; the height has to
// fit ry 64, the 20 px brow lift, the gaze offset and the overlay text.
const int16_t BOX_W = 132;
const int16_t BOX_H = 204;

const int16_t BROW_RADIUS = 3;
const float CORNER_RATIO = 0.55f;  // of the smaller radius: soft square, not oval
}  // namespace

void EyeRenderer::begin(TFT_eSPI *tft, int16_t cx, int16_t cy) {
  _tft = tft;
  _cx = cx;
  _cy = cy;
  _cur = EYE_PRESETS[EM_NORMAL];
  _target = _cur;
  _emotion = EM_NORMAL;
  _nextBlink = millis() + 1500;

  for (uint8_t i = 0; i < 2; i++) {
    _spr[i] = new TFT_eSprite(_tft);
    _spr[i]->setColorDepth(16);
    if (_spr[i]->createSprite(BOX_W, BOX_H) == nullptr) {
      Serial.printf("sprite %u allocation failed\n", i);
    }
    _spr[i]->fillSprite(TFT_BLACK);
  }

  _tft->fillRect(0, _cy - 96, _cx * 2, 210, TFT_BLACK);
}

void EyeRenderer::setEmotion(EyeEmotion emotion, bool immediate) {
  if (emotion >= EM_COUNT) emotion = EM_NORMAL;
  if (emotion == _emotion) return;
  _emotion = emotion;
  _target = EYE_PRESETS[emotion];
  if (immediate) _cur = _target;
}

void EyeRenderer::look(float x, float y) {
  _lookTargetX = constrain(x, -1.0f, 1.0f);
  _lookTargetY = constrain(y, -1.0f, 1.0f);
}

void EyeRenderer::blinkNow() {
  // Rate limited: touch jitter used to call this several times per second and
  // the lids never opened between blinks.
  if (_blinking || millis() - _lastBlinkEnd < 300) return;
  _blinking = true;
  _blinkStart = millis();
}

void EyeRenderer::fillRoundRect(TFT_eSprite &spr, int16_t x, int16_t y, int16_t w,
                                int16_t h, int16_t r, uint16_t colour) {
  if (w <= 0 || h <= 0) return;
  if (r <= 0) {
    spr.fillRect(x, y, w, h, colour);
    return;
  }
  if (r > w / 2) r = w / 2;
  if (r > h / 2) r = h / 2;

  for (int16_t row = 0; row < h; row++) {
    int16_t inset = 0;
    if (row < r) {
      float t = (float)(r - row) / (float)r;
      inset = r - (int16_t)(sqrtf(1.0f - t * t) * r + 0.5f);
    } else if (row >= h - r) {
      float t = (float)(row - (h - r - 1)) / (float)r;
      inset = r - (int16_t)(sqrtf(1.0f - t * t) * r + 0.5f);
    }
    spr.drawFastHLine(x + inset, y + row, w - 2 * inset, colour);
  }
}

void EyeRenderer::fillCircle(TFT_eSprite &spr, int16_t cx, int16_t cy, int16_t r,
                             uint16_t colour) {
  for (int16_t dy = -r; dy <= r; dy++) {
    float t = (float)dy / (float)r;
    int16_t hw = (int16_t)(sqrtf(1.0f - t * t) * r + 0.5f);
    if (hw <= 0) continue;
    spr.drawFastHLine(cx - hw, cy + dy, hw * 2 + 1, colour);
  }
}

void EyeRenderer::drawBrow(TFT_eSprite &spr, int16_t cx, float yTop, float len,
                           float tilt, float curve, uint8_t side) {
  if (len < 8.0f) return;

  const float dir = (side == 0) ? 1.0f : -1.0f;  // mirror the slant
  const float slope = tanf(tilt * 0.0174533f) * dir;
  const float x0 = (float)cx - len * 0.5f;

  const int steps = (int)len;
  for (int i = 0; i <= steps; i++) {
    float t = (float)i / (float)steps;
    float x = x0 + len * t;
    // Straight slant plus a parabolic arch; the arch is what keeps it friendly.
    float y = yTop + slope * (len * t - len * 0.5f) - curve * 4.0f * t * (1.0f - t);
    fillCircle(spr, (int16_t)x, (int16_t)y, BROW_RADIUS, g_eyeColour);
  }
}

void EyeRenderer::drawClosedArc(TFT_eSprite &spr, int16_t cx, int16_t cy,
                                float halfWidth, float arc) {
  const int steps = (int)(halfWidth * 2.0f);
  for (int i = 0; i <= steps; i++) {
    float t = (float)i / (float)steps;
    float x = (float)cx - halfWidth + halfWidth * 2.0f * t;
    // Bows upward: the classic pleased shut eye, not a flat sleeping line.
    float y = (float)cy - arc * 4.0f * t * (1.0f - t);
    fillCircle(spr, (int16_t)x, (int16_t)y, BROW_RADIUS, g_eyeColour);
  }
}
void EyeRenderer::renderEye(int16_t cx, float openFactor, uint8_t side,
                            uint32_t nowMs) {
  TFT_eSprite *spr = _spr[side];
  if (spr == nullptr) return;

  spr->fillSprite(TFT_BLACK);

  const int16_t ox = cx - BOX_W / 2;
  const int16_t oy = _cy - BOX_H / 2;
  const int16_t lx = cx - ox;
  const int16_t ly = _cy - oy;

  int16_t eyeCx = lx + (int16_t)(_lookX * 12.0f);
  int16_t eyeCy = ly + (int16_t)(_cur.dy + _lookY * 16.0f);
  const float squeeze = 1.0f - 0.45f * _yawn;
  int16_t rx = (int16_t)(_cur.rx * (1.0f - 0.12f * _yawn));
  int16_t ry = (int16_t)max(3.0f, _cur.ry * _cur.openness * openFactor * squeeze);

  const bool shut = _cur.closedArc > 1.0f;

  if (!shut) {
    const int16_t corner = (int16_t)(min(rx, ry) * CORNER_RATIO);
    fillRoundRect(*spr, eyeCx - rx, eyeCy - ry, rx * 2, ry * 2, corner, g_eyeColour);

    // Pupil disappears once the lid closes, otherwise it shows through the line.
    if (ry > _cur.pupil * 0.7f) {
      int16_t pr = (int16_t)_cur.pupil;
      int16_t px = (int16_t)(eyeCx + _lookX * 18.0f);
      int16_t py = (int16_t)(eyeCy + _lookY * 14.0f);
      fillCircle(*spr, px, py, pr, PUPIL_COLOUR);

      // Fixed light source at the upper left: the gaze moves, the glint does not
      // travel with the pupil, which is what reads as a glossy eye.
      if (pr >= 6) {
        int16_t gr = (int16_t)max(2.0f, pr * 0.42f);
        fillCircle(*spr, px - (int16_t)(pr * 0.45f), py - (int16_t)(pr * 0.45f), gr,
                   GLINT_COLOUR);
      }
    }

    if (_cur.lidDrop > 0.01f) {
      // Heavy upper lid: erase the top slice of the eye and give the cut a soft
      // edge, so the eye keeps its size instead of looking squashed.
      const int16_t dropPx = (int16_t)((float)(ry * 2) * _cur.lidDrop);
      const float u = constrain((float)(ry - dropPx) / (float)ry, 0.0f, 1.0f);
      const int16_t hw = (int16_t)((float)rx * sqrtf(1.0f - u * u));
      if (hw > 3 && dropPx > 2) {
        const int16_t lidY = eyeCy - ry + dropPx;
        spr->fillRect(eyeCx - hw, eyeCy - ry - 2, hw * 2, dropPx + 2, TFT_BLACK);
        fillRoundRect(*spr, eyeCx - hw, lidY - 3, hw * 2, 6, 3, g_eyeColour);
      }
    }
  } else {
    drawClosedArc(*spr, eyeCx, eyeCy, (float)rx, _cur.closedArc);
  }

  // Tear and mouth are gone: both needed to overlap the eyes on a 320x240 panel
  // to read correctly, and the overlap looked like a layout bug.

  float browTop = shut ? (float)eyeCy - _cur.closedArc * 0.6f - _cur.browLift
                       : (float)eyeCy - (float)ry - _cur.browLift;
  drawBrow(*spr, eyeCx, browTop, _cur.browLen, _cur.browTilt, _cur.browCurve, side);

  // Overlay text rides inside the right eye sprite, so it shares that blit and
  // cannot flicker between frames.
  if (side == 1 && _overlay != nullptr && _overlay[0] != '\0') {
    const int16_t bob = (int16_t)(2.0f * sinf((float)nowMs / 420.0f));
    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(g_eyeColour);
    spr->drawString(_overlay, 18, 2 + bob, 4);
  }

  spr->pushSprite(ox, oy);
}


void EyeRenderer::update(uint32_t nowMs) {
  _cur.openness += (_target.openness - _cur.openness) * LERP;
  _cur.rx += (_target.rx - _cur.rx) * LERP;
  _cur.ry += (_target.ry - _cur.ry) * LERP;
  _cur.pupil += (_target.pupil - _cur.pupil) * LERP;
  _cur.dy += (_target.dy - _cur.dy) * LERP;
  _cur.spacing += (_target.spacing - _cur.spacing) * LERP;
  _cur.browTilt += (_target.browTilt - _cur.browTilt) * LERP;
  _cur.browLift += (_target.browLift - _cur.browLift) * LERP;
  _cur.browLen += (_target.browLen - _cur.browLen) * LERP;
  _cur.browCurve += (_target.browCurve - _cur.browCurve) * LERP;
  _cur.lidDrop += (_target.lidDrop - _cur.lidDrop) * LERP;
  // Faster than the rest: this flag switches the drawn shape, so a slow blend
  // would show the eye body vanishing a frame late.
  _cur.closedArc += (_target.closedArc - _cur.closedArc) * 0.3f;

  _lookX += (_lookTargetX - _lookX) * LOOK_LERP;
  _lookY += (_lookTargetY - _lookY) * LOOK_LERP;

  if (!_blinking && nowMs >= _nextBlink) {
    _blinking = true;
    _blinkStart = nowMs;
  }

  float openFactor = 1.0f;
  if (_blinking) {
    float t = (float)(nowMs - _blinkStart) / (float)BLINK_MS;
    if (t >= 1.0f) {
      _blinking = false;
      _lastBlinkEnd = nowMs;
      _nextBlink = nowMs + random(2200, 6000);
    } else if (t < 0.45f) {
      openFactor = 1.0f - (t / 0.45f);
    } else {
      openFactor = (t - 0.45f) / 0.55f;
    }
  }

  renderEye((int16_t)(_cx - _cur.spacing), openFactor, 0, nowMs);
  renderEye((int16_t)(_cx + _cur.spacing), openFactor, 1, nowMs);
}

void EyeRenderer::setEyeColour(uint16_t colour) { g_eyeColour = colour; }
