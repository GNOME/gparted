AC_INIT([gparted],[0.19.0-git],[https://bugzilla.gnome.org/enter_bug.cgi?product=gparted])
AC_PREREQ([2.50])

AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-bzip2])
AM_MAINTAINER_MODE


dnl======================
dnl checks for programs
dnl======================
AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL


dnl======================
dnl checks for other programs
dnl======================
AC_CHECK_PROGS([GKSUPROG], [gksudo gksu kdesudo "xdg-su -c"], [])


dnl======================
dnl i18n stuff
dnl======================
GETTEXT_PACKAGE=gparted
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], "$GETTEXT_PACKAGE", [description])

AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.5])


dnl======================
dnl checks for libs
dnl======================
AC_CHECK_LIB([uuid], [uuid_generate], [], AC_MSG_ERROR([*** uuid library (libuuid) not found]))
AC_CHECK_LIB([dl], [dlopen], [], AC_MSG_ERROR([*** dl library (libdl) not found]))


dnl Check for minimum required libparted version
LIBPARTED_VERSION=1.7.1
AC_MSG_CHECKING([for libparted >= $LIBPARTED_VERSION])
LIBS_save="$LIBS"
LIBS="-lparted -luuid -ldl"
AC_RUN_IFELSE(
	[AC_LANG_SOURCE(
		[[
#include <stdio.h>
#include <parted/parted.h>

int main ()
{
	int min_major = 0;
	int min_minor = 0;
	int min_micro = 0;
	int major = 0;
	int minor = 0;
	int micro = 0;

	if ( ( sscanf( "$LIBPARTED_VERSION", "%d.%d.%d", &min_major, &min_minor, &min_micro ) == 3 ) ||
	     ( sscanf( "$LIBPARTED_VERSION", "%d.%d", &min_major, &min_minor ) == 2 ) ||
	     ( sscanf( "$LIBPARTED_VERSION", "%d", &min_major ) == 1 )
	   )
	{
		if ( ( sscanf( ped_get_version(), "%d.%d.%d", &major, &minor, &micro ) == 3 ) ||
		     ( sscanf( ped_get_version(), "%d.%d", &major, &minor ) == 2 ) ||
		     ( sscanf( ped_get_version(), "%d", &major ) == 1 )
		   )
		{
			printf( "Found libparted %s\t", ped_get_version() ) ;
			return ! ( (major > min_major) ||
			           ( (major == min_major) && (minor > min_minor) ) ||
			           ( (major == min_major) && (minor == min_minor) && (micro >= min_micro) )
			         ) ;
		}
	}

	return 1 ;
}
		]]
	)],
	[AC_MSG_RESULT([OK])],
	[AC_MSG_ERROR([*** Requires libparted >= $LIBPARTED_VERSION.  Perhaps development header files missing?])]
)
LIBS="$LIBS_save"


dnl======================
dnl check whether libparted >= 2.2 (has improved partition table re-read code)
dnl======================
LIBPARTED_VERSION=2.2
AC_MSG_CHECKING([if libparted >= $LIBPARTED_VERSION (has improved pt re-read)])
LIBS_save="$LIBS"
LIBS="-lparted -luuid -ldl"
need_work_around=yes
AC_RUN_IFELSE(
	[AC_LANG_SOURCE(
		[[
#include <stdio.h>
#include <parted/parted.h>

int main ()
{
	int min_major = 0;
	int min_minor = 0;
	int min_micro = 0;
	int major = 0;
	int minor = 0;
	int micro = 0;

	if ( ( sscanf( "$LIBPARTED_VERSION", "%d.%d.%d", &min_major, &min_minor, &min_micro ) == 3 ) ||
	     ( sscanf( "$LIBPARTED_VERSION", "%d.%d", &min_major, &min_minor ) == 2 ) ||
	     ( sscanf( "$LIBPARTED_VERSION", "%d", &min_major ) == 1 )
	   )
	{
		if ( ( sscanf( ped_get_version(), "%d.%d.%d", &major, &minor, &micro ) == 3 ) ||
		     ( sscanf( ped_get_version(), "%d.%d", &major, &minor ) == 2 ) ||
		     ( sscanf( ped_get_version(), "%d", &major ) == 1 )
		   )
		{
			return ! ( (major > min_major) ||
			           ( (major == min_major) && (minor > min_minor) ) ||
			           ( (major == min_major) && (minor == min_minor) && (micro >= min_micro) )
			         ) ;
		}
	}

	return 1 ;
}
		]]
	)],
	[AC_DEFINE([HAVE_LIBPARTED_2_2_0_PLUS], [1],
	           [Define to 1 if libparted contains improved partition table re-read code])
	 need_pt_reread_work_around=no; support_sector_size_gt_512=yes
	 AC_MSG_RESULT([yes])
	],
	[need_pt_reread_work_around=yes; support_sector_size_gt_512=no
	 AC_MSG_RESULT([no])
	]
)
LIBS="$LIBS_save"


