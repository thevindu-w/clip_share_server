#!/bin/bash

. init.sh

clear_clipboard

responseDump=$(echo -n "${PROTO_MAX_VERSION}${METHOD_GET_TEXT}" | hex2bin | client_tool)
expected="${PROTO_SUPPORTED}${METHOD_NO_DATA}"
if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

update_config max_proto_version 2

responseDump=$(echo -n "${PROTO_V3}00" | hex2bin | client_tool)
expected="${PROTO_UNKNOWN}${PROTO_V2}"
if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

responseDump=$(echo -n "${PROTO_V2}${METHOD_GET_TEXT}" | hex2bin | client_tool)
expected="${PROTO_SUPPORTED}${METHOD_NO_DATA}"
if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi
