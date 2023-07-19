#!/bin/bash

. init.sh

proto=$(printf "\x01" | xxd -p)
method=$(printf "\x03" | xxd -p)

responseDump=$(printf "${proto}${method}" | xxd -r -p | client_tool | xxd -p | tr -d '\n')

protoAck=$(printf "\x01" | xxd -p)
methodAck=$(printf "\x02" | xxd -p)

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi
