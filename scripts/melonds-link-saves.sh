#!/bin/bash
# Auto-links melonDS save locations to Google Drive DSSaves copies.
# Matches by: (1) exact filename, (2) NDS ROM header game code.
#
# Usage: ./melonds-link-saves.sh [--dry-run]

set -euo pipefail

DRY_RUN=false
[[ "${1:-}" == "--dry-run" ]] && DRY_RUN=true

DRIVE_BASE="$HOME/Library/CloudStorage/GoogleDrive-ty.meletiche@gmail.com/My Drive/DSSaves"
DRIVE_SAVES="$DRIVE_BASE/saves"
DRIVE_ROMS="$DRIVE_BASE/roms"
MELONDS_ROMS="$HOME/Documents/Roms"

if [ ! -d "$DRIVE_SAVES" ]; then
    echo "ERROR: Google Drive saves folder not found at:"
    echo "  $DRIVE_SAVES"
    echo "Make sure Google Drive for Desktop is running."
    exit 1
fi

# --- Sync ROMs to Google Drive so the 3DS can download them ---
echo "Syncing ROMs to Google Drive..."
mkdir -p "$DRIVE_ROMS"
rom_synced=0
for rom in "$MELONDS_ROMS"/*.nds; do
    [ -f "$rom" ] || continue
    rom_name=$(basename "$rom")
    drive_rom="$DRIVE_ROMS/$rom_name"
    if [ ! -f "$drive_rom" ]; then
        echo "  Copying: $rom_name"
        if [ "$DRY_RUN" = false ]; then
            cp "$rom" "$drive_rom"
        fi
        ((rom_synced++)) || true
    fi
done
echo "  $rom_synced new ROM(s) synced"
echo ""

# Read 4-byte game code from NDS ROM header at offset 0x0C
read_game_code() {
    dd if="$1" bs=1 skip=12 count=4 2>/dev/null | tr -d '\0'
}

# Build parallel arrays: rom_names[i] and rom_codes[i]
rom_names=()
rom_codes=()

echo "Scanning melonDS ROMs..."
for rom in "$MELONDS_ROMS"/*.nds; do
    [ -f "$rom" ] || continue
    rom_basename=$(basename "$rom" .nds)
    code=$(read_game_code "$rom")

    rom_names+=("$rom_basename")
    if [ -n "$code" ] && [ ${#code} -eq 4 ]; then
        rom_codes+=("$code")
        echo "  $rom_basename -> code: $code"
    else
        rom_codes+=("")
    fi
done
echo ""

# Match a save name to a melonDS ROM
# Sets match_rom and match_method, or leaves match_rom empty
find_match() {
    local save_name="$1"
    match_rom=""
    match_method=""

    # Strategy 1: Exact filename match
    for name in "${rom_names[@]}"; do
        if [ "$save_name" = "$name" ]; then
            match_rom="$name"
            match_method="exact"
            return
        fi
    done

    # Strategy 2: Game code match
    local i
    for ((i = 0; i < ${#rom_names[@]}; i++)); do
        local code="${rom_codes[$i]}"
        [ -z "$code" ] && continue
        case "$save_name" in
            *"$code"*)
                match_rom="${rom_names[$i]}"
                match_method="code($code)"
                return
                ;;
        esac
    done
}

# Scan Google Drive saves and link into both melonDS Roms dir and Drive roms dir
echo "Matching saves to ROMs..."
echo ""

linked=0
skipped=0
unmatched=0

link_save_to_dir() {
    local save="$1"
    local save_name="$2"
    local target_dir="$3"
    local label="$4"
    local link_path="$target_dir/$match_rom.sav"

    # Already correctly linked
    if [ -L "$link_path" ]; then
        existing_target=$(readlink "$link_path")
        if [ "$existing_target" = "$save" ]; then
            return 0
        fi
    fi

    # Back up existing real file (not a symlink)
    if [ -f "$link_path" ] && [ ! -L "$link_path" ]; then
        echo "    Backing up $label: $match_rom.sav -> $match_rom.sav.bak"
        if [ "$DRY_RUN" = false ]; then
            mv "$link_path" "$link_path.bak"
        fi
    fi

    if [ "$DRY_RUN" = false ]; then
        ln -sf "$save" "$link_path"
    fi
    return 1
}

for save in "$DRIVE_SAVES"/*.sav; do
    [ -f "$save" ] || continue
    save_name=$(basename "$save" .sav)

    find_match "$save_name"

    if [ -z "$match_rom" ]; then
        echo "  UNMATCHED: $save_name.sav"
        ((unmatched++)) || true
        continue
    fi

    did_link=false

    # Link into melonDS Roms directory
    if link_save_to_dir "$save" "$save_name" "$MELONDS_ROMS" "local"; then
        : # already linked
    else
        did_link=true
    fi

    # Link into Google Drive roms directory (where melonDS may load ROMs from)
    if [ -d "$DRIVE_ROMS" ]; then
        if link_save_to_dir "$save" "$save_name" "$DRIVE_ROMS" "drive"; then
            : # already linked
        else
            did_link=true
        fi
    fi

    if [ "$did_link" = true ]; then
        echo "  LINK [$match_method]: $match_rom.sav -> $save_name.sav"
        ((linked++)) || true
    else
        echo "  OK [$match_method]: $match_rom.sav"
        ((skipped++)) || true
    fi
done

echo ""
echo "Summary: $linked linked, $skipped already OK, $unmatched unmatched"
if [ "$DRY_RUN" = true ]; then
    echo "(dry run -- no changes made)"
fi
