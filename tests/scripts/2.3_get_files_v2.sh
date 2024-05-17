#!/bin/bash

files=(
    'file 1.txt'
    'file_2.txt'
    'sub/file 3.txt'
    'sub/file 4.txt'
    'sub 1/file 5.txt'
    'sub 1/subsub/file 6.txt'
    'sub 1/subsub/file_7.txt'
    'sub 1/subsub_2/file_8.txt'
)

proto="$PROTO_V2"

. scripts/common/x.3.x_get_files.sh
