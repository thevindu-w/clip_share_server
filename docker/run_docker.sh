#!/bin/bash

set -e

# start the Xvfb virtual display
Xvfb :0 -screen 0 1280x720x8 >/dev/null 2>&1 &
sleep .1

# run the tests
make test
