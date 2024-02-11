#!/bin/bash

. init.sh

update_config max_text_length 20

short_sample='short text'
proto="$PROTO_V2"
method="$METHOD_SEND_TEXT"
length=$(printf '%016x' "${#short_sample}")
short_sampleDump=$(echo -n "$short_sample" | bin2hex)

clear_clipboard

responseDump=$(echo -n "${proto}${method}${length}${short_sampleDump}" | hex2bin | client_tool)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"

expected="${protoAck}${methodAck}"

if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

clip="$(get_copied_text || echo fail)"

if [ "$clip" != "$short_sampleDump" ]; then
    showStatus info 'Not accepting short text.'
    echo 'Expected:' "$short_sampleDump"
    echo 'Received:' "$clip"
    exit 1
fi

long_sample='This is a long text that is longer than 20 characters.'
proto="$PROTO_V2"
method="$METHOD_SEND_TEXT"
length=$(printf '%016x' "${#long_sample}")
long_sampleDump=$(echo -n "$long_sample" | bin2hex)

responseDump=$(echo -n "${proto}${method}${length}" | hex2bin | client_tool)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"

expected="${protoAck}${methodAck}"

if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

echo -n "${proto}${method}${length}${long_sampleDump}" | hex2bin | client_tool &>/dev/null

clip="$(get_copied_text || echo fail)"
# still the clipboard should have the short text.
# It should not be replaced by long text as the long text is rejected by the server.
if [ "$clip" != "$short_sampleDump" ]; then
    showStatus info 'Not accepting short text.'
    echo 'Expected:' "$short_sampleDump"
    echo 'Received:' "$clip"
    exit 1
fi
