// CYD bring-up probe for ESP32-2432S028 (two USB ports, CH340C).
//
// What it checks:
//   1. Panel driver and colour order  - four labelled colour blocks.
//      RED/GREEN/BLUE/WHITE must appear in that order, black must stay black.
//      Swapped red and blue  -> colour order is wrong (need TFT_RGB_ORDER TFT_BGR).
//      Everything negative   -> panel needs invertDisplay(1), i.e. it is ST7789.
//   2. RGB LED on IO4/IO16/IO17 (active LOW) - cycles one channel at a time.
//   3. LDR on IO34 - live value and bar on screen, printed to serial.

#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

const int PIN_LED_R = 4;
const int PIN_LED_G = 16;
const int PIN_LED_B = 17;
const int PIN_LDR = 34;

const int BAR_X = 8;
const int BAR_Y = 196;
const int BAR_W = 304;
const int BAR_H = 22;

void ledAllOff() {
  // The CYD LEDs are active LOW.
  digitalWrite(PIN_LED_R, HIGH);
  digitalWrite(PIN_LED_G, HIGH);
  digitalWrite(PIN_LED_B, HIGH);
}

void ledChannel(int pin) {
  ledAllOff();
  digitalWrite(pin, LOW);
}

void drawLayout() {
  tft.fillScreen(TFT_BLACK);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("CYD PROBE", 8, 4, 4);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("320x240 landscape", 312, 8, 2);

  const uint16_t colours[4] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE};
  const char *names[4] = {"RED", "GREEN", "BLUE", "WHITE"};

  for (int i = 0; i < 4; i++) {
    int x = i * 80;
    tft.fillRect(x, 34, 80, 140, colours[i]);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(i == 3 ? TFT_BLACK : TFT_WHITE, colours[i]);
    tft.drawString(names[i], x + 40, 104, 2);
  }

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("LDR IO34", BAR_X, BAR_Y + BAR_H / 2, 2);
}

void drawLdr(int value) {
  tft.fillRect(BAR_X + 76, BAR_Y, 124, BAR_H, TFT_BLACK);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(value), BAR_X + 80, BAR_Y + BAR_H / 2, 2);

  int width = map(value, 0, 4095, 0, 100);
  tft.fillRect(BAR_X + 208, BAR_Y, 100, BAR_H, TFT_BLACK);
  tft.drawRect(BAR_X + 208, BAR_Y, 100, BAR_H, TFT_DARKGREY);
  tft.fillRect(BAR_X + 209, BAR_Y + 1, width, BAR_H - 2, TFT_YELLOW);
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println();
  Serial.println("CYD probe start");

  ledAllOff();
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  ledAllOff();

  analogReadResolution(12);

  tft.init();
  tft.setRotation(1);
  drawLayout();

  Serial.println("Panel initialised with ILI9341_2_DRIVER at 40 MHz.");
  Serial.println("Report: colour order, negative image, LED channels, LDR range.");
}

void loop() {
  static uint32_t lastLed = 0;
  static uint32_t lastLdr = 0;
  static uint8_t ledStep = 0;

  if (millis() - lastLed > 700) {
    lastLed = millis();
    switch (ledStep) {
      case 0: ledChannel(PIN_LED_R); Serial.println("LED red"); break;
      case 1: ledChannel(PIN_LED_G); Serial.println("LED green"); break;
      case 2: ledChannel(PIN_LED_B); Serial.println("LED blue"); break;
      default: ledAllOff(); Serial.println("LED off"); break;
    }
    ledStep = (ledStep + 1) % 4;
  }

  if (millis() - lastLdr > 250) {
    lastLdr = millis();
    int value = analogRead(PIN_LDR);
    drawLdr(value);
    Serial.printf("LDR %d\n", value);
  }
}
