#!/bin/bash
# Links melonDS save locations to Google Drive DSSaves copies.
# melonDS expects saves next to ROMs with matching names.
# This script creates symlinks from those expected paths to the
# canonical save files on Google Drive (synced via DS Save Sync).
#
# Usage: ./melonds-link-saves.sh

set -euo pipefail

DRIVE_SAVES="$HOME/Library/CloudStorage/GoogleDrive-ty.meletiche@gmail.com/My Drive/DSSaves/saves"
MELONDS_ROMS="$HOME/Documents/Roms"

if [ ! -d "$DRIVE_SAVES" ]; then
    echo "ERROR: Google Drive saves folder not found at:"
    echo "  $DRIVE_SAVES"
    echo "Make sure Google Drive for Desktop is running and DSSaves is accessible."
    exit 1
fi

link_save() {
    local drive_name="$1"
    local melonds_name="$2"
    local target="$DRIVE_SAVES/$drive_name"
    local link="$MELONDS_ROMS/$melonds_name.sav"

    # Back up existing local save if it's a real file (not already a symlink)
    if [ -f "$link" ] && [ ! -L "$link" ]; then
        local backup="$link.bak"
        echo "  Backing up existing save: $melonds_name.sav -> $melonds_name.sav.bak"
        mv "$link" "$backup"
    fi

    if [ -f "$target" ]; then
        ln -sf "$target" "$link"
        echo "  Linked: $melonds_name.sav -> $drive_name"
    else
        echo "  Skip (not on Drive yet): $drive_name"
    fi
}

echo "Linking melonDS saves to Google Drive..."
echo ""

# === Game Mappings ===
# Format: link_save "<3DS save name on Drive>" "<melonDS ROM name without .nds>"
# Add one line per game as you add games to both platforms.

link_save "NEW_MARIO_A2DE01_00.trim.sav" "New Super Mario Bros. (USA, Australia)"
link_save "Pokemon - Black Version (USA, Europe) (NDSi Enhanced).sav" "Pokemon - Black Version (USA, Europe) (NDSi Enhanced)"

echo ""
echo "Done. Launch melonDS to verify saves load correctly."
