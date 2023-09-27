#!/bin/bash

SERVER_NAME='clipshare_server'
CLIENT_NAME='clipshare_client'

set -e

if ! type openssl &>/dev/null; then
    echo 'Error: openssl command not found!'
    echo 'Please make sure that openssl is installed and added to the path environment variable.'
    exit 1
fi

if (type dirname &>/dev/null) && (type realpath &>/dev/null); then
    dname=$(dirname "$0")
    dpath=$(realpath "$dname")
    cd "$dpath"
fi

if [ ! -f clipshare.ext ]; then
    echo 'Error: clipshare.ext file is not found!'
    echo 'Please create or download the clipshare.ext file.'
    exit 1
fi

export MSYS2_ARG_CONV_EXCL='/CN'

echo 'Generating keys and certificates ...'
# generate CA keys
openssl genrsa -out ca.key 4096 >keygen.log 2>&1
openssl req -x509 -new -nodes -key ca.key -sha256 -days 1826 -out ca.crt -reqexts v3_req -extensions v3_ca -subj '/CN=clipshare_ca'

# generate server keys
openssl genrsa -out server.key 2048 >>keygen.log 2>&1
openssl req -new -key server.key -out server.csr -subj "/CN=$SERVER_NAME"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 730 -sha256 -extfile clipshare.ext >>keygen.log 2>&1

# generate client keys
openssl genrsa -out client.key 2048 >>keygen.log 2>&1
openssl req -new -key client.key -out client.csr -subj "/CN=$CLIENT_NAME"
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt -days 730 -sha256 -extfile clipshare.ext >>keygen.log 2>&1

echo 'Generated keys and certificates.'
echo
echo 'Exporting client keys ...'
# export keys to pkcs12 file
echo 'Please enter a password that you can remember. You will need this password when importing the key on the client.'
echo

if type winpty &>/dev/null; then
    winpty openssl pkcs12 -export -in client.crt -inkey client.key -out client.pfx
else
    openssl pkcs12 -export -in client.crt -inkey client.key -out client.pfx
fi

echo
echo 'Exported client keys.'
echo 'Cleaning up ...'
rm -f *.srl *.csr
rm client.key client.crt

echo 'Done.'
echo
echo '> server.key - The TLS private key file of the server. Keep this file securely.'
echo '> server.crt - The TLS certificate file of the server.'
echo '> ca.crt     - The TLS certificate file of the CA. You need this file on both the server and the client devices.'
echo
echo '> client.pfx - The TLS key and certificate store file for the client. Move this to the client device.'
echo
echo "# the server's name is $SERVER_NAME"
echo "# the client's name is $CLIENT_NAME"
echo
echo 'Note: If you do not plan to create more keys in the future, you may safely delete the "ca.key" and "clipshare.ext" files. Otherwise, store the "ca.key" file securely.'
rm -f keygen.log
