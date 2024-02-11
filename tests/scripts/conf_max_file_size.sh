#!/bin/bash

. init.sh

small_sample='short text'
large_sample='This is a long text that is longer than 20 characters.'
small_file='small.txt'
large_file='large.txt'

mkdir -p original && cd original

echo "$small_sample" >"$small_file"
echo "$large_sample" >"$large_file"

chunks=''
for fname in *; do
    nameLength=$(printf '%016x' "${#fname}")
    fileSize=$(printf '%016x' $(stat -c '%s' "$fname"))
    content=$(cat "$fname" | bin2hex | tr -d '\n')
    chunks+="${nameLength}$(echo -n "$fname" | bin2hex)${fileSize}${content}"
done

cd ..
mkdir -p copies
update_config working_dir copies

proto="$PROTO_V2"
method="$METHOD_SEND_FILES"
fileCount=$(printf '%016x' 2)

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
    showStatus info 'Files do not match before limiting max file size.'
    exit 1
fi

# clear copies
rm -f copies/*

update_config max_file_size 20

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

findOutput=$(find copies -type f -size -21c 2>&1 || echo failed)
if [ ! -z "$findOutput" ]; then
    showStatus info 'Large file is also saved.'
    exit 1
fi
