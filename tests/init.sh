set -e

showStatus () {
    script=$(basename -- "${0}" | sed 's/\(.*\)\..*/\1/g')
    if [[ "$1" = "pass" ]]; then
        setColor "green"
        echo "Test ${script} passed."
        setColor reset
    elif [[ "$1" = "fail" ]]; then
        setColor "red"
        echo -n "Test ${script} failed."
        setColor reset
        echo " $2"
    fi
}

program=$1

rm -rf tmp
mkdir -p tmp && cd tmp
"../../${program}" -r > /dev/null

printf "dummy" | xclip -in -sel clip &> /dev/null