#pragma once

#include <Arduino.h>

// File receiver over the serial line, used because this laptop has no card
// reader and the microSD sits inside the board.
//
// Protocol, one file at a time:
//   host -> "PUT <name> <bytes>\n"   then exactly <bytes> raw bytes
//   board -> "OK <name> <bytes>\n"   or "ERR <reason>\n"
//   host -> "LIST\n"                 prints "F <name> <size>" for each file
//   host -> "SHOW\n"                 triggers the registered handler
void uploaderService();
void uploaderSetShowHandler(void (*handler)());
void uploaderSetTestHandler(void (*handler)());
