#!/bin/bash

. init.sh

proto="$PROTO_V2"
method="$METHOD_GET_IMAGE"

clear_clipboard

responseDump=$(echo -n "${proto}${method}" | hex2bin | client_tool)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"

expected_proto_method_ack="${protoAck}${methodAck}"
len_expected_header="${#expected_proto_method_ack}"

if [ "${responseDump::len_expected_header}" != "${expected_proto_method_ack}" ]; then
    showStatus info 'Incorrect protocol:method ack.'
    echo 'Expected:' "${expected_proto_method_ack}"
    echo 'Received:' "${responseDump::len_expected_header}"
    exit 1
fi

length="$((16#${responseDump:len_expected_header:16}))"

if [ "$length" -le '8' ]; then
    showStatus info 'Invalid image length.'
    exit 1
fi

len_expected_header="$((len_expected_header + 16))"

expected_png_header="$(printf '\x89PNG\r\n\x1a\n' | bin2hex)"
png_header="${responseDump:len_expected_header:${#expected_png_header}}"

if [ "$png_header" != "$expected_png_header" ]; then
    showStatus info 'Invalid image header.'
    exit 1
fi
