#!/bin/bash

. init.sh

sample="Sample text for v2 get_text"

printf "${sample}" | xclip -in -sel clip

proto=$(printf "\x02" | xxd -p)
method=$(printf "\x01" | xxd -p)

responseDump=$(printf "${proto}${method}" | xxd -r -p | client_tool | xxd -p | tr -d '\n')

protoAck=$(printf "\x01" | xxd -p)
methodAck=$(printf "\x01" | xxd -p)
length=$(printf "%016x" "${#sample}")
sampleDump=$(printf "${sample}" | xxd -p | tr -d '\n')

expected="${protoAck}${methodAck}${length}${sampleDump}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi
