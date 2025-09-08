#!/bin/bash

. init.sh

update_config secure_mode_enabled true no-restart
update_config insecure_mode_enabled true no-restart
update_config app_port_secure 6338

if test_port 4338; then
    showStatus info 'Still accepting connections on 4338'
    exit 1
fi

if ! test_port 6338; then
    showStatus info 'Not accepting connections on 6338'
    exit 1
fi

responseDump=$(echo -n '7700' | hex2bin | client_tool -s -p 6338)
expected="${PROTO_UNKNOWN}${PROTO_MAX_VERSION}"
if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

if ! test_port 4337; then
    showStatus info 'Not accepting connections on 4337'
    exit 1
fi

responseDump="$(echo -n 'in' | client_tool -u)"
expected="$(echo -n 'clip_share' | bin2hex | tr -d '\n')"
if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Not listening on 4337/udp'
    exit 1
fi
