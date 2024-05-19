set -e

program="$1"

shopt -s expand_aliases

alias showStatus="showStatus $0"

cur_dir="$(pwd)"
if type 'xxd' &>/dev/null && ([ "$DETECTED_OS" = 'Linux' ] || [ "$DETECTED_OS" = 'macOS' ]); then
    alias bin2hex='xxd -p -c 512 2>/dev/null'
    alias hex2bin='xxd -p -r 2>/dev/null'
else
    alias bin2hex="python3 -u ${cur_dir}/utils/bin2hex.py 2>/dev/null"
    alias hex2bin="python3 -u ${cur_dir}/utils/bin2hex.py -r 2>/dev/null"
fi

"$program" -s &>/dev/null
rm -rf tmp
cp -r config tmp
cd tmp
if [ "$DETECTED_OS" = 'Windows' ]; then
    "$program" -r &>/dev/null &
    sleep 0.1
else
    "$program" -r &>/dev/null
fi
sleep 0.1
