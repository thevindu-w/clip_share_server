#!/bin/bash

. init.sh

update_config secure_mode_enabled false

if nc -zvn 127.0.0.1 4338 &>/dev/null; then
    showStatus info "Still accepts TLS connections"
    exit 1
fi

update_config secure_mode_enabled true

if ! nc -zvn 127.0.0.1 4338 &>/dev/null; then
    showStatus info "Not accepting TLS connections"
    exit 1
fi
