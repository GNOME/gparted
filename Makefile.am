SUBDIRS = compose doc include pixmaps po src

@INTLTOOL_DESKTOP_RULE@

DESKTOP_IN_IN_FILES = gparted.desktop.in.in
DESKTOP_IN_FILES = gparted.desktop.in
DESKTOP_FILES =$(DESKTOP_IN_FILES:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)

sbin_SCRIPTS = gparted
CLEANFILES = $(sbin_SCRIPTS) $(DESKTOP_IN_FILES)

do_subst = sed -e 's,[@]installdir[@],$(sbindir),g' \
	-e 's,[@]gksuprog[@],$(GKSUPROG),g'

gparted.desktop.in:  gparted.desktop.in.in Makefile
	$(do_subst) < $(srcdir)/gparted.desktop.in.in > gparted.desktop.in

gparted:  gparted.in Makefile
	$(do_subst) < $(srcdir)/gparted.in > gparted
	chmod +x gparted

EXTRA_DIST =			\
	$(DESKTOP_IN_IN_FILES)	\
	$(DESKTOP_FILES)	\
	intltool-extract.in  	\
	intltool-merge.in	\
	intltool-update.in	\
	gparted.in

DISTCLEANFILES =		\
	intltool-extract	\
	intltool-merge		\
	intltool-update		\
	$(desktop_DATA)
