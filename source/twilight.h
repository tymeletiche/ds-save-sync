#pragma once

#include <string>

#include "config.h"

namespace Twilight {

struct LastPlayed {
    std::string romPath;   // e.g. "sd:/roms/nds/Pokemon Platinum.nds"
    std::string savePath;  // e.g. "sd:/roms/nds/saves/Pokemon Platinum.sav"
    std::string gameName;  // e.g. "Pokemon Platinum"
    bool found;
};

LastPlayed detectLastPlayed(const Config& cfg);

} // namespace Twilight
