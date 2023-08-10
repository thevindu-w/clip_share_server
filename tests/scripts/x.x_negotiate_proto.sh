#!/bin/bash

. init.sh

sample="Sample text for negotiate_proto"
copy_text "${sample}"

proto=$(printf "\x77" | bin2hex)
protoAccept=$(printf '%02x' "$proto_max_version")
method=$(printf "\x01" | bin2hex)

responseDump=$(printf "${proto}${protoAccept}${method}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck=$(printf "\x03\x02" | bin2hex)
methodAck=$(printf "\x01" | bin2hex)
length=$(printf "%016x" "${#sample}")
sampleDump=$(printf "${sample}" | bin2hex | tr -d '\n')

expected="${protoAck}${methodAck}${length}${sampleDump}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi
