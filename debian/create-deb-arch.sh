#!/bin/bash
# This script builds debian packages.

bash=/bin/bash
set -a

PACKAGE_ARCH=`uname -m`

if [ `uname -m` = "x86_64" ]; then
	PACKAGE_ARCH="amd64"
else
	PACKAGE_ARCH="i386"
fi

DEBIAN_ALL=arch/$PACKAGE_ARCH/DEBIAN
DEBIAN_ARCH=arch/all/DEBIAN
PKGROOT=$PWD/debian
PKGDIR="$PKGROOT/$PACKAGE_NAME-$PACKAGE_VERSION-$PACKAGE_BUILD-$PACKAGE_ARCH-deb"
PKGDIR_SRC=$PKGROOT/$PACKAGE_NAME-$PACKAGE_VERSION-deb-src
SRCDIR=$PKGDIR_SRC/$PACKAGE_NAME-$PACKAGE_VERSION
ROOT=$PWD
PKGNAME="$PACKAGE_NAME-$PACKAGE_VERSION-$PACKAGE_BUILD-$PACKAGE_ARCH.deb"

# Copy binary package files.
copy_binpkg_files()
{
	echo "** Copying files..."
	set -e

	# create directories
	mkdir -p "$PKGDIR/DEBIAN"
	mkdir -p "$PKGDIR/usr"
	mkdir -p "$PKGDIR/usr/bin"
	mkdir -p "$PKGDIR/usr/lib"
	mkdir -p "$PKGDIR/usr/lib/pkgconfig"
	mkdir -p "$PKGDIR/usr/share"
	mkdir -p "$PKGDIR/usr/include"
	
	# copy debian package files
	for suffix in control changelog; do
		if [ -e $PKGROOT/$DEBIAN_ALL/$suffix.sh ]; then
			$bash $PKGROOT/$DEBIAN_ALL/$suffix.sh > "$PKGDIR/DEBIAN/$suffix"
		fi
		if [ -e $PKGROOT/$DEBIAN_ARCH/$suffix.sh ]; then
			$bash $PKGROOT/$DEBIAN_ARCH/$suffix.sh > "$PKGDIR/DEBIAN/$suffix"
		fi
	done
	for suffix in copyright preinst postinst postrm prerm; do
		if [ -e $PKGROOT/$DEBIAN_ALL/$suffix ]; then
			cp $PKGROOT/$DEBIAN_ALL/$suffix "$PKGDIR/DEBIAN"
		fi
		if [ -e $PKGROOT/$DEBIAN_ARCH/$suffix ]; then
			cp $PKGROOT/$DEBIAN_ARCH/$suffix "$PKGDIR/DEBIAN"
		fi
	done
	
	
	# copy project files
	for libname in $PACKAGE_LIBS; do
		for suffix in a la so so.0 so.0.0.0; do
			cp -d $ROOT/src/.libs/$libname.$suffix $PKGDIR/usr/lib/
		done
	done
	
	for binname in $PACKAGE_BINS; do
		cp -d $ROOT/src/.libs/$binname $PKGDIR/usr/bin/
	done
	
	# copy include files
	for suffix in $PACKAGE_HEADERS; do
		cp $ROOT/src/$suffix $PKGDIR/usr/include/
	done

	if [ -e $ROOT/src/$PACKAGE_NAME.pc ]; then
		cp $ROOT/src/$PACKAGE_NAME.pc $PKGDIR/usr/lib/pkgconfig
	fi

	set +e
}

echo "** Creating debian package ($PKGNAME)..."
echo "** Build $PACKAGE_BUILD (reset this by editing ./debian/build)"

if ! ./configure; then
	echo "** Error when running configure!"
	exit 1
fi

rm -rf $PKGDIR
rm -rf $PKGDIR_SRC

if ! make clean all; then
	echo "** Error while running make!"
	exit 1
fi

if ! copy_binpkg_files; then
	echo "** Error while copying files!"
	exit 1
fi

echo "** Creating binary package..."

cd $PKGROOT
if dpkg-deb -b "$PKGDIR" "$PKGNAME"; then
	echo "** Successfully finished building package."
else
	echo "** Error while building package!"
fi

let "PACKAGE_BUILD += 1"
echo $PACKAGE_BUILD > $ROOT/debian/build

