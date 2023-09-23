#!/bin/bash

. init.sh

imgSample="89504e470d0a1a0a0000000d4948445200000005000000050802000000020db1b20\
00000264944415408d755cb2112002010804070fcff973168f0681bb042b99501f5ac8bbf9ad6c\
dfc0f828c0e0522b1809c0000000049454e44ae426082"

copy_image "${imgSample}"

proto=$(printf "\x01" | bin2hex)
method=$(printf "\x05" | bin2hex)

responseDump=$(printf "${proto}${method}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck=$(printf "\x01" | bin2hex)
methodAck=$(printf "\x01" | bin2hex)
length=$(printf "%016x" $(("${#imgSample}" / 2)))

expected="${protoAck}${methodAck}${length}${imgSample}"

if [ "DETECTED_OS" = "Linux" ]; then
    if [ "${responseDump}" != "${expected}" ]; then
        showStatus info "Incorrect server response."
        echo 'Expected:' "${expected::20} ..."
        echo 'Received:' "${responseDump::20} ..."
        exit 1
    fi
elif [ "DETECTED_OS" = "Windows" ]; then
    if [ "${responseDump::36}" != "${expected::36}" ]; then
        showStatus info "Incorrect server response."
        echo 'Expected:' "${expected::20} ..."
        echo 'Received:' "${responseDump::20} ..."
        exit 1
    fi
fi
