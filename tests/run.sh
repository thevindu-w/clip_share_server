#!/bin/bash

program=$1

dependencies=(
    "../${program}"
    stat
    printf
    realpath
    seq
    diff
    python3
    nc
    timeout
    openssl
)

if [ "$OS" = 'Windows_NT' ]; then
    DETECTED_OS="Windows"
    dependencies+=(powershell)
elif [ "$(uname)" = 'Linux' ]; then
    DETECTED_OS="Linux"
    dependencies+=(xclip)
elif [ "$(uname)" = 'Darwin' ]; then
    DETECTED_OS="MacOS"
else
    DETECTED_OS="unknown"
fi

for dependency in "${dependencies[@]}"; do
    if ! type "$dependency" &> /dev/null; then
        echo '"'"$dependency"'"' not found
        exit 1
    fi
done

shopt -s expand_aliases
cur_dir="$(pwd)"
if type "xxd" &> /dev/null && [ "$DETECTED_OS" = "Linux" ]; then
    alias hex2bin="xxd -p -r 2>/dev/null"
else
    alias hex2bin="python3 -u ${cur_dir}/utils/bin2hex.py -r 2>/dev/null"
fi

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

copy_text () {
    if [ "$DETECTED_OS" = "Linux" ]; then
        echo -n "$1" | xclip -in -sel clip &>/dev/null
    elif [ "$DETECTED_OS" = "Windows" ]; then
        powershell -c "Set-Clipboard -Value '$1'"
    else
        echo "Copy text is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

get_copied_text () {
    if [ "$DETECTED_OS" = "Linux" ]; then
        xclip -out -sel clip
    elif [ "$DETECTED_OS" = "Windows" ]; then
        powershell -c "Get-Clipboard"
    else
        echo "Get copied text is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

copy_files () {
    local files=("$@")
    if [ "$DETECTED_OS" = "Linux" ]; then
        local urls=""
        for f in "${files[@]}"; do
            local absPath="$(realpath "${f}")"
            local fPathUrl="$(python3 -c 'from urllib import parse;print(parse.quote(input()))' <<< "${absPath}")"
            urls+=$'\n'"file://${fPathUrl}"
        done
        echo -n "copy${urls}" | xclip -in -sel clip -t x-special/gnome-copied-files &>/dev/null
    elif [ "$DETECTED_OS" = "Windows" ]; then
        local files_str="$(printf ", '%s'" "${files[@]}")"
        command="Set-Clipboard -Path ${files_str:2}"
        powershell -c "$command"
    else
        echo "Copy files is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

copy_image () {
    if [ "$DETECTED_OS" = "Linux" ]; then
        hex2bin <<<"$1" | xclip -in -sel clip -t image/png
    elif [ "$DETECTED_OS" = "Windows" ]; then
        hex2bin <<<"$1" > image.png
        powershell -ExecutionPolicy Bypass ../utils/copy_image.ps1
    else
        echo "Copy image is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

clear_clipboard () {
    if [ "$DETECTED_OS" = "Linux" ]; then
        xclip -in -sel clip -l 1 <<<"dummy" &> /dev/null
        xclip -out -sel clip &> /dev/null
    elif [ "$DETECTED_OS" = "Windows" ]; then
        powershell -c 'Set-Clipboard -Value $null'
    else
        echo "Clear clipboard is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

export DETECTED_OS
export -f setColor showStatus copy_text get_copied_text copy_files copy_image clear_clipboard

exitCode=0
passCnt=0
failCnt=0
for script in scripts/*.sh; do
    chmod +x "${script}"
    passed=
    attempts=3
    for attempt in $(seq "$attempts"); do
        if timeout 20 "${script}" "$@"; then
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
    "../${program}" -s &>/dev/null
    rm -rf tmp
done

clear_clipboard

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
