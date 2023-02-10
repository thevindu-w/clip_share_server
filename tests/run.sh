#!/bin/bash

set -e

for script in scripts/*.sh; do
    chmod +x "${script}"
    "${script}" "$@"
done