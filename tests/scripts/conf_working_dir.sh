#!/bin/bash

. init.sh

fileName='file.txt'
fileContent='clipshare test'
workdir='work dir'

mkdir -p "$workdir"
update_config working_dir "$workdir"

nameLength=$(printf "%016x" "${#fileName}")
fileSize=$(printf "%016x" "${#fileContent}")
content=$(echo -n "$fileContent" | bin2hex | tr -d '\n')
body="${nameLength}$(echo -n "${fileName}" | bin2hex)${fileSize}${content}"

proto=$(printf "\x01" | bin2hex)
method=$(printf "\x04" | bin2hex)

responseDump=$(echo -n "${proto}${method}${body}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck=$(printf "\x01" | bin2hex)
methodAck=$(printf "\x01" | bin2hex)

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info "Incorrect response."
    exit 1
fi

rm -f "${workdir}/server_err.log"

if [ ! -f "${workdir}/${fileName}" ]; then
    showStatus info "File not found in the working directory."
    exit 1
fi

if [ "$(cat "${workdir}/${fileName}")" != "$fileContent" ]; then
    showStatus info "Incorrect file content."
    exit 1
fi
