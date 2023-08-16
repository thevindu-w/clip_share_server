#!/bin/bash

export SECURE=1

. init.sh

sample="This is a sample text for secure mode"

copy_text "${sample}"

proto=$(printf "\x02" | bin2hex)
method=$(printf "\x01" | bin2hex)

responseDump=$(printf "${proto}${method}" | hex2bin | client_tool 2>/dev/null | bin2hex | tr -d '\n')

protoAck=$(printf "\x01" | bin2hex)
methodAck=$(printf "\x01" | bin2hex)
length=$(printf "%016x" "${#sample}")
sampleDump=$(printf "${sample}" | bin2hex | tr -d '\n')

expected="${protoAck}${methodAck}${length}${sampleDump}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi
