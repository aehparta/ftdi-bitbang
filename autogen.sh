#!/bin/bash

set -a

# you need to change these values
PACKAGE_AUTHORS="Antti Partanen <aehparta@iki.fi>"
# examples for section: libs, utils, misc, comm, admin, base
PACKAGE_SECTION="libs"
PACKAGE_PRIORITY="optional"
PACKAGE_NAME="ftdi-bitbang"
PACKAGE_DESC="bitbang control through FTDI FTx232 chips"
PACKAGE_VERSION_MAJOR="0"
PACKAGE_VERSION_MINOR="1"
PACKAGE_VERSION_MICRO="0"
PACKAGE_VERSION="$PACKAGE_VERSION_MAJOR.$PACKAGE_VERSION_MINOR.$PACKAGE_VERSION_MICRO"

# binaries/libraries to install
PACKAGE_BINS="ftdi-bitbang ftdi-hd44780"
PACKAGE_LIBS="libftdi-bitbang libftdi-hd44780"

# get build number
PACKAGE_BUILD=`cat debian/build`

# libraries/binaries to be checked
PKGLIBSADD="libftdi1:libftdi1"
LIBSADD=""
BINSCHECK="pkg-config:--version"

# include headers when making package
PACKAGE_HEADERS="ftdi-bitbang.h ftdi-hd44780.h"


# check binaries
for suffix in $BINSCHECK; do
	BIN=${suffix%:*}
	ARG=${suffix#*:}
	if $BIN $ARG >/dev/null 2>&1; then
		echo "$BIN found."
	else
		echo "$BIN not found! you need to install $BIN"
		exit 1
	fi
done

echo "Generating configure files... may take a while."

ln -f -s README.md README

if ! ./configure.ac.sh; then
	echo "Failed to create configure.ac!"
	exit 1
fi

if ! autoreconf --install --force; then
	echo "autoreconf failed!"
	exit 1
fi

case "$1" in
	deb)
		bash ./debian/create-deb-arch.sh $2
	;;
	
	*)
		./configure
		echo "now 'make' to build binaries"
		echo "then 'make install'"
	;;
esac
