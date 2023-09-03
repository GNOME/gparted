#!/bin/sh
# Name    : exclude_loopdev_tests.sh
# Purpose : Generate list of tests which require loopdev so they can be
#           excluded in GitLab Docker CI images because loop device
#           creation fails.  Suitable for assigning directly to the
#           GTEST_FILTER environment variable.
# Usage   : export_GTEST_FILTER=`exclude_loopdev_tests.sh tests/test_SupportedFileSystems.cc`
#
# Copyright (C) 2022 Mike Fleetwood
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


awk '
BEGIN {
	FS = "(, )|\\(|\\)|,"
	num_tests = 0
	param_fsname["FS_BTRFS"]   = "btrfs"
	param_fsname["FS_JFS"]     = "jfs"
	param_fsname["FS_LVM2_PV"] = "lvm2pv"
	param_fsname["FS_NILFS2"]  = "nilfs2"
	param_fsname["FS_XFS"]     = "xfs"
}
/^TEST_P/ {
	# Extract parameterised test name.
	ptest_name = $2 "." $3
	#printf "DEBUG: ptest_name=\"%s\"\n", ptest_name
}
/SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS/ && ptest_name != "" {
	# Save test name.
	test_name[num_tests] = ptest_name "/" param_fsname[$2]
	#printf "DEBUG: test_name[%d]=\"%s\"\n", num_tests, test_name[num_tests]
	num_tests++
}
/^INSTANTIATE_TEST_SUITE_P/ {
	# Save instantiation name.
	instance_name = $2
}
END {
	printf "-"
	for (i = 0; i < num_tests; i ++) {
		if (i > 0) printf ":"
		printf "%s/%s", instance_name, test_name[i]
	}
}
' "${@}"
