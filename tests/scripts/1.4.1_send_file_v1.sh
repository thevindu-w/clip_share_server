#!/bin/bash

. init.sh

fileName='文字檔案 1.txt'

mkdir -p original && cd original

echo "abc"$'\n'"$fileName"$'\n'"file content" >"$fileName"

printf -v _ '%s%n' "$fileName" utf8nameLen
nameLength="$(printf '%016x' $utf8nameLen)"
fileSize=$(printf '%016x' $(stat -c '%s' "${fileName}"))
content=$(cat "$fileName" | bin2hex | tr -d '\n')
body="${nameLength}$(echo -n "$fileName" | bin2hex)${fileSize}${content}"

cd ..
mkdir -p copy
update_config working_dir copy

proto="$PROTO_V1"
method="$METHOD_SEND_FILES"

responseDump=$(echo -n "${proto}${method}${body}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

diffOutput=$(diff -rq original copy 2>&1 || echo failed)
if [ ! -z "${diffOutput}" ]; then
    showStatus info "File does not match."
    exit 1
fi
