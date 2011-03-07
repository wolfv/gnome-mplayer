## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po icons

gnome_mplayerdocdir = ${docdir}
gnome_mplayerdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS \
	DOCS/keyboard_shortcuts.txt \
	DOCS/tech/dbus.txt \
	DOCS/tech/plugin-interaction.txt

if GCONF_SCHEMAS_INSTALL
schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = gnome-mplayer.schemas
endif

desktopdir = ${prefix}/share/applications
desktop_DATA = gnome-mplayer.desktop

prefappdir = ${prefix}/share/gnome-control-center/default-apps
prefapp_DATA = gnome-mplayer.xml


manpagedir = @mandir@/man1
manpage_DATA = gnome-mplayer.1

EXTRA_DIST = $(gnome_mplayerdoc_DATA) $(desktop_DATA) $(schema_DATA) $(manpage_DATA) $(prefapp_DATA) gnome-mplayer.spec.in config.h.in

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		$(gtk_update_icon_cache); \
	fi

install-schemas: $(schema_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) gconftool-2 --makefile-install-rule $(schema_DATA) 2>&1 > /dev/null

if GCONF_SCHEMAS_INSTALL
install-data-hook: install-schemas update-icon-cache
else
install-data-hook: update-icon-cache
endif

gsettings_ENUM_NAMESPACE = apps.gnome-mplayer.preferences
gsettings_ENUM_FILES = $(top_srcdir)/src/*.h
gsettings_SCHEMAS = apps.gnome-mplayer.preferences.gschema.xml apps.gecko-mediaplayer.preferences.gschema.xml

@GSETTINGS_RULES@

