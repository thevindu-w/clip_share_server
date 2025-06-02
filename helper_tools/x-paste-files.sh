#!/usr/bin/env bash
# Paste files from the clipboard, like a GUI file manager would do.
# Command-line options are forwarded to `cp` or `mv`.
# Usage:
#   x-paste-files [options] [TARGET]
# Examples:
#   x-paste-files
#   x-paste-files ~
#   x-paste-files subdir/ -f

set -euo pipefail
xclip -o -sel clip -t x-special/gnome-copied-files | (
    read -r action
    case $action in
        copy) action='cp';;
        cut)  action='mv';;
        *)    exit 1;;
    esac

    while read -r f; [[ "$f" ]]; do
        f=${f#file://}
        printf -v f %b "${f//%/\\x}"
        files+=("$f")
    done

    [[ $# -eq 0 ]] && set .
    exec "$action" -i "${files[@]}" "$@"
)
