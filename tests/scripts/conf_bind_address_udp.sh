#!/bin/bash

. init.sh

broadcast_addr='127.255.255.255'
if [ "$DETECTED_OS" != 'macOS' ]; then
    update_config bind_address_udp '10.20.30.40' # this address may or may not exist

    if [ -n "$(echo -n 'in' | client_tool -u -h 127.255.255.255)" ]; then
        showStatus info 'Still listening on all addresses'
        exit 1
    fi
else
    broadcast_addr='255.255.255.255'
fi

update_config bind_address_udp '127.0.0.1'

expected="$(echo -n 'clip_share' | bin2hex | tr -d '\n')"
if [ "$(echo -n 'in' | client_tool -u -h ${broadcast_addr})" != "$expected" ]; then
    showStatus info "Not listening on ${broadcast_addr}"
    exit 1
fi

update_config bind_address_udp '0.0.0.0'

if [ "$(echo -n 'in' | client_tool -u)" != "$expected" ] || [ "$(echo -n 'in' | client_tool -u -h ${broadcast_addr})" != "$expected" ]; then
    showStatus info 'Not listening on any address'
    exit 1
fi
