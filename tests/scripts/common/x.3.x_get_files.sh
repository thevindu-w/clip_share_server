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

regFileCount="$(find . -type f | wc -l)"
if [ "$proto" -gt "$PROTO_V2" ]; then
    emptyDirCount="$(find . -type d -empty | wc -l)"
    fileCount="$((regFileCount + emptyDirCount))"
else
    fileCount="$regFileCount"
fi

cd ..

shopt -s nullglob
file_list=(original/*)
shopt -u nullglob
copy_files "${file_list[@]}"

method="$METHOD_GET_FILES"

responseDump=$(echo -n "${proto}${method}" | hex2bin | client_tool)

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"
fileCountHex=$(printf '%016x' "$fileCount")

expectedHead="${protoAck}${methodAck}${fileCountHex}"
responseHead="${responseDump::${#expectedHead}}"

if [ "$responseHead" != "$expectedHead" ]; then
    showStatus info 'Incorrect response header'
    echo 'Expected:' "$expectedHead"
    echo 'Received:' "$responseHead"
    exit 1
fi

mkdir -p copies && cd copies

body="${responseDump:${#expectedHead}}"
for _ in $(seq "$fileCount"); do
    nameLength="$((0x${body::16}))"
    if [ "$nameLength" -gt '1024' ]; then
        showStatus info "File name too long. Length=${nameLength}."
        exit 1
    fi
    fileName="$(echo "${body:16:$((nameLength * 2))}" | hex2bin)"
    body="${body:$((16 + nameLength * 2))}"

    fileSize="$((0x${body::16}))"
    if [ "$fileSize" -gt '1048576' ]; then
        showStatus info "File is too large. size=${fileSize}."
        exit 1
    fi
    if [ "$proto" -gt "$PROTO_V2" ] && [ "$fileSize" = '-1' ]; then
        mkdir -p "$fileName"
        body="${body:16}"
        continue
    fi
    if [[ $fileName == */* ]]; then
        mkdir -p "${fileName%/*}"
    fi
    echo "${body:16:$((fileSize * 2))}" | hex2bin >"$fileName"
    body="${body:$((16 + fileSize * 2))}"
done

if [ "$body" != '' ]; then
    showStatus info 'Incorrect response body'
    exit 1
fi

cd ..

# Proto v2 does not get empty directories
[ "$proto" -gt "$PROTO_V2" ] || find original -depth -type d -empty -delete

diffOutput=$(diff -rq original copies 2>&1 || echo failed)
if [ ! -z "$diffOutput" ]; then
    showStatus info 'Files do not match.'
    exit 1
fi
