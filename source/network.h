#pragma once

#include <string>

#include "config.h"

struct _LIBSSH2_SESSION;
struct _LIBSSH2_SFTP;

namespace Network {

struct Connection {
    int sock;
    _LIBSSH2_SESSION* session;
    _LIBSSH2_SFTP* sftp;
};

typedef void (*ProgressCallback)(int current, int total);

bool connect(Connection& conn, const Config& cfg);
void disconnect(Connection& conn);

bool execCommand(Connection& conn, const char* cmd, std::string& output);

bool downloadFile(Connection& conn, const char* remotePath,
                  const char* localPath, ProgressCallback progress);
bool uploadFile(Connection& conn, const char* localPath,
                const char* remotePath, ProgressCallback progress);

const char* getLastError();

} // namespace Network
