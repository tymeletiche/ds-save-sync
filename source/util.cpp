#include "util.h"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace Util {

std::string basename(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

std::string dirname(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return ".";
    return path.substr(0, pos);
}

bool endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string readIniValue(const std::string& filepath,
                         const std::string& section,
                         const std::string& key) {
    FILE* fp = fopen(filepath.c_str(), "r");
    if (!fp) return "";

    char line[512];
    bool inSection = false;
    std::string sectionHeader = "[" + section + "]";

    while (fgets(line, sizeof(line), fp)) {
        std::string l = trim(std::string(line));

        // Skip comments and empty lines
        if (l.empty() || l[0] == '#' || l[0] == ';') continue;

        // Check for section header
        if (l[0] == '[') {
            // Case-insensitive section comparison
            bool match = (l.size() == sectionHeader.size());
            if (match) {
                for (size_t i = 0; i < l.size(); i++) {
                    if (tolower(l[i]) != tolower(sectionHeader[i])) {
                        match = false;
                        break;
                    }
                }
            }
            inSection = match;
            continue;
        }

        if (!inSection) continue;

        // Parse key = value
        size_t eq = l.find('=');
        if (eq == std::string::npos) continue;

        std::string k = trim(l.substr(0, eq));
        std::string v = trim(l.substr(eq + 1));

        // Case-insensitive key comparison
        if (k.size() == key.size()) {
            bool match = true;
            for (size_t i = 0; i < k.size(); i++) {
                if (tolower(k[i]) != tolower(key[i])) {
                    match = false;
                    break;
                }
            }
            if (match) {
                fclose(fp);
                return v;
            }
        }
    }

    fclose(fp);
    return "";
}

time_t getFileModTime(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return 0;
    return st.st_mtime;
}

size_t getFileSize(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return 0;
    return (size_t)st.st_size;
}

bool fileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

std::string formatSize(size_t bytes) {
    char buf[32];
    if (bytes < 1024) {
        snprintf(buf, sizeof(buf), "%zu B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buf, sizeof(buf), "%zu KB", bytes / 1024);
    } else {
        snprintf(buf, sizeof(buf), "%.1f MB", (double)bytes / (1024.0 * 1024.0));
    }
    return std::string(buf);
}

std::string formatTimestamp(time_t t) {
    if (t == 0) return "unknown";
    struct tm* tm = localtime(&t);
    char buf[64];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min);
    return std::string(buf);
}

time_t readSyncTimestamp(const std::string& metaPath, const std::string& saveName) {
    FILE* fp = fopen(metaPath.c_str(), "r");
    if (!fp) return 0;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        std::string l = trim(std::string(line));
        size_t eq = l.find('=');
        if (eq == std::string::npos) continue;
        std::string name = l.substr(0, eq);
        if (name == saveName) {
            time_t t = (time_t)atoll(l.substr(eq + 1).c_str());
            fclose(fp);
            return t;
        }
    }
    fclose(fp);
    return 0;
}

void writeSyncTimestamp(const std::string& metaPath, const std::string& saveName, time_t t) {
    // Read existing entries (excluding the one we're updating)
    std::string contents;
    FILE* fp = fopen(metaPath.c_str(), "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            std::string l = trim(std::string(line));
            size_t eq = l.find('=');
            if (eq != std::string::npos && l.substr(0, eq) == saveName)
                continue;
            if (!l.empty())
                contents += l + "\n";
        }
        fclose(fp);
    }

    // Append updated entry
    char buf[64];
    snprintf(buf, sizeof(buf), "%lld", (long long)t);
    contents += saveName + "=" + buf + "\n";

    // Write back
    fp = fopen(metaPath.c_str(), "w");
    if (fp) {
        fwrite(contents.c_str(), 1, contents.size(), fp);
        fclose(fp);
    }
}

} // namespace Util
