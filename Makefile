
BUILD ?= ftdi-bitbang ftdi-control
CFLAGS += -Os -std=c11 -D_GNU_SOURCE -Wall
LDFLAGS += -lftdi1

ftdi-bitbang_SRC = src/ftdi-bitbang-cmd.c src/opt.c src/ftdic.c
ftdi-control_SRC = src/ftdi-control-cmd.c src/opt.c src/ftdic.c

# no need to touch things
BUILDDIR = ./build
CC = gcc
REMOVE = rm
OBJ_EXT = .o

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
