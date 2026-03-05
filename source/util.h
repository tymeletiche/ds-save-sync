#pragma once

#include <string>
#include <ctime>

namespace Util {

std::string basename(const std::string& path);
std::string dirname(const std::string& path);
bool endsWith(const std::string& str, const std::string& suffix);
std::string trim(const std::string& str);

std::string readIniValue(const std::string& filepath,
                         const std::string& section,
                         const std::string& key);

time_t getFileModTime(const std::string& path);
size_t getFileSize(const std::string& path);
bool fileExists(const std::string& path);

std::string formatSize(size_t bytes);
std::string formatTimestamp(time_t t);

// Sync metadata: stores last-synced timestamps per save
// Workaround for 3DS sdmc stat() not returning valid mtime
time_t readSyncTimestamp(const std::string& metaPath, const std::string& saveName);
void writeSyncTimestamp(const std::string& metaPath, const std::string& saveName, time_t t);

} // namespace Util
