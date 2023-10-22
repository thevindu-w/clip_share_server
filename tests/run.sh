#!/bin/bash

program=$1

dependencies=(
    "../${program}"
    stat
    pwd
    printf
    realpath
    seq
    diff
    python3
    nc
    timeout
    openssl
    sed
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

# Check if all dependencies are available
for dependency in "${dependencies[@]}"; do
    if ! type "$dependency" &>/dev/null; then
        echo '"'"$dependency"'"' not found
        exit 1
    fi
done

# Get the absolute path of clip_share executable
program="$(realpath "../${program}")"

shopt -s expand_aliases
TEST_ROOT="$(pwd)"
if type "xxd" &>/dev/null && [ "$DETECTED_OS" = "Linux" ]; then
    alias bin2hex="xxd -p -c 512 2>/dev/null"
    alias hex2bin="xxd -p -r 2>/dev/null"
else
    alias bin2hex="python3 -u ${TEST_ROOT}/utils/bin2hex.py 2>/dev/null"
    alias hex2bin="python3 -u ${TEST_ROOT}/utils/bin2hex.py -r 2>/dev/null"
fi

# Set the color for console output
setColor() {
    # Get the color code from color name
    getColorCode() {
        if [ "$1" = "red" ]; then
            echo 31
        elif [ "$1" = "green" ]; then
            echo 32
        elif [ "$1" = "yellow" ]; then
            echo 33
        elif [ "$1" = "blue" ]; then
            echo 34
        else
            echo 0
        fi
    }

    # Set the color for console output
    colorSet() {
        color_code="$(getColorCode $1)"
        if [ "$2" = "bold" ]; then
            printf "\033[1;${color_code}m"
        else
            printf "\033[${color_code}m"
        fi
    }
    case "$TERM" in
        xterm-color | *-256color) colorSet "$@" ;;
    esac
}

# Show status message. Usage showStatus <script-name> <status:pass|fail|warn|info> [message]
showStatus() {
    name=$(basename -- "$1" | sed 's/\(.*\)\..*/\1/g')
    if [ "$2" = "pass" ]; then
        setColor "green"
        echo -n 'PASS: '
        setColor reset
        echo "$name"
    elif [ "$2" = "fail" ]; then
        setColor "red"
        echo -n 'FAIL: '
        setColor reset
        printf '%-27s %s\n' "$name" "$3"
    elif [ "$2" = "warn" ]; then
        setColor "yellow"
        echo -n 'WARN: '
        setColor reset
        printf '%-27s %s\n' "$name" "$3"
    elif [ "$2" = "info" ]; then
        setColor "blue"
        echo -n 'INFO: '
        setColor reset
        printf '%-27s %s\n' "$name" "$3"
    else
        setColor reset
    fi
}

# Copy a string to clipboard
copy_text() {
    if [ "$DETECTED_OS" = "Linux" ]; then
        echo -n "$1" | xclip -in -sel clip &>/dev/null
    elif [ "$DETECTED_OS" = "Windows" ]; then
        powershell -c "Set-Clipboard -Value '$1'"
    else
        echo "Copy text is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

# Get copied text from clipboard
get_copied_text() {
    if [ "$DETECTED_OS" = "Linux" ]; then
        xclip -out -sel clip | bin2hex
    elif [ "$DETECTED_OS" = "Windows" ]; then
        prev_dir="$(pwd)"
        cd /tmp
        powershell -c 'Get-Clipboard -Raw | Out-File "clip.txt" -Encoding utf8'
        local copied_text="$(cat 'clip.txt')"
        copied_text="$(echo -n "$copied_text" | bin2hex)"
        cd "$prev_dir"
        # remove BOM if present
        if [ "${copied_text::6}" = 'efbbbf' ]; then
            copied_text="${copied_text:6}"
        fi
        echo -n "$copied_text"
    else
        echo "Get copied text is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

# Copy a list of files, given by file names, to clipboard
copy_files() {
    local files=("$@")
    if [ "$DETECTED_OS" = "Linux" ]; then
        local urls=""
        for f in "${files[@]}"; do
            local absPath="$(realpath "${f}")"
            local fPathUrl="$(python3 -c 'from urllib import parse;print(parse.quote(input()))' <<<"${absPath}")"
            urls+=$'\n'"file://${fPathUrl}"
        done
        echo -n "copy${urls}" | xclip -in -sel clip -t x-special/gnome-copied-files &>/dev/null
    elif [ "$DETECTED_OS" = "Windows" ]; then
        local files_str="$(printf ", '%s'" "${files[@]}")"
        powershell -c "Set-Clipboard -Path ${files_str:2}"
    else
        echo "Copy files is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

# Copy an image to clipboard
copy_image() {
    if [ "$DETECTED_OS" = "Linux" ]; then
        hex2bin <<<"$1" | xclip -in -sel clip -t image/png
    elif [ "$DETECTED_OS" = "Windows" ]; then
        hex2bin <<<"$1" >image.png
        powershell -ExecutionPolicy Bypass ../utils/copy_image.ps1
    else
        echo "Copy image is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

# Clear the content of clipboard
clear_clipboard() {
    if [ "$DETECTED_OS" = "Linux" ]; then
        xclip -in -sel clip -l 1 <<<"dummy" &>/dev/null
        xclip -out -sel clip &>/dev/null
    elif [ "$DETECTED_OS" = "Windows" ]; then
        powershell -c 'Set-Clipboard -Value $null'
    else
        echo "Clear clipboard is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

# Update the test clipshare.conf file and restart the program. Usage: update_config <key> <value>
update_config() {
    key="$1"
    value="$2"
    [[ $key =~ ^[A-Za-z0-9_]+$ ]]
    sed -i -E '/^(\s|#)*'"${key}"'\s*=/c\'"${key}=${value}"'\' clipshare.conf
    "$program" -r &>/dev/null &
    sleep 0.1
}

export DETECTED_OS
export -f setColor showStatus copy_text get_copied_text copy_files copy_image clear_clipboard update_config

exitCode=0
passCnt=0
failCnt=0
for script in scripts/*.sh; do
    chmod +x "${script}"
    passed=
    attempts=3 # number of retries before failure
    for attempt in $(seq "$attempts"); do
        if timeout 60 "${script}" "$program"; then
            passed=1
            showStatus "${script}" pass
            break
        fi
        scriptExitCode="${PIPESTATUS[0]}"
        if [ "$scriptExitCode" == '124' ]; then
            showStatus "${script}" info 'Test timeout'
        fi
        attemt_msg="Attempt ${attempt} / ${attempts} failed."
        if [ "$attempt" != "$attempts" ]; then
            attemt_msg="${attemt_msg} trying again ..."
        fi
        showStatus "${script}" warn "$attemt_msg"
    done
    if [ "$passed" = "1" ]; then
        passCnt=$(("$passCnt" + 1))
    else
        exitCode=1
        failCnt=$(("$failCnt" + 1))
        showStatus "${script}" fail
    fi
    "${program}" -s &>/dev/null
    rm -rf tmp
done

clear_clipboard

totalTests=$(("$passCnt" + "$failCnt"))
test_s="tests"
if [ "$totalTests" = "1" ]; then
    test_s="test"
fi
if [ "$failCnt" = "0" ]; then
    setColor "green" "bold"
    echo '======================================================'
    echo "Passed all ${totalTests} ${test_s}."
    echo '======================================================'
elif [ "$failCnt" = "1" ]; then
    setColor "red" "bold"
    echo '======================================================'
    echo -n "Failed ${failCnt} test."
    setColor "reset"
    echo " Passed ${passCnt} out of ${totalTests} ${test_s}."
    setColor "red" "bold"
    echo '======================================================'
else
    setColor "red" "bold"
    echo '======================================================'
    echo -n "Failed ${failCnt} tests."
    setColor "reset"
    echo " Passed ${passCnt} out of ${totalTests} ${test_s}."
    setColor "red" "bold"
    echo '======================================================'
fi
setColor "reset"

exit "$exitCode"
