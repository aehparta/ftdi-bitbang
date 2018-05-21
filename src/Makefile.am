## Makefile.am for ftdi-something libs and commands

bin_PROGRAMS = ftdi-bitbang ftdi-hd44780 ftdi-control ftdi-spi ftdi-simple-capture
lib_LTLIBRARIES = libftdi-bitbang.la libftdi-hd44780.la libftdi-spi.la

ftdi_bitbang_SOURCES = cmd-bitbang.c cmd-common.c
ftdi_hd44780_SOURCES = cmd-hd44780.c cmd-common.c
ftdi_control_SOURCES = cmd-control.c cmd-common.c
ftdi_spi_SOURCES = cmd-spi.c cmd-common.c
ftdi_simple_capture_SOURCES = cmd-simple-capture.c cmd-common.c

libftdi_bitbang_la_SOURCES = ftdi-bitbang.c
libftdi_bitbang_la_LDFLAGS = @libftdi1_LIBS@
libftdi_bitbang_la_CFLAGS = @libftdi1_CFLAGS@

libftdi_hd44780_la_SOURCES = ftdi-hd44780.c
libftdi_hd44780_la_LDFLAGS = -lftdi-bitbang @libftdi1_LIBS@
libftdi_hd44780_la_CFLAGS = @libftdi1_CFLAGS@

libftdi_spi_la_SOURCES = ftdi-spi.c
libftdi_spi_la_LDFLAGS = -lftdi-bitbang @libftdi1_LIBS@
libftdi_spi_la_CFLAGS = @libftdi1_CFLAGS@

ftdi_bitbang_LDFLAGS = -lftdi-bitbang @libftdi1_LIBS@
ftdi_bitbang_CFLAGS = @libftdi1_CFLAGS@
ftdi_hd44780_LDFLAGS = -lftdi-bitbang -lftdi-hd44780 @libftdi1_LIBS@
ftdi_hd44780_CFLAGS = @libftdi1_CFLAGS@
ftdi_control_LDFLAGS = -lftdi-bitbang @libftdi1_LIBS@
ftdi_control_CFLAGS = @libftdi1_CFLAGS@
ftdi_spi_LDFLAGS = -lftdi-bitbang -lftdi-spi @libftdi1_LIBS@
ftdi_spi_CFLAGS = @libftdi1_CFLAGS@
ftdi_simple_capture_LDFLAGS = -lftdi-bitbang -lpthread @libftdi1_LIBS@
ftdi_simple_capture_CFLAGS = @libftdi1_CFLAGS@

include_HEADERS = ftdi-bitbang.h ftdi-hd44780.h ftdi-spi.h

pkgconfigdir = @libdir@/pkgconfig
pkgconfig_DATA = @PACKAGE_NAME@.pc

