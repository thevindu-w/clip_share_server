#!/bin/bash

proto="$PROTO_V4"
method="$METHOD_GET_SCREENSHOT"
disp_num=1
disp="$(printf '%016x' $disp_num)"
DISPLAY_ACK="$METHOD_OK"
ack_v4="$ACK_V4"

copy_image "$imgSample"

. scripts/common/get_screenshot.sh
