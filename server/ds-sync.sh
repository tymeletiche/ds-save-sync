#!/usr/bin/env bash
set -euo pipefail

DS_SYNC_DIR="$HOME/ds-sync"
SAVES_DIR="$DS_SYNC_DIR/saves"
ROMS_DIR="$DS_SYNC_DIR/roms"
RCLONE_REMOTE="gdrive"
DRIVE_FOLDER="DSSaves"
LOCK_FILE="$DS_SYNC_DIR/.ds-sync.lock"

# Locking to prevent concurrent runs
lock() {
    exec 200>"$LOCK_FILE"
    flock -n 200 || { echo "ERROR: Another sync is running"; exit 1; }
}

# Validate save_name is a safe basename (no slashes, no ..)
validate_save_name() {
    local name="$1"
    if [[ -z "$name" ]] || [[ "$name" == */* ]] || [[ "$name" == *..* ]]; then
        echo "ERROR: Invalid save name" >&2
        exit 1
    fi
}

# --- Subcommands ---

# Pull saves from Google Drive to server
cmd_pull_cloud() {
    lock
    rclone sync "${RCLONE_REMOTE}:${DRIVE_FOLDER}/saves/" "$SAVES_DIR/" \
        --update --verbose 2>&1
}

# Push saves from server to Google Drive
cmd_push_cloud() {
    lock
    rclone sync "$SAVES_DIR/" "${RCLONE_REMOTE}:${DRIVE_FOLDER}/saves/" \
        --update --verbose 2>&1
}

# List saves on server with timestamps (machine-readable)
cmd_list_saves() {
    for f in "$SAVES_DIR"/*.sav; do
        [ -f "$f" ] || continue
        name=$(basename "$f")
        mtime=$(stat -c '%Y' "$f")
        size=$(stat -c '%s' "$f")
        echo "${name}|${mtime}|${size}"
    done
}

# List ROMs on server
cmd_list_roms() {
    for f in "$ROMS_DIR"/*.nds; do
        [ -f "$f" ] || continue
        name=$(basename "$f")
        size=$(stat -c '%s' "$f")
        echo "${name}|${size}"
    done
}

# Get timestamp for a specific save
cmd_save_info() {
    local save_name="$1"
    validate_save_name "$save_name"
    local save_path="$SAVES_DIR/$save_name"
    if [ -f "$save_path" ]; then
        mtime=$(stat -c '%Y' "$save_path")
        size=$(stat -c '%s' "$save_path")
        echo "EXISTS|${mtime}|${size}"
    else
        echo "MISSING"
    fi
}

# Sync: pull from cloud, then respond with save info
cmd_sync_prepare() {
    local save_name="$1"
    validate_save_name "$save_name"
    lock
    # Pull latest from cloud first
    local rclone_output
    if ! rclone_output=$(rclone copy "${RCLONE_REMOTE}:${DRIVE_FOLDER}/saves/${save_name}" \
        "$SAVES_DIR/" --update 2>&1); then
        echo "CLOUD_ERROR|${rclone_output}"
        exit 0
    fi
    # Return save info
    cmd_save_info "$save_name"
}

# After 3DS uploads a save, push it to cloud
cmd_sync_finish() {
    local save_name="$1"
    validate_save_name "$save_name"
    lock
    rclone copy "$SAVES_DIR/${save_name}" \
        "${RCLONE_REMOTE}:${DRIVE_FOLDER}/saves/" --update 2>&1
    echo "DONE"
}

# --- Main dispatch ---
case "${1:-help}" in
    pull-cloud)     cmd_pull_cloud ;;
    push-cloud)     cmd_push_cloud ;;
    list-saves)     cmd_list_saves ;;
    list-roms)      cmd_list_roms ;;
    save-info)      cmd_save_info "${2:?Missing save name}" ;;
    sync-prepare)   cmd_sync_prepare "${2:?Missing save name}" ;;
    sync-finish)    cmd_sync_finish "${2:?Missing save name}" ;;
    help|*)
        echo "Usage: ds-sync.sh <command> [args]"
        echo "Commands: pull-cloud, push-cloud, list-saves, list-roms,"
        echo "          save-info <name.sav>, sync-prepare <name.sav>,"
        echo "          sync-finish <name.sav>"
        ;;
esac
