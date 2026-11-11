#include "gallery.h"

#include <SD.h>
#include <SPI.h>
#include <TJpg_Decoder.h>

namespace {

const int PIN_SD_CS = 5;
const int PIN_SD_SCK = 18;
const int PIN_SD_MISO = 19;
const int PIN_SD_MOSI = 23;
const uint32_t SD_FREQ = 20000000;

const char *PHOTO_DIR = "/cyd";

TFT_eSPI *g_tft = nullptr;

// TJpgDec hands back MCU-sized blocks; pushing each one straight to the panel
// keeps the whole image out of RAM.
bool pushBlock(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (g_tft == nullptr) return false;
  if (y >= g_tft->height()) return false;
  g_tft->pushImage(x, y, w, h, bitmap);
  return true;
}

bool isJpeg(const char *name) {
  const size_t len = strlen(name);
  if (len < 5) return false;
  const char *ext = name + len - 4;
  return (strncasecmp(ext, ".jpg", 4) == 0) || (strncasecmp(ext, ".jpeg", 4) == 0);
}

}  // namespace

bool Gallery::begin() {
  static SPIClass sdSpi(VSPI);
  sdSpi.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);

  _ready = SD.begin(PIN_SD_CS, sdSpi, SD_FREQ);
  Serial.printf("sd %s\n", _ready ? "ok" : "missing");
  if (!_ready) return false;

  const uint16_t n = count();
  Serial.printf("photos %u\n", n);
  return true;
}

uint16_t Gallery::count() {
  if (!_ready) return 0;

  File dir = SD.open(PHOTO_DIR);
  if (!dir || !dir.isDirectory()) {
    Serial.println("no /cyd folder");
    return 0;
  }

  uint16_t n = 0;
  File entry = dir.openNextFile();
  while (entry) {
    if (!entry.isDirectory() && isJpeg(entry.name())) n++;
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  return n;
}

bool Gallery::showRandom(TFT_eSPI *tft) {
  if (!_ready) return false;

  const uint16_t total = count();
  if (total == 0) return false;

  uint16_t pick = (uint16_t)random(total);
  // Avoid showing the same picture twice in a row when advancing.
  static uint16_t lastPick = 0xFFFF;
  if (total > 1 && pick == lastPick) pick = (uint16_t)((pick + 1) % total);
  lastPick = pick;

  File dir = SD.open(PHOTO_DIR);
  if (!dir) return false;

  // Walk to the chosen entry, closing every handle as we go. Keeping a child
  // open while closing the parent corrupts the SD library state, which is why
  // the decoder then reported "Jpeg file not found".
  char chosen[64] = {0};
  uint16_t index = 0;
  File entry = dir.openNextFile();
  while (entry) {
    if (!entry.isDirectory() && isJpeg(entry.name())) {
      if (index == pick) {
        strncpy(chosen, entry.name(), sizeof(chosen) - 1);
        entry.close();
        break;
      }
      index++;
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();

  if (chosen[0] == '\0') return false;

  char path[80];
  snprintf(path, sizeof(path), "%s/%s", PHOTO_DIR, chosen);

  // Read the whole file into RAM and decode from the array. Handing a File to
  // the decoder failed part way through on this core (the image was drawn and
  // then abandoned), which looked like flicker because the eyes repainted over
  // it. At 320x240 the file is about 16 kB, so a buffer costs little.
  File photo = SD.open(path, FILE_READ);
  if (!photo) {
    Serial.printf("open failed %s\n", path);
    return false;
  }

  const size_t size = photo.size();
  if (size == 0 || size > 65536) {
    Serial.printf("bad size %u\n", (unsigned)size);
    photo.close();
    return false;
  }

  uint8_t *data = (uint8_t *)malloc(size);
  if (data == nullptr) {
    Serial.println("no ram for jpeg");
    photo.close();
    return false;
  }
  const size_t got = photo.read(data, size);
  photo.close();
  if (got != size) {
    Serial.printf("short read %u/%u\n", (unsigned)got, (unsigned)size);
    free(data);
    return false;
  }

  uint16_t w = 0, h = 0;
  if (TJpgDec.getJpgSize(&w, &h, data, size) != 0 || w == 0 || h == 0) {
    Serial.printf("bad jpeg %s\n", path);
    free(data);
    return false;
  }

  // Only 1:1, 1:2, 1:4 and 1:8 are supported by the decoder.
  uint8_t scale = 1;
  while ((w / scale) > tft->width() || (h / scale) > tft->height()) {
    scale *= 2;
    if (scale > 8) break;
  }

  const int16_t dw = w / scale;
  const int16_t dh = h / scale;
  const int16_t ox = (tft->width() - dw) / 2;
  const int16_t oy = (tft->height() - dh) / 2;

  g_tft = tft;
  tft->fillScreen(TFT_BLACK);
  TJpgDec.setJpgScale(scale);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(pushBlock);

  Serial.printf("show %s %ux%u scale 1/%u\n", path, w, h, scale);
  const int result = TJpgDec.drawJpg(ox, oy, data, size);
  g_tft = nullptr;
  free(data);
  if (result != 0) Serial.printf("draw rc %d\n", result);
  return result == 0;
}
