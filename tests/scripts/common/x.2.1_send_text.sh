#!/bin/bash

. init.sh

sample='send_text 範例文字 1'

method="$METHOD_SEND_TEXT"
printf -v _ '%s%n' "$sample" utf8len
length="$(printf '%016x' $utf8len)"
sampleDump=$(echo -n "$sample" | bin2hex)

clear_clipboard

responseDump=$(echo -n "${proto}${method}${length}${sampleDump}" | hex2bin | client_tool)

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

if [ "$clip" != "$sampleDump" ]; then
    showStatus info 'Clipcoard content not matching.'
    echo 'Expected:' "$sampleDump"
    echo 'Received:' "$clip"
    exit 1
fi
