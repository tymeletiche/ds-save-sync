#include "twilight.h"
#include "util.h"

namespace Twilight {

LastPlayed detectLastPlayed(const Config& cfg) {
    LastPlayed result;
    result.found = false;

    // Primary: nds-bootstrap.ini [NDS-BOOTSTRAP] NDS_PATH
    std::string ndsPath = Util::readIniValue(
        cfg.ndsBootstrapIniPath, "NDS-BOOTSTRAP", "NDS_PATH");

    // Fallback: TWiLightMenu settings.ini [SRLOADER] ROM_PATH
    if (ndsPath.empty()) {
        ndsPath = Util::readIniValue(
            cfg.twilightIniPath, "SRLOADER", "ROM_PATH");
    }

    if (ndsPath.empty()) return result;

    result.romPath = ndsPath;
    result.found = true;

    // Derive game name: strip directory and .nds extension
    std::string filename = Util::basename(ndsPath);
    if (Util::endsWith(filename, ".nds")) {
        filename = filename.substr(0, filename.size() - 4);
    }
    result.gameName = filename;

    // Derive save path: saves directory + name + .sav
    result.savePath = cfg.localSavesPath + "/" + filename + ".sav";

    return result;
}

} // namespace Twilight
