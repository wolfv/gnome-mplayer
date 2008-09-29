## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po

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

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = gnome-mplayer.schemas

desktopdir = ${prefix}/share/applications
desktop_DATA = gnome-mplayer.desktop

icondir = ${prefix}/share/pixmaps
icon_DATA = gnome-mplayer.png

EXTRA_DIST = $(gnome_mplayerdoc_DATA) $(desktop_DATA) $(icon_DATA) gnome-mplayer.spec.in

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

if GCONF_SCHEMAS_INSTALL
install-data-hook:
        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_srcdir)/$(schema_DATA)
endif
