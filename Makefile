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

PROGRAM_NAME=clip_share
PROGRAM_NAME_WEB=clip_share_web

INFO_NAME=clip_share

CC=gcc
CFLAGS=-c -pipe -I. -Wall -Wextra -Wdouble-promotion -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wnull-dereference -Winit-self -Wmissing-include-dirs -Wshift-overflow=2 -Wswitch-default -Wstrict-overflow=4 -Wstringop-overflow -Walloc-zero -Wconversion -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wfloat-equal -Wshadow -Wpointer-arith -Wundef -Wexpansion-to-defined -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Wjump-misses-init -Wlogical-op -Waggregate-return -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Wvla-larger-than=65536 -Woverlength-strings --std=gnu11 -fstack-protector -fstack-protector-all -DINFO_NAME=\"$(INFO_NAME)\" -DPROTOCOL_MIN=1 -DPROTOCOL_MAX=2
CFLAGS_DEBUG=-g -DDEBUG_MODE

OBJS=main.o clip_share.o udp_serve.o proto/server.o proto/versions.o proto/methods.o utils/utils.o utils/net_utils.o utils/list_utils.o config.o

_WEB_OBJS_C=clip_share_web.o
_WEB_OBJS_S=page_blob.o

OTHER_DEPENDENCIES=
LINK_FLAGS_BUILD=-no-pie

ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

ifeq ($(detected_OS),Linux)
	OBJS+= xclip/xclip.o xclip/xclib.o xscreenshot/xscreenshot.o
	CFLAGS_OPTIM=-Os
	LDLIBS=-lunistring -lssl -lcrypto -lX11 -lXmu -lpng
	LINK_FLAGS_BUILD+= -Wl,-s,--gc-sections
else ifeq ($(detected_OS),Windows)
	OBJS+= utils/win_image.o win_getopt/getopt.o
	CFLAGS_OPTIM=-O3
	OTHER_DEPENDENCIES+= winres/app.res
	LDLIBS=-l:libunistring.a -l:libssl.a -l:libcrypto.a -l:libpthread.a -lws2_32 -lgdi32 -l:libpng16.a -l:libz.a
	LINK_FLAGS_BUILD+= -mwindows
	CFLAGS+= -D__USE_MINGW_ANSI_STDIO
	PROGRAM_NAME:=$(PROGRAM_NAME).exe
	PROGRAM_NAME_WEB:=$(PROGRAM_NAME_WEB).exe
else
$(error ClipShare is not supported on this platform!)
endif
CFLAGS_OPTIM+= -Werror

# append '_web' to objects for clip_share_web to prevent overwriting objects for clip_share
WEB_OBJS_C=$(OBJS:.o=_web.o) $(_WEB_OBJS_C:.o=_web.o)
WEB_OBJS_S=$(_WEB_OBJS_S:.o=_web.o)
WEB_OBJS=$(WEB_OBJS_C) $(WEB_OBJS_S)

# append '_debug' to objects for clip_share debug executable to prevent overwriting objects for clip_share
DEBUG_OBJS_C=$(OBJS:.o=_debug.o) $(_WEB_OBJS_C:.o=_debug.o)
DEBUG_OBJS_S=$(_WEB_OBJS_S:.o=_debug.o)
DEBUG_OBJS=$(DEBUG_OBJS_C) $(DEBUG_OBJS_S)

$(PROGRAM_NAME): $(OBJS) $(OTHER_DEPENDENCIES)
	$(CC) -Werror $^ $(LINK_FLAGS_BUILD) $(LDLIBS) -o $@

$(PROGRAM_NAME_WEB): $(WEB_OBJS) $(OTHER_DEPENDENCIES)
	$(CC) -Werror $^ $(LINK_FLAGS_BUILD) $(LDLIBS) -o $@

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS_OPTIM) $(CFLAGS) -DNO_WEB -fno-pie $^ -o $@

$(WEB_OBJS_C): %_web.o: %.c
$(WEB_OBJS_S): %_web.o: %.S
$(WEB_OBJS):
	$(CC) $(CFLAGS_OPTIM) $(CFLAGS) -fno-pie $^ -o $@

$(DEBUG_OBJS_C): %_debug.o: %.c
$(DEBUG_OBJS_S): %_debug.o: %.S
$(DEBUG_OBJS):
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) $^ -o $@

winres/app.res: winres/app.rc winres/resource.h
	windres -I. $< -O coff -o $@

.PHONY: all clean debug web test check install

all: $(PROGRAM_NAME) $(PROGRAM_NAME_WEB)

debug: $(DEBUG_OBJS) $(OTHER_DEPENDENCIES)
	$(CC) $^ $(LDLIBS) -o $(PROGRAM_NAME)

web: $(PROGRAM_NAME_WEB)

test: $(PROGRAM_NAME) $(PROGRAM_NAME_WEB)
	@chmod +x tests/run.sh && cd tests && ./run.sh $(PROGRAM_NAME)

check: test

install: $(PROGRAM_NAME)
	@echo
	@echo "No need to install. Just move the $(PROGRAM_NAME) to anywhere and run it."
	@echo

clean:
	$(RM) $(OBJS) $(WEB_OBJS) $(DEBUG_OBJS)
	$(RM) $(PROGRAM_NAME)
	$(RM) $(PROGRAM_NAME_WEB)
