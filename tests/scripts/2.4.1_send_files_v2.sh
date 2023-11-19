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
mkdir -p copies
update_config working_dir copies

proto=$(printf '\x02' | bin2hex)
method=$(printf '\x04' | bin2hex)
fileCount=$(printf '%016x' $(echo -n "${#files[@]}"))

responseDump=$(echo -n "${proto}${method}${fileCount}${chunks}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck=$(printf '\x01' | bin2hex)
methodAck=$(printf '\x01' | bin2hex)

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

diffOutput=$(diff -rq original copies 2>&1 || echo failed)
if [ ! -z "${diffOutput}" ]; then
    showStatus info "Files do not match."
    exit 1
fi
