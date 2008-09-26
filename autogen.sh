#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="gparted"

which gnome-autogen.sh || {
    echo "You need to install gnome-common from the GNOME CVS"
    exit 1
}

# Check for GNOME-DOC-UTILS
GDUMAKE="gnome-doc-utils.make"
GDUMAKEFULLPATH=`locate $GDUMAKE | grep -m 1 "gnome-doc-utils/"`
if test "x${GDUMAKEFULLPATH}" = "x" ; then 
	echo "Cannot find file: $GDUMAKE"
	echo "You need to install gnome-doc-utils from the GNOME CVS"
	exit 1
fi
# Ensure a copy of gnome-doc-utils.make exists in the top source directory
test -e $srcdir/$GDUMAKE || {
	echo "Copying $GDUMAKEFULLPATH to $srcdir"
	cp -p $GDUMAKEFULLPATH $srcdir
}

REQUIRED_AUTOMAKE_VERSION=1.9 USE_GNOME2_MACROS=1 USE_COMMON_DOC_BUILD=yes . gnome-autogen.sh
