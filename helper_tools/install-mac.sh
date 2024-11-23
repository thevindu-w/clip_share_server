#!/bin/bash

set -e

if [ "$(id -u)" = 0 ]; then
    echo 'Do not run install with sudo.'
    echo 'Aborted.'
    exit 1
fi

echo 'This will install clip_share to run on startup.'
read -p 'Proceed? [y/N] ' confirm
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

exec_path=~/.local/bin/clip_share
exec_not_found=1
missingLib=
for exec_name in "${exec_names[@]}"; do
    if [ -f "$exec_name" ]; then
        chmod +x "$exec_name"
        if ! sh -c "./$exec_name -h 2>&1" &>/dev/null; then
            missing="$( (sh -c "./$exec_name -h 2>&1" 2>/dev/null || true) | grep -oE 'lib[a-zA-Z0-9.-]+\.dylib' | grep -oE 'lib[a-zA-Z0-9-]+' | head -n 1)"
            [ -n "$missing" ] && missingLib="$missing"
            continue
        fi
        exec_not_found=0
        mkdir -p ~/.local/bin/
        mv "$exec_name" "$exec_path"
        chmod +x "$exec_path"
        echo Moved "$exec_name" to "$exec_path"
        break
    fi
done

if [ "$exec_not_found" = 1 ]; then
    if [ -n "$missingLib" ]; then
        echo "Error: The ${missingLib} library is not installed. Please install ${missingLib} and run the installer again."
    else
        echo "Error: 'clip_share' program was not found in the current directory"
    fi
    exit 1
fi

cd
export HOME="$(pwd)"

CONF_DIR="$HOME"
if [ -d "$XDG_CONFIG_HOME" ] && [ -w "$XDG_CONFIG_HOME" ]; then
    CONF_DIR="$XDG_CONFIG_HOME"
fi
CONF_FILE="${CONF_DIR}/clipshare.conf"

if [ ! -d "$CONF_DIR" ]; then
    mkdir -p "$CONF_DIR"
fi

if [ ! -f "$CONF_FILE" ]; then
    mkdir -p Downloads
    echo "working_dir=${HOME}/Downloads" >"$CONF_FILE"
    echo "Created a new configuration file $CONF_FILE"
fi

mkdir -p Library/LaunchAgents/

if [ -f Library/LaunchAgents/com.tw.clipshare.plist ]; then
    echo
    echo 'A previous installation of ClipShare is available.'
    read -p 'Update? [y/N] ' confirm_update
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
        <key>EnvironmentVariables</key>
        <dict>
            <key>XDG_CONFIG_HOME</key>
            <string>$CONF_DIR</string>
        </dict>
        <key>Label</key>
        <string>com.tw.clipshare</string>
        <key>ProgramArguments</key>
        <array>
            <string>$exec_path</string>
            <string>-D</string>
        </array>
        <key>RunAtLoad</key>
        <true />
    </dict>
</plist>
EOF

launchctl unload -w ~/Library/LaunchAgents/com.tw.clipshare.plist &>/dev/null || true
launchctl load -w ~/Library/LaunchAgents/com.tw.clipshare.plist
echo 'Installed clip_share to run on startup.'
