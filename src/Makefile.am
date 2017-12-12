## Makefile.am for ftdi-bitbang

bin_PROGRAMS = ftdi-bitbang
lib_LTLIBRARIES = libftdi-bitbang.la

ftdi_bitbang_SOURCES = cmd-bitbang.c

libftdi_bitbang_la_SOURCES = ftdi-bitbang.c
libftdi_bitbang_la_LDFLAGS = @libftdi1_LIBS@
libftdi_bitbang_la_CFLAGS = @libftdi1_CFLAGS@

ftdi_bitbang_LDFLAGS = -lftdi-bitbang @libftdi1_LIBS@
ftdi_bitbang_CFLAGS = @libftdi1_CFLAGS@

include_HEADERS = ftdi-bitbang.h

pkgconfigdir = @libdir@/pkgconfig
pkgconfig_DATA = @PACKAGE_NAME@.pc

