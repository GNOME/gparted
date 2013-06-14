#!/bin/sh
# autogen.sh - Run this to generate all the initial makefiles, etc.
#
# Copyright (C) 2004 Bart 'plors' Hakvoort
# Copyright (C) 2008 Curtis Gedak
#
#  Copying and distribution of this file, with or without modification,
#  are permitted in any medium without royalty provided the copyright
#  notice and this notice are preserved.  This file is offered as-is,
#  without any warranty.


srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="gparted"

which gnome-autogen.sh || {
    echo "You need to install gnome-common"
    exit 1
}

# Ensure that m4 directory exists
if ! test -d $srcdir/m4; then
	mkdir $srcdir/m4
fi

# Check for GNOME-DOC-UTILS
GDUMAKE="gnome-doc-utils.make"
datadir=`pkg-config --variable=datadir gnome-doc-utils`
GDUMAKEFULLPATH="$datadir/gnome-doc-utils/$GDUMAKE"
if test "X$datadir" = 'X' || ! test -f "${GDUMAKEFULLPATH}" ; then
	echo "Cannot find file: $GDUMAKE"
	echo "You need to install gnome-doc-utils"
	exit 1
fi
# Ensure a copy of gnome-doc-utils.make exists in the top source directory
test -e $srcdir/$GDUMAKE || {
	echo "Copying $GDUMAKEFULLPATH to $srcdir"
	cp -p $GDUMAKEFULLPATH $srcdir
}

REQUIRED_AUTOMAKE_VERSION=1.9 USE_GNOME2_MACROS=1 USE_COMMON_DOC_BUILD=yes . gnome-autogen.sh
