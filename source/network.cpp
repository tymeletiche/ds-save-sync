#include "network.h"

#include <3ds.h>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <libssh2.h>
#include <libssh2_sftp.h>

namespace Network {

static char s_lastError[256] = "";

static void setError(const char* msg) {
    strncpy(s_lastError, msg, sizeof(s_lastError) - 1);
    s_lastError[sizeof(s_lastError) - 1] = '\0';
}

const char* getLastError() {
    return s_lastError;
}

bool connect(Connection& conn, const Config& cfg) {
    // DNS resolution
    struct hostent* host = gethostbyname(cfg.serverHost.c_str());
    if (!host) {
        setError("DNS resolution failed");
        return false;
    }

    // TCP socket
    conn.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (conn.sock < 0) {
        setError("Socket creation failed");
        return false;
    }

    // Connect
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg.serverPort);
    memcpy(&addr.sin_addr, host->h_addr_list[0], host->h_length);

    if (::connect(conn.sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        setError("TCP connection failed");
        close(conn.sock);
        conn.sock = -1;
        return false;
    }

    // SSH handshake
    conn.session = (LIBSSH2_SESSION*)libssh2_session_init();
    if (!conn.session) {
        setError("SSH session init failed");
        close(conn.sock);
        conn.sock = -1;
        return false;
    }

    libssh2_session_set_blocking(conn.session, 1);
    libssh2_session_set_timeout(conn.session, 10000);

    if (libssh2_session_handshake(conn.session, conn.sock) != 0) {
        char* errmsg = nullptr;
        libssh2_session_last_error(conn.session, &errmsg, nullptr, 0);
        setError(errmsg ? errmsg : "SSH handshake failed");
        libssh2_session_free(conn.session);
        conn.session = nullptr;
        close(conn.sock);
        conn.sock = -1;
        return false;
    }

    // Public key authentication
    if (libssh2_userauth_publickey_fromfile(
            conn.session,
            cfg.sshUser.c_str(),
            cfg.sshPubKeyPath.c_str(),
            cfg.sshPrivKeyPath.c_str(),
            "") != 0) {
        char* errmsg = nullptr;
        libssh2_session_last_error(conn.session, &errmsg, nullptr, 0);
        setError(errmsg ? errmsg : "SSH auth failed");
        libssh2_session_disconnect(conn.session, "Auth failed");
        libssh2_session_free(conn.session);
        conn.session = nullptr;
        close(conn.sock);
        conn.sock = -1;
        return false;
    }

    // Init SFTP subsystem
    conn.sftp = (LIBSSH2_SFTP*)libssh2_sftp_init(conn.session);
    if (!conn.sftp) {
        setError("SFTP init failed");
        libssh2_session_disconnect(conn.session, "SFTP init failed");
        libssh2_session_free(conn.session);
        conn.session = nullptr;
        close(conn.sock);
        conn.sock = -1;
        return false;
    }

    return true;
}

void disconnect(Connection& conn) {
    if (conn.sftp) {
        libssh2_sftp_shutdown(conn.sftp);
        conn.sftp = nullptr;
    }
    if (conn.session) {
        libssh2_session_disconnect(conn.session, "Goodbye");
        libssh2_session_free(conn.session);
        conn.session = nullptr;
    }
    if (conn.sock >= 0) {
        close(conn.sock);
        conn.sock = -1;
    }
}

bool execCommand(Connection& conn, const char* cmd, std::string& output) {
    output.clear();

    LIBSSH2_CHANNEL* channel = libssh2_channel_open_session(conn.session);
    if (!channel) {
        setError("Failed to open SSH channel");
        return false;
    }

    if (libssh2_channel_exec(channel, cmd) != 0) {
        setError("Failed to exec command");
        libssh2_channel_free(channel);
        return false;
    }

    char buf[4096];
    for (;;) {
        ssize_t n = libssh2_channel_read(channel, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            output += buf;
        } else if (n == 0) {
            break;
        } else {
            break;
        }
    }

    int exitCode = 0;
    // Wait for channel close to get exit status
    while (libssh2_channel_close(channel) == LIBSSH2_ERROR_EAGAIN) {}
    exitCode = libssh2_channel_get_exit_status(channel);
    libssh2_channel_free(channel);

    if (exitCode != 0) {
        char errBuf[128];
        snprintf(errBuf, sizeof(errBuf), "Command exited with code %d", exitCode);
        setError(errBuf);
        return false;
    }

    return true;
}

bool downloadFile(Connection& conn, const char* remotePath,
                  const char* localPath, ProgressCallback progress) {
    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open(
        conn.sftp, remotePath, LIBSSH2_FXF_READ, 0);
    if (!handle) {
        setError("Failed to open remote file");
        return false;
    }

    // Get file size for progress
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int total = 0;
    if (libssh2_sftp_fstat(handle, &attrs) == 0) {
        total = (int)attrs.filesize;
    }

    FILE* fp = fopen(localPath, "wb");
    if (!fp) {
        setError("Failed to open local file for writing");
        libssh2_sftp_close(handle);
        return false;
    }

    char buf[32768]; // 32KB buffer
    int downloaded = 0;
    for (;;) {
        ssize_t n = libssh2_sftp_read(handle, buf, sizeof(buf));
        if (n > 0) {
            fwrite(buf, 1, n, fp);
            downloaded += n;
            if (progress && total > 0) {
                progress(downloaded, total);
            }
        } else if (n == 0) {
            break;
        } else {
            setError("SFTP read error");
            fclose(fp);
            libssh2_sftp_close(handle);
            return false;
        }
    }

    fclose(fp);
    libssh2_sftp_close(handle);
    return true;
}

bool uploadFile(Connection& conn, const char* localPath,
                const char* remotePath, ProgressCallback progress) {
    FILE* fp = fopen(localPath, "rb");
    if (!fp) {
        setError("Failed to open local file for reading");
        return false;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    int total = (int)ftell(fp);
    fseek(fp, 0, SEEK_SET);

    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open(
        conn.sftp, remotePath,
        LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
        LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
        LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
    if (!handle) {
        setError("Failed to open remote file for writing");
        fclose(fp);
        return false;
    }

    char buf[32768]; // 32KB buffer
    int uploaded = 0;
    for (;;) {
        size_t n = fread(buf, 1, sizeof(buf), fp);
        if (n == 0) break;

        char* ptr = buf;
        size_t remaining = n;
        while (remaining > 0) {
            ssize_t written = libssh2_sftp_write(handle, ptr, remaining);
            if (written < 0) {
                setError("SFTP write error");
                fclose(fp);
                libssh2_sftp_close(handle);
                return false;
            }
            ptr += written;
            remaining -= written;
            uploaded += written;
        }
        if (progress && total > 0) {
            progress(uploaded, total);
        }
    }

    fclose(fp);
    libssh2_sftp_close(handle);
    return true;
}

} // namespace Network
