#!/bin/bash

ldd_head="$(ldd --version | head -n 1)"
glibc_version=$(echo -n "${ldd_head##* }" | grep -oE '[0-9]+\.[0-9]+' | head -n 1)

libssl_version="$(ldd clip_share | grep -oE 'libssl\.so\.[0-9]+' | head -n 1 | cut -d '.' -f 3)"

mv -n clip_share "dist/clip_share_GLIBC-${glibc_version}_libssl-${libssl_version}" || echo 'Not replacing'
