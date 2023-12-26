#!/bin/bash

. init.sh

fileName='file 1.txt'
fileContent='clipshare test'
workdir='work dir'

mkdir -p "$workdir"
update_config working_dir "$workdir"

nameLength=$(printf '%016x' "${#fileName}")
fileSize=$(printf '%016x' "${#fileContent}")
content=$(echo -n "$fileContent" | bin2hex | tr -d '\n')
body="${nameLength}$(echo -n "$fileName" | bin2hex)${fileSize}${content}"

proto="$PROTO_V1"
method="$METHOD_SEND_FILES"

responseDump=$(echo -n "${proto}${method}${body}" | hex2bin | client_tool | bin2hex | tr -d '\n')

protoAck="$PROTO_SUPPORTED"
methodAck="$METHOD_OK"

expected="${protoAck}${methodAck}"

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect response.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

rm -f "${workdir}/server_err.log"

if [ ! -f "${workdir}/${fileName}" ]; then
    showStatus info 'File not found in the working directory.'
    exit 1
fi

if [ "$(cat "${workdir}/${fileName}")" != "$fileContent" ]; then
    showStatus info 'Incorrect file content.'
    exit 1
fi

"${program}" -s &>/dev/null
rm -rf "${workdir}"

# Non-ASCII file name and directory
fileName='文件 1.txt'
workdir='文件夹 1'

mkdir -p "$workdir"
update_config working_dir "$workdir"

printf -v _ '%s%n' "$fileName" utf8len
nameLength="$(printf '%016x' $utf8len)"
body="${nameLength}$(echo -n "$fileName" | bin2hex)${fileSize}${content}"

responseDump=$(echo -n "${proto}${method}${body}" | hex2bin | client_tool | bin2hex | tr -d '\n')

if [ "${responseDump}" != "${expected}" ]; then
    showStatus info 'Incorrect response for non-ascii.'
    echo 'Expected:' "$expected"
    echo 'Received:' "$responseDump"
    exit 1
fi

rm -f "${workdir}/server_err.log"

if [ ! -f "${workdir}/${fileName}" ]; then
    showStatus info 'File not found in the working directory for non-ascii.'
    exit 1
fi

if [ "$(cat "${workdir}/${fileName}")" != "$fileContent" ]; then
    showStatus info 'Incorrect file content for non-ascii.'
    exit 1
fi
