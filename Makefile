
BUILD ?= ftdi-bitbang

CFLAGS += -Os -std=c11 -D_GNU_SOURCE -Wall
LDFLAGS = 

BUILDDIR = ./build

# no need to touch things
CC = gcc
OBJ_EXT = .o

# sources
ftdi-bitbang_SRC = src/ftdi-bitbang-cmd.c src/opt.c

# object files from sources
ftdi_bitbang_OBJ = $(SRC:%.c=%.o)

all:: $(BUILD)

$(BUILDDIR)/%$(OBJ_EXT): %.c
	@mkdir -p `dirname $@`
	$(CC) -c $(CFLAGS) $< -o $@ 

# compile binaries, this must be last because of secondary expansion
.SECONDEXPANSION:
$(BUILD): $$(patsubst %.c,$(BUILDDIR)/%$(OBJ_EXT),$$($$@_SRC)) $$(patsubst %$(OBJ_EXT),$(BUILDDIR)/%$(OBJ_EXT),$$(extra_OBJ)) $$(patsubst %,$(BUILDDIR)/%$(OBJ_EXT),$$($$@_RAW_SRC))
	$(CC) $^ -o $@ $(LDFLAGS)
