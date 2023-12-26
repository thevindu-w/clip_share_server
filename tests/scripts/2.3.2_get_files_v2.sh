#!/bin/bash

. init.sh

files=(
    "文字檔案 1.txt"
    "另一個文件_2.txt"
    "資料夾_1/文字檔案 3.txt"
    "資料夾_1/另一個文件 4.txt"
    "資料夾 2/文字檔案 5.txt"
    "資料夾 2/子資料夾/文字檔案 6.txt"
    "資料夾 2/子資料夾/另一個文件_7.txt"
    "資料夾 2/子資料夾 2/文字檔案 8.txt"
)

mkdir -p original && cd original

for f in "${files[@]}"; do
    if [[ $f == */* ]]; then
        mkdir -p "${f%/*}"
    fi
    echo "${f}"$'\n'"abc" >"${f}"
done

chunks=""

appendToChunks() {
    fname="$1"
    if [ -d "${fname}" ]; then
        for f in "${fname}"/*; do
            appendToChunks "${f}"
        done
    elif [ -f "${fname}" ]; then
        printf -v _ '%s%n' "$fname" utf8nameLen
        nameLength="$(printf '%016x' $utf8nameLen)"
        fileSize=$(printf '%016x' $(stat -c '%s' "${fname}"))
        content=$(cat "${fname}" | bin2hex | tr -d '\n')
        chunks+="${nameLength}$(echo -n "$fname" | bin2hex)${fileSize}${content}"
    fi
}

for f in *; do
    appendToChunks "${f}"
done

cd ..

shopt -s nullglob
file_list=(original/*)
shopt -u nullglob
copy_files "${file_list[@]}"

proto="$PROTO_V2"
method="$METHOD_GET_FILES"

responseDump=$(echo -n "${proto}${method}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"
fileCount=$(printf '%016x' $(echo -n "${#files[@]}"))

expectedHead="${protoAck}${methodAck}${fileCount}"
responseHead="${responseDump::${#expectedHead}}"

if [ "${responseHead}" != "${expectedHead}" ]; then
    showStatus info "Incorrect response header"
    echo 'Expected:' "$expectedHead"
    echo 'Received:' "$responseHead"
    exit 1
fi

mkdir -p copies && cd copies

body="${responseDump:${#expectedHead}}"
for _ in $(seq "${#files[@]}"); do
    nameLength="$((0x${body::16}))"
    if [ "$nameLength" -gt "1024" ]; then
        showStatus info "File name too long. Length=${nameLength}."
        exit 1
    fi
    fileName="$(echo "${body:16:$((nameLength * 2))}" | hex2bin)"
    body="${body:$((16 + nameLength * 2))}"

    fileSize="$((0x${body::16}))"
    if [ "$fileSize" -gt "1048576" ]; then
        showStatus info "File is too large. size=${fileSize}."
        exit 1
    fi
    if [[ $fileName == */* ]]; then
        mkdir -p "${fileName%/*}"
    fi
    echo "${body:16:$((fileSize * 2))}" | hex2bin >"$fileName"
    body="${body:$((16 + fileSize * 2))}"
done

if [ "${body}" != "" ]; then
    showStatus info "Incorrect response body"
    exit 1
fi

cd ..

diffOutput=$(diff -rq original copies 2>&1 || echo failed)
if [ ! -z "${diffOutput}" ]; then
    showStatus info "Files do not match."
    exit 1
fi
