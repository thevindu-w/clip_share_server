#!/bin/bash

. init.sh

proto=$(printf "\x00" | bin2hex)

responseDump=$(printf "${proto}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck=$(printf "\x02" | bin2hex)

expected="${protoAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi
