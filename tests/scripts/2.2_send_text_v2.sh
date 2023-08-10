#!/bin/bash

. init.sh

sample="Sample text for v2 send_text"

proto=$(printf "\x02" | bin2hex)
method=$(printf "\x02" | bin2hex)
length=$(printf "%016x" "${#sample}")
sampleDump=$(printf "${sample}" | bin2hex)

response=$(printf "${proto}${method}${length}${sampleDump}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck=$(printf "\x01" | bin2hex)
methodAck=$(printf "\x01" | bin2hex)

expected="${protoAck}${methodAck}"

if [ "${response}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi

clip="$(get_copied_text || echo fail)"

if [ "${clip}" != "${sample}" ]; then
    showStatus info "Clipcoard content not matching."
    exit 1
fi
