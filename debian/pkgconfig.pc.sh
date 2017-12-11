#!/bin/sh
prefix=/usr
exec_prefix=$prefix
libdir=$exec_prefix/lib
includedir=$exec_prefix/include
echo "
prefix=$prefix
exec_prefix=$exec_prefix
libdir=$libdir
includedir=$includedir

Name: $PACKAGE_NAME
Description: $PACKAGE_DESC
Version: $PACKAGE_VERSION
Libs:
Cflags:
"