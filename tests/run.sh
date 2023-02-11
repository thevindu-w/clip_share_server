#!/bin/bash

program=$1

if ! type nc &> /dev/null; then
    echo \"nc\" not found
    exit 1
fi

if ! type "../${program}" &> /dev/null; then
    echo "\"../${program}\" not found"
    exit 1
fi

if ! type xclip &> /dev/null; then
    echo \"xclip\" not found
    exit 1
fi

if ! type python &> /dev/null; then
    echo \"python\" not found
    exit 1
fi

if ! type xxd &> /dev/null; then
    echo \"xxd\" not found
    exit 1
fi

if ! type diff &> /dev/null; then
    echo \"diff\" not found
    exit 1
fi

if ! type seq &> /dev/null; then
    echo \"diff\" not found
    exit 1
fi

if ! type realpath &> /dev/null; then
    echo \"realpath\" not found
    exit 1
fi

if ! type stat &> /dev/null; then
    echo \"stat\" not found
    exit 1
fi

setColor () {
    colorSet () {
        if [[ "$1" = "red" ]]; then
            if [[ "$2" = "bold" ]]; then
                printf "\033[1;31m";
            else
                printf "\033[31m";
            fi
        elif [[ "$1" = "green" ]]; then
            if [[ "$2" = "bold" ]]; then
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
    "${script}" "$@"
    if [[ "$?" == "0" ]]; then
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
if [[ "$totalTests" == "1" ]]; then
    test_s="test"
fi
if [[ "$failCnt" == "0" ]]; then
    setColor "green" "bold"
    echo "Passed all ${totalTests} ${test_s}."
elif [[ "$failCnt" == "1" ]]; then
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