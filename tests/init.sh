set -e

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

"../${program}" -r > /dev/null

printf "dummy" | xclip -in -sel clip &> /dev/null