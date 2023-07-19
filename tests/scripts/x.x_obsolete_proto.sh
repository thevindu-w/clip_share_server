#!/bin/bash

. init.sh

proto=$(printf "\x00" | xxd -p)

responseDump=$(printf "${proto}" | xxd -r -p | client_tool | xxd -p | tr -d '\n')

protoAck=$(printf "\x02" | xxd -p)

expected="${protoAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi
