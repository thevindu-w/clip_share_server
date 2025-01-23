#!/bin/bash

. init.sh

update_config bind_address_udp '10.20.30.40' # this address may or may not exist

if [ -n "$(echo -n 'in' | client_tool -u)" ]; then
    showStatus info 'Still listening on all addresses'
    exit 1
fi

update_config bind_address_udp '127.0.0.1'

expected="$(echo -n 'clip_share' | bin2hex | tr -d '\n')"
if [ "$(echo -n 'in' | client_tool -u)" != "$expected" ]; then
    showStatus info 'Not listening on 127.0.0.1'
    exit 1
fi

update_config bind_address_udp '0.0.0.0'

if [ "$(echo -n 'in' | client_tool -u)" != "$expected" ]; then
    showStatus info 'Not listening on any address'
    exit 1
fi
