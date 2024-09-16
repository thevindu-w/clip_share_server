#!/bin/bash

. init.sh

proto="$PROTO_V3"
method="$METHOD_GET_SCREENSHOT"

disp_num=30000 # not existing display
disp="$(printf '%016x' $disp_num)"

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

displayAck="$METHOD_NO_DATA"
if [ "${responseDump::2}" != "$displayAck" ]; then
    showStatus info 'Incorrect display ack.'
    echo 'Expected:' "$displayAck"
    echo 'Received:' "${responseDump::2}"
    exit 1
fi
responseDump="${responseDump:2}"

if [ ! -z "$responseDump" ]; then
    showStatus info 'Has image data for invalid display.'
    echo 'Image data:' "${responseDump::10}"
    exit 1
fi

update_config client_selects_display 'false'

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

displayAck="$METHOD_OK"
if [ "${responseDump::2}" != "$displayAck" ]; then
    showStatus info 'Incorrect display ack.'
    echo 'Expected:' "$displayAck"
    echo 'Received:' "${responseDump::2}"
    exit 1
fi
responseDump="${responseDump:2}"

if [ "${#responseDump}" -lt 16 ]; then
    showStatus info 'Invalid length.'
    echo 'Data:' "${responseDump::16}"
    exit 1
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
