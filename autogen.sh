#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="gparted"
REQUIRED_AUTOMAKE_VERSION=1.7

which gnome-autogen.sh || {
    echo "You need to install gnome-common from the GNOME CVS"
    exit 1
}

#i needed to put this one here on order to fix an weird depcomp error
aclocal


USE_GNOME2_MACROS=1 . gnome-autogen.sh
