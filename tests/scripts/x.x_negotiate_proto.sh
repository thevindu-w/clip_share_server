#!/bin/bash

. init.sh

proto=$(printf '\x77' | bin2hex)
protoAccept="$PROTO_MAX_VERSION"
method="$METHOD_GET_TEXT"

clear_clipboard

responseDump=$(echo -n "${proto}${protoAccept}${method}" | hex2bin | client_tool)

protoAck="${PROTO_UNKNOWN}${protoAccept}"
methodAck="$METHOD_NO_DATA"

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect server response.'
    exit 1
fi
