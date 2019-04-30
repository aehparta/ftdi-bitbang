## Makefile.am for ftdi-something libs and commands

bin_PROGRAMS = ftdi-bitbang ftdi-hd44780 ftdi-control ftdi-spi ftdi-simple-capture ftdi-simple-scope
lib_LTLIBRARIES = libftdi-bitbang.la libftdi-hd44780.la libftdi-spi.la

ftdi_bitbang_SOURCES = cmd-bitbang.c cmd-common.c
ftdi_hd44780_SOURCES = cmd-hd44780.c cmd-common.c
ftdi_control_SOURCES = cmd-control.c cmd-common.c
ftdi_spi_SOURCES = cmd-spi.c cmd-common.c
ftdi_simple_capture_SOURCES = cmd-simple-capture.c cmd-common.c
ftdi_simple_scope_SOURCES = cmd-simple-scope.c cmd-common.c

libftdi_bitbang_la_SOURCES = ftdi-bitbang.c
libftdi_bitbang_la_LDFLAGS = @libftdi1_LIBS@
libftdi_bitbang_la_CFLAGS = @libftdi1_CFLAGS@

libftdi_hd44780_la_SOURCES = ftdi-hd44780.c
libftdi_hd44780_la_LIBADD = libftdi-bitbang.la
libftdi_hd44780_la_LDFLAGS = @libftdi1_LIBS@
libftdi_hd44780_la_CFLAGS = @libftdi1_CFLAGS@

libftdi_spi_la_SOURCES = ftdi-spi.c
libftdi_spi_la_LIBADD = libftdi-bitbang.la
libftdi_spi_la_LDFLAGS = @libftdi1_LIBS@
libftdi_spi_la_CFLAGS = @libftdi1_CFLAGS@

ftdi_bitbang_LDADD = libftdi-bitbang.la
ftdi_bitbang_LDFLAGS = @libftdi1_LIBS@
ftdi_bitbang_CFLAGS = @libftdi1_CFLAGS@
ftdi_hd44780_LDADD = libftdi-bitbang.la libftdi-hd44780.la
ftdi_hd44780_LDFLAGS = @libftdi1_LIBS@
ftdi_hd44780_CFLAGS = @libftdi1_CFLAGS@
ftdi_control_LDADD = libftdi-bitbang.la
ftdi_control_LDFLAGS = @libftdi1_LIBS@
ftdi_control_CFLAGS = @libftdi1_CFLAGS@
ftdi_spi_LDADD = libftdi-bitbang.la libftdi-spi.la
ftdi_spi_LDFLAGS = @libftdi1_LIBS@
ftdi_spi_CFLAGS = @libftdi1_CFLAGS@
ftdi_simple_capture_LDADD = libftdi-bitbang.la
ftdi_simple_capture_LDFLAGS = -lpthread @libftdi1_LIBS@
ftdi_simple_capture_CFLAGS = @libftdi1_CFLAGS@
ftdi_simple_scope_LDADD = libftdi-bitbang.la
ftdi_simple_scope_LDFLAGS = -lpthread @libftdi1_LIBS@ @sdl2_LIBS@
ftdi_simple_scope_CFLAGS = @libftdi1_CFLAGS@ @sdl2_CFLAGS@

include_HEADERS = ftdi-bitbang.h ftdi-hd44780.h ftdi-spi.h

pkgconfigdir = @libdir@/pkgconfig
pkgconfig_DATA = @PACKAGE_NAME@.pc

