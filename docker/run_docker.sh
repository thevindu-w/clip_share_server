#!/bin/bash

set -e

if type Xvfb &>/dev/null; then
    # start the Xvfb virtual display
    Xvfb :0 -screen 0 1280x720x8 >/dev/null 2>&1 &
    sleep .1
fi

# run the tests
make test
