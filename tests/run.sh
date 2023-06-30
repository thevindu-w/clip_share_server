#!/bin/bash

program=$1

dependencies=(
    "../${program}"
    stat
    printf
    realpath
    seq
    diff
    xxd
    python3
    nc
    openssl
    xclip
)

for dependency in "${dependencies[@]}"; do
    if ! type "$dependency" &> /dev/null; then
        echo '"'"$dependency"'"' not found
        exit 1
    fi
done

setColor () {
    colorSet () {
        if [ "$1" = "red" ]; then
            if [ "$2" = "bold" ]; then
                printf "\033[1;31m";
            else
                printf "\033[31m";
            fi
        elif [ "$1" = "green" ]; then
            if [ "$2" = "bold" ]; then
                printf "\033[1;32m";
            else
                printf "\033[32m";
            fi
        else
            printf "\033[0m";
        fi
    }
    case "$TERM" in
        xterm-color|*-256color) colorSet "$@";
    esac
}

export -f setColor

exitCode=0
passCnt=0
failCnt=0
for script in scripts/*.sh; do
    chmod +x "${script}"
    passed=
    attempts=3
    for attempt in $(seq "$attempts"); do
        if "${script}" "$@"; then
            passed=1
            break
        fi
        echo -n "Attempt ${attempt} / ${attempts} failed."
        if [ "$attempt" != "$attempts" ]; then
            echo " trying again ..."
        else
            echo
        fi
    done
    if [ "$passed" = "1" ]; then
        passCnt=$(("$passCnt"+1))
    else
        exitCode=1
        failCnt=$(("$failCnt"+1))
    fi
    rm -rf tmp
done

if type xclip &> /dev/null; then
    printf "" | xclip -in -sel clip &> /dev/null
fi

totalTests=$(("$passCnt"+"$failCnt"))
test_s="tests"
if [ "$totalTests" = "1" ]; then
    test_s="test"
fi
if [ "$failCnt" = "0" ]; then
    setColor "green" "bold"
    echo "Passed all ${totalTests} ${test_s}."
elif [ "$failCnt" = "1" ]; then
    setColor "red" "bold"
    echo -n "Failed ${failCnt} test."
    setColor "reset"
    echo " Passed ${passCnt} out of ${totalTests} ${test_s}."
else
    setColor "red" "bold"
    echo -n "Failed ${failCnt} tests."
    setColor "reset"
    echo " Passed ${passCnt} out of ${totalTests} ${test_s}."
fi
setColor "reset"

exit "$exitCode"