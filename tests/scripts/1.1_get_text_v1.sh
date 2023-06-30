#!/bin/bash

. init.sh

sample="This is a sample text"

printf "${sample}" | xclip -in -sel clip

proto=$(printf "\x01" | xxd -p)
method=$(printf "\x01" | xxd -p)

responseDump=$(printf "${proto}${method}" | xxd -r -p | client_tool | xxd -p | tr -d '\n')

protoAck=$(printf "\x01" | xxd -p)
methodAck=$(printf "\x01" | xxd -p)
length=$(printf "%016x" $(printf "${sample}" | wc -c))
sampleDump=$(printf "${sample}" | xxd -p | tr -d '\n')

expected="${protoAck}${methodAck}${length}${sampleDump}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus fail "Incorrect server response."
    exit 1
fi

showStatus pass