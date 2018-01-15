#!/bin/bash

bash=/bin/bash
set -a

# set and clear config
CONFIG="configure.ac"
echo "" > $CONFIG

# create pkgconfig-file (.pc) if needed
$bash "./debian/pkgconfig.pc.sh" > "./src/$PACKAGE_NAME.pc"

echo "
dnl use this file with autoconf to produce a configure script.
AC_INIT([$PACKAGE_NAME], [$PACKAGE_VERSION])
AC_CONFIG_SRCDIR([README])
AC_CANONICAL_TARGET
dnl Setup for automake
AM_INIT_AUTOMAKE([foreign])
" >> $CONFIG

for suffix in $PKGLIBSADD; do
	ADD=${suffix%:*}
	LIB=${suffix#*:}
	echo "PKG_CHECK_MODULES($ADD, $LIB)" >> $CONFIG
done

for suffix in $LIBSADD; do
	LIB=${suffix%:*}
	FUNC=${suffix#*:}
	echo "AC_CHECK_LIB($LIB, $FUNC,, AC_MSG_ERROR([library $LIB or $FUNC in $LIB not found!]))" >> $CONFIG
done

echo "
dnl Some defined values
AC_DEFINE(PACKAGE_DESC, \"$PACKAGE_DESC\")
AC_DEFINE(PACKAGE_VERSION_MAJOR, $PACKAGE_VERSION_MAJOR)
AC_DEFINE(PACKAGE_VERSION_MINOR, $PACKAGE_VERSION_MINOR)
AC_DEFINE(PACKAGE_VERSION_MICRO, $PACKAGE_VERSION_MICRO)
AC_DEFINE(PACKAGE_BUILD, \"$PACKAGE_BUILD\")
" >> $CONFIG

echo "
dnl Check types and sizes
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(long long, 8)
" >> $CONFIG

echo "
dnl Check for tools
AC_PROG_CC
AM_PROG_LIBTOOL

CXXFLAGS=\"\"
CFLAGS=\"\"
AC_DEFINE(_GNU_SOURCE, 1)
" >> $CONFIG

echo "
AC_ARG_ENABLE([debug],
              AC_HELP_STRING([--enable-debug], [Enable debug mode (default YES)]),
              [debug=[yes]], [debug=[no]])

AC_MSG_CHECKING([debug mode])
if test x\$debug == xyes ; then
	AC_DEFINE(_DEBUG, 1, [Debug mode])
	AC_SUBST(GDB_CFLAG, \"-g\")
	CFLAGS=\"-g -O0 \$CFLAGS\"
fi
AC_MSG_RESULT([\$debug])
" >> $CONFIG

echo "
dnl Finally create all the generated files
AC_CONFIG_FILES([ Makefile
                  src/Makefile
                  ])
AC_OUTPUT

echo \"
Configuration for \$PACKAGE_NAME \$PACKAGE_VERSION:
  debug mode:			\$debug
\"
" >> $CONFIG

