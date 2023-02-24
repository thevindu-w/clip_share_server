set -e

showStatus () {
    script=$(basename -- "${0}" | sed 's/\(.*\)\..*/\1/g')
    if [[ "$1" = "pass" ]]; then
        setColor "green"
        echo "Test ${script} passed."
        setColor reset
    elif [[ "$1" = "fail" ]]; then
        setColor "red"
        echo -n "Test ${script} failed."
        setColor reset
        echo " $2"
    fi
}

program=$1
shopt -s expand_aliases
if [[ "${SECURE}" == "1" ]]; then
    # echo "Setting secure alias"
    alias client_tool='openssl s_client -tls1_3 -quiet -verify_quiet -noservername -connect 127.0.0.1:4338 -CAfile testCA.crt -cert testClient_cert.pem -key testClient_key.pem'
else
    # echo "Setting insecure alias"
    alias client_tool='nc -w 1 127.0.0.1 4337'
fi
# alias client_tool='nc -w 1 127.0.0.1 4337'
# client_tool

rm -rf tmp
cp -r config tmp
cd tmp
"../../${program}" -r > /dev/null

printf "dummy" | xclip -in -sel clip &> /dev/null