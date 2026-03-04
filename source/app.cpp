#include "app.h"
#include "ui.h"
#include "util.h"

#include <cstdio>
#include <cstring>

static PrintConsole* s_progressConsole = nullptr;

static void progressCallback(int current, int total) {
    if (s_progressConsole) {
        UI::drawProgressBar(s_progressConsole, current, total, nullptr);
    }
}

App::App(PrintConsole* top, PrintConsole* bottom)
    : m_top(top)
    , m_bottom(bottom)
    , m_screen(Screen::MainMenu)
    , m_menuSelection(0)
    , m_needsRedraw(true)
    , m_connected(false)
    , m_conflictChoice(Sync::ConflictAction::Cancel)
    , m_syncInProgress(false)
    , m_syncPush(false)
    , m_romSelection(0) {
    m_conn = {};
    m_saveInfo = {};
}

void App::run() {
    m_config.load();
    m_lastPlayed = Twilight::detectLastPlayed(m_config);
    m_screen = Screen::MainMenu;
    m_needsRedraw = true;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START && m_screen == Screen::MainMenu) break;

        switch (m_screen) {
            case Screen::MainMenu:       drawMainMenu(kDown); break;
            case Screen::SyncSave:       drawSyncSave(kDown); break;
            case Screen::SyncProgress:   drawSyncProgress(kDown); break;
            case Screen::ConflictResolve: drawConflictResolve(kDown); break;
            case Screen::RomLibrary:     drawRomLibrary(kDown); break;
            case Screen::Settings:       drawSettings(kDown); break;
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    if (m_connected) {
        Network::disconnect(m_conn);
    }
}

void App::drawMainMenu(u32 kDown) {
    if (m_needsRedraw) {
        UI::clear(m_top);
        consoleSelect(m_top);
        UI::drawHeader(m_top, "DS Save Sync");
        printf("\n");
        if (m_lastPlayed.found) {
            printf("  Last played:\n");
            UI::setColor(36); // cyan
            printf("  %s\n", m_lastPlayed.gameName.c_str());
            UI::resetColor();
        } else {
            UI::setColor(33); // yellow
            printf("  No last played game detected\n");
            UI::resetColor();
        }
        printf("\n  Server: %s\n", m_config.serverHost.c_str());

        if (!m_statusMsg.empty()) {
            printf("\n");
            UI::setColor(32); // green
            printf("  %s\n", m_statusMsg.c_str());
            UI::resetColor();
        }

        UI::clear(m_bottom);
        consoleSelect(m_bottom);

        const char* items[] = {
            "Sync Save",
            "ROM Library",
            "Settings"
        };
        int count = 3;

        for (int i = 0; i < count; i++) {
            if (i == m_menuSelection) {
                UI::setColor(36);
                printf("  > %s\n", items[i]);
                UI::resetColor();
            } else {
                printf("    %s\n", items[i]);
            }
        }
        printf("\n  [START] Exit\n");

        m_needsRedraw = false;
    }

    if (kDown & KEY_DOWN) {
        m_menuSelection = (m_menuSelection + 1) % 3;
        m_needsRedraw = true;
    }
    if (kDown & KEY_UP) {
        m_menuSelection = (m_menuSelection + 2) % 3;
        m_needsRedraw = true;
    }
    if (kDown & KEY_A) {
        switch (m_menuSelection) {
            case 0:
                m_screen = Screen::SyncSave;
                m_needsRedraw = true;
                break;
            case 1:
                m_screen = Screen::RomLibrary;
                m_needsRedraw = true;
                break;
            case 2:
                m_screen = Screen::Settings;
                m_needsRedraw = true;
                break;
        }
    }
}

