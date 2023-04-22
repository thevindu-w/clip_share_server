# Makefile - makefile
# Copyright (C) 2022-2023 H. Thevindu J. Wijesekera

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
PROGRAM_NAME_WEB=clip_share_web

ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

OBJS=main.o clip_share.o udp_serve.o proto/server.o proto/versions.o proto/methods.o utils/utils.o utils/net_utils.o utils/list_utils.o conf_parse.o

WEB_OBJS_C=clip_share_web.o
WEB_OBJS_S=page_blob.o
WEB_OBJS=$(WEB_OBJS_C) $(WEB_OBJS_S)

SRC_FILES=main.c clip_share.c udp_serve.c proto/server.c proto/versions.c proto/methods.c utils/utils.c utils/net_utils.c utils/list_utils.c conf_parse.c
WEB_SRC=clip_share_web.c page_blob.S

CFLAGS=-pipe -Wall -Wextra -DINFO_NAME=\"$(INFO_NAME)\" -DPROTOCOL_MIN=1 -DPROTOCOL_MAX=2
CFLAGS_DEBUG=-g -c -DDEBUG_MODE

ifeq ($(detected_OS),Linux)
	OBJS+= xclip/xclip.o xclip/xclib.o xscreenshot/xscreenshot.o
	SRC_FILES+= xclip/xclip.c xclip/xclib.c xscreenshot/xscreenshot.c
	LDLIBS=-lssl -lcrypto -lX11 -lXmu -lpng
endif
ifeq ($(detected_OS),Windows)
	OBJS+= utils/win_image.o win_getopt/getopt.o
	SRC_FILES+= utils/win_image.c win_getopt/getopt.c
	LDLIBS=-l:libssl.a -l:libcrypto.a -lws2_32 -lgdi32 -l:libpng16.a -l:libz.a
	CFLAGS+= -D__USE_MINGW_ANSI_STDIO
	PROGRAM_NAME:=$(PROGRAM_NAME).exe
	PROGRAM_NAME_WEB:=$(PROGRAM_NAME_WEB).exe
endif

ifeq ($(detected_OS),Linux)

$(PROGRAM_NAME): $(SRC_FILES)
	gcc -Os $(CFLAGS) -DNO_WEB -fno-pie $^ -no-pie $(LDLIBS) -o $@

endif

ifeq ($(detected_OS),Windows)

$(PROGRAM_NAME): $(SRC_FILES) winres/app.res
	gcc -O3 $(CFLAGS) -DNO_WEB -fno-pie $^ -no-pie -mwindows $(LDLIBS) -o $@

endif

$(OBJS) $(WEB_OBJS_C): %.o: %.c
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $< -o $@

$(WEB_OBJS_S): %.o: %.S
	gcc $(CFLAGS_DEBUG) $(CFLAGS) $< -o $@

ifeq ($(detected_OS),Windows)

winres/app.res: winres/app.rc
	windres $^ -O coff -o $@

endif

.PHONY: clean debug web test

debug: $(OBJS) $(WEB_OBJS)
	gcc -g $(CFLAGS) $^ $(LDLIBS) -o $(PROGRAM_NAME)

ifeq ($(detected_OS),Linux)

web: $(SRC_FILES) $(WEB_SRC)
	gcc -Os $(CFLAGS) -fno-pie $^ -no-pie $(LDLIBS) -o $(PROGRAM_NAME_WEB)

test: $(PROGRAM_NAME)
	@chmod +x tests/run.sh && cd tests && ./run.sh $(PROGRAM_NAME)

endif
ifeq ($(detected_OS),Windows)

web: $(SRC_FILES) $(WEB_SRC) winres/app.res
	gcc -O3 $(CFLAGS) -fno-pie $^ -no-pie -mwindows $(LDLIBS) -o $(PROGRAM_NAME_WEB)

endif

clean:
	$(RM) $(OBJS) $(WEB_OBJS)
	$(RM) $(PROGRAM_NAME)
	$(RM) $(PROGRAM_NAME_WEB)
