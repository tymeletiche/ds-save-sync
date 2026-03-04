#pragma once

#include <string>
#include <ctime>

#include "config.h"
#include "network.h"

namespace Sync {

enum class ConflictAction { UseLocal, UseRemote, Cancel };

struct SaveInfo {
    std::string name;
    bool existsLocally;
    bool existsRemotely;
    time_t localTimestamp;
    time_t remoteTimestamp;
    size_t localSize;
    size_t remoteSize;
};

// Get info about a save on both local SD and server (after cloud pull)
SaveInfo getSaveInfo(Network::Connection& conn, const char* saveName,
                     const char* localSavePath);

// Check if there's a conflict (both exist with different timestamps)
bool hasConflict(const SaveInfo& info);

// Push local save to server, then trigger cloud push
bool pushSave(Network::Connection& conn, const char* localPath,
              const char* saveName, const Config& cfg);

// Pull save from server to local SD
bool pullSave(Network::Connection& conn, const char* saveName,
              const char* localPath, const Config& cfg);

} // namespace Sync
