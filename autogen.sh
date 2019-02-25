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

which gnome-autogen.sh || {
    echo "You need to install gnome-common"
    exit 1
}

# Ensure that m4 directory exists
if ! test -d $srcdir/m4; then
	mkdir $srcdir/m4
fi

which yelp-build || {
	echo "ERROR: Command 'yelp-build' not found"
	echo "ERROR: Package 'yelp-tools' needs to be installed"
	exit 1
}

REQUIRED_AUTOMAKE_VERSION=1.9 . gnome-autogen.sh
