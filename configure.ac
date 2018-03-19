AC_INIT([gparted],[0.31.0],[https://bugzilla.gnome.org/enter_bug.cgi?product=gparted])
AC_PREREQ([2.50])

AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.9 subdir-objects])
AM_MAINTAINER_MODE


dnl======================
dnl checks for programs
dnl======================
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_AWK
dnl Check for pkg-config early to avoid splitting message when first used.
PKG_CHECK_EXISTS


dnl======================
dnl Find graphical privilege escalation program
dnl======================
dnl Check for pkexec >= 0.102 for it's ability to run X11 apps.
AM_CONDITIONAL([INSTALL_POLKIT_ACTIONS], false)
PKEXEC_REQUIRED_VERSION='0.102'
AC_MSG_CHECKING([for pkexec >= $PKEXEC_REQUIRED_VERSION])
PKEXEC_REQUIRED_INT=`echo "$PKEXEC_REQUIRED_VERSION" | $AWK -F. '{print $1 * 10000 + $2}'`
PKEXEC_VERSION_OUTPUT=`pkexec --version 2> /dev/null` ||
	AC_MSG_RESULT([not found])
if test "x$PKEXEC_VERSION_OUTPUT" != 'x'; then
	PKEXEC_FOUND_VERSION=`echo "$PKEXEC_VERSION_OUTPUT" | head -1 | cut -d' ' -f3`
	PKEXEC_FOUND_INT=`echo "$PKEXEC_FOUND_VERSION" | $AWK -F. '{print $1 * 10000 + $2}'`
	AC_MSG_RESULT([$PKEXEC_FOUND_VERSION found])
	if test "$PKEXEC_FOUND_INT" -ge "$PKEXEC_REQUIRED_INT"; then
		GKSUPROG='pkexec --disable-internal-agent'
		AC_SUBST([GKSUPROG])
		AM_CONDITIONAL([INSTALL_POLKIT_ACTIONS], true)
	fi
fi

dnl Check for alternative graphical privilege escalation programs.
if test "x$GKSUPROG" = 'x'; then
	AC_CHECK_PROGS([GKSUPROG], [gksudo gksu kdesudo "xdg-su -c"], [])
fi


dnl======================
dnl i18n stuff
dnl======================
GETTEXT_PACKAGE=gparted
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], "$GETTEXT_PACKAGE", [description])

AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.36.0])


dnl======================
dnl checks for libs
dnl======================
AC_CHECK_LIB([uuid], [uuid_generate], [], AC_MSG_ERROR([*** libuuid not found.]))
AC_CHECK_LIB([dl], [dlopen], [], AC_MSG_ERROR([*** libdl not found.]))
AC_CHECK_LIB([parted], [ped_device_read], [], AC_MSG_ERROR([*** libparted not found.]))


dnl Check for minimum required libparted version.
dnl 1) Check using pkg-config.
dnl    (Older distros tend to not provide pkg-config information for libparted).
dnl 2) Check by linking and running a program to report libparted version directly.
LIBPARTED_REQUIRED_VERSION='1.7.1'
AC_MSG_CHECKING([for libparted >= $LIBPARTED_REQUIRED_VERSION (querying pkg-config)])
LIBPARTED_REQUIRED_INT=`echo "$LIBPARTED_REQUIRED_VERSION" |
	$AWK -F. '{print $1 * 1000000 + $2 * 10000 + $3}'`
