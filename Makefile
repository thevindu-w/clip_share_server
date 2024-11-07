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
PROGRAM_NAME_WEB:=$(PROGRAM_NAME)_web
PROGRAM_NAME_NO_SSL:=$(PROGRAM_NAME)_no_ssl

MIN_PROTO=1
MAX_PROTO=3
INFO_NAME=clip_share

CC=gcc
CFLAGS=-c -pipe -I. --std=gnu11 -fstack-protector -fstack-protector-all -Wall -Wextra -Wdouble-promotion -Wformat=2 -Wformat-nonliteral -Wformat-security -Wnull-dereference -Winit-self -Wmissing-include-dirs -Wswitch-default -Wstrict-overflow=4 -Wconversion -Wfloat-equal -Wshadow -Wpointer-arith -Wundef -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Woverlength-strings
CFLAGS_DEBUG=-g -DDEBUG_MODE

OBJS=main.o servers/clip_share.o servers/udp_serve.o proto/server.o proto/versions.o proto/methods.o utils/utils.o utils/net_utils.o utils/list_utils.o utils/config.o utils/kill_others.o

_WEB_OBJS_C=servers/clip_share_web.o
_WEB_OBJS_S=servers/page_blob.o
OBJS_M=

OTHER_DEPENDENCIES=
LINK_FLAGS_BUILD=

ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

ifeq ($(detected_OS),Linux)
	OBJS+= xclip/xclip.o xclip/xclib.o xscreenshot/xscreenshot.o
	CFLAGS+= -ftree-vrp -Wformat-signedness -Wshift-overflow=2 -Wstringop-overflow=4 -Walloc-zero -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wjump-misses-init -Wlogical-op -Wvla-larger-than=65536
	CFLAGS_OPTIM=-Os
	LDLIBS_NO_SSL=-lunistring -lX11 -lXmu -lXt -lxcb -lxcb-randr -lpng
	LDLIBS_SSL=-lssl -lcrypto
	LINK_FLAGS_BUILD=-no-pie -Wl,-s,--gc-sections
else ifeq ($(detected_OS),Windows)
	OBJS+= utils/win_image.o
	CFLAGS+= -ftree-vrp -Wformat-signedness -Wshift-overflow=2 -Wstringop-overflow=4 -Walloc-zero -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wjump-misses-init -Wlogical-op -Wvla-larger-than=65536
	CFLAGS+= -D__USE_MINGW_ANSI_STDIO
	CFLAGS_OPTIM=-O3
	OTHER_DEPENDENCIES+= winres/app.res
	LDLIBS_NO_SSL=-l:libunistring.a -l:libpthread.a -lws2_32 -lgdi32 -l:libpng16.a -l:libz.a -lcrypt32 -lShcore -lUserenv
	LDLIBS_SSL=-l:libssl.a -l:libcrypto.a -l:libpthread.a
	LINK_FLAGS_BUILD=-no-pie -mwindows
	PROGRAM_NAME:=$(PROGRAM_NAME).exe
	PROGRAM_NAME_WEB:=$(PROGRAM_NAME_WEB).exe
	PROGRAM_NAME_NO_SSL:=$(PROGRAM_NAME_NO_SSL).exe
else ifeq ($(detected_OS),Darwin)
export CPATH=$(shell brew --prefix)/include
export LIBRARY_PATH=$(shell brew --prefix)/lib
	OBJS_M=utils/mac_utils.o utils/mac_menu.o
	CFLAGS+= -fobjc-arc
	CFLAGS_OPTIM=-O3
	LDLIBS_NO_SSL=-framework AppKit -lunistring -lpng -lobjc
	LDLIBS_SSL=-lssl -lcrypto
else
$(error ClipShare is not supported on this platform!)
endif
LDLIBS=$(LDLIBS_SSL) $(LDLIBS_NO_SSL)
CFLAGS+= -DINFO_NAME=\"$(INFO_NAME)\" -DPROTOCOL_MIN=$(MIN_PROTO) -DPROTOCOL_MAX=$(MAX_PROTO)
CFLAGS_OPTIM+= -Werror

