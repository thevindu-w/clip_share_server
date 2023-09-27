#!/bin/bash

. init.sh

proto=$(printf "\x77" | bin2hex)
protoAccept=$(printf '%02x' "$proto_max_version")
method=$(printf "\x01" | bin2hex)

clear_clipboard

responseDump=$(printf "${proto}${protoAccept}${method}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck=$(printf "\x03\x02" | bin2hex)
methodAck=$(printf "\x02" | bin2hex)

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi
