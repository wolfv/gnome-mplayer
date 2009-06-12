## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po icons

gnome_mplayerdocdir = ${prefix}/share/doc/gnome-mplayer
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

icondir = ${prefix}/share/pixmaps
icon_DATA = gnome-mplayer.png

EXTRA_DIST = $(gnome_mplayerdoc_DATA) $(desktop_DATA) $(icon_DATA) $(schema_DATA) gnome-mplayer.spec.in config.h.in

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

install-schemas: $(schema_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) gconftool-2 --makefile-install-rule $(schema_DATA) 2>&1 > /dev/null

if GCONF_SCHEMAS_INSTALL
install-data-hook: install-schemas
else
install-data-hook:
endif
