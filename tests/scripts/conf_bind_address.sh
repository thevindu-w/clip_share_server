#!/bin/bash

. init.sh

update_config bind_address "10.20.30.40" # this address may or may not exist

if nc -zvn 127.0.0.1 4337 &>/dev/null; then
    showStatus info "Still listens on 127.0.0.1"
    exit 1
fi

update_config bind_address "127.0.0.1"

if ! nc -zvn 127.0.0.1 4337 &>/dev/null; then
    showStatus info "Not listening on 127.0.0.1"
    exit 1
fi

update_config bind_address "0.0.0.0"

if ! nc -zvn 127.0.0.1 4337 &>/dev/null; then
    showStatus info "Not listening on any address"
    exit 1
fi
