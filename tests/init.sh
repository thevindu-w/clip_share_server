set -e

program=$1
proto_max_version=2

shopt -s expand_aliases

alias showStatus="showStatus $0"
if [ "${SECURE}" = "1" ]; then
    alias client_tool='openssl s_client -tls1_3 -quiet -verify_quiet -noservername -connect 127.0.0.1:4338 -CAfile testCA.crt -cert testClient_cert.pem -key testClient_key.pem'
else
    alias client_tool='nc -w 1 127.0.0.1 4337'
fi

rm -rf tmp
cp -r config tmp
cd tmp
"../../${program}" -r > /dev/null

printf "dummy" | xclip -in -sel clip -l 1 &> /dev/null
xclip -out -sel clip &> /dev/null
