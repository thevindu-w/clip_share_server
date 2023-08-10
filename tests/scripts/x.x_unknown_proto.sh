#!/bin/bash

. init.sh

proto=$(printf "\x77" | bin2hex)
protoReject=$(printf "\x00" | bin2hex)

responseDump=$(printf "${proto}${protoReject}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck=$(printf '\x03' | bin2hex)
protoOffer=$(printf '%02x' "$proto_max_version")

expected="${protoAck}${protoOffer}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi
