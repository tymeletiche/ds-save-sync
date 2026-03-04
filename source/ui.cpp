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

void drawProgressBar(PrintConsole* con, int current, int total, const char* label) {
    consoleSelect(con);
    int pct = (total > 0) ? (current * 100 / total) : 0;
    if (pct > 100) pct = 100;
    printf("\x1b[10;1H");  // fixed position row 10, overwrite each call
    if (label) printf("  %s\n", label);
    int filled = pct * 30 / 100;
    printf("  [");
    for (int i = 0; i < 30; i++) printf(i < filled ? "#" : "-");
    printf("] %3d%%\n", pct);
}

} // namespace UI
