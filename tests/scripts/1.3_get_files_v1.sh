#!/bin/bash

. init.sh

files=(
    "file 1.txt"
    "file_2.txt"
)

mkdir -p original && cd original

for f in "${files[@]}"; do
    echo "${f}"$'\n'"abc" > "${f}"
done

chunks=""
for fname in *; do
    nameLength=$(printf "%016x" "${#fname}")
    fileSize=$(printf "%016x" $(stat -c '%s' "${fname}"))
    content=$(cat "${fname}" | xxd -p | tr -d '\n')
    chunks+="${nameLength}$(printf "${fname}" | xxd -p)${fileSize}${content}"
done

cd ..

urls=""
for f in original/*; do
    absPath=$(realpath "${f}")
    fPathUrl=$(python3 -c 'from urllib import parse;print(parse.quote(input()))' <<< "${absPath}")
    urls+=$'\n'"file://${fPathUrl}"
done

echo -n "copy${urls}" | xclip -in -sel clip -t x-special/gnome-copied-files

proto=$(printf "\x01" | xxd -p)
method=$(printf "\x03" | xxd -p)

responseDump=$(printf "${proto}${method}" | xxd -r -p | client_tool | xxd -p | tr -d '\n')

protoAck=$(printf "\x01" | xxd -p)
methodAck=$(printf "\x01" | xxd -p)
fileCount=$(printf "%016x" $(printf "${#files[@]}"))

expected="${protoAck}${methodAck}${fileCount}${chunks}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect server response."
    exit 1
fi
