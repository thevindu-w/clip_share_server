#!/bin/bash

. init.sh

imgSample="89504e470d0a1a0a0000000d4948445200000005000000050802000000020db1b20\
00000264944415408d755cb2112002010804070fcff973168f0681bb042b99501f5ac8bbf9ad6c\
dfc0f828c0e0522b1809c0000000049454e44ae426082"

printf "${imgSample}" | xxd -r -p | xclip -in -sel clip -t image/png

proto=$(printf "\x01" | xxd -p)
method=$(printf "\x05" | xxd -p)

responseDump=$(printf "${proto}${method}" | xxd -r -p | client_tool | xxd -p | tr -d '\n')

protoAck=$(printf "\x01" | xxd -p)
methodAck=$(printf "\x01" | xxd -p)
length=$(printf "%016x" $(($(printf "${imgSample}" | wc -c)/2)))

expected="${protoAck}${methodAck}${length}${imgSample}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus fail "Incorrect server response."
    exit 1
fi

showStatus pass
