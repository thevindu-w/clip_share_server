#!/bin/bash

proto="$PROTO_V3"
method="$METHOD_GET_SCREENSHOT"
disp_num=1
disp="$(printf '%016x' $disp_num)"

copy_image "$imgSample"

. scripts/common/get_screenshot.sh
