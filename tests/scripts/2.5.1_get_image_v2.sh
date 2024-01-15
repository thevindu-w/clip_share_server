#!/bin/bash

. init.sh

imgSample="89504e470d0a1a0a0000000d4948445200000005000000050802000000020db1b20\
00000264944415408d755cb2112002010804070fcff973168f0681bb042b99501f5ac8bbf9ad6c\
dfc0f828c0e0522b1809c0000000049454e44ae426082"

copy_image "${imgSample}"

proto="$PROTO_V2"
method="$METHOD_GET_IMAGE"

responseDump=$(echo -n "${proto}${method}" | hex2bin | client_tool)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"
length=$(printf '%016x' "$((${#imgSample} / 2))")

expected="${protoAck}${methodAck}${length}${imgSample}"

if [ "$DETECTED_OS" = 'Linux' ]; then
    if [ "${responseDump}" != "${expected}" ]; then
        showStatus info 'Incorrect server response.'
        echo 'Expected:' "${expected::20} ..."
        echo 'Received:' "${responseDump::20} ..."
        exit 1
    fi
elif [ "$DETECTED_OS" = 'Windows' ]; then
    if [ "${responseDump::36}" != "${expected::36}" ]; then
        showStatus info 'Incorrect server response.'
        echo 'Expected:' "${expected::20} ..."
        echo 'Received:' "${responseDump::20} ..."
        exit 1
    fi
elif [ "$DETECTED_OS" = 'macOS' ]; then
    if [ "${responseDump::17}" != "${expected::17}" ]; then
        showStatus info 'Incorrect server response.'
        echo 'Expected:' "${expected::17} ..."
        echo 'Received:' "${responseDump::17} ..."
        exit 1
    fi
    imgSize="$((0x${responseDump:4:16}))"
    if [ "$imgSize" -gt '512' ]; then
        showStatus info "Image is too large. size=${imgSize}."
        exit 1
    fi
    if [ "${responseDump:20:16}" != "${expected:20:16}" ]; then
        showStatus info 'Incorrect server response.'
        echo 'Expected:' "${expected::17} ..."
        echo 'Received:' "${responseDump::17} ..."
        exit 1
    fi
else
    showStatus info "Unknown OS."
    exit 1
fi
