#include "sync.h"
#include "util.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Sync {

SaveInfo getSaveInfo(Network::Connection& conn, const char* saveName,
                     const char* localSavePath, const Config& cfg) {
    SaveInfo info;
    info.name = saveName;
    info.existsLocally = false;
    info.existsRemotely = false;
    info.localTimestamp = 0;
    info.remoteTimestamp = 0;
    info.localSize = 0;
    info.remoteSize = 0;

    // Check local save
    if (Util::fileExists(localSavePath)) {
        info.existsLocally = true;
        info.localTimestamp = Util::getFileModTime(localSavePath);
        info.localSize = Util::getFileSize(localSavePath);
    }

    // Get remote save info (this also pulls latest from cloud)
    std::string cmd = cfg.scriptPath + " sync-prepare \"";
    cmd += saveName;
    cmd += "\"";

    std::string output;
    if (Network::execCommand(conn, cmd.c_str(), output)) {
        // Parse: EXISTS|<timestamp>|<size> or MISSING
        output = Util::trim(output);
        if (output.substr(0, 6) == "EXISTS") {
            info.existsRemotely = true;
            // Parse timestamp and size
            size_t pipe1 = output.find('|');
            size_t pipe2 = output.find('|', pipe1 + 1);
            if (pipe1 != std::string::npos && pipe2 != std::string::npos) {
                info.remoteTimestamp = (time_t)atoll(
                    output.substr(pipe1 + 1, pipe2 - pipe1 - 1).c_str());
                info.remoteSize = (size_t)atoll(
                    output.substr(pipe2 + 1).c_str());
            }
        } else if (output.substr(0, 11) == "CLOUD_ERROR") {
            // Cloud pull failed — remote status unknown, treat as not found
        }
    }

    return info;
}

bool hasConflict(const SaveInfo& info) {
    if (!info.existsLocally || !info.existsRemotely) return false;
    // Conflict if timestamps differ by more than 60 seconds
    time_t diff = info.localTimestamp - info.remoteTimestamp;
    if (diff < 0) diff = -diff;
    return diff > 60;
}

bool pushSave(Network::Connection& conn, const char* localPath,
              const char* saveName, const Config& cfg,
              Network::ProgressCallback progress) {
    // Upload save via SFTP
    std::string remotePath = cfg.serverSavesPath + "/" + saveName;
    if (!Network::uploadFile(conn, localPath, remotePath.c_str(), progress)) {
        return false;
    }

    // Tell server to push to cloud
    std::string cmd = cfg.scriptPath + " sync-finish \"";
    cmd += saveName;
    cmd += "\"";

    std::string output;
    return Network::execCommand(conn, cmd.c_str(), output);
}

bool pullSave(Network::Connection& conn, const char* saveName,
              const char* localPath, const Config& cfg,
              Network::ProgressCallback progress) {
    // Download save via SFTP
    std::string remotePath = cfg.serverSavesPath + "/" + saveName;
    return Network::downloadFile(conn, remotePath.c_str(), localPath, progress);
}

} // namespace Sync
