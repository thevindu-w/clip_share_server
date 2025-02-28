#!/bin/bash

. init.sh

copy_image "$imgSample"

responseDump=$(echo -n "${proto}${method}" | hex2bin | client_tool)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"
length=$(printf '%016x' "$((${#imgSample} / 2))")

expected="${protoAck}${methodAck}${length}${imgSample}"

if [ "$DETECTED_OS" = 'Linux' ]; then
    if [ "$responseDump" != "$expected" ]; then
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
    if [ "$imgSize" -gt 512 ]; then
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
    showStatus info 'Unknown OS.'
    exit 1
fi
