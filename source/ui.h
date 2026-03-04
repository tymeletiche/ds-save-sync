#pragma once

#include <3ds.h>

namespace UI {

void clear(PrintConsole* con);
void setColor(int ansiCode);
void resetColor();
void drawHeader(PrintConsole* con, const char* title);
void drawProgressBar(PrintConsole* con, int current, int total, const char* label);

} // namespace UI
