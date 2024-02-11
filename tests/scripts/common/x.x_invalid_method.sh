#!/bin/bash

. init.sh

method=$(printf '\x06' | bin2hex)

responseDump=$(echo -n "${proto}${method}" | hex2bin | client_tool)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_UNKNOWN_METHOD"

expected="${protoAck}${methodAck}"

if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi
