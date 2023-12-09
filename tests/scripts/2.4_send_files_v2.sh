#!/bin/bash

. init.sh

files=(
    'file 1.txt'
    'file_2.txt'
    'sub/file 3.txt'
    'sub/file 4.txt'
    'sub 1/file 5.txt'
    'sub 1/subsub/file 6.txt'
    'sub 1/subsub/file_7.txt'
    'sub 1/subsub_2/file_8.txt'
)

mkdir -p original && cd original

for f in "${files[@]}"; do
    if [[ $f == */* ]]; then
        mkdir -p "${f%/*}"
    fi
    echo "${f}"$'\n''abc' >"${f}"
done

chunks=''

appendToChunks() {
    fname="$1"
    if [ -d "${fname}" ]; then
        for f in "${fname}"/*; do
            appendToChunks "${f}"
        done
    elif [ -f "${fname}" ]; then
        nameLength=$(printf '%016x' "${#fname}")
        fileSize=$(printf '%016x' $(stat -c '%s' "${fname}"))
        content=$(cat "${fname}" | bin2hex | tr -d '\n')
        chunks+="${nameLength}$(echo -n "${fname}" | bin2hex)${fileSize}${content}"
    fi
}

for f in *; do
    appendToChunks "${f}"
done

cd ..
mkdir -p copies
update_config working_dir copies

proto="$PROTO_V2"
method="$METHOD_SEND_FILES"
fileCount=$(printf '%016x' $(echo -n "${#files[@]}"))

responseDump=$(echo -n "${proto}${method}${fileCount}${chunks}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

diffOutput=$(diff -rq original copies 2>&1 || echo failed)
if [ ! -z "${diffOutput}" ]; then
    showStatus info 'Files do not match.'
    exit 1
fi
