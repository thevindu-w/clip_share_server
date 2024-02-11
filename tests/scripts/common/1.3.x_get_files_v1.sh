#!/bin/bash

. init.sh

mkdir -p original && cd original

for f in "${files[@]}"; do
    if [[ $f == */* ]]; then
        mkdir -p "${f%/*}"
    fi
    if [[ $f != */ ]]; then
        echo "$f"$'\n''abc' >"$f"
    fi
done

chunks=''
fileCount=0
for fname in *; do
    if [ ! -f "$fname" ]; then
        continue
    fi
    printf -v _ '%s%n' "$fname" utf8nameLen
    nameLength="$(printf '%016x' $utf8nameLen)"
    fileSize=$(printf '%016x' $(stat -c '%s' "$fname"))
    content=$(cat "$fname" | bin2hex | tr -d '\n')
    chunks+="${nameLength}$(echo -n "$fname" | bin2hex)${fileSize}${content}"
    fileCount="$((fileCount + 1))"
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
fileCountHex=$(printf '%016x' "$fileCount")

expected="${protoAck}${methodAck}${fileCountHex}${chunks}"

if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Incorrect server response.'
    echo 'Expected:' "${expected::20} ..."
    echo 'Received:' "${responseDump::20} ..."
    exit 1
fi
