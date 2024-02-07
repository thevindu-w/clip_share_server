#!/bin/bash

. init.sh

method="$METHOD_GET_FILES"

clear_clipboard

responseDump=$(echo -n "${proto}${method}" | hex2bin | client_tool)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_NO_DATA"

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi
