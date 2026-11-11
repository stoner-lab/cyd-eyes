// CYD eyes: two animated eyes that follow a finger. No hunger, no stats,
// no HUD. Stroking the spot between the eyes shuts them in a pleased arc.

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <math.h>

#include "board.h"
#include "eye_renderer.h"
#include "gallery.h"
#include "game2048.h"
#include "pomodoro.h"
#include "settings.h"
#include "uploader.h"
#include "xpt2046.h"

namespace {

const uint32_t FRAME_MS = 25;  // 40 fps
const uint32_t TOUCH_MS = 15;

// A finger resting on the glass produces the occasional failed read. Treating
// each gap as a release used to fire a blink every time.
const uint32_t RELEASE_MS = 180;
const uint32_t TAP_MAX_MS = 600;

// Short timings for evaluation sessions: dazed at 8 s, sleepy at 20 s, asleep
// at 35 s. Set to 0 for the real cadence.
#define FAST_IDLE_TEST 0

#if FAST_IDLE_TEST
const uint32_t IDLE_DAZED_MS = 8000;
const uint32_t IDLE_SLEEPY_MS = 20000;
const uint32_t IDLE_ASLEEP_MS = 35000;
#else
// Real cadence aligned with the backlight: dim at 15 s, dazed at 45 s, sleepy at
// 2 minutes and asleep exactly when the screen turns off at 3 minutes, so the
// pet is never "visibly asleep" behind a dark panel.
const uint32_t IDLE_DAZED_MS = 45000;
const uint32_t IDLE_SLEEPY_MS = 120000;
const uint32_t IDLE_ASLEEP_MS = 180000;
#endif

const uint32_t SURPRISE_MS = 1200;
const uint32_t ANGRY_MS = 1500;
const uint32_t TAP_WINDOW_MS = 2000;
const uint8_t TAPS_FOR_ANGRY = 3;

const int EYE_CX = 160;
const int EYE_CY = 120;
const float STROKE_FOR_PLAYFUL = 120.0f;  // accumulated px in one contact

// Petting zone: the gap between the eyes, a little above the centre line.
const int PET_ZONE_HALF_W = 74;
const int PET_ZONE_TOP = 26;
const int PET_ZONE_BOTTOM = 118;
const float PET_STROKE_FOR_CONTENT = 40.0f;
const uint32_t CONTENT_LINGER_MS = 700;

TFT_eSPI tft;
EyeRenderer eyes;
XPT2046 touch;
Gallery gallery;
Game2048 game;
Pomodoro pomodoro;
Settings settings;

enum View : uint8_t { VIEW_EYES, VIEW_PHOTO, VIEW_GAME, VIEW_POMODORO, VIEW_SETTINGS };
View view = VIEW_EYES;
uint32_t photoUntil = 0;
bool wantPhoto = false;
bool exitPressed = false;

// Backlight on IO21 through LEDC, so brightness can be lowered after idle.
const int PIN_BL = 21;
const int BL_CHANNEL = 0;
const int BL_FULL = 255;
const int BL_DIM = 60;  // ~23 percent
const int BL_OFF = 0;
const uint32_t DIM_AFTER_MS = 15000;
// Off after three minutes: the backlight is the part that wears out, and a board
// that runs 24/7 should not keep it lit for nobody.
const uint32_t OFF_AFTER_MS = 180000;

// Gesture thresholds. The main screen competes with gaze control, so a swipe
// there must be long and quick; otherwise a slow drag is just looking around.
// Inside the game and the viewer there is nothing to mistake for a gesture, so
// the bar is lower.
// Gesture thresholds. Opening a photo by accident costs nothing (swipe down to
// leave), so the upward flick is easy; entering the game costs a move, so the
// rightward flick stays long and quick.
// Speed, not duration: a fixed time limit still let a slow 90 px drag through,
// because 90 px in 700 ms is only 128 px/s but passes a millisecond test.
const int16_t SWIPE_UP_Y = 90;
const int16_t SWIPE_UP_SPEED = 675;  // px per second
const int16_t SWIPE_GAME_X = 160;
const int16_t SWIPE_GAME_SPEED = 520;
const int16_t SWIPE_DOWN_Y = 90;
const int16_t SWIPE_DOWN_SPEED = 675;
const int16_t SWIPE_PHOTO_DOWN = 100;
const int16_t SWIPE_GAME = 60;

const uint32_t YAWN_DURATION_MS = 3200;

uint32_t lastFrame = 0;
uint32_t lastTouch = 0;
uint32_t lastLog = 0;
uint32_t framesRendered = 0;

bool touching = false;
uint32_t lastSeen = 0;
uint32_t contactAt = 0;
uint32_t lastRelease = 0;
float strokePx = 0.0f;
float petStroke = 0.0f;
int16_t prevX = 0;
int16_t prevY = 0;
int16_t pressX = 0;
int16_t pressY = 0;
bool swipeArmed = false;

uint32_t surpriseUntil = 0;
uint32_t angryUntil = 0;
uint32_t contentUntil = 0;
uint32_t tapWindowStart = 0;
uint8_t tapCount = 0;

}  // namespace

static bool inPetZone(int16_t sx, int16_t sy) {
  return abs(sx - EYE_CX) < PET_ZONE_HALF_W && sy > PET_ZONE_TOP && sy < PET_ZONE_BOTTOM;
}

static void exitPhoto() {
  view = VIEW_EYES;
  tft.fillScreen(TFT_BLACK);
}

static void enterGame() {
  view = VIEW_GAME;
  if (!game.started()) {
    game.newGame();
  } else {
    game.draw();
    if (game.over()) {
      tft.fillRect(60, 96, 200, 56, 0x0000);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(0xF800);
      tft.drawString("GAME OVER", 160, 112, 4);
      tft.setTextColor(0xFFFF);
      tft.drawString("tap to restart", 160, 138, 2);
    }
  }
}

static void exitGame() {
  view = VIEW_EYES;
  tft.fillScreen(TFT_BLACK);
}

static void enterPomodoro() {
  view = VIEW_POMODORO;
  pomodoro.draw();
}

static void exitPomodoro() {
  view = VIEW_EYES;
  tft.fillScreen(TFT_BLACK);
}

static void enterSettings() {
  view = VIEW_SETTINGS;
  settings.draw();
}

static void exitSettings() {
  view = VIEW_EYES;
  tft.fillScreen(TFT_BLACK);
}

static void showGameOver() {
  tft.fillRect(60, 96, 200, 56, 0x0000);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(0xF800);
  tft.drawString("GAME OVER", 160, 112, 4);
  tft.setTextColor(0xFFFF);
  tft.drawString("tap to restart", 160, 138, 2);
}

static void readTouch(uint32_t now) {
  uint16_t rawX = 0, rawY = 0;

  if (touch.read(rawX, rawY)) {
    lastSeen = now;

    int16_t sx = touch.screenX();
    int16_t sy = touch.screenY();

    if (!touching) {
      touching = true;
      contactAt = now;
      strokePx = 0.0f;
      petStroke = 0.0f;
      prevX = sx;
      prevY = sy;
      pressX = sx;
      pressY = sy;
      swipeArmed = false;
      Serial.printf("down %d %d\n", sx, sy);

      if (view == VIEW_EYES) {
        if (now - lastRelease > IDLE_SLEEPY_MS) surpriseUntil = now + SURPRISE_MS;

        if (now - tapWindowStart > TAP_WINDOW_MS) {
          tapWindowStart = now;
          tapCount = 0;
        }
        tapCount++;
        if (tapCount >= TAPS_FOR_ANGRY) {
          angryUntil = now + ANGRY_MS;
          tapCount = 0;
        }
      }
      exitPressed = (view == VIEW_GAME) && game.inExitButton(sx, sy);
      if (view == VIEW_GAME) {
        Serial.printf("game press %d %d %s\n", sx, sy, exitPressed ? "button" : "board");
      }
    } else {
      float move = (float)(abs(sx - prevX) + abs(sy - prevY));
      strokePx += move;
      if (view == VIEW_EYES && inPetZone(sx, sy)) {
        petStroke += move;
        if (petStroke > PET_STROKE_FOR_CONTENT) contentUntil = now + CONTENT_LINGER_MS;
      }
      prevX = sx;
      prevY = sy;

      // Detect the swipe while the finger is still moving. Waiting for the
      // release failed whenever the stroke began at the very bottom edge, where
      // readings are marginal and the contact never registered.
      if (view == VIEW_GAME && !swipeArmed && now - contactAt > 1500 && strokePx < 20.0f) {
        // Fallback exit: hold still, no button needed. The corner may simply not
        // register on this panel.
        Serial.println("hold -> exit game");
        exitGame();
        return;
      }

      if (!swipeArmed && !exitPressed) {
        const int16_t dxNow = sx - pressX;
        const int16_t dyNow = sy - pressY;
        const int16_t adxN = (int16_t)abs(dxNow);
        const int16_t adyN = (int16_t)abs(dyNow);
        const uint32_t held = now - contactAt;
        const int16_t speedY = (int16_t)((int32_t)adyN * 1000 / (int32_t)max<uint32_t>(held, 1));
        const int16_t speedX = (int16_t)((int32_t)adxN * 1000 / (int32_t)max<uint32_t>(held, 1));

        if (view == VIEW_GAME) {
          // Modal screen: a short flick is unambiguous here.
          if (adxN > SWIPE_GAME || adyN > SWIPE_GAME) {
            const MoveDir dir =
                (adxN > adyN) ? (dxNow > 0 ? MOVE_RIGHT : MOVE_LEFT)
                              : (dyNow > 0 ? MOVE_DOWN : MOVE_UP);
            swipeArmed = true;
            game.move(dir);
            if (game.over()) showGameOver();
          }
        } else if (view == VIEW_POMODORO) {
          // Modal screen: no gaze to fight with, so a short flick is enough.
          // Down resets the timer, up leaves.
          if (dyNow > SWIPE_GAME && adyN > adxN) {
            swipeArmed = true;
            pomodoro.reset();
            Serial.println("pomodoro reset");
          } else if (dyNow < -SWIPE_GAME && adyN > adxN) {
            swipeArmed = true;
            exitPomodoro();
            Serial.println("pomodoro exit");
          }
        } else if (view == VIEW_SETTINGS) {
          // Swipe right goes back, mirroring the left swipe that opened it.
          if (dxNow > SWIPE_GAME && adxN > adyN) {
            swipeArmed = true;
            exitSettings();
            Serial.println("settings exit");
          }
        } else if (dyNow < -SWIPE_UP_Y && adyN > adxN && speedY >= SWIPE_UP_SPEED) {
          swipeArmed = true;
          wantPhoto = true;  // opens the viewer, or advances it
          Serial.printf("swipe up (%d px/s)\n", speedY);
        } else if (view == VIEW_EYES && dxNow > SWIPE_GAME_X && adxN > adyN &&
                   speedX >= SWIPE_GAME_SPEED) {
          swipeArmed = true;
          enterGame();
          Serial.printf("swipe right (%d px/s)\n", speedX);
        } else if (view == VIEW_EYES && dyNow > SWIPE_DOWN_Y && adyN > adxN &&
                   speedY >= SWIPE_DOWN_SPEED) {
          swipeArmed = true;
          enterPomodoro();
          Serial.printf("swipe down (%d px/s)\n", speedY);
        } else if (view == VIEW_EYES && dxNow < -SWIPE_GAME_X && adxN > adyN &&
                   speedX >= SWIPE_GAME_SPEED) {
          swipeArmed = true;
          enterSettings();
          Serial.printf("swipe left (%d px/s)\n", speedX);
        }
      }
    }

    // While stroking the petting zone the eyes are shut anyway, so the gaze
    // target is left alone instead of jumping to the finger.
    if (view == VIEW_EYES && !inPetZone(sx, sy)) {
      eyes.look((float)(sx - EYE_CX) / (float)EYE_CX, (float)(sy - 120) / 120.0f);
    }
    return;
  }

  if (!touching || now - lastSeen <= RELEASE_MS) return;

  touching = false;
  lastRelease = now;

  const int16_t dx = prevX - pressX;
  const int16_t dy = prevY - pressY;
  const int16_t adx = (int16_t)abs(dx);
  const int16_t ady = (int16_t)abs(dy);
  const bool tap = (now - contactAt) < TAP_MAX_MS;

  Serial.printf("gesture dx %d dy %d\n", dx, dy);

  if (swipeArmed) {
    // The swipe already acted while the finger was moving. Acting again on
    // release is what drew two pictures from a single swipe.
    swipeArmed = false;
    return;
  }

  if (view == VIEW_SETTINGS) {
    if (tap) {
      const int id = settings.hit(pressX, pressY);
      if (id >= 0) {
        settings.apply(id);
        eyes.setEyeColour(settings.eyeColour());
        Serial.printf("settings apply %d\n", id);
      }
    }
    return;
  }

  if (view == VIEW_POMODORO) {
    if (tap) pomodoro.toggle(now);
    return;
  }

  if (view == VIEW_GAME) {
    // A press that began on the button counts as a click even if the panel
    // jittered a few pixels on the way up.
    if (exitPressed && adx < 30 && ady < 30) {
      exitPressed = false;
      exitGame();
      return;
    }
    exitPressed = false;
    if (game.over() && tap) {
      game.newGame();
    }
    return;
  }

  if (view == VIEW_PHOTO) {
    // Up advances to another picture, down returns to the face. A tap does
    // nothing here on purpose: it is too easy to hit by accident while looking.
    if (dy < -SWIPE_UP_Y && ady > adx) {
      wantPhoto = true;
      Serial.println("swipe up -> next photo");
    } else if (dy > SWIPE_PHOTO_DOWN && ady > adx) {
      exitPhoto();
    }
    return;
  }

  if (tap) eyes.blinkNow();

  // No swipe handling here: the live detector already acted while the finger was
  // moving. Reacting again on release is what fired on slow drags meant only to
  // steer the gaze.
}

static EyeEmotion moodFor(uint32_t now) {
  if (now < angryUntil) return EM_ANGRY;
  if (now < surpriseUntil) return EM_SURPRISED;
  if (now < contentUntil) return EM_CONTENT;

  if (touching) {
    return strokePx > STROKE_FOR_PLAYFUL ? EM_PLAYFUL : EM_HAPPY;
  }

  uint32_t idle = now - lastSeen;
  if (idle > IDLE_ASLEEP_MS) return EM_ASLEEP;
  if (idle > IDLE_SLEEPY_MS) return EM_SLEEPY;
  if (idle > IDLE_DAZED_MS) return EM_DAZED;
  return EM_NORMAL;
}

// No mouth drawing: removed deliberately, it always overlapped the eyes.


// Test pattern drawn straight to the panel, no JPEG involved: pure colour bars,
// a 16-step grey ramp and 1 px / 2 px checkerboards. It separates "the display
// cannot show it" from "the image data is bad".
static void drawTestPattern() {
  view = VIEW_PHOTO;  // reuse the photo view so the eyes do not repaint over it
  photoUntil = millis() + 30000;

  tft.fillScreen(TFT_BLACK);

  const uint8_t levels[8][3] = {
      {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255},
      {0, 255, 255}, {255, 0, 255}, {255, 255, 0}, {0, 0, 0}};
  for (uint8_t i = 0; i < 8; i++) {
    tft.fillRect(i * 40, 0, 40, 50, tft.color565(levels[i][0], levels[i][1], levels[i][2]));
  }

  // Continuous gradient, not stepped: 320 px wide across the full 0..255 range.
  // Any banding here is the panel's RGB565 quantisation (~32 grey levels).
  for (int16_t x = 0; x < 320; x++) {
    const uint8_t v = (uint8_t)((x * 255) / 319);
    tft.drawFastVLine(x, 60, 22, tft.color565(v, v, v));
  }

  // Same gradient with a 4x4 ordered dither, to see whether noise hides the
  // bands better than clean quantisation does.
  static const uint8_t bayer[4][4] = {
      {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
  for (int16_t x = 0; x < 320; x++) {
    const int16_t base = (x * 255) / 319;
    for (int16_t y = 0; y < 22; y++) {
      const int16_t d = base + (bayer[y & 3][x & 3] - 8);
      const uint8_t v = (uint8_t)constrain(d, 0, 255);
      tft.drawPixel(x, 88 + y, tft.color565(v, v, v));
    }
  }

  // Fine detail: if these merge into grey, resolution is being lost somewhere.
  for (int16_t y = 0; y < 40; y++) {
    for (int16_t x = 0; x < 120; x++) {
      if ((x ^ y) & 1) tft.drawPixel(10 + x, 120 + y, TFT_WHITE);
    }
  }
  for (int16_t y = 0; y < 40; y++) {
    for (int16_t x = 0; x < 120; x++) {
      if (((x >> 1) ^ (y >> 1)) & 1) tft.drawPixel(170 + x, 120 + y, TFT_WHITE);
    }
  }

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("bars / gradient / dithered / 1px+2px", 8, 186, 2);
  Serial.println("test pattern");
}

void setup() {
  Serial.setRxBufferSize(4096);
  Serial.begin(115200);
  delay(200);
  Serial.println("\nCYD eyes boot");

  board::begin();
  touch.begin();

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Take the backlight over so it can be dimmed; TFT_eSPI drove it high at init.
  ledcSetup(BL_CHANNEL, 5000, 8);
  ledcAttachPin(PIN_BL, BL_CHANNEL);
  ledcWrite(BL_CHANNEL, BL_FULL);

  gallery.begin();
  game.begin(&tft);
  pomodoro.begin(&tft);
  settings.begin(&tft);
  eyes.setEyeColour(settings.eyeColour());
  randomSeed((uint32_t)analogRead(board::PIN_LDR) ^ (uint32_t)micros());
  uploaderSetShowHandler([]() { wantPhoto = true; });
  uploaderSetTestHandler([]() { drawTestPattern(); });

  eyes.begin(&tft, EYE_CX, EYE_CY);
  eyes.setEmotion(EM_NORMAL, true);

  lastSeen = millis();
  lastRelease = millis();

  Serial.println("ready");
}

void loop() {
  uint32_t now = millis();

  // Cheap check; blocks only while a file is actually arriving.
  uploaderService();

  if (now - lastTouch >= TOUCH_MS) {
    lastTouch = now;
    readTouch(now);
  }

  if (wantPhoto) {
    wantPhoto = false;
    if (gallery.showRandom(&tft)) {
      view = VIEW_PHOTO;
      photoUntil = now + 12000;
    }
  }

  if (view == VIEW_PHOTO && now > photoUntil) {
    view = VIEW_EYES;
    tft.fillScreen(TFT_BLACK);
  }

  if (view == VIEW_POMODORO) {
    pomodoro.tick(now);
  }

  // Dim the backlight after a while with no interaction; any touch restores it.
  static int lastDuty = -1;
  static bool backlightCut = false;
  const uint32_t idle = now - lastSeen;
  int duty = settings.fullDuty();
  if (idle >= settings.offMs()) {
    duty = BL_OFF;
  } else if (view == VIEW_EYES && idle >= DIM_AFTER_MS) {
    duty = settings.dimDuty();
  }
  if (duty != lastDuty) {
    lastDuty = duty;
    if (duty == BL_OFF) {
      // Turn the panel off and take the pin away from LEDC entirely: PWM at 0
      // still leaves the output driven, and some backlight transistors leak.
      tft.writecommand(0x28);  // display off
      ledcWrite(BL_CHANNEL, 0);
      ledcDetachPin(PIN_BL);
      pinMode(PIN_BL, OUTPUT);
      digitalWrite(PIN_BL, LOW);
      backlightCut = true;
    } else {
      if (backlightCut) {
        tft.writecommand(0x29);  // display on
        ledcSetup(BL_CHANNEL, 5000, 8);
        ledcAttachPin(PIN_BL, BL_CHANNEL);
        backlightCut = false;
      }
      ledcWrite(BL_CHANNEL, duty);
    }
    Serial.printf("bl %d idle %lus\n", duty, (unsigned long)(idle / 1000));
  }

  // Frame budget follows what is actually visible: 40 fps when someone is
  // looking, 25 when dimmed, 10 while asleep, and nothing at all with the
  // backlight off.
  uint32_t interval = FRAME_MS;
  if (duty == BL_DIM) {
    interval = 40;
  } else if (duty == BL_FULL && !touching && idle >= IDLE_SLEEPY_MS) {
    interval = 100;
  }

  if (view == VIEW_EYES && duty != BL_OFF && now - lastFrame >= interval) {
    lastFrame = now;

    EyeEmotion mood = moodFor(now);
    static EyeEmotion lastMood = EM_NORMAL;
    if (mood != lastMood) {
      Serial.printf("mood %u\n", (unsigned)mood);
      lastMood = mood;
    }
    eyes.setEmotion(mood);
    // The glyph is composed inside the right eye sprite, so it cannot be
    // erased between frames the way a separate overlay was.
    eyes.setOverlay(mood == EM_ASLEEP ? "Zzz" : nullptr);

    // Yawn: a timed animation inside SLEEPY, not a static pose. sin() gives
    // open, hold and close in one curve, and the mouth rises over the eyes
    // while the eyes squeeze shut.
    static bool yawning = false;
    static uint32_t yawnStart = 0;
    static uint32_t nextYawn = 0;
    float yawn = 0.0f;
    if (mood == EM_SLEEPY) {
      if (!yawning && (nextYawn == 0 || now >= nextYawn)) {
        yawning = true;
        yawnStart = now;
      }
      if (yawning) {
        float t = (float)(now - yawnStart) / (float)YAWN_DURATION_MS;
        if (t >= 1.0f) {
          yawning = false;
          nextYawn = now + (uint32_t)random(5000, 9000);
        } else {
          yawn = sinf(3.14159f * t);
        }
      }
    } else {
      yawning = false;
      nextYawn = 0;
    }
    eyes.setYawn(yawn);

    if (mood == EM_ASLEEP || mood == EM_CONTENT) {
      eyes.look(0.0f, 0.0f);
    } else if (!touching) {
      // Slow idle drift so the face never looks frozen. A dazed pet drifts
      // markedly slower: motion is the tell, not the eyelid angle.
      const bool dazed = (mood == EM_DAZED);
      const float speed = dazed ? 0.14f : 0.35f;
      const float amp = dazed ? 0.38f : 0.60f;
      float t = (float)now / 1000.0f;
      eyes.look(sinf(t * speed) * amp, sinf(t * speed * 0.65f) * amp * 0.7f);
    }

    eyes.update(now);
    framesRendered++;
  }

  if (now - lastLog >= 10000) {
    lastLog = now;
    Serial.printf("fps %u\n", framesRendered / 10);
    framesRendered = 0;
  }
}
