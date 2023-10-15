#!/bin/bash

. init.sh

update_config app_port 6337

if nc -zvn 127.0.0.1 4337 &>/dev/null; then
    showStatus info "Still accepts connections on 4337"
    exit 1
fi

if ! nc -zvn 127.0.0.1 6337 &>/dev/null; then
    showStatus info "Not accepting connections on 6337"
    exit 1
fi

sample="Sample text for conf test"
copy_text "${sample}"
responseDump=$(echo -n "0201" | hex2bin | nc -w 1 127.0.0.1 6337 | bin2hex | tr -d '\n')
length=$(printf "%016x" "${#sample}")
sampleDump=$(echo -n "${sample}" | bin2hex | tr -d '\n')
expected="0101${length}${sampleDump}"
if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi

if ! nc -zvn 127.0.0.1 4338 &>/dev/null; then
    showStatus info "Not accepting connections on 4338"
    exit 1
fi

responseDump=$(printf "in" | timeout 1 nc -w 1 -u 127.0.0.1 4337 | head -n 1 | bin2hex | tr -d '\n')
expected="$(printf "clip_share" | bin2hex | tr -d '\n')"
if [ "$responseDump" != "$expected" ]; then
    showStatus info "Not listening on 4337/udp"
    exit 1
fi