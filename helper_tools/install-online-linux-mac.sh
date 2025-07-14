#!/bin/bash

if [ "${0: -4}" != 'bash' ] && [ "${0: -3}" != '.sh' ]; then
    echo 'Please run using bash'
    echo 'Aborted.'
    exit 1
fi

error_exit() {
    echo -e '\n\033[1;31mError\033[0m' ':' "$1"
    exit 1
}

shopt -s expand_aliases 2>/dev/null || error_exit 'Please run using bash'
set -e

if [ "$(id -u)" = 0 ]; then
    echo 'Do not run install with sudo.'
    echo 'Aborted.'
    exit 1
fi

require_tool() {
    tool="$1"
    if ! type "$tool" &>/dev/null; then
        echo "Please make sure that ${tool} is installed and added to the PATH environment variable."
        error_exit "${tool} command not found!"
    fi
}

OS="$(uname 2>/dev/null || echo Unknown)"

if [ "$OS" = 'Linux' ]; then
    if ! type tar &>/dev/null; then
        export PATH="/usr/bin:$PATH"
    fi
    require_tool tar
    alias extract='tar -xzf'
    installer='install-linux.sh'
elif [ "$OS" = 'Darwin' ]; then
    if ! type unzip &>/dev/null; then
        export PATH="/usr/bin:$PATH"
    fi
    require_tool unzip
    alias extract='unzip'
    installer='install-mac.sh'
else
    echo 'This installer supports Linux and macOS only.'
    error_exit 'Installation failed!'
fi

if ! type curl &>/dev/null; then
    export PATH="/usr/bin:$PATH"
fi
if ! type wget &>/dev/null; then
    require_tool curl
fi
if type curl &>/dev/null; then
    alias download='curl -fOL'
else
    alias download='wget'
fi
require_tool mkdir
require_tool chmod
require_tool grep

VERSION_DEFAULT=
if [ -z "$VERSION_DEFAULT" ]; then
    read -p 'Enter version (ex: 3.2.0): ' VERSION_DEFAULT
fi
VERSION="$(echo -n "${VERSION_DEFAULT}" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')" || error_exit 'Invalid version'

echo "You are installing ClipShare version $VERSION"
read -p 'Proceed? [y/N] ' confirm
if [ "${confirm::1}" != 'y' ] && [ "${confirm::1}" != 'Y' ]; then
    echo 'Aborted.'
    exit 0
fi

if [ "$OS" = 'Linux' ]; then
    machine="$(uname -m)"
    if [ "$machine" = 'x86_64' ]; then
        ARCH=x86_64
    elif [ -n "$(grep -i armv7 <<<"$machine" || true)" ]; then
        ARCH=armv7
    else
        ARCH=arm64
    fi
fi

filename_prefix="clip_share_server-$VERSION"
if [ "$OS" = 'Linux' ]; then
    filename="${filename_prefix}-linux-${ARCH}.tar.gz"
elif [ "$OS" = 'Darwin' ]; then
    filename="${filename_prefix}-macos.zip"
fi

url="https://github.com/thevindu-w/clip_share_server/releases/download/v${VERSION}/${filename}"

suffix=0
tmpdir='clipsharetmp'
while [ -e "$tmpdir" ]; do
    [ "$suffix" -gt "1000" ] && error_exit 'Creating temporary directory failed'
    suffix="$((suffix + 1))"
    tmpdir="clipsharetmp_$suffix"
done
mkdir "$tmpdir" &>/dev/null || error_exit 'Creating temporary directory failed'
original_dir="$(pwd)"
cd "$tmpdir"

cleanup() {
    cd "$original_dir"
    rm -r "$tmpdir" &>/dev/null || true
}

echo -n 'Downloading binaries ...'
if ! download "$url" &>/dev/null; then
    cleanup
    error_exit 'Download failed'
fi
echo -e '\rDownload completed                   '

LINUX_AMD64_SHA=
LINUX_ARM64_SHA=
LINUX_ARMv7_SHA=
MAC_SHA=
if [ "$OS" = 'Linux' ]; then
    if [ "$ARCH" = 'x86_64' ]; then
        SHA256="$LINUX_AMD64_SHA"
    elif [ "$ARCH" = 'arm64' ]; then
        SHA256="$LINUX_ARM64_SHA"
    elif [ "$ARCH" = 'armv7' ]; then
        SHA256="$LINUX_ARMv7_SHA"
    fi
elif [ "$OS" = 'Darwin' ]; then
    SHA256="$MAC_SHA"
fi
if [ -n "$SHA256" ] && type sha256sum &>/dev/null && type cut &>/dev/null; then
    if [ "$(sha256sum -b "$filename" | cut -d ' ' -f 1)" != "$SHA256" ]; then
        cleanup
        error_exit 'File integrity check failed for downloaded file'
    fi
fi

echo -n 'Extracting ...'
if ! extract "$filename" &>/dev/null; then
    cleanup
    error_exit 'Extraction failed'
fi
echo -e '\rExtracting completed            '

if [ ! -f "$installer" ]; then
    cd "$filename_prefix"*/ || (
        cleanup
        error_exit 'Cannot find installer for this operating system'
    )
fi
if [ ! -f "$installer" ]; then
    cleanup
    error_exit 'Cannot find installer for this operating system'
fi

chmod +x "$installer"
"./$installer" || error_exit 'Installation failed'

cleanup
