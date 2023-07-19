#!/bin/bash

. init.sh

proto=$(printf "\x77" | xxd -p)
protoReject=$(printf "\x00" | xxd -p)

responseDump=$(printf "${proto}${protoReject}" | xxd -r -p | client_tool | xxd -p | tr -d '\n')

protoAck=$(printf '\x03' | xxd -p)
protoOffer=$(printf '%02x' "$proto_max_version")

expected="${protoAck}${protoOffer}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi
