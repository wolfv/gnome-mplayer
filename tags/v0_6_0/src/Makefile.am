## Process this file with automake to produce Makefile.in

## Created by Anjuta

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-I../pixmaps \
	$(GTK_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(GCONF_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g\
	 $(GTK2_12_DEFINES)\
	 $(GLIB2_14_DEFINES)

bin_PROGRAMS = gnome-mplayer 

gnome_mplayer_SOURCES = \
	../config.h \
	gui.c \
	gui.h \
	playlist.c \
	playlist.h \
	support.c \
	support.h \
	main.c \
	dbus-interface.c \
	dbus-interface.h \
	thread.h \
	thread.c \
	common.h \
	../pixmaps/gnome_mplayer.xpm \
	../pixmaps/media-playback-pause.xpm \
	../pixmaps/media-playback-start.xpm \
	../pixmaps/media-playback-stop.xpm \
	../pixmaps/media-seek-backward.xpm \
	../pixmaps/media-seek-forward.xpm \
	../pixmaps/media-skip-backward.xpm \
	../pixmaps/media-skip-forward.xpm \
	../pixmaps/view-fullscreen.xpm \
	langlist.h 

gnome_mplayer_LDFLAGS = 

gnome_mplayer_LDADD = $(GTK_LIBS) $(DBUS_LIBS) $(GCONF_LIBS)

EXTRA_DIST = 
	