dnl 1) Check using pkg-config.
PKG_CHECK_EXISTS(
	[libparted],
	[LIBPARTED_FOUND_VERSION=`$PKG_CONFIG --modversion libparted`
	 LIBPARTED_FOUND_INT=`echo "$LIBPARTED_FOUND_VERSION" |
		$AWK -F. '{print $1 * 1000000 + $2 * 10000 + $3}'`
	 AC_MSG_RESULT([$LIBPARTED_FOUND_VERSION])
	 test "$LIBPARTED_FOUND_INT" -ge "$LIBPARTED_REQUIRED_INT" ||
		AC_MSG_ERROR([*** libparted too old.  Require libparted >= $LIBPARTED_REQUIRED_VERSION but only found libparted $LIBPARTED_FOUND_VERSION.])
	],
	[AC_MSG_RESULT([not found])
	 dnl 2) Check by linking and running a program to report libparted version
	 dnl    directly.
	 AC_MSG_CHECKING([for libparted >= $LIBPARTED_REQUIRED_VERSION (querying libparted)])
	 AC_RUN_IFELSE(
		[AC_LANG_SOURCE(
			[[
#include <stdio.h>
#include <stdlib.h>
#include <parted/parted.h>

int main()
{
	const char *version = ped_get_version();
	if (version == NULL)
	{
		fprintf(stderr, "ERROR: ped_get_version() returned NULL\n");
		return EXIT_FAILURE;
	}
	printf("%s\n", version);
	return EXIT_SUCCESS;
}
			]]
		)],
		dnl Run test program again to cache libparted version.
		[LIBPARTED_FOUND_VERSION=`./conftest$EXEEXT`
		 LIBPARTED_FOUND_INT=`echo "$LIBPARTED_FOUND_VERSION" |
			$AWK -F. '{print $1 * 1000000 + $2 * 10000 + $3}'`
		 test "$LIBPARTED_FOUND_INT" -ge "$LIBPARTED_REQUIRED_INT" ||
			AC_MSG_ERROR([*** libparted too old.  Require libparted >= $LIBPARTED_REQUIRED_VERSION but only found libparted $LIBPARTED_FOUND_VERSION.])
		],
		[AC_MSG_ERROR([*** Error querying libparted version.  Check config.log for details.])]
	)]
)


