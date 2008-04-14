SUBDIRS = compose doc include pixmaps po src

@INTLTOOL_DESKTOP_RULE@

DESKTOP_IN_FILES = gparted.desktop.in
DESKTOP_FILES =$(DESKTOP_IN_FILES:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)

EXTRA_DIST =			\
	$(DESKTOP_IN_FILES)	\
	$(DESKTOP_FILES)	\
	intltool-extract.in  	\
	intltool-merge.in	\
	intltool-update.in

DISTCLEANFILES =		\
	intltool-extract	\
	intltool-merge		\
	intltool-update		\
	$(desktop_DATA)
