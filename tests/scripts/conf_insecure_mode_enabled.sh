#!/bin/bash

. init.sh

update_config insecure_mode_enabled false

if nc -zvn 127.0.0.1 4337 &>/dev/null; then
    showStatus info "Still accepts plaintext connections"
    exit 1
fi

update_config insecure_mode_enabled true

if ! nc -zvn 127.0.0.1 4337 &>/dev/null; then
    showStatus info "Not accepting plaintext connections"
    exit 1
fi