# append '_web' to objects for clip_share_web to prevent overwriting objects for clip_share
WEB_OBJS_C=$(OBJS:.o=_web.o) $(_WEB_OBJS_C:.o=_web.o)
WEB_OBJS_S=$(_WEB_OBJS_S:.o=_web.o)
WEB_OBJS_M=$(OBJS_M:.o=_web.o)
WEB_OBJS=$(WEB_OBJS_C) $(WEB_OBJS_S) $(WEB_OBJS_M)

# append '_debug' to objects for clip_share debug executable to prevent overwriting objects for clip_share
DEBUG_OBJS_C=$(OBJS:.o=_debug.o) $(_WEB_OBJS_C:.o=_debug.o)
DEBUG_OBJS_M=$(OBJS_M:.o=_debug.o)
DEBUG_OBJS=$(DEBUG_OBJS_C) $(DEBUG_OBJS_M)

# append '_no_ssl' to objects for clip_share_no_ssl executable to prevent overwriting objects for clip_share
NO_SSL_OBJS_C=$(OBJS:.o=_no_ssl.o) # Web mode is not supported with no_ssl
NO_SSL_OBJS_M=$(OBJS_M:.o=_no_ssl.o)
NO_SSL_OBJS=$(NO_SSL_OBJS_C) $(NO_SSL_OBJS_M)

$(PROGRAM_NAME): $(OBJS) $(OBJS_M) $(OTHER_DEPENDENCIES)
$(PROGRAM_NAME_WEB): $(WEB_OBJS) $(OTHER_DEPENDENCIES)
$(PROGRAM_NAME) $(PROGRAM_NAME_WEB):
	$(CC) $^ $(LINK_FLAGS_BUILD) $(LDLIBS) -o $@

$(PROGRAM_NAME_NO_SSL): $(NO_SSL_OBJS) $(OTHER_DEPENDENCIES)
	$(CC) $^ $(LINK_FLAGS_BUILD) $(LDLIBS_NO_SSL) -o $@

$(OBJS): %.o: %.c
$(OBJS_M): %.o: %.m
$(OBJS) $(OBJS_M):
	$(CC) $(CFLAGS_OPTIM) $(CFLAGS) -fno-pie $^ -o $@

$(WEB_OBJS_C): %_web.o: %.c
$(WEB_OBJS_M): %_web.o: %.m
$(WEB_OBJS_S): %_web.o: %.S
$(WEB_OBJS):
	$(CC) $(CFLAGS_OPTIM) $(CFLAGS) -DWEB_ENABLED -fno-pie $^ -o $@

$(DEBUG_OBJS_C): %_debug.o: %.c
$(DEBUG_OBJS_M): %_debug.o: %.m
$(DEBUG_OBJS):
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) $^ -o $@

$(NO_SSL_OBJS_C): %_no_ssl.o: %.c
$(NO_SSL_OBJS_M): %_no_ssl.o: %.m
$(NO_SSL_OBJS):
	$(CC) $(CFLAGS_OPTIM) $(CFLAGS) -DNO_SSL -fno-pie $^ -o $@

winres/app.res: winres/app.rc winres/resource.h
	windres -I. $< -O coff -o $@

.PHONY: all clean debug web test check install

all: $(PROGRAM_NAME) $(PROGRAM_NAME_WEB)

debug: $(DEBUG_OBJS) $(OTHER_DEPENDENCIES)
	$(CC) $^ $(LDLIBS) -o $(PROGRAM_NAME)

web: $(PROGRAM_NAME_WEB)
no_ssl: $(PROGRAM_NAME_NO_SSL)

test: $(PROGRAM_NAME)
	@chmod +x tests/run.sh && cd tests && MIN_PROTO=$(MIN_PROTO) MAX_PROTO=$(MAX_PROTO) ./run.sh $(PROGRAM_NAME)

check: test

install: $(PROGRAM_NAME) helper_tools/install.sh
	@echo
	@chmod +x helper_tools/install.sh && helper_tools/install.sh

clean:
	$(RM) $(OBJS) $(OBJS_M) $(WEB_OBJS) $(DEBUG_OBJS) $(NO_SSL_OBJS)
	$(RM) $(PROGRAM_NAME) $(PROGRAM_NAME_WEB) $(PROGRAM_NAME_NO_SSL)
