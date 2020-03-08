#!/bin/sh
#    Name: tests/makedev.sh
# Purpose: Create /dev special files needed for GParted unit testing
#          inside GitLab Docker CI images.
#
# Copyright (C) 2020 Mike Fleetwood
#
#  Copying and distribution of this file, with or without modification,
#  are permitted in any medium without royalty provided the copyright
#  notice and this notice are preserved.  This file is offered as-is,
#  without any warranty.


# Create first two block special devices named in /proc/partitions, if
# they don't already exist, for test_BlockSpecial.
awk '$1=="major" {next} NF==4 {printf "/dev/%s %s %s\n", $4, $1, $2; p++} p>=2 {exit}' /proc/partitions | \
while read name maj min
do
	if test ! -e "$name"; then
		echo mknod -m 0660 "$name" b $maj $min
		mknod -m 0660 "$name" b $maj $min
		chown root:disk "$name"
	fi
done
