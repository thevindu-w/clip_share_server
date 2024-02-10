#!/bin/bash

. init.sh

mkdir -p original && cd original

for f in "${files[@]}"; do
    echo "${f}"$'\n''abc' >"$f"
done

chunks=''
for fname in *; do
    printf -v _ '%s%n' "$fname" utf8nameLen
    nameLength="$(printf '%016x' $utf8nameLen)"
    fileSize=$(printf '%016x' $(stat -c '%s' "$fname"))
    content=$(cat "$fname" | bin2hex | tr -d '\n')
    chunks+="${nameLength}$(echo -n "$fname" | bin2hex)${fileSize}${content}"
done

cd ..

shopt -s nullglob
file_list=(original/*)
shopt -u nullglob
copy_files "${file_list[@]}"

proto="$PROTO_V1"
method="$METHOD_GET_FILES"

responseDump=$(echo -n "${proto}${method}" | hex2bin | client_tool)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"
fileCount=$(printf '%016x' $(echo -n "${#files[@]}"))

expected="${protoAck}${methodAck}${fileCount}${chunks}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "${expected::20} ..."
    echo 'Received:' "${responseDump::20} ..."
    exit 1
fi
