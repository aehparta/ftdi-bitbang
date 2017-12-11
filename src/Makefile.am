## Makefile.am for ftdi-bitbang

bin_PROGRAMS = ftdi-bitbang
lib_LTLIBRARIES = libftdi-bitbang.la

ftdi_bitbang_SOURCES = main.c

libftdi_bitbang_la_SOURCES = ftdi-bitbang.c

ftdi_bitbang_LDFLAGS = @libftdi_LIBS@ -lftdi-bitbang
ftdi_bitbang_CFLAGS = @libftdi_CFLAGS@

include_HEADERS = ftdi-bitbang.h

pkgconfigdir = @libdir@/pkgconfig
pkgconfig_DATA = @PACKAGE_NAME@.pc

