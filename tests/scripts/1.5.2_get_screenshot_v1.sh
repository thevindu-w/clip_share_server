#!/bin/bash

. init.sh

proto=$(printf "\x01" | bin2hex)
method=$(printf "\x05" | bin2hex)

responseDump=$(printf "${proto}${method}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck=$(printf "\x01" | bin2hex)
methodAck=$(printf "\x01" | bin2hex)

expected_proto_method_ack="${protoAck}${methodAck}"
len_expected_header="${#expected_proto_method_ack}"

if [ "${responseDump::len_expected_header}" != "${expected_proto_method_ack}" ]; then
    showStatus info "Incorrect protocol:method ack."
    exit 1
fi

length="$((16#${responseDump:len_expected_header:16}))"

if [ "$length" -le "8" ]; then
    showStatus info "Invalid image length."
    exit 1
fi

len_expected_header="$((len_expected_header + 16))"

expected_png_header="$(printf '\x89PNG\r\n\x1a\n' | bin2hex)"
png_header="${responseDump:len_expected_header:${#expected_png_header}}"

if [ "$png_header" != "$expected_png_header" ]; then
    showStatus info "Invalid image header."
    exit 1
fi
