#!/bin/bash

files=(
    'file 1.txt'
    'file_2.txt'
    'dir 1/'
    'dir_2/sub/'
    'dir_3/sub/file'
    'dir 4/file 1'
)

. scripts/common/1.3.x_get_files_v1.sh
