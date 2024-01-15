#!/bin/bash

. init.sh

update_config app_port_secure 6338

if nc -zvn 127.0.0.1 4338 &>/dev/null; then
    showStatus info 'Still accepting connections on 4338'
    exit 1
fi

if ! nc -zvn 127.0.0.1 6338 &>/dev/null; then
    showStatus info 'Not accepting connections on 6338'
    exit 1
fi

responseDump=$(echo -n '7700' | hex2bin | client_tool -s -p 6338)
expected="${PROTO_UNKNOWN}${PROTO_MAX_VERSION}"
if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

if ! nc -zvn 127.0.0.1 4337 &>/dev/null; then
    showStatus info 'Not accepting connections on 4337'
    exit 1
fi

responseDump="$(echo -n 'in' | client_tool -u)"
expected="$(echo -n 'clip_share' | bin2hex | tr -d '\n')"
if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Not listening on 4337/udp'
    exit 1
fi
