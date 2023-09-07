#!/bin/bash

. init.sh

fileName="file 1.txt"

mkdir -p original && cd original

echo "abc"$'\n'"def"$'\n'"file content" >"${fileName}"

nameLength=$(printf "%016x" "${#fileName}")
fileSize=$(printf "%016x" $(stat -c '%s' "${fileName}"))
content=$(cat "${fileName}" | bin2hex | tr -d '\n')
body="${nameLength}$(printf "${fileName}" | bin2hex)${fileSize}${content}"

cd ..
mkdir -p copy
update_config working_dir copy

proto=$(printf "\x01" | bin2hex)
method=$(printf "\x04" | bin2hex)

responseDump=$(printf "${proto}${method}${body}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck=$(printf "\x01" | bin2hex)
methodAck=$(printf "\x01" | bin2hex)

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect response."
    exit 1
fi

rm -f original/server_err.log copy/server_err.log

diffOutput=$(diff -rq original copy 2>&1 || echo failed)
if [ ! -z "${diffOutput}" ]; then
    showStatus info "File does not match."
    exit 1
fi
