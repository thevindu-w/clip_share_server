#!/bin/bash

. init.sh

update_config secure_mode_enabled false

if test_port 4338; then
    showStatus info 'Still accepting TLS connections'
    exit 1
fi

update_config secure_mode_enabled true

if ! test_port 4338; then
    showStatus info 'Not accepting TLS connections'
    exit 1
fi
