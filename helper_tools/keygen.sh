#!/bin/bash

SERVER_NAME='clipshare_server'
CLIENT_NAME='clipshare_client'
CLIENT_COMPAT_FLAGS=(
    # Uncomment for Android <=13:
    # -certpbe PBE-SHA1-3DES -keypbe PBE-SHA1-3DES
)

set -e

if ! type openssl &>/dev/null; then
    echo 'Error: openssl command not found!'
    echo 'Please make sure that openssl is installed and added to the PATH environment variable.'
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

create_encrypted_pfx() {
    local name="$1"
    shift
    if type winpty &>/dev/null; then
        winpty openssl pkcs12 -export -in "${name}.crt" -inkey "${name}.key" -out "${name}.pfx" "$@"
    else
        openssl pkcs12 -export -in "${name}.crt" -inkey "${name}.key" -out "${name}.pfx" "$@"
    fi
}

export MSYS2_ARG_CONV_EXCL='/CN'
rm -f keygen.log

reused_ca=0
encrypted_ca=0
if [ -f ca.pfx ] || [ -f ca.enc.pfx ]; then
    echo 'Found previously created CA file.'
    read -rp 'Use it? [Y/n] ' confirm
    if [ "${confirm::1}" != 'n' ] && [ "${confirm::1}" != 'N' ]; then
        echo
        if [ -f ca.enc.pfx ]; then
            echo 'Please enter the password for ca.enc.pfx file.'
            if type winpty &>/dev/null; then
                winpty openssl pkcs12 -in ca.enc.pfx -nodes -out ca.pem
            else
                openssl pkcs12 -in ca.enc.pfx -nodes -out ca.pem
            fi
            encrypted_ca=1
        else
            openssl pkcs12 -in ca.pfx -passin pass: -nodes -out ca.pem
        fi
        openssl x509 -in ca.pem -out ca.crt >>keygen.log 2>&1
        openssl rsa -in ca.pem -out ca.key >>keygen.log 2>&1
        rm ca.pem >>keygen.log 2>&1
        reused_ca=1
        echo 'Extracted CA key and certificate.'
    fi
fi

if [ "$reused_ca" != 1 ]; then
    echo 'Generating CA key and certificate ...'
    openssl genrsa -out ca.key 4096 >>keygen.log 2>&1
    openssl req -x509 -new -nodes -key ca.key -sha256 -days 1826 -out ca.crt -reqexts v3_req -extensions v3_ca -subj '/CN=clipshare_ca' >>keygen.log 2>&1
fi

skip_server=0
if [ -f server.pfx ]; then
    skip_server=1
    echo
    echo 'Found existing server.pfx.'
    read -rp 'Overwrite it to create a new server key and certificate? [y/N] ' confirm
    if [ "${confirm::1}" = 'y' ] || [ "${confirm::1}" = 'Y' ]; then
        skip_server=0
    fi
fi
if [ "$skip_server" = 0 ]; then
    read -rp "Name of the server? ($SERVER_NAME) " server_cn
    if [ "$server_cn" != '' ]; then
        SERVER_NAME="$server_cn"
    fi
    openssl genrsa -out server.key 2048 >>keygen.log 2>&1
    openssl req -new -key server.key -out server.csr -subj "/CN=$SERVER_NAME" >>keygen.log 2>&1
    openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 730 -sha256 -extfile clipshare.ext >>keygen.log 2>&1
    openssl pkcs12 -export -in server.crt -inkey server.key -passout pass: -out server.pfx >>keygen.log 2>&1
    echo 'Generated server keys and certificates.'
fi

skip_client=0
if [ -f client.pfx ]; then
    skip_client=1
    echo
    echo 'Found existing client.pfx.'
    read -rp 'Overwrite it to create a new client key and certificate? [y/N] ' confirm
    if [ "${confirm::1}" = 'y' ] || [ "${confirm::1}" = 'Y' ]; then
        skip_client=0
    fi
fi
if [ "$skip_client" = 0 ]; then
    read -rp "Name of the client? ($CLIENT_NAME) " client_cn
    if [ "$client_cn" != '' ]; then
        CLIENT_NAME="$client_cn"
    fi
    openssl genrsa -out client.key 2048 >>keygen.log 2>&1
    openssl req -new -key client.key -out client.csr -subj "/CN=$CLIENT_NAME" >>keygen.log 2>&1
    openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt -days 730 -sha256 -extfile clipshare.ext >>keygen.log 2>&1
    echo 'Generated client keys and certificates.'
    echo
    echo 'Exporting client keys ...'
    echo 'Please enter a password that you can remember. You will need this password when importing the key on the client.'
    echo
    create_encrypted_pfx client "${CLIENT_COMPAT_FLAGS[@]}"
    echo
    echo 'Exported client keys.'
fi
echo 'Cleaning up ...'
rm -f *.srl *.csr >>keygen.log 2>&1
rm -f client.key client.crt server.key server.crt >>keygen.log 2>&1
echo 'Done.'

if [ "$reused_ca" != 1 ]; then
    echo
    read -rp 'Do you plan to create more keys in the future and like to encrypt the CA key file to secure it? [y/N] ' confirm
    if [ "${confirm::1}" = 'y' ] || [ "${confirm::1}" = 'Y' ]; then
        echo 'Encrypting CA key ...'
        echo
        echo 'Please enter a password that you can remember. You will need this password when using the key to sign more certificates in the future.'
        echo
        create_encrypted_pfx ca
        mv ca.pfx ca.enc.pfx >>keygen.log 2>&1
        encrypted_ca=1
        echo 'Stored the encrypted CA key in "ca.enc.pfx"'
    else
        openssl pkcs12 -export -in ca.crt -inkey ca.key -passout pass: -out ca.pfx >>keygen.log 2>&1
        echo
        echo 'The CA key is stored in "ca.pfx" without encrypting. If you do not plan to create more keys, please delete that file for security.'
    fi
fi
rm ca.key >>keygen.log 2>&1

echo
echo '> server.pfx - The TLS key and certificate store file of the server. Keep this file securely.'
echo '> ca.crt     - The TLS certificate file of the CA. You need this file on both the server and the client devices.'
echo
echo '> client.pfx - The TLS key and certificate store file of the client. Move this to the client device.'
echo
echo "# the server's name is $SERVER_NAME"
echo "# the client's name is $CLIENT_NAME"
if [ "$encrypted_ca" = 0 ]; then
    echo
    echo 'Note: If you do not plan to create more keys in the future, you may safely delete the "ca.pfx" file. Otherwise, store the file securely.'
fi

rm -f keygen.log
