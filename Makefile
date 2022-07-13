MAKEFLAGS += -j4

INFO_NAME=clip_share

PROGRAM_NAME=clip_share
PROGRAM_NAME_NO_WEB=clip_share_no_web

OBJS=main.o clip_share.o xclip/xclip.o xclip/xclib.o xscreenshot/xscreenshot.o
WEB_OBJS=clip_share_web.o page_blob.o cert_blob.o key_blob.o
SRC_FILES=main.c clip_share.c xclip/xclip.c xclip/xclib.c xscreenshot/xscreenshot.c
WEB_SRC=clip_share_web.c page_blob.S cert_blob.S key_blob.S
CFLAGS=-pipe -Wall -Wextra -DINFO_NAME=\"$(INFO_NAME)\" -DPROTOCOL_MIN=1 -DPROTOCOL_MAX=1
CFLAGS_DEBUG=-g -c -DDEBUG_MODE -DPROGRAM_NAME=\"$(PROGRAM_NAME)\"
LDLIBS=-lssl -lcrypto -lX11 -lXmu -lpng

$(PROGRAM_NAME): $(OBJS) $(WEB_OBJS)
	gcc -g $^ -o $@ $(LDLIBS)

main.o: main.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) -DNO_WEB $^ -o $@

clip_share.o: clip_share.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

clip_share_web.o: clip_share_web.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) -DNO_WEB $^ -o $@

xclip/xclip.o: xclip/xclip.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

xclip/xclib.o: xclip/xclib.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

xscreenshot/xscreenshot.o: xscreenshot/xscreenshot.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

page_blob.o: page_blob.S
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

cert_blob.o: cert_blob.S
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

key_blob.o: key_blob.S
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $^ -o $@

.PHONY: clean release no_web

no_web: $(SRC_FILES)
	gcc -Os $(CFLAGS) -DPROGRAM_NAME=\"$(PROGRAM_NAME_NO_WEB)\" -DNO_WEB -fno-pie $^ $(LDLIBS) -no-pie -o $(PROGRAM_NAME_NO_WEB)

release: $(SRC_FILES) $(WEB_SRC)
	gcc -Os $(CFLAGS) -DPROGRAM_NAME=\"$(PROGRAM_NAME)\" -fno-pie $^ $(LDLIBS) -no-pie -o $(PROGRAM_NAME)

clean:
	rm -f $(OBJS) $(WEB_OBJS)
	rm -f $(PROGRAM_NAME)
	rm -f $(PROGRAM_NAME_NO_WEB)
	rm -f $(PROGRAM_NAME)_static