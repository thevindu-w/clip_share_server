import sys

if '-r' in sys.argv[1:]:
    tmp = ''
    while (s := sys.stdin.read(1)):
        if s == '\n':
            continue
        if tmp:
            val=int(tmp + s, 16)
            bval=val.to_bytes(1, 'big')
            assert isinstance(bval, bytes)
            assert len(bval)==1
            sys.stdout.buffer.write(bval)
            tmp = ''
        else:
            tmp = s
else:
    while (b := sys.stdin.buffer.read(1)):
        sys.stdout.buffer.write(b'%02x' % b[0])
    sys.stdout.buffer.write(b'\n')
