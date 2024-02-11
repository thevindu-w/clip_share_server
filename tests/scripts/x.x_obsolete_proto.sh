#!/bin/bash

. init.sh

proto=$(printf '\x00' | bin2hex)

responseDump=$(echo -n "$proto" | hex2bin | client_tool)

protoAck="$PROTO_OBSOLETE"

expected="$protoAck"

if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi
