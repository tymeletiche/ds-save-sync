#pragma once

#include <string>

struct Config {
    std::string serverHost = "razerver";
    int serverPort = 22;
    std::string sshUser = "tyme";
    std::string sshPrivKeyPath = "sdmc:/3ds/ds-save-sync/id_rsa";
    std::string sshPubKeyPath = "sdmc:/3ds/ds-save-sync/id_rsa.pub";
    std::string scriptPath = "~/ds-sync/ds-sync.sh";
    std::string serverSavesPath = "~/ds-sync/saves";
    std::string serverRomsPath = "~/ds-sync/roms";
    std::string localSavesPath = "sdmc:/roms/nds/saves";
    std::string localRomsPath = "sdmc:/roms/nds";
    std::string twilightIniPath = "sdmc:/_nds/TWiLightMenu/settings.ini";
    std::string ndsBootstrapIniPath = "sdmc:/_nds/nds-bootstrap.ini";

    bool load();
    bool save();
};
