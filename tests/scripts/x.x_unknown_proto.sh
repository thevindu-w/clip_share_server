#!/bin/bash

. init.sh

proto=$(printf '\x77' | bin2hex)
protoReject=$(printf '\x00' | bin2hex)

responseDump=$(echo -n "${proto}${protoReject}" | hex2bin | client_tool)

protoAck="$PROTO_UNKNOWN"
protoOffer="$PROTO_MAX_VERSION"

expected="${protoAck}${protoOffer}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi
