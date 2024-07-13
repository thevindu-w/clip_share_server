#!/bin/bash

files=(
    'file 1.txt'
    'file_2.txt'
    'empty/'
    'sub/file 3.txt'
    'sub/file 4.txt'
    'sub 1/file 5.txt'
    'sub 1/empty 1/'
    'sub 1/subsub/file 6.txt'
    'sub 1/subsub/file_7.txt'
    'sub 1/subsub_2/empty 2/'
)

proto="$PROTO_V3"

. scripts/common/x.4.x_send_files.sh
