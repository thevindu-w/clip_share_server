#!/bin/bash

files=(
    '文字檔案 1.txt'
    '另一個文件_2.txt'
    '資料夾_1/文字檔案 3.txt'
    '資料夾_1/另一個文件 4.txt'
    '資料夾 2/文字檔案 5.txt'
    '資料夾 2/子資料夾/文字檔案 6.txt'
    '資料夾 2/子資料夾/另一個文件_7.txt'
    '資料夾 2/子資料夾 2/文字檔案 8.txt'
    '資料夾 2/資料夾 1/'
    '資料夾_3/'
)

proto="$PROTO_V3"

. scripts/common/x.3.x_get_files.sh
