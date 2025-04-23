# Makefile - makefile
# Copyright (C) 2022-2025 H. Thevindu J. Wijesekera

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

MAKEFLAGS+= -j4 --warn-undefined-variables --no-builtin-rules

SHELL:=bash
.SHELLFLAGS:=-eu -o pipefail -c
.ONESHELL:
.DELETE_ON_ERROR:

PROGRAM_NAME=clip_share
PROGRAM_NAME_WEB:=$(PROGRAM_NAME)_web
PROGRAM_NAME_NO_SSL:=$(PROGRAM_NAME)_no_ssl

SRC_DIR=src
BUILD_DIR=build

MIN_PROTO=1
MAX_PROTO=3
INFO_NAME=clip_share

CC=gcc
CPP=cpp
CFLAGS=-c -pipe -I$(SRC_DIR) --std=gnu11 -fstack-protector -fstack-protector-all -Wall -Wextra -Wdouble-promotion -Wformat=2 -Wformat-nonliteral -Wformat-security -Wnull-dereference -Winit-self -Wmissing-include-dirs -Wswitch-default -Wstrict-overflow=4 -Wconversion -Wfloat-equal -Wshadow -Wpointer-arith -Wundef -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Woverlength-strings
CFLAGS_DEBUG=-g -DDEBUG_MODE

OBJS_C=main.o servers/clip_share.o servers/udp_serve.o proto/server.o proto/versions.o proto/methods.o utils/utils.o utils/net_utils.o utils/list_utils.o utils/config.o utils/kill_others.o

_WEB_OBJS_C=servers/clip_share_web.o
_WEB_OBJS_S=servers/page_blob.o
OBJS_M=
OBJS_BIN=

OTHER_DEPENDENCIES=
LINK_FLAGS_BUILD=

ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

ARCH?=x86_64
ifeq ($(ARCH),x86)
	BUILD_DIR:=$(BUILD_DIR)_x86
	CFLAGS+= -m32
	LDLIBS+= -m32
	PROGRAM_NAME:=$(PROGRAM_NAME)32
	PROGRAM_NAME_WEB:=$(PROGRAM_NAME_WEB)32
	PROGRAM_NAME_NO_SSL:=$(PROGRAM_NAME_NO_SSL)32
endif

ifeq ($(detected_OS),Linux)
	OBJS_C+= xclip/xclip.o xclip/xclib.o xscreenshot/xscreenshot.o
	CFLAGS+= -ftree-vrp -Wformat-signedness -Wshift-overflow=2 -Wstringop-overflow=4 -Walloc-zero -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wjump-misses-init -Wlogical-op -Wvla-larger-than=65536
	CFLAGS_OPTIM=-Os
	LDLIBS_NO_SSL=-lunistring -lX11 -lXmu -lXt -lxcb -lxcb-randr -lpng
	LDLIBS_SSL=-lssl -lcrypto
	LINK_FLAGS_BUILD=-no-pie -Wl,-s,--gc-sections
else ifeq ($(detected_OS),Windows)
	OBJS_C+= utils/win_image.o
	CFLAGS+= -ftree-vrp -Wformat-signedness -Wshift-overflow=2 -Wstringop-overflow=4 -Walloc-zero -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wjump-misses-init -Wlogical-op -Wvla-larger-than=65536
	CFLAGS+= -D__USE_MINGW_ANSI_STDIO
	CFLAGS_OPTIM=-O3
	OTHER_DEPENDENCIES+= res/win/app.coff
	LDLIBS_NO_SSL=-l:libunistring.a -l:libpthread.a -lws2_32 -lgdi32 -l:libpng16.a -l:libz.a -lIphlpapi -lcrypt32 -lShcore -lUserenv
	LDLIBS_SSL=-l:libssl.a -l:libcrypto.a -l:libpthread.a
	LINK_FLAGS_BUILD=-no-pie -mwindows
	PROGRAM_NAME:=$(PROGRAM_NAME).exe
	PROGRAM_NAME_WEB:=$(PROGRAM_NAME_WEB).exe
	PROGRAM_NAME_NO_SSL:=$(PROGRAM_NAME_NO_SSL).exe
else ifeq ($(detected_OS),Darwin)
export CPATH=$(shell brew --prefix)/include
export LIBRARY_PATH=$(shell brew --prefix)/lib
	OBJS_M=utils/mac_utils.o utils/mac_menu.o
	OBJS_BIN+= res/mac/icon.o
	CFLAGS+= -fobjc-arc
	CFLAGS_OPTIM=-O3
	LDLIBS_NO_SSL=-framework AppKit -lunistring -lpng -lobjc
	LDLIBS_SSL=-lssl -lcrypto
else
$(error ClipShare is not supported on this platform!)
endif
LDLIBS+= $(LDLIBS_SSL) $(LDLIBS_NO_SSL)
CFLAGS+= -DINFO_NAME=\"$(INFO_NAME)\" -DPROTOCOL_MIN=$(MIN_PROTO) -DPROTOCOL_MAX=$(MAX_PROTO)
CFLAGS_OPTIM+= -Werror

VERSION_FILE=$(SRC_DIR)/res/version
ifeq (4.2,$(firstword $(sort $(MAKE_VERSION) 4.2)))
	VERSION_INFO=$(file < $(VERSION_FILE))
else
	VERSION_INFO=$(shell cat $(VERSION_FILE))
endif
VERSION_MAJOR=$(word 2 ,$(subst =, ,$(filter VERSION_MAJOR=%,$(VERSION_INFO))))
VERSION_MINOR=$(word 2 ,$(subst =, ,$(filter VERSION_MINOR=%,$(VERSION_INFO))))
VERSION_PATCH=$(word 2 ,$(subst =, ,$(filter VERSION_PATCH=%,$(VERSION_INFO))))
VERSION=$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
CFLAGS+= -DVERSION=\"$(VERSION)\"

OBJS_C:=$(addprefix $(BUILD_DIR)/,$(OBJS_C))
OBJS_M:=$(addprefix $(BUILD_DIR)/,$(OBJS_M))
OBJS_BIN:=$(addprefix $(BUILD_DIR)/,$(OBJS_BIN))
_WEB_OBJS_C:=$(addprefix $(BUILD_DIR)/,$(_WEB_OBJS_C))
_WEB_OBJS_S:=$(addprefix $(BUILD_DIR)/,$(_WEB_OBJS_S))
OTHER_DEPENDENCIES:=$(addprefix $(BUILD_DIR)/,$(OTHER_DEPENDENCIES))

# append '_web' to objects for clip_share_web to prevent overwriting objects for clip_share
WEB_OBJS_C=$(OBJS_C:.o=_web.o) $(_WEB_OBJS_C:.o=_web.o)
WEB_OBJS_S=$(_WEB_OBJS_S:.o=_web.o)
WEB_OBJS_M=$(OBJS_M:.o=_web.o)
WEB_OBJS_BIN=$(OBJS_BIN:.o=_web.o)
WEB_OBJS=$(WEB_OBJS_C) $(WEB_OBJS_S) $(WEB_OBJS_M) $(WEB_OBJS_BIN)

# append '_debug' to objects for clip_share debug executable to prevent overwriting objects for clip_share
DEBUG_OBJS_C=$(OBJS_C:.o=_debug.o) $(_WEB_OBJS_C:.o=_debug.o)
DEBUG_OBJS_M=$(OBJS_M:.o=_debug.o)
DEBUG_OBJS_BIN=$(OBJS_BIN:.o=_debug.o)
DEBUG_OBJS=$(DEBUG_OBJS_C) $(DEBUG_OBJS_M) $(DEBUG_OBJS_BIN)

# append '_no_ssl' to objects for clip_share_no_ssl executable to prevent overwriting objects for clip_share
NO_SSL_OBJS_C=$(OBJS_C:.o=_no_ssl.o) # Web mode is not supported with no_ssl
NO_SSL_OBJS_M=$(OBJS_M:.o=_no_ssl.o)
NO_SSL_OBJS_BIN=$(OBJS_BIN:.o=_no_ssl.o)
NO_SSL_OBJS=$(NO_SSL_OBJS_C) $(NO_SSL_OBJS_M) $(NO_SSL_OBJS_BIN)

OBJS=$(OBJS_C) $(OBJS_M) $(OBJS_BIN)
ALL_DEPENDENCIES=$(OBJS) $(WEB_OBJS) $(DEBUG_OBJS) $(NO_SSL_OBJS) $(OTHER_DEPENDENCIES)
DIRS=$(foreach file,$(ALL_DEPENDENCIES),$(dir $(file)))
DIRS:=$(sort $(DIRS))

$(PROGRAM_NAME): $(OBJS) $(OTHER_DEPENDENCIES)
$(PROGRAM_NAME_WEB): $(WEB_OBJS) $(OTHER_DEPENDENCIES)
$(PROGRAM_NAME) $(PROGRAM_NAME_WEB):
	@echo CCLD $$'\t' $@
	@$(CC) $^ $(LINK_FLAGS_BUILD) $(LDLIBS) -o $@

$(PROGRAM_NAME_NO_SSL): $(NO_SSL_OBJS) $(OTHER_DEPENDENCIES)
	@echo CCLD $$'\t' $@
	@$(CC) $^ $(LINK_FLAGS_BUILD) $(LDLIBS_NO_SSL) -o $@

.SECONDEXPANSION:
$(ALL_DEPENDENCIES): %: | $$(dir %)

