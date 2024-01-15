#!/bin/bash

. init.sh

responseDump=$(echo -n 'in' | client_tool -u)

expected="$(echo -n 'clip_share' | bin2hex | tr -d '\n')"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi
