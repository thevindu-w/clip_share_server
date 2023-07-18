#!/bin/bash

. init.sh

sample="Sample text for v1 send_text"

proto=$(printf "\x01" | xxd -p)
method=$(printf "\x02" | xxd -p)
length=$(printf "%016x" "${#sample}")
sampleDump=$(printf "${sample}" | xxd -c 256 -p)

response=$(printf "${proto}${method}${length}${sampleDump}" | xxd -r -p | client_tool | xxd -p | tr -d '\n')

protoAck=$(printf "\x01" | xxd -p)
methodAck=$(printf "\x01" | xxd -p)

expected="${protoAck}${methodAck}"

if [ "${response}" != "${expected}" ]; then
    showStatus fail "Incorrect server response."
    exit 1
fi

clip=$(xclip -out -sel clip || echo fail)

if [ "${clip}" != "${sample}" ]; then
    showStatus fail "Clipcoard content not matching."
    exit 1
fi

showStatus pass