$(OBJS_C): $(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
$(OBJS_M): $(BUILD_DIR)/%.o: $(SRC_DIR)/%.m
$(OBJS_BIN): $(BUILD_DIR)/%.o: $(SRC_DIR)/%_.c
$(BUILD_DIR)/main.o: $(VERSION_FILE)
$(OBJS):
	@echo CC $$'\t' $@
	@$(CC) $(CFLAGS_OPTIM) $(CFLAGS) -fno-pie $< -o $@

$(WEB_OBJS_C): $(BUILD_DIR)/%_web.o: $(SRC_DIR)/%.c
$(WEB_OBJS_M): $(BUILD_DIR)/%_web.o: $(SRC_DIR)/%.m
$(WEB_OBJS_S): $(BUILD_DIR)/%_web.o: $(SRC_DIR)/%.S
$(WEB_OBJS_BIN): $(BUILD_DIR)/%_web.o: $(SRC_DIR)/%_.c
$(BUILD_DIR)/main_web.o: $(VERSION_FILE)
$(WEB_OBJS):
	@echo CC $$'\t' $@
	@$(CC) $(CFLAGS_OPTIM) $(CFLAGS) -DWEB_ENABLED -fno-pie $< -o $@

$(DEBUG_OBJS_C): $(BUILD_DIR)/%_debug.o: $(SRC_DIR)/%.c
$(DEBUG_OBJS_M): $(BUILD_DIR)/%_debug.o: $(SRC_DIR)/%.m
$(DEBUG_OBJS_BIN): $(BUILD_DIR)/%_debug.o: $(SRC_DIR)/%_.c
$(BUILD_DIR)/main_debug.o: $(VERSION_FILE)
$(DEBUG_OBJS):
	@echo CC $$'\t' $@
	@$(CC) $(CFLAGS) $(CFLAGS_DEBUG) $< -o $@

$(NO_SSL_OBJS_C): $(BUILD_DIR)/%_no_ssl.o: $(SRC_DIR)/%.c
$(NO_SSL_OBJS_M): $(BUILD_DIR)/%_no_ssl.o: $(SRC_DIR)/%.m
$(NO_SSL_OBJS_BIN): $(BUILD_DIR)/%_no_ssl.o: $(SRC_DIR)/%_.c
$(BUILD_DIR)/main_no_ssl.o: $(VERSION_FILE)
$(NO_SSL_OBJS):
	@echo CC $$'\t' $@
	@$(CC) $(CFLAGS_OPTIM) $(CFLAGS) -DNO_SSL -fno-pie $< -o $@

$(BUILD_DIR)/res/win/app.coff: $(SRC_DIR)/res/win/app_.rc $(SRC_DIR)/res/win/resource.h
	@echo windres $$'\t' $@
	@windres -I$(SRC_DIR) $< -O coff -o $@

$(SRC_DIR)/res/win/app_.rc: $(SRC_DIR)/res/win/app.rc $(VERSION_FILE)
	@echo CPP $$'\t' $@
	@$(CPP) -I$(SRC_DIR) -P -DVERSION_MAJOR=$(VERSION_MAJOR) -DVERSION_MINOR=$(VERSION_MINOR) -DVERSION_PATCH=$(VERSION_PATCH) -DVERSION=\"$(VERSION)\" $< -o $@

$(SRC_DIR)/res/mac/icon_.c: $(SRC_DIR)/res/mac/icon.png
	@echo generate $$'\t' $@
	@cd $(dir $@) && \
	xxd -i $(notdir $<) >$(notdir $@)

$(DIRS):
	@echo create directory $@
	@mkdir -p $@

.PHONY: all clean debug web test check install

all: $(PROGRAM_NAME) $(PROGRAM_NAME_NO_SSL) $(PROGRAM_NAME_WEB)

debug: $(DEBUG_OBJS) $(OTHER_DEPENDENCIES)
	@echo CCLD $$'\t' $@ $(PROGRAM_NAME)
	@$(CC) $^ $(LDLIBS) -o $(PROGRAM_NAME)

web: $(PROGRAM_NAME_WEB)
no_ssl: $(PROGRAM_NAME_NO_SSL)

test: $(PROGRAM_NAME)
	@chmod +x tests/run.sh && cd tests && MIN_PROTO=$(MIN_PROTO) MAX_PROTO=$(MAX_PROTO) ./run.sh $(PROGRAM_NAME)

check: test

install: $(PROGRAM_NAME) helper_tools/install.sh
	@echo
	@chmod +x helper_tools/install.sh && helper_tools/install.sh

clean:
	@echo RM
	@$(RM) -r $(BUILD_DIR) $(ALL_DEPENDENCIES)
	@$(RM) $(PROGRAM_NAME) $(PROGRAM_NAME_WEB) $(PROGRAM_NAME_NO_SSL)
