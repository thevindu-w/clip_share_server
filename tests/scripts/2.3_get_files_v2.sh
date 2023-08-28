#!/bin/bash

. init.sh

files=(
    "file 1.txt"
    "file_2.txt"
    "sub/file 3.txt"
    "sub/file 4.txt"
    "sub 1/file 5.txt"
    "sub 1/subsub/file 6.txt"
    "sub 1/subsub/file_7.txt"
    "sub 1/subsub_2/file_8.txt"
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
        nameLength=$(printf "%016x" "${#fname}")
        fileSize=$(printf "%016x" $(stat -c '%s' "${fname}"))
        content=$(cat "${fname}" | bin2hex | tr -d '\n')
        chunks+="${nameLength}$(printf "${fname}" | bin2hex)${fileSize}${content}"
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

proto=$(printf "\x02" | bin2hex)
method=$(printf "\x03" | bin2hex)

responseDump=$(printf "${proto}${method}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck=$(printf "\x01" | bin2hex)
methodAck=$(printf "\x01" | bin2hex)
fileCount=$(printf "%016x" $(printf "${#files[@]}"))

expectedHead="${protoAck}${methodAck}${fileCount}"
responseHead="${responseDump::${#expectedHead}}"

if [ "${responseHead}" != "${expectedHead}" ]; then
    showStatus info "Incorrect response header"
    exit 1
fi

mkdir -p copies && cd copies

body="${responseDump:${#expectedHead}}"
for _ in $(seq "${#files[@]}"); do
    nameLength="$((0x"${body::16}"))"
    if [ "$nameLength" -gt "1024" ]; then
        showStatus info "File name too long. Length=${nameLength}."
        exit 1
    fi
    fileName="$(echo "${body:16:$(("$nameLength" * 2))}" | hex2bin)"
    body="${body:$((16 + "$nameLength" * 2))}"

    fileSize="$((0x"${body::16}"))"
    if [ "$fileSize" -gt "1048576" ]; then
        showStatus info "File is too large. size=${fileSize}."
        exit 1
    fi
    if [[ $fileName == */* ]]; then
        mkdir -p "${fileName%/*}"
    fi
    echo "${body:16:$(("$fileSize" * 2))}" | hex2bin >"$fileName"
    body="${body:$((16 + "$fileSize" * 2))}"
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
