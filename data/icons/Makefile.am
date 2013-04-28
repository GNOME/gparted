NULL =

public_icons_themes = \
	hicolor	\
	$(NULL)

public_icons = \
	hicolor_apps_16x16_gparted.png \
	hicolor_apps_22x22_gparted.png \
	hicolor_apps_24x24_gparted.png \
	hicolor_apps_32x32_gparted.png \
	hicolor_apps_48x48_gparted.png \
	hicolor_apps_scalable_gparted.svg \
	$(NULL)

private_icons = \
	$(NULL)

# Name of the fallback icon to install in /usr/share/pixmaps [1]
# --
# The GNOME 2 desktop wants a 16x16 "Menu" type icon [2] for the title
# bar and the open window list, and a 32x32 icon for the Alt-Tab task
# switcher.  GParted only specifies a single icon file in the fallback
# case so just use the 16x16 one and let it be scaled as required.
# [1] https://developer.gnome.org/icon-theme-spec/
# [2] https://developer.gnome.org/hig-book/2.32/hig-book.html#icons
pixmap = hicolor_apps_16x16_gparted.png

noinst_DATA = \
	$(NULL)

EXTRA_DIST = \
	$(public_icons)		\
	$(private_icons)	\
	$(pixmap)		\
	$(noinst_DATA)		\
	$(NULL)

# Only install fallback icon to /usr/share/pixmaps if required
if INSTALL_PIXMAPS_DIR
INSTALL_PIXMAP   = install-pixmap
UNINSTALL_PIXMAP = uninstall-pixmap
endif

###############################################################################

gtk_update_icon_cache = gtk-update-icon-cache -f -t

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		for theme in $(public_icons_themes); do \
			$(gtk_update_icon_cache) $(datadir)/icons/$$theme; \
		done; \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		for theme in $(public_icons_themes); do \
			echo "***   $(gtk_update_icon_cache) $(datadir)/icons/$$theme"; \
		done; \
	fi

install-icons:
	for icon in $(public_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		mkdir -p $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done; \
	for icon in $(private_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		mkdir -p $(DESTDIR)$(pkgdatadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(pkgdatadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done

uninstall-icons:
	-for icon in $(public_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		rm -f $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done; \
	for icon in $(private_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		rm -f $(DESTDIR)$(pkgdatadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done

install-pixmap:
	mkdir -p $(DESTDIR)$(datadir)/pixmaps; \
	$(INSTALL_DATA) $(srcdir)/$(pixmap) $(DESTDIR)$(datadir)/pixmaps/gparted.png

uninstall-pixmap:
	rm -f $(DESTDIR)$(datadir)/pixmaps/gparted.png

install-data-local: install-icons $(INSTALL_PIXMAP) update-icon-cache

uninstall-local: uninstall-icons $(UNINSTALL_PIXMAP) update-icon-cache
