#!/bin/bash

. init.sh

responseDump=$(echo -n 'in' | timeout 1 nc -w 1 -u 127.0.0.1 4337 | head -n 1 | bin2hex | tr -d '\n')

expected="$(echo -n 'clip_share' | bin2hex | tr -d '\n')"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi
