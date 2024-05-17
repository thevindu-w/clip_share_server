#!/bin/bash

files=(
    'file 1.txt'
    'file_2.txt'
    'empty/'
    'sub/file 3.txt'
    'sub 1/empty dir/'
    'sub 1/file 4.txt'
    'sub 1/subsub/empty/'
    'sub 1/subsub/file 5.txt'
    'sub_2/subsub/empty/'
)

proto="$PROTO_V3"

. scripts/common/x.3.x_get_files.sh