dnl======================
dnl check whether libparted >= 3.0 (libparted is missing file system resizing API)
dnl======================
LIBPARTED_VERSION=3.0
AC_MSG_CHECKING([if libparted >= $LIBPARTED_VERSION (libparted is missing file system resizing API)])
LIBS_save="$LIBS"
LIBS="-lparted -luuid -ldl"
need_work_around=yes
AC_RUN_IFELSE(
	[AC_LANG_SOURCE(
		[[
#include <stdio.h>
#include <parted/parted.h>

int main ()
{
	int min_major = 0;
	int min_minor = 0;
	int min_micro = 0;
	int major = 0;
	int minor = 0;
	int micro = 0;

	if ( ( sscanf( "$LIBPARTED_VERSION", "%d.%d.%d", &min_major, &min_minor, &min_micro ) == 3 ) ||
	     ( sscanf( "$LIBPARTED_VERSION", "%d.%d", &min_major, &min_minor ) == 2 ) ||
	     ( sscanf( "$LIBPARTED_VERSION", "%d", &min_major ) == 1 )
	   )
	{
		if ( ( sscanf( ped_get_version(), "%d.%d.%d", &major, &minor, &micro ) == 3 ) ||
		     ( sscanf( ped_get_version(), "%d.%d", &major, &minor ) == 2 ) ||
		     ( sscanf( ped_get_version(), "%d", &major ) == 1 )
		   )
		{
			return ! ( (major > min_major) ||
			           ( (major == min_major) && (minor > min_minor) ) ||
			           ( (major == min_major) && (minor == min_minor) && (micro >= min_micro) )
			         ) ;
		}
	}

	return 1 ;
}
		]]
	)],
	[AC_DEFINE([HAVE_LIBPARTED_3_0_0_PLUS], [1], [Define to 1 if libparted >= 3.0])
	 have_old_lp_fs_resize_api=no
	 AC_MSG_RESULT([yes])
	],
	[have_old_lp_fs_resize_api=yes
	 AC_MSG_RESULT([no])
	]
)
LIBS="$LIBS_save"


dnl======================
dnl check whether libparted >= 3.1 (libparted has new file system resizing LIB)
dnl======================
LIBPARTED_VERSION=3.1
AC_MSG_CHECKING([if libparted >= $LIBPARTED_VERSION (libparted has new file system resizing LIB)])
LIBS_save="$LIBS"
LIBS="-lparted -luuid -ldl"
need_work_around=yes
AC_RUN_IFELSE(
	[AC_LANG_SOURCE(
		[[
#include <stdio.h>
#include <parted/parted.h>

int main ()
{
	int min_major = 0;
	int min_minor = 0;
	int min_micro = 0;
	int major = 0;
	int minor = 0;
	int micro = 0;

	if ( ( sscanf( "$LIBPARTED_VERSION", "%d.%d.%d", &min_major, &min_minor, &min_micro ) == 3 ) ||
	     ( sscanf( "$LIBPARTED_VERSION", "%d.%d", &min_major, &min_minor ) == 2 ) ||
	     ( sscanf( "$LIBPARTED_VERSION", "%d", &min_major ) == 1 )
	   )
	{
		if ( ( sscanf( ped_get_version(), "%d.%d.%d", &major, &minor, &micro ) == 3 ) ||
		     ( sscanf( ped_get_version(), "%d.%d", &major, &minor ) == 2 ) ||
		     ( sscanf( ped_get_version(), "%d", &major ) == 1 )
		   )
		{
			return ! ( (major > min_major) ||
			           ( (major == min_major) && (minor > min_minor) ) ||
			           ( (major == min_major) && (minor == min_minor) && (micro >= min_micro) )
			         ) ;
		}
	}

	return 1 ;
}
		]]
	)],
	[AC_DEFINE([HAVE_LIBPARTED_3_1_0_PLUS], [1], [Define to 1 if libparted >= 3.1])
	 have_new_lp_fs_resize_lib=yes
	 AC_MSG_RESULT([yes])
	],
	[have_new_lp_fs_resize_lib=no
	 AC_MSG_RESULT([no])
	]
)
LIBS="$LIBS_save"


dnl Check for availability of libparted fs resize library
AM_CONDITIONAL([USE_LIBPARTED_FS_RESIZE_LIBRARY], [ test ${have_new_lp_fs_resize_lib} = yes])


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


