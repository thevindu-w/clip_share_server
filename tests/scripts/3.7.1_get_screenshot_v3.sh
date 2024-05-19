#!/bin/bash

proto="$PROTO_V3"
method="$METHOD_GET_SCREENSHOT"
disp_num=30000 # not existing display
disp="$(printf '%016x' $disp_num)"
DISPLAY_ACK="$METHOD_NO_DATA"

copy_image "$imgSample"

. scripts/common/get_screenshot.sh
