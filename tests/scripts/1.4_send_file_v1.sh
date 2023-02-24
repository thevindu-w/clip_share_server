#!/bin/bash

. init.sh

fileName="file 1.txt"

mkdir -p original && cd original

echo "abc"$'\n'"def"$'\n'"file content" > "${fileName}"

nameLength=$(printf "%016x" $(printf "${fileName}" | wc -c))
fileSize=$(printf "%016x" $(stat -c '%s' "${fileName}"))
content=$(cat "${fileName}" | xxd -p | tr -d '\n')
body="${nameLength}$(printf "${fileName}" | xxd -p)${fileSize}${content}"

cd ..
mkdir -p copy
mv clipshare.conf copy/
cd copy

# restart the server in new directory
"../../../$1" -r &> /dev/null

# remove the conf file
rm -f clipshare.conf

proto=$(printf "\x01" | xxd -p)
method=$(printf "\x04" | xxd -p)

responseDump=$(printf "${proto}${method}${body}" | xxd -r -p | client_tool | xxd -p | tr -d '\n')

protoAck=$(printf "\x01" | xxd -p)
methodAck=$(printf "\x01" | xxd -p)

expected="${protoAck}${methodAck}"

if [[ "${responseDump}" != "${expected}" ]]; then
    showStatus fail "Incorrect response."
    exit 1
fi

cd ..

diffOutput=$(diff -rq original copy 2>&1)
if [ ! -z "${diffOutput}" ]; then
    showStatus fail "File does not match."
    exit 1
fi

showStatus pass