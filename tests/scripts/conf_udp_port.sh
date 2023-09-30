#!/bin/bash

. init.sh

update_config udp_port 6337

responseDump=$(printf "in" | timeout 1 nc -w 1 -u 127.0.0.1 4337 | bin2hex | tr -d '\n')
if [ ! -z "$responseDump" ]; then
    showStatus info "Still listens on 4337/udp"
    exit 1
fi

responseDump=$(printf "in" | timeout 1 nc -w 1 -u 127.0.0.1 6337 | head -n 1 | bin2hex | tr -d '\n')
expected="$(printf "clip_share" | bin2hex | tr -d '\n')"
if [ "$responseDump" != "$expected" ]; then
    showStatus info "Not listening on 6337/udp"
    exit 1
fi

if ! nc -zvn 127.0.0.1 4337 &>/dev/null; then
    showStatus info "Not accepting connections on 4337"
    exit 1
fi

if ! nc -zvn 127.0.0.1 4338 &>/dev/null; then
    showStatus info "Not accepting connections on 4338"
    exit 1
fi
