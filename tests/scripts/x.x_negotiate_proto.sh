#!/bin/bash

. init.sh

sample="Sample text for negotiate_proto"
printf "${sample}" | xclip -in -sel clip

proto=$(printf "\x77" | xxd -p)
protoAccept=$(printf '%02x' "$proto_max_version")
method=$(printf "\x01" | xxd -p)

responseDump=$(printf "${proto}${protoAccept}${method}" | xxd -r -p | client_tool | xxd -p | tr -d '\n')

protoAck=$(printf "\x03\x02" | xxd -p)
methodAck=$(printf "\x01" | xxd -p)
length=$(printf "%016x" "${#sample}")
sampleDump=$(printf "${sample}" | xxd -p | tr -d '\n')

expected="${protoAck}${methodAck}${length}${sampleDump}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi
