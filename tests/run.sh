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
    timeout
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
    getColorCode () {
        if [ "$1" = "red" ]; then
            echo 31;
        elif [ "$1" = "green" ]; then
            echo 32;
        elif [ "$1" = "yellow" ]; then
            echo 33;
        elif [ "$1" = "blue" ]; then
            echo 34;
        else
            echo 0;
        fi
    }
    colorSet () {
        color_code="$(getColorCode $1)"
        if [ "$2" = "bold" ]; then
            printf "\033[1;${color_code}m";
        else
            printf "\033[${color_code}m";
        fi
    }
    case "$TERM" in
        xterm-color|*-256color) colorSet "$@";
    esac
}

showStatus () {
    name=$(basename -- "$1" | sed 's/\(.*\)\..*/\1/g')
    if [ "$2" = "pass" ]; then
        setColor "green"
        echo "Test ${name} passed"
        setColor reset
    elif [ "$2" = "fail" ]; then
        setColor "red"
        echo -n "Test ${name} failed"
        setColor reset
        echo " $3"
    elif [ "$2" = "warn" ]; then
        setColor "yellow"
        echo -n "Test ${name}"
        setColor reset
        echo " $3"
    elif [ "$2" = "info" ]; then
        setColor "blue"
        echo "$3"
        setColor reset
    else
        setColor reset
    fi
}

export -f setColor showStatus

exitCode=0
passCnt=0
failCnt=0
for script in scripts/*.sh; do
    chmod +x "${script}"
    passed=
    attempts=3
    for attempt in $(seq "$attempts"); do
        if timeout 3 "${script}" "$@"; then
            passed=1
            showStatus "${script}" pass
            break
        fi
        attemt_msg="Attempt ${attempt} / ${attempts} failed."
        if [ "$attempt" != "$attempts" ]; then
            attemt_msg="${attemt_msg} trying again ..."
        fi
        showStatus "${script}" warn "$attemt_msg"
    done
    if [ "$passed" = "1" ]; then
        passCnt=$(("$passCnt"+1))
    else
        exitCode=1
        failCnt=$(("$failCnt"+1))
        showStatus "${script}" fail
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