void App::drawSyncSave(u32 kDown) {
    if (m_needsRedraw) {
        UI::clear(m_top);
        consoleSelect(m_top);
        UI::drawHeader(m_top, "Sync Save");

        if (!m_lastPlayed.found) {
            UI::setColor(31); // red
            printf("\n  No last played game detected.\n");
            printf("  Play a game first via TWiLight Menu++.\n");
            UI::resetColor();

            UI::clear(m_bottom);
            consoleSelect(m_bottom);
            printf("  [B] Back\n");
            m_needsRedraw = false;

            if (kDown & KEY_B) {
                m_screen = Screen::MainMenu;
                m_needsRedraw = true;
            }
            return;
        }

        printf("\n  Game: %s\n", m_lastPlayed.gameName.c_str());
        printf("  Connecting to %s...\n", m_config.serverHost.c_str());

        m_needsRedraw = false;

        // Connect and get save info
        if (!m_connected) {
            if (Network::connect(m_conn, m_config)) {
                m_connected = true;
            } else {
                UI::setColor(31);
                printf("\n  Connection failed!\n");
                printf("  %s\n", Network::getLastError());
                UI::resetColor();

                UI::clear(m_bottom);
                consoleSelect(m_bottom);
                printf("  [B] Back\n");
                return;
            }
        }

        // Get save info from both sides
        std::string saveName = m_lastPlayed.gameName + ".sav";
        m_saveInfo = Sync::getSaveInfo(m_conn, saveName.c_str(),
                                        m_lastPlayed.savePath.c_str(), m_config);

        // Display comparison
        consoleSelect(m_top);
        printf("\n  Save: %s\n\n", saveName.c_str());

        if (m_saveInfo.existsLocally) {
            printf("  Local:  %s (%s)\n",
                   Util::formatTimestamp(m_saveInfo.localTimestamp).c_str(),
                   Util::formatSize(m_saveInfo.localSize).c_str());
        } else {
            printf("  Local:  (not found)\n");
        }

        if (m_saveInfo.existsRemotely) {
            printf("  Server: %s (%s)\n",
                   Util::formatTimestamp(m_saveInfo.remoteTimestamp).c_str(),
                   Util::formatSize(m_saveInfo.remoteSize).c_str());
        } else {
            printf("  Server: (not found)\n");
        }

        // Determine action
        UI::clear(m_bottom);
        consoleSelect(m_bottom);

        if (Sync::hasConflict(m_saveInfo)) {
            UI::setColor(33);
            printf("\n  Conflict detected!\n");
            UI::resetColor();
            printf("  [A] Resolve conflict\n");
        } else if (m_saveInfo.existsLocally && !m_saveInfo.existsRemotely) {
            printf("\n  Local save only. Push to cloud?\n");
            printf("  [A] Push save\n");
        } else if (!m_saveInfo.existsLocally && m_saveInfo.existsRemotely) {
            printf("\n  Cloud save only. Pull to 3DS?\n");
            printf("  [A] Pull save\n");
        } else if (m_saveInfo.existsLocally && m_saveInfo.existsRemotely) {
            UI::setColor(32);
            printf("\n  Saves are in sync!\n");
            UI::resetColor();
        } else {
            printf("\n  No save found on either side.\n");
        }
        printf("  [B] Back\n");
    }

    if (kDown & KEY_B) {
        m_screen = Screen::MainMenu;
        m_needsRedraw = true;
    }
    if (kDown & KEY_A) {
        if (Sync::hasConflict(m_saveInfo)) {
            m_screen = Screen::ConflictResolve;
            m_needsRedraw = true;
        } else if (m_saveInfo.existsLocally && !m_saveInfo.existsRemotely) {
            startSync(true);
        } else if (!m_saveInfo.existsLocally && m_saveInfo.existsRemotely) {
            startSync(false);
        }
    }
}

void App::drawSyncProgress(u32 kDown) {
    if (m_needsRedraw) {
        UI::clear(m_top);
        consoleSelect(m_top);
        UI::drawHeader(m_top, "Syncing...");
        printf("\n  %s %s\n",
               m_syncPush ? "Pushing" : "Pulling",
               m_lastPlayed.gameName.c_str());

        m_needsRedraw = false;
        doSyncStep();
    }
}

void App::drawConflictResolve(u32 kDown) {
    if (m_needsRedraw) {
        UI::clear(m_top);
        consoleSelect(m_top);
        UI::drawHeader(m_top, "Conflict Detected");

        UI::setColor(33);
        printf("\n  Both saves have been modified!\n\n");
        UI::resetColor();

        printf("  Save: %s.sav\n\n", m_lastPlayed.gameName.c_str());
        printf("  Local:  %s (%s)\n",
               Util::formatTimestamp(m_saveInfo.localTimestamp).c_str(),
               Util::formatSize(m_saveInfo.localSize).c_str());
        printf("  Server: %s (%s)\n",
               Util::formatTimestamp(m_saveInfo.remoteTimestamp).c_str(),
               Util::formatSize(m_saveInfo.remoteSize).c_str());

        UI::clear(m_bottom);
        consoleSelect(m_bottom);
        printf("\n  Choose which save to keep:\n\n");
        printf("  [A] Keep LOCAL (overwrite cloud)\n");
        printf("  [X] Keep CLOUD (overwrite local)\n");
        printf("  [B] Cancel (do nothing)\n");

        m_needsRedraw = false;
    }

    if (kDown & KEY_A) {
        m_conflictChoice = Sync::ConflictAction::UseLocal;
        startSync(true);
    }
    if (kDown & KEY_X) {
        m_conflictChoice = Sync::ConflictAction::UseRemote;
        startSync(false);
    }
    if (kDown & KEY_B) {
        m_screen = Screen::SyncSave;
        m_needsRedraw = true;
    }
}

void App::drawRomLibrary(u32 kDown) {
    if (m_needsRedraw) {
        UI::clear(m_top);
        consoleSelect(m_top);
        UI::drawHeader(m_top, "ROM Library");

        if (!m_connected) {
            printf("\n  Connecting to %s...\n", m_config.serverHost.c_str());
            if (Network::connect(m_conn, m_config)) {
                m_connected = true;
            } else {
                UI::setColor(31);
                printf("\n  Connection failed!\n");
                printf("  %s\n", Network::getLastError());
                UI::resetColor();

                UI::clear(m_bottom);
                consoleSelect(m_bottom);
                printf("  [B] Back\n");
                m_needsRedraw = false;

                if (kDown & KEY_B) {
                    m_screen = Screen::MainMenu;
                    m_needsRedraw = true;
                }
                return;
            }
        }

        // Fetch ROM list from server
        m_romList.clear();
        std::string output;
        std::string cmd = m_config.scriptPath + " list-roms";
        if (Network::execCommand(m_conn, cmd.c_str(), output)) {
            // Parse pipe-delimited output: name|size per line
            size_t pos = 0;
            while (pos < output.size()) {
                size_t nl = output.find('\n', pos);
                if (nl == std::string::npos) nl = output.size();
                std::string line = output.substr(pos, nl - pos);
                size_t pipe = line.find('|');
                if (pipe != std::string::npos) {
                    m_romList.push_back(line.substr(0, pipe));
                }
                pos = nl + 1;
            }
        }

        UI::clear(m_bottom);
        consoleSelect(m_bottom);

        if (m_romList.empty()) {
            printf("  No ROMs found on server.\n");
        } else {
            int total = (int)m_romList.size();
            int visible = 22;
            int startIdx = 0;
            if (total > visible) {
                startIdx = m_romSelection - visible / 2;
                if (startIdx < 0) startIdx = 0;
                if (startIdx > total - visible) startIdx = total - visible;
            }
            for (int i = startIdx; i < startIdx + visible && i < total; i++) {
                std::string localPath = m_config.localRomsPath + "/" + m_romList[i];
                bool localExists = Util::fileExists(localPath);

                if (i == m_romSelection) {
                    UI::setColor(36);
                    printf(" %s %s\n",
                           localExists ? "+" : " ",
                           m_romList[i].c_str());
                    UI::resetColor();
                } else {
                    printf(" %s %s\n",
                           localExists ? "+" : " ",
                           m_romList[i].c_str());
                }
            }
            if (total > visible) {
                printf("  (%d/%d)\n", m_romSelection + 1, total);
            }
        }
        printf("\n  [A] Download  [B] Back\n");

        m_needsRedraw = false;
    }

    if (kDown & KEY_DOWN && !m_romList.empty()) {
        m_romSelection = (m_romSelection + 1) % (int)m_romList.size();
        m_needsRedraw = true;
    }
    if (kDown & KEY_UP && !m_romList.empty()) {
        m_romSelection = (m_romSelection + (int)m_romList.size() - 1) % (int)m_romList.size();
        m_needsRedraw = true;
    }
    if (kDown & KEY_A && !m_romList.empty()) {
        // Download selected ROM
        std::string romName = m_romList[m_romSelection];
        std::string remotePath = m_config.serverRomsPath + "/" + romName;
        std::string localPath = m_config.localRomsPath + "/" + romName;

        UI::clear(m_top);
        consoleSelect(m_top);
        printf("  Downloading %s...\n", romName.c_str());

        s_progressConsole = m_top;
        if (Network::downloadFile(m_conn, remotePath.c_str(),
                                   localPath.c_str(), progressCallback)) {
            s_progressConsole = nullptr;
            UI::setColor(32);
            printf("  Download complete!\n");
            UI::resetColor();
        } else {
            s_progressConsole = nullptr;
            UI::setColor(31);
            printf("  Download failed!\n");
            printf("  %s\n", Network::getLastError());
            UI::resetColor();
        }
        m_needsRedraw = true;
    }
    if (kDown & KEY_B) {
        m_screen = Screen::MainMenu;
        m_needsRedraw = true;
    }
}

