# Makefile - makefile
# Copyright (C) 2022 H. Thevindu J. Wijesekera

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.


MAKEFLAGS += -j4

INFO_NAME=clip_share

PROGRAM_NAME=clip_share
PROGRAM_NAME_NO_WEB=clip_share_no_web

ifeq ($(OS),Windows_NT) 
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

OBJS=main.o clip_share.o udp_serve.o proto/server.o proto/v1.o utils/utils.o utils/net_utils.o utils/list_utils.o xclip/xclip.o xclip/xclib.o xscreenshot/xscreenshot.o
WEB_OBJS=clip_share_web.o page_blob.o cert_blob.o key_blob.o
SRC_FILES=main.c clip_share.c udp_serve.c proto/server.c proto/v1.c utils/utils.c utils/net_utils.c utils/list_utils.c xclip/xclip.c xclip/xclib.c xscreenshot/xscreenshot.c
WEB_SRC=clip_share_web.c page_blob.S cert_blob.S key_blob.S
CFLAGS=-pipe -Wall -Wextra -DINFO_NAME=\"$(INFO_NAME)\" -DPROTOCOL_MIN=1 -DPROTOCOL_MAX=1 -DPROTO_V1
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

udp_serve.o: udp_serve.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) -DNO_WEB $^ -o $@

proto/server.o: proto/server.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) -DNO_WEB $^ -o $@

proto/v1.o: proto/v1.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) -DNO_WEB $^ -o $@

utils/utils.o: utils/utils.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) -DNO_WEB $^ -o $@

utils/net_utils.o: utils/net_utils.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) -DNO_WEB $^ -o $@

utils/list_utils.o: utils/list_utils.c
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