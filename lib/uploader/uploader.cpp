#include "uploader.h"

#include <SD.h>

namespace {

const char *DIR = "/cyd";
const size_t CHUNK = 512;

void (*g_showHandler)() = nullptr;
void (*g_testHandler)() = nullptr;

String readLine(uint32_t timeoutMs) {
  String line;
  uint32_t lastByte = millis();
  while (millis() - lastByte < timeoutMs) {
    if (!Serial.available()) continue;
    const char c = (char)Serial.read();
    lastByte = millis();
    if (c == '\n') break;
    if (c != '\r' && line.length() < 96) line += c;
  }
  return line;
}

}  // namespace

void uploaderSetShowHandler(void (*handler)()) { g_showHandler = handler; }
void uploaderSetTestHandler(void (*handler)()) { g_testHandler = handler; }

void uploaderService() {
  if (Serial.available() < 5) return;

  const String header = readLine(600);
  if (header == "SHOW") {
    if (g_showHandler) g_showHandler();
    return;
  }
  if (header == "TEST") {
    if (g_testHandler) g_testHandler();
    return;
  }
  if (header == "LIST") {
    File dir = SD.open(DIR);
    if (!dir) {
      Serial.println("ERR nodir");
      return;
    }
    File entry = dir.openNextFile();
    while (entry) {
      Serial.printf("F %s %u\n", entry.name(), (unsigned)entry.size());
      entry.close();
      entry = dir.openNextFile();
    }
    dir.close();
    Serial.println("END");
    return;
  }
  if (!header.startsWith("PUT ")) return;

  char name[64] = {0};
  uint32_t size = 0;
  if (sscanf(header.c_str(), "PUT %63s %u", name, &size) != 2 || size == 0) {
    Serial.println("ERR header");
    return;
  }

  if (!SD.exists(DIR)) SD.mkdir(DIR);
  const String path = String(DIR) + "/" + name;
  File file = SD.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("ERR open");
    return;
  }

  uint8_t buffer[CHUNK];
  uint32_t written = 0;
  uint32_t lastProgress = millis();

  while (written < size) {
    const size_t want = min((uint32_t)CHUNK, size - written);
    const size_t got = Serial.readBytes(buffer, want);
    if (got == 0) {
      // Nothing arrived: give the host more time, but not forever.
      if (millis() - lastProgress > 5000) {
        file.close();
        Serial.printf("ERR timeout %u\n", written);
        return;
      }
      continue;
    }
    file.write(buffer, got);
    written += got;
    lastProgress = millis();
  }

  file.close();
  Serial.printf("OK %s %u\n", name, written);
}
