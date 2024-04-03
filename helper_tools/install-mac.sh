#!/bin/bash

set -e

if [ "$(id -u)" = 0 ]; then
    echo 'Do not run install with sudo.'
    echo 'Aborted.'
    exit 1
fi

echo 'This will install clip_share to run on startup.'
read -p 'Proceed? [y/n] ' confirm
if [ "${confirm::1}" != 'y' ] && [ "${confirm::1}" != 'Y' ]; then
    echo 'Aborted.'
    echo 'You can still use clip_share by manually running the executable.'
    exit 0
fi

exec_names=(
    clip_share
    clip_share-arm64
    clip_share-x86_64
)

exec_not_found=1
for exec_name in "${exec_names[@]}"; do
    if [ -f "$exec_name" ]; then
        chmod +x "$exec_name"
        "./$exec_name" -h &>/dev/null || continue
        exec_not_found=0
        mkdir -p ~/.local/bin/
        mv "$exec_name" ~/.local/bin/
        chmod +x ~/.local/bin/clip_share
        echo Moved "$exec_name" to ~/.local/bin/clip_share
        break
    fi
done

if [ "$exec_not_found" = 1 ]; then
    echo "Error: 'clip_share' program was not found in the current directory"
    exit 1
fi

cd
export HOME="$(pwd)"

CONF_DIR="$HOME"
if [ -d "$XDG_CONFIG_HOME" ] && [ -w "$XDG_CONFIG_HOME" ]; then
    CONF_DIR="$XDG_CONFIG_HOME"
fi
CONF_FILE="${CONF_DIR}/clipshare.conf"

if [ ! -f "$CONF_FILE" ]; then
    mkdir -p Downloads
    echo "working_dir=${HOME}/Downloads" >"$CONF_FILE"
    echo "Created a new configuration file $CONF_FILE"
fi

mkdir -p Library/LaunchAgents/

if [ -f Library/LaunchAgents/com.tw.clipshare.plist ]; then
    echo
    echo 'A previous installation of ClipShare is available.'
    read -p 'Update? [y/n] ' confirm_update
    if [ "${confirm_update}" != 'y' ] && [ "${confirm_update}" != 'Y' ]; then
        echo 'Aborted.'
        exit 0
    fi
fi

cat >Library/LaunchAgents/com.tw.clipshare.plist <<EOF
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
   "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
    <dict>
        <key>Label</key>
        <string>com.tw.clipshare</string>
        <key>ProgramArguments</key>
        <array>
            <string>$HOME/.local/bin/clip_share</string>
            <string>-D</string>
        </array>
        <key>RunAtLoad</key>
        <true />
    </dict>
</plist>
EOF

launchctl unload ~/Library/LaunchAgents/com.tw.clipshare.plist &>/dev/null || true
launchctl load ~/Library/LaunchAgents/com.tw.clipshare.plist
echo 'Installed clip_share to run on startup.'
