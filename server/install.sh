#!/usr/bin/env bash
set -euo pipefail

DS_SYNC_DIR="$HOME/ds-sync"
RCLONE_REMOTE="gdrive"
DRIVE_FOLDER="DSSaves"

echo "=== DS Save Sync - Server Setup ==="
echo ""

# Create directory structure
echo "Creating directory structure..."
mkdir -p "$DS_SYNC_DIR"/{roms,saves}

# Install rclone if not present
if ! command -v rclone &>/dev/null; then
    echo "Installing rclone..."
    sudo apt-get update && sudo apt-get install -y rclone
    echo ""
fi

# Check rclone remote
if ! rclone listremotes 2>/dev/null | grep -q "^${RCLONE_REMOTE}:$"; then
    echo "rclone remote '$RCLONE_REMOTE' not configured."
    echo ""
    echo "To configure (headless server):"
    echo "  1. From your Mac, SSH with port forwarding:"
    echo "     ssh -L localhost:53682:localhost:53682 razerver"
    echo ""
    echo "  2. On the server, run:"
    echo "     rclone config"
    echo ""
    echo "  3. Choose:"
    echo "     - 'n' for new remote"
    echo "     - Name: $RCLONE_REMOTE"
    echo "     - Type: drive (Google Drive)"
    echo "     - Follow the OAuth prompts"
    echo ""
    echo "After configuring rclone, re-run this script."
    exit 1
fi

echo "rclone remote '$RCLONE_REMOTE' found."

# Create Drive folder structure
echo "Creating Google Drive folders..."
rclone mkdir "${RCLONE_REMOTE}:${DRIVE_FOLDER}/saves"
rclone mkdir "${RCLONE_REMOTE}:${DRIVE_FOLDER}/roms"

# Copy the sync script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f "$SCRIPT_DIR/ds-sync.sh" ]; then
    echo "Installing ds-sync.sh..."
    cp "$SCRIPT_DIR/ds-sync.sh" "$DS_SYNC_DIR/ds-sync.sh"
    chmod +x "$DS_SYNC_DIR/ds-sync.sh"
fi

# Set up SSH directory
mkdir -p ~/.ssh
chmod 700 ~/.ssh

echo ""
echo "=== Setup Complete ==="
echo ""
echo "Server directory: $DS_SYNC_DIR/"
echo "  roms/  - Place your DS ROM files here"
echo "  saves/ - Save files synced to Google Drive"
echo ""
echo "Next steps:"
echo "  1. Add your 3DS public key to ~/.ssh/authorized_keys"
echo "  2. Place ROMs in $DS_SYNC_DIR/roms/"
echo "  3. Test: $DS_SYNC_DIR/ds-sync.sh list-roms"
