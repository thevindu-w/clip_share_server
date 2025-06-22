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
    clip_share_GLIBC*
)

IFS=$'\n' exec_names=($(sort -r <<<"${exec_names[*]}"))
unset IFS

exec_path=~/.local/bin/clip_share
exec_not_found=1
for exec_name in "${exec_names[@]}"; do
    if [ -f "$exec_name" ]; then
        chmod +x "$exec_name"
        "./$exec_name" -h &>/dev/null || continue
        exec_not_found=0
        mkdir -p ~/.local/bin/
        mv "$exec_name" "$exec_path"
        chmod +x "$exec_path"
        echo Moved "$exec_name" to "$exec_path"
        break
    fi
done

if [ "$exec_not_found" = 1 ]; then
    echo "Error: 'clip_share' program was not found in the current directory"
    exit 1
fi

cd
export HOME="$(pwd)"
mkdir -p .config

CONF_PATHS=("$XDG_CONFIG_HOME" "${HOME}/.config" "$HOME")
for directory in "${CONF_PATHS[@]}"; do
    [ -d "$directory" ] || continue
    conf_path="${directory}/clipshare.conf"
    if [ -f "$conf_path" ] && [ -r "$conf_path" ]; then
        CONF_DIR="$directory"
        break
    fi
done
for directory in "${CONF_PATHS[@]}"; do
    [ -n "$CONF_DIR" ] && break
    if [ -d "$directory" ] && [ -w "$directory" ]; then
        CONF_DIR="$directory"
    fi
done
if [ -z "$CONF_DIR" ]; then
    echo "Error: Could not find a directory for the configuration file!"
    exit 1
fi
CONF_FILE="${CONF_DIR}/clipshare.conf"

if [ ! -f "$CONF_FILE" ]; then
    mkdir -p Downloads
    echo "working_dir=${HOME}/Downloads" >"$CONF_FILE"
    echo "Created a new configuration file $CONF_FILE"
fi

mkdir -p .config/systemd/user

if [ -f .config/systemd/user/clipshare.service ]; then
    echo
    echo 'A previous installation of ClipShare is available.'
    read -p 'Update? [y/N] ' confirm_update
    if [ "${confirm_update,,}" != 'y' ] && [ "${confirm_update,,}" != 'yes' ]; then
        echo 'Aborted.'
        exit 0
    fi
fi

if [ -z "$DISPLAY" ]; then
    export DISPLAY=:0
fi

cat >.config/systemd/user/clipshare.service <<EOF
[Unit]
Description=ClipShare

[Service]
Type=forking
Environment="XDG_CONFIG_HOME=$CONF_DIR" "DISPLAY=$DISPLAY"
ExecStart=$exec_path
ExecStop=$exec_path -s
Restart=no

[Install]
WantedBy=default.target
EOF

systemctl --user daemon-reload
systemctl --user enable clipshare.service >/dev/null
echo 'Installed clip_share to run on startup. This takes effect from the next login.'

echo
read -p 'Start clip_share now? [y/N] ' start_now
if [ "${start_now,,}" = 'y' ] || [ "${start_now,,}" = 'yes' ]; then
    systemctl --user start clipshare.service
    echo 'Started clip_share.'
fi