dnl Check for Gtk::Window::set_default_icon_name() method
dnl NOTE:
dnl The documentation says that set_default_icon_name() is available in gtkmm >= 2.6.
dnl     gtkmm GTK::Window Class Reference
dnl     https://developer.gnome.org/gtkmm/3.6/classGtk_1_1Window.html#a533d03e9b92d8ccd142ab3a44005cae4
dnl However it is not available in RHEL/CentOS 5.9 with gtkmm 2.10.  Therefore a package
dnl version check isn't sufficient.  Instead perform a more accurate, but slower, compile
dnl and link a test program check.
AC_LANG_PUSH([C++])
CXXFLAGS_save="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS `pkg-config gtkmm-2.4 --cflags`"
LIBS_save="$LIBS"
LIBS="`pkg-config gtkmm-2.4 --libs`"
AC_MSG_CHECKING([for Gtk::Window::set_default_icon_name method])
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <gtkmm.h>]],
		[[Gtk::Window mywindow;
		  mywindow.set_default_icon_name("myappname");
		]]
	)],
	[AC_DEFINE([HAVE_SET_DEFAULT_ICON_NAME], 1,
	           [Define to 1 if gtkmm-2.4 provides Gtk::Window::set_default_icon_name() method.])
	 AM_CONDITIONAL([INSTALL_PIXMAPS_DIR], false)
	 AC_MSG_RESULT([yes])
	],
	[AM_CONDITIONAL([INSTALL_PIXMAPS_DIR], true)
	 AC_MSG_RESULT([no])
        ]
)
CXXFLAGS="$CXXFLAGS_save"
LIBS="$LIBS_save"
AC_LANG_POP([C++])


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


dnl======================
dnl check whether to build documentation - Gnome-Doc-Utils
dnl======================
AC_ARG_ENABLE([doc],
  [  --disable-doc           do not build documentation],,)
if test "x${enable_doc}" = "x" ; then
	enable_doc=yes
fi

AC_MSG_CHECKING([whether documentation should be built])
if test ${enable_doc} = no; then
	AC_MSG_RESULT([no])
else
	AC_MSG_RESULT([yes])
fi

if test ${enable_doc} = yes; then
	GNOME_DOC_INIT
else
	dnl Do not care if GDU is not found
	GNOME_DOC_INIT(,,[:])
	dnl Set   #define HAVE_DISABLE_DOC 1   to indicate documentation not wanted
	AC_DEFINE([HAVE_DISABLE_DOC], [1], [Define to 1 if --disable-doc specified])
fi

AM_CONDITIONAL([DISABLE_DOC], [test ${enable_doc} = no])


dnl======================
dnl check whether to use native libparted dmraid support
dnl======================
AC_ARG_ENABLE([libparted_dmraid],
  [  --enable-libparted-dmraid   do use native libparted /dev/mapper dmraid support],,)
if test "x${enable_libparted_dmraid}" = "x" ; then
	enable_libparted_dmraid=no
fi

AC_MSG_CHECKING([whether native libparted /dev/mapper dmraid support should be used])
if test ${enable_libparted_dmraid} = yes; then
	AC_MSG_RESULT([yes])
	dnl Set   #define USE_LIBPARTED_DMRAID 1   to indicate documentation not wanted
	AC_DEFINE([USE_LIBPARTED_DMRAID], [1], [Define to 1 if --enable-libparted-dmraid specified])
else
	AC_MSG_RESULT([no])
fi

dnl======================
dnl check whether to enable online resize support
dnl======================
AC_ARG_ENABLE([online_resize],
  [  --enable-online-resize   enable online resize support],,)
if test "x${enable_online_resize}" = "x" ; then
	enable_online_resize=no
fi

AC_MSG_CHECKING([whether online resize support is enabled])
if test ${enable_online_resize} = yes; then
	AC_MSG_RESULT([yes])
	AC_DEFINE([ENABLE_ONLINE_RESIZE], [1], [Define to 1 if --enable-online-resize specified])
else
	AC_MSG_RESULT([no])
fi

AC_CONFIG_FILES([
Makefile
compose/Makefile
data/Makefile
data/icons/Makefile
doc/Makefile
help/Makefile
include/Makefile
src/Makefile
po/Makefile.in
])

AC_OUTPUT


dnl======================
dnl Summary
dnl======================

echo ""
echo "======================== Final configuration ==========================="
echo "                         Installing into prefix  :  $prefix"
echo ""
echo "                           Build documentation?  :  $enable_doc"
echo ""
echo "           Use native libparted dmraid support?  :  $enable_libparted_dmraid"
echo "                  Enable online resize support?  :  $enable_online_resize"
echo ""
echo "       --- Features Based On Libparted Version ---"
echo "      Need partition table re-read work around?  :  $need_pt_reread_work_around"
echo "             Supports sector sizes > 512 bytes?  :  $support_sector_size_gt_512"
echo "   Have old libparted file system resizing API?  :  $have_old_lp_fs_resize_api"
echo "   Have new libparted file system resizing LIB?  :  $have_new_lp_fs_resize_lib"
echo ""
echo " If all settings are OK, type make and then (as root) make install"
echo "========================================================================"
