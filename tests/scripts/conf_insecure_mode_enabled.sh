#!/bin/bash

. init.sh

update_config insecure_mode_enabled false

if test_port 4337; then
    showStatus info 'Still accepting plaintext connections'
    exit 1
fi

update_config insecure_mode_enabled true

if ! test_port 4337; then
    showStatus info 'Not accepting plaintext connections'
    exit 1
fi
