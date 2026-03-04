#include "ui.h"
#include <cstdio>
#include <cstring>

namespace UI {

void clear(PrintConsole* con) {
    consoleSelect(con);
    printf("\x1b[2J\x1b[H");
}

void setColor(int ansiCode) {
    printf("\x1b[%dm", ansiCode);
}

void resetColor() {
    printf("\x1b[0m");
}

void drawHeader(PrintConsole* con, const char* title) {
    consoleSelect(con);
    setColor(1); // bold
    setColor(36); // cyan
    printf("\n  %s\n", title);
    resetColor();

    // Draw separator line
    printf("  ");
    for (int i = 0; i < 46; i++) printf("-");
    printf("\n");
}

} // namespace UI