void App::drawSettings(u32 kDown) {
    if (m_needsRedraw) {
        UI::clear(m_top);
        consoleSelect(m_top);
        UI::drawHeader(m_top, "Settings");

        printf("\n  [Server]\n");
        printf("  Host: %s\n", m_config.serverHost.c_str());
        printf("  Port: %d\n", m_config.serverPort);
        printf("  User: %s\n", m_config.sshUser.c_str());
        printf("\n  [SSH]\n");
        printf("  Key:  %s\n", m_config.sshPrivKeyPath.c_str());
        printf("\n  [Paths]\n");
        printf("  Saves: %s\n", m_config.localSavesPath.c_str());
        printf("  ROMs:  %s\n", m_config.localRomsPath.c_str());

        UI::clear(m_bottom);
        consoleSelect(m_bottom);
        printf("\n  Edit config.ini on your PC to\n");
        printf("  change these settings.\n\n");
        printf("  Config: sd:/3ds/ds-save-sync/\n");
        printf("          config.ini\n\n");
        printf("  [B] Back\n");

        m_needsRedraw = false;
    }

    if (kDown & KEY_B) {
        m_screen = Screen::MainMenu;
        m_needsRedraw = true;
    }
}

void App::startSync(bool push) {
    m_syncPush = push;
    m_syncInProgress = true;
    m_screen = Screen::SyncProgress;
    m_needsRedraw = true;
}

void App::doSyncStep() {
    std::string saveName = m_lastPlayed.gameName + ".sav";
    bool ok = false;

    consoleSelect(m_top);

    s_progressConsole = m_top;
    if (m_syncPush) {
        printf("  Uploading save...\n");
        ok = Sync::pushSave(m_conn, m_lastPlayed.savePath.c_str(),
                            saveName.c_str(), m_config, progressCallback);
    } else {
        printf("  Downloading save...\n");
        ok = Sync::pullSave(m_conn, saveName.c_str(),
                            m_lastPlayed.savePath.c_str(), m_config, progressCallback);
    }
    s_progressConsole = nullptr;

    if (ok) {
        UI::setColor(32);
        printf("\n  Sync complete!\n");
        UI::resetColor();
        m_statusMsg = "Last sync: " + saveName;
    } else {
        UI::setColor(31);
        printf("\n  Sync failed!\n");
        printf("  %s\n", Network::getLastError());
        UI::resetColor();
    }

    m_syncInProgress = false;

    UI::clear(m_bottom);
    consoleSelect(m_bottom);
    printf("  [B] Back to menu\n");

    // Wait for B press to return
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_B) break;
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    m_screen = Screen::MainMenu;
    m_needsRedraw = true;
}
