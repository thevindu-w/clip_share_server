#!/bin/bash

. init.sh

sample="get_text 範例文字 1"

copy_text "$sample"

proto=$(printf "\x01" | bin2hex)
method=$(printf "\x01" | bin2hex)

responseDump=$(printf "${proto}${method}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck=$(printf "\x01" | bin2hex)
methodAck=$(printf "\x01" | bin2hex)
printf -v _ '%s%n' "$sample" utf8len
length="$(printf '%016x' $utf8len)"
sampleDump=$(printf "$sample" | bin2hex | tr -d '\n')

expected="${protoAck}${methodAck}${length}${sampleDump}"

if [ "$responseDump" != "$expected" ]; then
    showStatus info "Incorrect server response."
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi
