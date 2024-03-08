#!/bin/bash

set -e

if [ "$(id -u)" = 0 ]; then
    echo 'Do not run install with sudo.'
    echo 'Aborted.'
    exit 1
fi

echo 'This will install clip_share to run on startup.'
read -p 'Proceed? [y/n] ' confirm
if [ "${confirm,,}" != 'y' ] && [ "${confirm,,}" != 'yes' ]; then
    echo 'Aborted.'
    echo 'You can still use clip_share by manually running the executable.'
    exit 0
fi

exec_names=(
    clip_share
    clip_share_web
    clip_share_GLIBC*
    clip_share_web_GLIBC*
)

exec_not_found=1
for exec_name in "${exec_names[@]}"; do
    if [ -f "$exec_name" ]; then
        exec_not_found=0
        mkdir -p ~/.local/bin/
        mv "$exec_name" ~/.local/bin/
        chmod +x ~/.local/bin/clip_share
        echo Moved "$exec_name" to ~/.local/bin/clip_share
    fi
done

if [ "$exec_not_found" = 1 ]; then
    echo "Error: 'clip_share' program was not found in the current directory"
    exit 1
fi

cd
export HOME="$(pwd)"

if [ ! -f clipshare.conf ]; then
    mkdir -p Downloads
    echo "working_dir=${HOME}/Downloads" >clipshare.conf
    echo "Created a new configuration file ${HOME}/clipshare.conf"
fi

mkdir -p .config/systemd/user

cat >.config/systemd/user/clipshare.service <<EOF
[Unit]
Description=ClipShare

[Service]
Type=forking
ExecStart=$HOME/.local/bin/clip_share
ExecStop=$HOME/.local/bin/clip_share -s
Restart=no

[Install]
WantedBy=graphical-session.target
EOF

systemctl --user daemon-reload
systemctl --user enable clipshare.service >/dev/null
echo 'Installed clip_share to run on startup. This takes effect from the next login.'

read -p 'Start clip_share now? [y/n] ' start_now
if [ "${start_now,,}" = 'y' ] || [ "${start_now,,}" = 'yes' ]; then
    systemctl --user start clipshare.service
    echo 'Started clip_share.'
fi
