#!/bin/bash

set -e

Xvfb :0 -screen 0 1280x720x30 > /dev/null 2>&1 & sleep .1
make test
