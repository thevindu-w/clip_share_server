#!/bin/bash

. init.sh

sample='get_text 範例文字 1'

copy_text "$sample"

proto="$PROTO_V1"
method="$METHOD_GET_TEXT"

responseDump=$(echo -n "${proto}${method}" | hex2bin | client_tool)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"
printf -v _ '%s%n' "$sample" utf8len
length="$(printf '%016x' $utf8len)"
sampleDump=$(echo -n "$sample" | bin2hex | tr -d '\n')

expected="${protoAck}${methodAck}${length}${sampleDump}"

if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi
