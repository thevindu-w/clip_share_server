#!/bin/bash

. init.sh

update_config bind_address '10.20.30.40' # this address may or may not exist

if test_port 4337; then
    showStatus info 'Still listening on 127.0.0.1'
    exit 1
fi

update_config bind_address '127.0.0.1'

if ! test_port 4337; then
    showStatus info 'Not listening on 127.0.0.1'
    exit 1
fi

update_config bind_address '0.0.0.0'

if ! test_port 4337; then
    showStatus info 'Not listening on any address'
    exit 1
fi
