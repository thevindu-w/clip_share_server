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

for fname in "${files[@]}"; do
    printf -v _ '%s%n' "$fname" utf8nameLen
    nameLength="$(printf '%016x' $utf8nameLen)"
    if [ -d "$fname" ]; then
        fileSize=-1
        content=''
    else
        fileSize="$(stat -c '%s' "$fname")"
        content=$(cat "$fname" | bin2hex | tr -d '\n')
    fi
    fileSize16=$(printf '%016x' "$fileSize")
    chunks+="${nameLength}$(echo -n "$fname" | bin2hex)${fileSize16}${content}"
done

cd ..
mkdir -p copies
update_config working_dir copies

method="$METHOD_SEND_FILES"
fileCount=$(printf '%016x' $(echo -n "${#files[@]}"))

responseDump=$(echo -n "${proto}${method}${fileCount}${chunks}" | hex2bin | client_tool)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"

expected="${protoAck}${methodAck}"

if [ "$responseDump" != "$expected" ]; then
    showStatus info 'Incorrect response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

diffOutput=$(diff -rq original copies 2>&1 || echo failed)
if [ ! -z "$diffOutput" ]; then
    showStatus info 'Files do not match.'
    exit 1
fi
