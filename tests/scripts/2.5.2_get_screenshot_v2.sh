#!/bin/bash

. init.sh

# clear any images in clipboard
echo "dummy" | xclip -in -sel clip

proto=$(printf "\x02" | xxd -p)
method=$(printf "\x05" | xxd -p)

responseDump=$(printf "${proto}${method}" | xxd -r -p | client_tool | xxd -p | tr -d '\n')

protoAck=$(printf "\x01" | xxd -p)
methodAck=$(printf "\x01" | xxd -p)

expected_proto_method_ack="${protoAck}${methodAck}"
len_expected_header="${#expected_proto_method_ack}"

if [ "${responseDump::${len_expected_header}}" != "${expected_proto_method_ack}" ]; then
    showStatus fail "Incorrect protocol:method ack."
    exit 1
fi

length="$((16#${responseDump:${len_expected_header}:16}))"

if [ "$length" -le "8" ]; then
    showStatus fail "Invalid image length."
    exit 1
fi

len_expected_header="$((${len_expected_header}+16))"

expected_png_header="$(printf '\x89PNG\r\n\x1a\n' | xxd -p)"
png_header="${responseDump:${len_expected_header}:${#expected_png_header}}"

if [ "$png_header" != "$expected_png_header" ]; then
    showStatus fail "Invalid image header."
    exit 1
fi

showStatus pass
