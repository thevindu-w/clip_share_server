#!/bin/bash

if [ "$OS" = 'Windows_NT' ]; then
    echo 'Please run `install-windows.bat` to install.'
    exit 0
elif [ "$(uname)" = 'Linux' ]; then
    . helper_tools/install-linux.sh
elif [ "$(uname)" = 'Darwin' ]; then
    . helper_tools/install-mac.sh
else
    echo Could not detect the OS.
    exit 1
fi
