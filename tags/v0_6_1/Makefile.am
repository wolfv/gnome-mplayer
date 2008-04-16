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

gnome_mplayeretcdir = ${sysconfdir}/gconf/schemas
gnome_mplayeretc_DATA = gnome-mplayer.schemas

desktopdir = ${prefix}/share/applications
desktop_DATA = gnome-mplayer.desktop

icondir = ${prefix}/share/pixmaps
icon_DATA = gnome-mplayer.png

EXTRA_DIST = $(gnome_mplayerdoc_DATA) $(desktop_DATA) $(icon_DATA) $(gnome_mplayeretc_DATA) gnome-mplayer.spec.in

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

install-data-hook:
	if [ -e ${sysconfdir}/gconf/schemas/gnome-mplayer.schemas ]; then GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source` gconftool-2 --makefile-install-rule ${sysconfdir}/gconf/schemas/gnome-mplayer.schemas > /dev/null;	fi
