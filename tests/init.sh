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

cur_dir="$(pwd)"
if type "xxd" &>/dev/null && [ "$DETECTED_OS" = "Linux" ]; then
    alias bin2hex="xxd -p -c 512 2>/dev/null"
    alias hex2bin="xxd -p -r 2>/dev/null"
else
    alias bin2hex="python3 -u ${cur_dir}/utils/bin2hex.py 2>/dev/null"
    alias hex2bin="python3 -u ${cur_dir}/utils/bin2hex.py -r 2>/dev/null"
fi

"../${program}" -s &>/dev/null
rm -rf tmp
cp -r config tmp
cd tmp
"../../${program}" -r &>/dev/null &
sleep 0.1

clear_clipboard
