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
        info.localSize = Util::getFileSize(localSavePath);
        // 3DS sdmc stat() doesn't return valid mtime, so use our metadata file
        std::string metaPath = cfg.localSavesPath + "/.sync-meta";
        info.localTimestamp = Util::readSyncTimestamp(metaPath, saveName);
        // Fall back to stat() mtime if metadata doesn't have it
        if (info.localTimestamp == 0)
            info.localTimestamp = Util::getFileModTime(localSavePath);
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
    if (!Network::execCommand(conn, cmd.c_str(), output))
        return false;

    // Record sync timestamp: get the server's file timestamp after push
    std::string infoCmd = cfg.scriptPath + " save-info \"";
    infoCmd += saveName;
    infoCmd += "\"";
    std::string infoOutput;
    if (Network::execCommand(conn, infoCmd.c_str(), infoOutput)) {
        infoOutput = Util::trim(infoOutput);
        size_t pipe1 = infoOutput.find('|');
        size_t pipe2 = infoOutput.find('|', pipe1 + 1);
        if (pipe1 != std::string::npos && pipe2 != std::string::npos) {
            time_t serverTs = (time_t)atoll(
                infoOutput.substr(pipe1 + 1, pipe2 - pipe1 - 1).c_str());
            std::string metaPath = cfg.localSavesPath + "/.sync-meta";
            Util::writeSyncTimestamp(metaPath, saveName, serverTs);
        }
    }

    return true;
}

bool pullSave(Network::Connection& conn, const char* saveName,
              const char* localPath, const Config& cfg,
              Network::ProgressCallback progress, time_t remoteTimestamp) {
    // Download save via SFTP
    std::string remotePath = cfg.serverSavesPath + "/" + saveName;
    if (!Network::downloadFile(conn, remotePath.c_str(), localPath, progress)) {
        return false;
    }

    // Record sync timestamp in metadata file
    // (3DS sdmc doesn't support utimes, so we track timestamps ourselves)
    if (remoteTimestamp > 0) {
        std::string metaPath = cfg.localSavesPath + "/.sync-meta";
        Util::writeSyncTimestamp(metaPath, saveName, remoteTimestamp);
    }

    return true;
}

} // namespace Sync
