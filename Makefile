
# build all tools as default
BUILD ?= ftdi-bb ftdi-cfg

# optimize for speed as default
OPT ?= s

ftdi-bb_SRC = src/ftdi-bb.c src/opt.c src/ftdic.c src/arg.c
ftdi-cfg_SRC = src/ftdi-cfg.c src/opt.c src/ftdic.c src/arg.c

# no need to touch things
BUILDDIR = ./build
CC = gcc
REMOVE = rm
PKGCFG = pkg-config
OBJ_EXT = .o

# check tools
TOOLS = $(CC) $(REMOVE) $(PKGCFG)
T := $(foreach tool,$(TOOLS),\
	$(if $(shell which $(tool)),,$(error "required binary $(tool) not found")))

# check libs
LIBS = libftdi1 libusb-1.0
L := $(foreach lib,$(LIBS),\
	$(if $(shell pkg-config --modversion --silence-errors $(lib)),,$(error "required library $(lib) not found")))

# add flags
CFLAGS += -O$(OPT) -std=c11 -D_GNU_SOURCE -Wall $(shell pkg-config --cflags libftdi1)
ifeq ($(OPT),0)
	CFLAGS += -g
endif
LDFLAGS += $(shell pkg-config --libs libftdi1)

.PHONY: all clean

all:: $(BUILD)

clean:
	$(REMOVE) -rf $(BUILDDIR)
	$(REMOVE) -f $(BUILD)

$(BUILDDIR)/%$(OBJ_EXT): %.c
	@mkdir -p `dirname $@`
	$(CC) -c $(CFLAGS) $< -o $@ 

# compile binaries, this must be last because of secondary expansion
.SECONDEXPANSION:
$(BUILD): $$(patsubst %.c,$(BUILDDIR)/%$(OBJ_EXT),$$($$@_SRC)) $$(patsubst %$(OBJ_EXT),$(BUILDDIR)/%$(OBJ_EXT),$$(extra_OBJ)) $$(patsubst %,$(BUILDDIR)/%$(OBJ_EXT),$$($$@_RAW_SRC))
	$(CC) $^ -o $@ $(LDFLAGS)
