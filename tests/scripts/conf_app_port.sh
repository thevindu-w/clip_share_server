#!/bin/bash

. init.sh

update_config app_port 6337

if nc -zvn 127.0.0.1 4337 &>/dev/null; then
    showStatus info 'Still accepting connections on 4337'
    exit 1
fi

if ! nc -zvn 127.0.0.1 6337 &>/dev/null; then
    showStatus info 'Not accepting connections on 6337'
    exit 1
fi

responseDump=$(echo -n '7700' | hex2bin | nc -w 1 127.0.0.1 6337 | bin2hex | tr -d '\n')
expected="03$(printf '%02x' $proto_max_version)"
if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

if ! nc -zvn 127.0.0.1 4338 &>/dev/null; then
    showStatus info 'Not accepting connections on 4338'
    exit 1
fi

responseDump="$(echo -n 'in' | timeout 1 nc -w 1 -u 127.0.0.1 4337 | head -n 1 | bin2hex | tr -d '\n')"
expected="$(echo -n 'clip_share' | bin2hex | tr -d '\n')"
if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Not listening on 4337/udp'
    exit 1
fi
