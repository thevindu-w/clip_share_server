#!/bin/bash

. init.sh

update_config udp_server_enabled false

responseDump="$(echo -n 'in' | client_tool -u)"
if [ ! -z "$responseDump" ]; then
    showStatus info 'Still listening for UDP requests'
    exit 1
fi

update_config udp_server_enabled true

responseDump="$(echo -n 'in' | client_tool -u)"
expected="$(echo -n 'clip_share' | bin2hex | tr -d '\n')"
if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Not listening for UDP requests'
    exit 1
fi
