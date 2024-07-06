#!/bin/bash

. init.sh

responseDump=$(echo -n "${proto}${method}${disp}" | hex2bin | client_tool)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"

expected_proto_method_ack="${protoAck}${methodAck}"
len_expected_header="${#expected_proto_method_ack}"

if [ "${responseDump::len_expected_header}" != "$expected_proto_method_ack" ]; then
    showStatus info 'Incorrect protocol:method ack.'
    echo 'Expected:' "$expected_proto_method_ack"
    echo 'Received:' "${responseDump::len_expected_header}"
    exit 1
fi

responseDump="${responseDump:len_expected_header}"

if [ ! -z "$disp" ]; then
    if [ "${responseDump::2}" != "$DISPLAY_ACK" ]; then
        showStatus info 'Incorrect display ack.'
        echo 'Expected:' "$DISPLAY_ACK"
        echo 'Received:' "${responseDump::2}"
        exit 1
    fi
    if [ "$DISPLAY_ACK" = "$METHOD_NO_DATA" ]; then
        exit 0
    fi
    responseDump="${responseDump:2}"
fi

length="$((16#${responseDump::16}))"
responseDump="${responseDump:16}"

if [ "$length" -le '512' ] || [ "$length" != "$((${#responseDump} / 2))" ]; then
    echo "$length" does not match with "${#responseDump}"
    showStatus info 'Invalid image length.'
    exit 1
fi

expected_png_header="$(printf '\x89PNG\r\n\x1a\n' | bin2hex)"
png_header="${responseDump::${#expected_png_header}}"

if [ "$png_header" != "$expected_png_header" ]; then
    showStatus info 'Invalid image header.'
    exit 1
fi
