#!/bin/bash

. init.sh

responseDump=$(printf "in" | nc -w 1 -u -W 1 127.0.0.1 4337 | head -n 1 | xxd -p | tr -d '\n')

expected="$(printf "clip_share" | xxd -p | tr -d '\n')"

if [[ "${responseDump}" != "${expected}" ]]; then
    showStatus fail "Incorrect server response."
    exit 1
fi

showStatus pass