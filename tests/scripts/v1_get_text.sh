#!/bin/bash

. init.sh

printf "This is a sample text" | xclip -in -sel clip

response=$(printf "\x01\x01" | nc 127.0.0.1 4337 | sed 's/\x00/00/g' | sed 's/\x01/01/g' | sed 's/\x15/15/g')

if [[ "${response}" != "01010000000000000015This is a sample text" ]]; then
    echo "Test 1. v1 get_text failed"
    exit 1
fi

echo "Test 1. v1 get_text passed"