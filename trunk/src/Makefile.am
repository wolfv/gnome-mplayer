## Process this file with automake to produce Makefile.in

## Created by Anjuta

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-I../pixmaps \
	$(GTK_CFLAGS) \
    $(GIO_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(ALSA_CFLAGS) \
    $(NOTIFY_CFLAGS) \
    $(GPOD_CFLAGS) \
    $(MUSICBRAINZ_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g\
	 $(GTK2_12_DEFINES) \
	 $(GLIB2_14_DEFINES) \
     $(GIO_DEFINES) \
	 $(ALSA_DEFINES) \
     $(GCONF_DEFINES) \
     $(NOTIFY_DEFINES) \
     $(GPOD_DEFINES) \
     $(MUSICBRAINZ_DEFINES)

bin_PROGRAMS = gnome-mplayer 

gnome_mplayer_SOURCES = \
	../config.h \
	gui.c \
	gui.h \
	playlist.c \
	playlist.h \
	support.c \
	support.h \
	mntent_compat.h \
	mntent_compat.c \
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
	langlist.h \
    libgmlib/gmlib.h \
    libgmlib/gm_file.h \
    libgmlib/gm_file.c \
    libgmtk/gmtk.h \
    libgmtk/gmtk_audio_meter.h \
    libgmtk/gmtk_audio_meter.c
    

gnome_mplayer_LDFLAGS = 

gnome_mplayer_LDADD = $(GTK_LIBS) $(GIO_LIBS) $(DBUS_LIBS) $(GCONF_LIBS) $(ALSA_LIBS) $(NOTIFY_LIBS) $(GPOD_LIBS) $(MUSICBRAINZ_LIBS)

if HAVE_NAUTILUS

nautilusdir = $(NAUTILUSDIR)
nautilus_LTLIBRARIES = libgnome-mplayer-properties-page.la

libgnome_mplayer_properties_page_la_SOURCES = \
    ../config.h \
    mime_types.h \
    nautilus_property_page.c

libgnome_mplayer_properties_page_la_CFLAGS = \
    $(NAUTILUS_CFLAGS) \
    $(AM_CFLAGS)
    

endif #HAVE_NAUTILUS

EXTRA_DIST = 
	
indent:
	indent -kr -l100 -i4 -nut *.c
	indent -kr -l100 -i4 -nut *.h
	rm *~
	
	
