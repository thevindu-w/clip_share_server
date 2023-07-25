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

OBJS_C=main.o clip_share.o udp_serve.o proto/server.o proto/versions.o proto/methods.o utils/utils.o utils/net_utils.o utils/list_utils.o config.o

_WEB_OBJS_C=clip_share_web.o
_WEB_OBJS_S=page_blob.o

CFLAGS=-c -pipe -Wall -Wextra -DINFO_NAME=\"$(INFO_NAME)\" -DPROTOCOL_MIN=1 -DPROTOCOL_MAX=2
CFLAGS_DEBUG=-g -DDEBUG_MODE

OTHER_DEPENDENCIES=
LINK_FLAGS_BUILD=

ifeq ($(detected_OS),Linux)
	OBJS_C+= xclip/xclip.o xclip/xclib.o xscreenshot/xscreenshot.o
	CFLAGS_OPTIM=-Os
	LDLIBS=-lssl -lcrypto -lX11 -lXmu -lpng
endif
ifeq ($(detected_OS),Windows)
	OBJS_C+= utils/win_image.o win_getopt/getopt.o
	CFLAGS_OPTIM=-O3
	OTHER_DEPENDENCIES+= winres/app.res
	LDLIBS=-l:libssl.a -l:libcrypto.a -lws2_32 -lgdi32 -l:libpng16.a -l:libz.a
	LINK_FLAGS_BUILD+= -mwindows
	CFLAGS+= -D__USE_MINGW_ANSI_STDIO
	PROGRAM_NAME:=$(PROGRAM_NAME).exe
	PROGRAM_NAME_WEB:=$(PROGRAM_NAME_WEB).exe
endif

OBJS=$(OBJS_C)

WEB_OBJS_C=$(OBJS_C:.o=_web.o) $(_WEB_OBJS_C:.o=_web.o)
WEB_OBJS_S=$(_WEB_OBJS_S:.o=_web.o)
WEB_OBJS=$(WEB_OBJS_C) $(WEB_OBJS_S)

DEBUG_OBJS_C=$(OBJS_C:.o=_debug.o) $(_WEB_OBJS_C:.o=_debug.o)
DEBUG_OBJS_S=$(_WEB_OBJS_S:.o=_debug.o)
DEBUG_OBJS=$(DEBUG_OBJS_C) $(DEBUG_OBJS_S)

$(PROGRAM_NAME): $(OBJS) $(OTHER_DEPENDENCIES)
	gcc $^ -no-pie $(LINK_FLAGS_BUILD) $(LDLIBS) -o $@

$(PROGRAM_NAME_WEB): $(WEB_OBJS) $(OTHER_DEPENDENCIES)
	gcc $^ -no-pie $(LINK_FLAGS_BUILD) $(LDLIBS) -o $@

$(OBJS_C): %.o: %.c
	gcc $(CFLAGS_OPTIM) $(CFLAGS) -DNO_WEB -fno-pie $^ -o $@

$(WEB_OBJS_C): %_web.o: %.c
$(WEB_OBJS_S): %_web.o: %.S
$(WEB_OBJS):
	gcc $(CFLAGS_OPTIM) $(CFLAGS) -fno-pie $^ -o $@

$(DEBUG_OBJS_C): %_debug.o: %.c
$(DEBUG_OBJS_S): %_debug.o: %.S
$(DEBUG_OBJS):
	gcc $(CFLAGS) $(CFLAGS_DEBUG) $^ -o $@

winres/app.res: winres/app.rc
	windres $^ -O coff -o $@

.PHONY: clean debug web test

debug: $(DEBUG_OBJS) $(OTHER_DEPENDENCIES)
	gcc $^ $(LDLIBS) -o $(PROGRAM_NAME)

web: $(PROGRAM_NAME_WEB)

test: $(PROGRAM_NAME) $(PROGRAM_NAME_WEB)
	@chmod +x tests/run.sh && cd tests && ./run.sh $(PROGRAM_NAME)

clean:
	$(RM) $(OBJS) $(WEB_OBJS) $(DEBUG_OBJS)
	$(RM) $(PROGRAM_NAME)
	$(RM) $(PROGRAM_NAME_WEB)
