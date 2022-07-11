MAKEFLAGS += -j4

INFO_NAME=clip_server

PROGRAM_NAME=clip_server
PROGRAM_NAME_NO_WEB=clip_server_no_web

OBJS=main.o clip_server.o xclip_src/xclip.o xclip_src/xclib.o screenshot/screenshot.o
WEB_OBJS=clip_server_web.o page_blob.o cert_blob.o key_blob.o
SRC_FILES=main.c clip_server.c xclip_src/xclip.c xclip_src/xclib.c screenshot/screenshot.c
WEB_SRC=clip_server_web.c page_blob.S cert_blob.S key_blob.S
CFLAGS=-pipe -Wall -Wextra -DINFO_NAME=\"$(INFO_NAME)\" -DPROTOCOL_MIN=1 -DPROTOCOL_MAX=1
CFLAGS_DEBUG=-g -c -DDEBUG_MODE -DPROGRAM_NAME=\"$(PROGRAM_NAME)\"
LDLIBS=-lssl -lcrypto -lX11 -lXmu -lpng

$(PROGRAM_NAME): $(OBJS) $(WEB_OBJS)
	gcc -g $^ -o $@ $(LDLIBS)

main.o: main.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) -DNO_WEB $^ -o $@

clip_server.o: clip_server.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

clip_server_web.o: clip_server_web.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) -DNO_WEB $^ -o $@

xclip_src/xclip.o: xclip_src/xclip.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

xclip_src/xclib.o: xclip_src/xclib.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

screenshot/screenshot.o: screenshot/screenshot.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

page_blob.o: page_blob.S
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

cert_blob.o: cert_blob.S
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

key_blob.o: key_blob.S
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

.PHONY: clean release no_web static dist

# dist: $(SRC_FILES) $(WEB_SRC)
# 	gcc -Os -D PROGRAM_NAME=\"$(PROGRAM_NAME)\" -fno-pie $^ $(LDLIBS) -no-pie -o $(PROGRAM_NAME)
# 	mkdir -p dist/bin
# 	cp $(PROGRAM_NAME) dist/bin/
# 	tar -cvf $(PROGRAM_NAME).tar --transform 's/dist/$(PROGRAM_NAME)/' dist/*
# 	xz -z9v -T 4 $(PROGRAM_NAME).tar

static: $(SRC_FILES) $(WEB_SRC)
	gcc -Os $(CFLAGS) -DPROGRAM_NAME=\"$(PROGRAM_NAME)_static\" -fno-pie $^ -l:libssl.a -l:libcrypto.a -l:libpng.a -l:libz.a -l:libpthread.a -static-libgcc -lm -ldl -lX11 -lXmu -no-pie -o $(PROGRAM_NAME)_static

no_web: $(SRC_FILES)
	gcc -Os $(CFLAGS) -DPROGRAM_NAME=\"$(PROGRAM_NAME_NO_WEB)\" -DNO_WEB -fno-pie $^ $(LDLIBS) -no-pie -o $(PROGRAM_NAME_NO_WEB)

release: $(SRC_FILES) $(WEB_SRC)
	gcc -Os $(CFLAGS) -DPROGRAM_NAME=\"$(PROGRAM_NAME)\" -fno-pie $^ $(LDLIBS) -no-pie -o $(PROGRAM_NAME)

clean:
	rm -f $(OBJS) $(WEB_OBJS)
	rm -f $(PROGRAM_NAME)
	rm -f $(PROGRAM_NAME_NO_WEB)
	rm -f $(PROGRAM_NAME)_static