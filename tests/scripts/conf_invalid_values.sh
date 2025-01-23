#!/bin/bash

. init.sh

expect_running() {
    if [ -f server_err.log ] || (! nc -zvn 127.0.0.1 4337 &>/dev/null); then
        showStatus info 'Server is not running'
        exit 1
    fi
}

expect_not_running() {
    if nc -zvn 127.0.0.1 4337 &>/dev/null; then
        showStatus info "Invalid $1 config accepted"
        exit 1
    fi
    if [ ! -f server_err.log ] || ! (grep '^Error' server_err.log >/dev/null 2>&1); then
        showStatus info "Error log not created for invalid $1"
        exit 1
    fi
    rm server_err.log
}

check() {
    conf="$1"
    valid="$2"
    invalid="$3"
    update_config "$conf" "$invalid"
    expect_not_running "$conf"
    update_config "$conf" "$valid"
    expect_running
}

expect_running

check app_port 4337 65537
check udp_port 4337 a
check app_port_secure 4338 -1
check insecure_mode_enabled true 2
check secure_mode_enabled TRUE other
check server_cert server.pfx not_existing.pfx
touch empty.txt
check ca_cert testCA.crt empty.txt
check allowed_clients allowed_clients.txt missing.txt
check restart true 01
check max_text_length 4M 5G
check max_file_size 2G 2P
check working_dir ./ server.pfx
check bind_address 127.0.0.1 127.0.0.256
check bind_address_udp 0.0.0.0 127.0.0.a
check client_selects_display false F
check cut_sent_files False T
check min_proto_version 1 1K
check max_proto_version "$PROTO_MAX_VERSION" 10000000000000
check method_get_text_enabled 1 TRUE1
check method_send_text_enabled 0 0False
check method_get_files_enabled FAlSe '0 0'
check method_send_files_enabled trUE tru
check method_get_image_enabled false 00
check method_get_copied_image_enabled 1 -1
check method_get_screenshot_enabled 0 +0
check method_info_enabled FALSE no
