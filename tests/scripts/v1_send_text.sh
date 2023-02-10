#!/bin/bash

. init.sh

response=$(printf "\x01\x02\x00\x00\x00\x00\x00\x00\x00\x1cSample text for v1 send_text" | nc 127.0.0.1 4337 | sed 's/\x01/01/g')

if [[ "${response}" != "0101" ]]; then
    echo "Test 1. v1 send_text failed. Incorrect server response."
    exit 1
fi

clip=$(xclip -out -sel clip)

if [[ "${clip}" != "Sample text for v1 send_text" ]]; then
    echo "Test 1. v1 send_text failed. Clipcoard content not matching."
    exit 1
fi

echo "Test 2. v1 send_text passed"