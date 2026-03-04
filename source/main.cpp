#include <3ds.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>

#include "app.h"
#include "network.h"

// Default 3DS stack is 32KB — far too small for 32KB SFTP buffers
u32 __stacksize__ = 256 * 1024; // 256KB

int main(int argc, char* argv[]) {
    gfxInitDefault();

    PrintConsole topConsole, bottomConsole;
    consoleInit(GFX_TOP, &topConsole);
    consoleInit(GFX_BOTTOM, &bottomConsole);

    // Initialize networking (1MB SOC buffer)
    u32* socBuffer = (u32*)memalign(0x1000, 0x100000);
    if (socBuffer) {
        socInit(socBuffer, 0x100000);
    }

    App app(&topConsole, &bottomConsole);
    app.run();

    if (socBuffer) {
        socExit();
        free(socBuffer);
    }
    gfxExit();
    return 0;
}
