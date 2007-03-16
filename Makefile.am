## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po debian

gnome_mplayerdocdir = ${prefix}/doc/gnome-mplayer
gnome_mplayerdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS \
	DOCS/tech/dbus.txt \
	DOCS/tech/plugin-interaction.txt

desktopdir = ${prefix}/share/applications
desktop_DATA = gnome-mplayer.desktop

icondir = ${prefix}/share/pixmaps
icon_DATA = gnome-mplayer.png

EXTRA_DIST = $(gnome_mplayerdoc_DATA) $(desktop_DATA) $(icon_DATA)

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

