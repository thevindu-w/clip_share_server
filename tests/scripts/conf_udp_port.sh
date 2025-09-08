#!/bin/bash

. init.sh

update_config secure_mode_enabled true no-restart
update_config insecure_mode_enabled true no-restart
update_config udp_port 6337

responseDump="$(echo -n 'in' | client_tool -u)"
if [ ! -z "$responseDump" ]; then
    showStatus info 'Still listening on 4337/udp'
    exit 1
fi

responseDump="$(echo -n 'in' | client_tool -u -p 6337)"
expected="$(echo -n 'clip_share' | bin2hex | tr -d '\n')"
if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Not listening on 6337/udp'
    exit 1
fi

if ! test_port 4337; then
    showStatus info 'Not accepting connections on 4337'
    exit 1
fi

if ! test_port 4338; then
    showStatus info 'Not accepting connections on 4338'
    exit 1
fi
