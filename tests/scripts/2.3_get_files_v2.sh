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
    if [[ "$f" = */* ]]; then
        mkdir -p "${f%/*}";
    fi;
    echo "${f}"$'\n'"abc" > "${f}"
done

chunks=""

appendToChunks () {
    fname="$1"
    if [ -d "${fname}" ]; then
        for f in "${fname}"/*; do
            appendToChunks "${f}"
        done
    elif [ -f "${fname}" ]; then
        nameLength=$(printf "%016x" $(printf "${fname}" | wc -c))
        fileSize=$(printf "%016x" $(stat -c '%s' "${fname}"))
        content=$(cat "${fname}" | xxd -p | tr -d '\n')
        chunks+="${nameLength}$(printf "${fname}" | xxd -p)${fileSize}${content}"
    fi
}

for f in *; do
    appendToChunks "${f}"
done

cd ..

urls=""
for f in original/*; do
    absPath=$(realpath "${f}")
    fPathUrl=$(python -c "import urllib;print urllib.quote(raw_input())" <<< "${absPath}")
    urls+=$'\n'"file://${fPathUrl}"
done

echo -n "copy${urls}" | xclip -in -sel clip -t x-special/gnome-copied-files

proto=$(printf "\x02" | xxd -p)
method=$(printf "\x03" | xxd -p)

responseDump=$(printf "${proto}${method}" | xxd -r -p | client_tool | xxd -p | tr -d '\n')

protoAck=$(printf "\x01" | xxd -p)
methodAck=$(printf "\x01" | xxd -p)
fileCount=$(printf "%016x" $(printf "${#files[@]}"))

expectedHead="${protoAck}${methodAck}${fileCount}"
responseHead="${responseDump::${#expectedHead}}"

if [[ "${responseHead}" != "${expectedHead}" ]]; then
    showStatus fail "Incorrect response header"
    exit 1
fi

mkdir -p copies && cd copies

body="${responseDump:${#expectedHead}}"
for _ in $(seq "${#files[@]}"); do
    nameLength=$((0x"${body::16}"))
    if [ "$nameLength" -gt "1024" ]; then
        showStatus fail "File name too long. Length=${nameLength}."
        exit 1
    fi
    fileName=$(echo "${body:16:$(("$nameLength"*2))}" | xxd -r -p)
    body="${body:$((16+"$nameLength"*2))}"

    fileSize=$((0x"${body::16}"))
    if [ "$nameLength" -gt "1048576" ]; then
        showStatus fail "File is too large. size=${fileSize}."
        exit 1
    fi
    if [[ "$fileName" = */* ]]; then
        mkdir -p "${fileName%/*}";
    fi;
    echo "${body:16:$(("$fileSize"*2))}" | xxd -r -p > "${fileName}"
    body="${body:$((16+"$fileSize"*2))}"
done

if [[ "${body}" != "" ]]; then
    showStatus fail "Incorrect response body"
    exit 1
fi

cd ..

diffOutput=$(diff -rq original copies 2>&1)
if [ ! -z "${diffOutput}" ]; then
    showStatus "fail" "Files do not match."
    exit 1
fi

showStatus pass