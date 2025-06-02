#!/usr/bin/env bash
# Copy or cut the specified file names to the clipboard, like a GUI file manager would do.
# You need to have https://jqlang.org installed.
# Usage:
#   x-copy-files copy FILE...
#   x-copy-files cut FILE...
# Examples:
#   x-copy-files copy a.txt
#   x-copy-files cut 0.png 1.png
#   alias xcopy='x-copy-files copy' xcut='x-copy-files cut'

set -euo pipefail
action=${1?'the first argument must be `copy` or `cut`'}
shift
{
    printf '%s\n' "$action"
    realpath -e -- "$@" | jq -Rr '"file://\(split("/") | map(@uri) | join("/"))"'
} | xclip -sel clip -t x-special/gnome-copied-files
