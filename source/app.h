#pragma once

#include <3ds.h>
#include <string>
#include <vector>

#include "config.h"
#include "network.h"
#include "sync.h"
#include "twilight.h"

enum class Screen {
    MainMenu,
    SyncSave,
    SyncProgress,
    ConflictResolve,
    RomLibrary,
    Settings
};

class App {
public:
    App(PrintConsole* top, PrintConsole* bottom);

    void run();

private:
    void drawMainMenu(u32 kDown);
    void drawSyncSave(u32 kDown);
    void drawSyncProgress(u32 kDown);
    void drawConflictResolve(u32 kDown);
    void drawRomLibrary(u32 kDown);
    void drawSettings(u32 kDown);

    void startSync(bool push);
    void doSyncStep();

    PrintConsole* m_top;
    PrintConsole* m_bottom;
    Screen m_screen;
    int m_menuSelection;
    bool m_needsRedraw;

    Config m_config;
    Twilight::LastPlayed m_lastPlayed;
    Network::Connection m_conn;
    bool m_connected;

    // Sync state
    Sync::SaveInfo m_saveInfo;
    Sync::ConflictAction m_conflictChoice;
    std::string m_statusMsg;
    bool m_syncInProgress;
    bool m_syncPush; // true = push local to server, false = pull from server

    // ROM library state
    std::vector<std::string> m_romList;
    int m_romSelection;
};
