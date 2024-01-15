#!/bin/bash

. init.sh

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

if ! nc -zvn 127.0.0.1 4337 &>/dev/null; then
    showStatus info 'Not accepting connections on 4337'
    exit 1
fi

if ! nc -zvn 127.0.0.1 4338 &>/dev/null; then
    showStatus info 'Not accepting connections on 4338'
    exit 1
fi