dnl Check for libparted 2.0 to 3.0 inclusive for a bug where loop table
dnl creation doesn't delete old partitions.
LIBPARTED_MIN_WANTED_VERSION='2.0'
LIBPARTED_MAX_WANTED_VERSION='3.0'
AC_MSG_CHECKING([for $LIBPARTED_MIN_WANTED_VERSION <= libparted <= $LIBPARTED_MAX_WANTED_VERSION (loop table creation doesn't delete old partitions)])
LIBPARTED_MIN_WANTED_INT=`echo "$LIBPARTED_MIN_WANTED_VERSION" |
	$AWK -F. '{print $1 * 1000000 + $2 * 10000 + $3}'`
LIBPARTED_MAX_WANTED_INT=`echo "$LIBPARTED_MAX_WANTED_VERSION" |
	$AWK -F. '{print $1 * 1000000 + $2 * 10000 + $3}'`
if test "$LIBPARTED_MIN_WANTED_INT" -le "$LIBPARTED_FOUND_INT" -a \
        "$LIBPARTED_FOUND_INT" -le "$LIBPARTED_MAX_WANTED_INT"; then
	need_loop_delete_old_ptns_workaround=yes
	AC_DEFINE([ENABLE_LOOP_DELETE_OLD_PTNS_WORKAROUND], 1,
	          [Define to 1 to enable deletion of old partitions before creating a loop table workaround])
	AC_MSG_RESULT([(cached) yes])
else
	need_loop_delete_old_ptns_workaround=no
	AC_MSG_RESULT([(cached) no])
fi


dnl Check for libparted >= 2.2 for improved informing the kernel to
dnl re-read the partition table code and support of larger sector sizes
dnl (> 512 bytes).
LIBPARTED_WANTED_VERSION='2.2'
AC_MSG_CHECKING([for libparted >= $LIBPARTED_WANTED_VERSION (improved pt re-read)])
LIBPARTED_WANTED_INT=`echo "$LIBPARTED_WANTED_VERSION" |
	$AWK -F. '{print $1 * 1000000 + $2 * 10000 + $3}'`
if test "$LIBPARTED_FOUND_INT" -ge "$LIBPARTED_WANTED_INT"; then
	AC_DEFINE([USE_LIBPARTED_LARGE_SECTOR_SUPPORT], 1,
	          [Define to 1 to use libparted large sector support])
	need_pt_reread_workaround=no; support_large_sector_sizes=yes
	AC_MSG_RESULT([(cached) yes])
else
	AC_DEFINE([ENABLE_PT_REREAD_WORKAROUND], 1,
	          [Define to 1 to enable partition re-read workaround])
	need_pt_reread_workaround=yes; support_large_sector_sizes=no
	AC_MSG_RESULT([(cached) no])
fi


dnl Check for ped_file_system_resize() function to determine the existence
dnl of the API in the original parted library.  Available in parted <= 2.4.
dnl
dnl NOTE:
dnl For AC_CHECK_LIB the default action-if-found ($3) includes extending
dnl LIBS with the newly found library ($1) thus:
dnl     LIBS="-l$1 $LIBS"
dnl
dnl As the default action-if-found is overridden, LIBS isn't extended so
dnl saving and restoring LIBS isn't required.
have_old_lp_fs_resize_api=no
AC_CHECK_LIB(
	[parted], [ped_file_system_resize],
	[have_old_lp_fs_resize_api=yes]
)


dnl If not already found, check for ped_file_system_resize() function in
dnl the parted-fs-resize library to determine the need to use the new
dnl library.  Available in parted >= 3.1.
have_new_lp_fs_resize_lib=no
AS_IF(
	[test "x$have_old_lp_fs_resize_api" != xyes],
	[AC_CHECK_LIB(
		[parted-fs-resize], [ped_file_system_resize],
		[have_new_lp_fs_resize_lib=yes
		 LIBS="-lparted-fs-resize $LIBS"
		]
	)]
)


dnl Check for libparted >= 3.2 for online resize support.
LIBPARTED_WANTED_VERSION='3.2'
AC_MSG_CHECKING([for libparted >= $LIBPARTED_WANTED_VERSION (online resize)])
LIBPARTED_WANTED_INT=`echo "$LIBPARTED_WANTED_VERSION" |
	$AWK -F. '{print $1 * 1000000 + $2 * 10000 + $3}'`
if test "$LIBPARTED_FOUND_INT" -ge "$LIBPARTED_WANTED_INT"; then
	have_online_resize=yes
	AC_MSG_RESULT([(cached) yes])
else
	have_online_resize=no
	AC_MSG_RESULT([(cached) no])
fi


dnl Check if have libparted fs resize capability
if test [ ${have_old_lp_fs_resize_api} = yes -o ${have_new_lp_fs_resize_lib} = yes ]; then
	AC_DEFINE([HAVE_LIBPARTED_FS_RESIZE], [1], [Define to 1 if have libparted fs resize capability])
fi


dnl gthread
PKG_CHECK_MODULES([GTHREAD], [gthread-2.0])
AC_SUBST([GTHREAD_LIBS])
AC_SUBST([GTHREAD_CFLAGS])

dnl GTKMM
PKG_CHECK_MODULES([GTKMM], [gtkmm-2.4 > 2.8])
AC_SUBST([GTKMM_LIBS])
AC_SUBST([GTKMM_CFLAGS])


dnl Check for glibmm >= 2.14 to determine availability of Glib::Regex class
AC_MSG_CHECKING([for Glib::Regex class])
PKG_CHECK_EXISTS(
	[glibmm-2.4 >= 2.14.0],
	[AC_DEFINE([HAVE_GLIB_REGEX], 1, [Define to 1 if glibmm provides Glib::Regex class.])
	 AC_MSG_RESULT([yes])
	],
	[AC_MSG_RESULT([no])]
)


dnl Check for gtkmm >= 2.11.1 to determine availability of Gtk::Window::set_default_icon_name()
dnl NOTE:
dnl The documentation says that set_default_icon_name() is available in gtkmm >= 2.6,
dnl however the code reveals that it is only available in gtkmm >= 2.11.1.
dnl *   gtkmm GTK::Window Class Reference
dnl     https://developer.gnome.org/gtkmm/3.6/classGtk_1_1Window.html#a533d03e9b92d8ccd142ab3a44005cae4
dnl *   gtkmm NEWS file
dnl     https://git.gnome.org/browse/gtkmm/tree/NEWS?h=gtkmm-2.14.0#n565
AC_MSG_CHECKING([for Gtk::Window::set_default_icon_name() method])
PKG_CHECK_EXISTS(
	[gtkmm-2.4 >= 2.11.1],
	[AC_DEFINE([HAVE_SET_DEFAULT_ICON_NAME], 1,
	           [Define to 1 if gtkmm-2.4 provides Gtk::Window::set_default_icon_name() method.])
	 AM_CONDITIONAL([INSTALL_PIXMAPS_DIR], false)
	 AC_MSG_RESULT([yes])
	],
	[AM_CONDITIONAL([INSTALL_PIXMAPS_DIR], true)
	 AC_MSG_RESULT([no])
        ]
)


dnl GTKMM 2.16 needed for gtk_show_uri()
AC_MSG_CHECKING([for gtk_show_uri function])
PKG_CHECK_EXISTS(
	[gtkmm-2.4 >= 2.16.0],
	[AC_DEFINE([HAVE_GTK_SHOW_URI], 1, [Define to 1 if gtkmm provides gtk_show_uri() function.])
	 AC_MSG_RESULT([yes])
	],
	[AC_MSG_RESULT([no])]
)


dnl Check for gtkmm >= 2.22 to determine availability of Gtk::MessageDialog::get_message_area().
AC_MSG_CHECKING([for Gtk::MessageDialog::get_message_area() method])
PKG_CHECK_EXISTS(
	[gtkmm-2.4 >= 2.22.0],
	[AC_DEFINE([HAVE_GET_MESSAGE_AREA], 1,
	           [Define to 1 if gtkmm provides Gtk::MessageDialog::get_message_area() method.])
	 AC_MSG_RESULT([yes])
	],
	[AC_MSG_RESULT([no])]
)


need_cxx_compile_stdcxx_11=no
dnl Check for glibmm >= 2.45.40 which requires C++11 compilation.
AC_MSG_CHECKING([for glibmm >= 2.45.40 which requires C++11 compilation])
PKG_CHECK_EXISTS(
	[glibmm-2.4 >= 2.45.40],
	[need_cxx_compile_stdcxx_11=yes
	 AC_MSG_RESULT([yes])
	],
	[AC_MSG_RESULT([no])]
)
dnl Check for libsigc++ >= 2.5.1 which requires C++11 compilation.
AC_MSG_CHECKING([for libsigc++ >= 2.5.1 which requires C++11 compilation])
PKG_CHECK_EXISTS(
	[sigc++-2.0 >= 2.5.1],
	[need_cxx_compile_stdcxx_11=yes
	 AC_MSG_RESULT([yes])
	],
	[AC_MSG_RESULT([no])]
)
dnl Enable C++11 compilation only if required.
if test "x$need_cxx_compile_stdcxx_11" = xyes; then
	AX_CXX_COMPILE_STDCXX_11()
fi



dnl Definitions for building of and with gtest.  Gets flags for pthread via earlier
dnl gthread package check.
GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"
GTEST_CXXFLAGS="$GTHREAD_CFLAGS $GTKMM_CFLAGS"
GTEST_LDFLAGS=
GTEST_LIBS="$GTHREAD_LIBS $GTKMM_LIBS"
AC_SUBST([GTEST_CPPFLAGS])
AC_SUBST([GTEST_CXXFLAGS])
AC_SUBST([GTEST_LDFLAGS])
AC_SUBST([GTEST_LIBS])


dnl======================
dnl check whether to build documentation - gnome-doc-utils
dnl======================
AC_ARG_ENABLE(
	[doc],
	AS_HELP_STRING(
		[--disable-doc],
		dnl Autoconf quadrigraphs '@<:@' = '[' and '@:>@' = ']'
		[disable building help documentation @<:@default=enabled@:>@]),
	[enable_doc=$enableval],
	[enable_doc=yes]
)

AC_MSG_CHECKING([whether help documentation should be built])
if test "x$enable_doc" = xyes; then
	AC_DEFINE([ENABLE_HELP_DOC], [1], [Define to 1 when help documentation is built])
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi

if test "x$enable_doc" = xyes; then
	GNOME_DOC_INIT
else
	dnl Do not care if GDU is not found
	GNOME_DOC_INIT(,,[:])
fi

AM_CONDITIONAL([BUILD_HELP_DOC], [test "x$enable_doc" = xyes])

dnl GNOME 2 yelp needs scrollkeeper database updates to find installed GNOME help files.
dnl GNOME 3 yelp doesn't need scrollkeeper and its use is deprecated.  To avoid requiring
dnl deprecated scrollkeeper on GNOME 3, automatically disable scrollkeeper database
dnl updates when the command is not available.
dnl (See /usr/share/aclocal/gnome-doc-utils.m4 for the GNOME_DOC_INIT macro which has
dnl already parsed the --disable-scrollkeeper option and set enable_scrollkeeper and
dnl ENABLE_SK).
AC_CHECK_PROG([have_scrollkeeper_update], [scrollkeeper-update], yes, no)
AC_MSG_CHECKING([whether to enable scrollkeeper database update])
if test "$enable_scrollkeeper" = "yes" -a "$have_scrollkeeper_update" = "yes"; then
	AC_MSG_RESULT([yes])
else
	enable_scrollkeeper=no
	AM_CONDITIONAL([ENABLE_SK], false)
	AC_MSG_RESULT([no])
fi


dnl======================
dnl check whether to use native libparted dmraid support
dnl======================
AC_ARG_ENABLE(
	[libparted_dmraid],
	AS_HELP_STRING(
		[--enable-libparted-dmraid],
		[use native libparted /dev/mapper dmraid support @<:@default=disabled@:>@]),
	[enable_libparted_dmraid=$enableval],
	[enable_libparted_dmraid=no]
)

AC_MSG_CHECKING([whether to use native libparted /dev/mapper dmraid support])
if test "x$enable_libparted_dmraid" = xyes; then
	AC_DEFINE([USE_LIBPARTED_DMRAID], [1],
	          [Define to 1 to use native libparted /dev/mapper dmraid support])
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi


dnl======================
dnl check whether to enable online resize support
dnl======================
AC_ARG_ENABLE(
	[online-resize],
	AS_HELP_STRING(
		[--enable-online-resize],
		[enable online resize support @<:@default=auto@:>@]),
	[enable_online_resize=$enableval],
	[enable_online_resize=$have_online_resize]
)

AC_MSG_CHECKING([whether online resize support is enabled])
if test "x$enable_online_resize" = xyes; then
	AC_DEFINE([ENABLE_ONLINE_RESIZE], [1], [Define to 1 if online resize is enabled])
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi


dnl Check whether to explicitly grant root access to the display.
AC_ARG_ENABLE(
	[xhost-root],
	AS_HELP_STRING(
		[--enable-xhost-root],
		[enable explicitly granting root access to the display @<:@default=disabled@:>@]),
	[enable_xhost_root=$enableval],
	[enable_xhost_root=no]
)

AC_MSG_CHECKING([whether to explicitly grant root access to the display])
if test "x$enable_xhost_root" = 'xyes'; then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi
AC_SUBST([ENABLE_XHOST_ROOT], $enable_xhost_root)


AC_CONFIG_FILES([
Makefile
compose/Makefile
data/Makefile
data/icons/Makefile
doc/Makefile
help/Makefile
include/Makefile
lib/Makefile
lib/gtest/Makefile
src/Makefile
po/Makefile.in
tests/Makefile
])

AC_OUTPUT


dnl======================
dnl Summary
dnl======================

echo ""
echo "======================== Final configuration ==========================="
echo "                         Installing into prefix  :  $prefix"
echo ""
echo "                      Build help documentation?  :  $enable_doc"
echo "                  Update scrollkeeper database?  :  $enable_scrollkeeper"
echo ""
echo "           Use native libparted dmraid support?  :  $enable_libparted_dmraid"
echo ""
echo "   Explicitly grant root access to the display?  :  $enable_xhost_root"
echo ""
echo "       --- Features Based On Libparted Version ---"
echo "       Need delete old partitions before"
echo "              creating a loop table workaround?  :  $need_loop_delete_old_ptns_workaround"
echo "       Need partition table re-read workaround?  :  $need_pt_reread_workaround"
echo "     Supports large sector sizes (> 512 bytes)?  :  $support_large_sector_sizes"
echo "   Have old libparted file system resizing API?  :  $have_old_lp_fs_resize_api"
echo "   Have new libparted file system resizing LIB?  :  $have_new_lp_fs_resize_lib"
echo "                  Enable online resize support?  :  $enable_online_resize"
echo ""
echo " If all settings are OK, type make and then (as root) make install"
echo "========================================================================"
