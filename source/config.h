#pragma once

#include <string>

struct Config {
    std::string serverHost = "razerver";
    int serverPort = 22;
    std::string sshUser = "tyme";
    std::string sshPrivKeyPath = "sd:/3ds/ds-save-sync/id_rsa";
    std::string sshPubKeyPath = "sd:/3ds/ds-save-sync/id_rsa.pub";
    std::string scriptPath = "~/ds-sync/ds-sync.sh";
    std::string serverSavesPath = "~/ds-sync/saves";
    std::string localSavesPath = "sd:/roms/nds/saves";
    std::string localRomsPath = "sd:/roms/nds";
    std::string twilightIniPath = "sd:/_nds/TWiLightMenu/settings.ini";
    std::string ndsBootstrapIniPath = "sd:/_nds/nds-bootstrap.ini";

    bool load();
    bool save();
};
