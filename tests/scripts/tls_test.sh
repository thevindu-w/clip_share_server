#!/bin/bash

export SECURE=1

. init.sh

proto=$(printf "\x02" | bin2hex)
method=$(printf "\x01" | bin2hex)

clear_clipboard

responseDump=$(printf "${proto}${method}" | hex2bin | client_tool 2>/dev/null | bin2hex | tr -d '\n')

protoAck=$(printf "\x01" | bin2hex)
methodAck=$(printf "\x02" | bin2hex)

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi
