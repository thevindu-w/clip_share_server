#!/bin/bash

. init.sh

proto="$PROTO_V2"
method=$(printf '\x06' | bin2hex)

responseDump=$(echo -n "${proto}${method}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_UNKNOWN_METHOD"

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi
