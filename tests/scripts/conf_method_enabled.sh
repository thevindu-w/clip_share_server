#!/bin/bash

. init.sh

check_method() {
    payload="$1"
    status="$2"
    responseDump=$(echo -n "${PROTO_MAX_VERSION}${payload}" | hex2bin | client_tool)
    expected="${PROTO_SUPPORTED}${status}"
    if [ "${responseDump::4}" != "$expected" ]; then
        showStatus info 'Incorrect server response.'
        echo 'Expected:' "$expected"
        echo 'Received:' "$responseDump"
        exit 1
    fi
}

GET_TEXT_STATUS="$METHOD_NO_DATA"
GET_FILES_STATUS="$METHOD_NO_DATA"
GET_IMAGE_STATUS="$METHOD_OK"
GET_COPIED_IMAGE_STATUS="$METHOD_NO_DATA"
GET_SCREENSHOT_STATUS="$METHOD_OK"
SEND_TEXT_STATUS="$METHOD_OK"
SEND_FILES_STATUS="$METHOD_OK"
INFO_STATUS="$METHOD_OK"

TEXT="0000000000000000"
FILE_CNT="0000000000000000"

check_all_methods() {
    check_method "$METHOD_GET_TEXT" "$GET_TEXT_STATUS"
    check_method "$METHOD_GET_FILES" "$GET_FILES_STATUS"
    check_method "$METHOD_GET_IMAGE" "$GET_IMAGE_STATUS"
    check_method "$METHOD_GET_COPIED_IMAGE" "$GET_COPIED_IMAGE_STATUS"
    check_method "$METHOD_GET_SCREENSHOT" "$GET_SCREENSHOT_STATUS"
    check_method "${METHOD_SEND_TEXT}${TEXT}" "$SEND_TEXT_STATUS"
    check_method "${METHOD_SEND_FILES}${FILE_CNT}" "$SEND_FILES_STATUS"
    check_method "$METHOD_INFO" "$INFO_STATUS"
}

clear_clipboard

check_all_methods

update_config method_get_image_enabled false

GET_IMAGE_STATUS="$METHOD_NOT_IMPLEMENTED"
check_all_methods

update_config method_get_screenshot_enabled false

GET_SCREENSHOT_STATUS="$METHOD_NOT_IMPLEMENTED"
check_all_methods

update_config method_info_enabled false

INFO_STATUS="$METHOD_NOT_IMPLEMENTED"
check_all_methods

update_config method_get_text_enabled false
clear_clipboard

GET_TEXT_STATUS="$METHOD_NOT_IMPLEMENTED"
check_all_methods

update_config method_get_files_enabled false
clear_clipboard

GET_FILES_STATUS="$METHOD_NOT_IMPLEMENTED"
check_all_methods

update_config method_get_copied_image_enabled false

GET_COPIED_IMAGE_STATUS="$METHOD_NOT_IMPLEMENTED"
check_all_methods

update_config method_send_text_enabled false

TEXT=''
SEND_TEXT_STATUS="$METHOD_NOT_IMPLEMENTED"
check_all_methods

update_config method_send_files_enabled false

FILE_CNT=''
SEND_FILES_STATUS="$METHOD_NOT_IMPLEMENTED"
check_all_methods
