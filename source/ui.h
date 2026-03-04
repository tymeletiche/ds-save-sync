#pragma once

#include <3ds.h>

namespace UI {

void clear(PrintConsole* con);
void setColor(int ansiCode);
void resetColor();
void drawHeader(PrintConsole* con, const char* title);

} // namespace UI
