#!/bin/bash

. init.sh

clear_clipboard

proto="$PROTO_V2"
method="$METHOD_GET_TEXT"

responseDump=$(echo -n "${proto}${method}" | hex2bin | client_tool -s)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_NO_DATA"

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect server response.'
    exit 1
fi
