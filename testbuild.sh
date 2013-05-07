#!/bin/sh
# testbuild.sh - Build GParted, logging top commit and build results.
#                Command line parameters are passed to autogen.sh.
#                MAKEFLAGS environment variable overrides make flags.
#
# Copyright (C) 2013 Mike Fleetwood
#
#  Copying and distribution of this file, with or without modification,
#  are permitted in any medium without royalty provided the copyright
#  notice and this notice are preserved.  This file is offered as-is,
#  without any warranty.


exec 1>> testbuild.log 2>&1
echo '############################################################'
echo "## Build date: `date`"
echo '############################################################'
git log -1
echo '############################################################'

set -x

if [ "X$MAKEFLAGS" = 'X' ]; then
	# Default to using the number of processors to tell make how
	# many jobs to run simultaneously
	nproc=`grep -c '^processor' /proc/cpuinfo` || nproc=1
	MAKEFLAGS="-j $nproc"
	export MAKEFLAGS
fi

# Disable bold text escape sequences from gnome-autogen.sh
TERM=

./autogen.sh "${@}" && make clean && make $MAKEFLAGS
