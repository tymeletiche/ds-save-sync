#include "config.h"
#include "util.h"

#include <cstdio>

static const char* CONFIG_PATH = "sd:/3ds/ds-save-sync/config.ini";

bool Config::load() {
    if (!Util::fileExists(CONFIG_PATH)) return false;

    serverHost = Util::readIniValue(CONFIG_PATH, "server", "host");
    if (serverHost.empty()) serverHost = "razerver";

    std::string portStr = Util::readIniValue(CONFIG_PATH, "server", "port");
    if (!portStr.empty()) serverPort = atoi(portStr.c_str());

    std::string user = Util::readIniValue(CONFIG_PATH, "server", "user");
    if (!user.empty()) sshUser = user;

    std::string privKey = Util::readIniValue(CONFIG_PATH, "ssh", "private_key");
    if (!privKey.empty()) sshPrivKeyPath = privKey;

    std::string pubKey = Util::readIniValue(CONFIG_PATH, "ssh", "public_key");
    if (!pubKey.empty()) sshPubKeyPath = pubKey;

    std::string script = Util::readIniValue(CONFIG_PATH, "server", "script_path");
    if (!script.empty()) scriptPath = script;

    std::string savesPath = Util::readIniValue(CONFIG_PATH, "server", "saves_path");
    if (!savesPath.empty()) serverSavesPath = savesPath;

    std::string romsPath = Util::readIniValue(CONFIG_PATH, "server", "roms_path");
    if (!romsPath.empty()) serverRomsPath = romsPath;

    std::string localSaves = Util::readIniValue(CONFIG_PATH, "paths", "local_saves");
    if (!localSaves.empty()) localSavesPath = localSaves;

    std::string localRoms = Util::readIniValue(CONFIG_PATH, "paths", "local_roms");
    if (!localRoms.empty()) localRomsPath = localRoms;

    std::string twlIni = Util::readIniValue(CONFIG_PATH, "paths", "twilight_ini");
    if (!twlIni.empty()) twilightIniPath = twlIni;

    std::string ndsIni = Util::readIniValue(CONFIG_PATH, "paths", "nds_bootstrap_ini");
    if (!ndsIni.empty()) ndsBootstrapIniPath = ndsIni;

    return true;
}

bool Config::save() {
    FILE* fp = fopen(CONFIG_PATH, "w");
    if (!fp) return false;

    fprintf(fp, "[server]\n");
    fprintf(fp, "host = %s\n", serverHost.c_str());
    fprintf(fp, "port = %d\n", serverPort);
    fprintf(fp, "user = %s\n", sshUser.c_str());
    fprintf(fp, "script_path = %s\n", scriptPath.c_str());
    fprintf(fp, "saves_path = %s\n", serverSavesPath.c_str());
    fprintf(fp, "roms_path = %s\n", serverRomsPath.c_str());
    fprintf(fp, "\n[ssh]\n");
    fprintf(fp, "private_key = %s\n", sshPrivKeyPath.c_str());
    fprintf(fp, "public_key = %s\n", sshPubKeyPath.c_str());
    fprintf(fp, "\n[paths]\n");
    fprintf(fp, "local_saves = %s\n", localSavesPath.c_str());
    fprintf(fp, "local_roms = %s\n", localRomsPath.c_str());
    fprintf(fp, "twilight_ini = %s\n", twilightIniPath.c_str());
    fprintf(fp, "nds_bootstrap_ini = %s\n", ndsBootstrapIniPath.c_str());

    fclose(fp);
    return true;
}
