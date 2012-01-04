## Process this file with automake to produce Makefile.in

## Created by Anjuta
SUBDIRS = 

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-I../pixmaps \
	$(GTK_CFLAGS) \
	$(GMTK_CFLAGS) \
    $(GIO_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(ALSA_CFLAGS) \
	$(PULSE_CFLAGS) \
    $(NOTIFY_CFLAGS) \
    $(GPOD_CFLAGS) \
    $(MUSICBRAINZ_CFLAGS) \
	$(XSCRNSAVER_CFLAGS)    

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
     ## -DGTK_DISABLE_DEPRECATED 
     ## -DGSEAL_ENABLE

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

gnome_mplayer_LDADD = \
    -lm \
    $(X11_LIBS) \
    $(GTK_LIBS) \
    $(GMTK_LIBS) \
    $(GIO_LIBS) \
    $(DBUS_LIBS) \
    $(GCONF_LIBS) \
    $(ALSA_LIBS) \
	$(PULSE_LIBS) \
    $(NOTIFY_LIBS) \
    $(GPOD_LIBS) \
    $(MUSICBRAINZ_LIBS) \
    $(XSCRNSAVER_LIBS)

if HAVE_NAUTILUS

nautilusdir = $(NAUTILUSDIR)
nautilus_PROGRAMS = libgnome-mplayer-properties-page.so

libgnome_mplayer_properties_page_so_SOURCES = \
    ../config.h \
    mime_types.h \
    nautilus_property_page.c

libgnome_mplayer_properties_page_so_CFLAGS = -fPIC \
    $(NAUTILUS_CFLAGS) \
	$(GTK_CFLAGS) \
    $(GIO_CFLAGS) \
    $(AM_CFLAGS)
    
libgnome_mplayer_properties_page_so_LDFLAGS = -shared

libgnome_mplayer_properties_page_so_LDADD = \
	$(NAUTILUS_LIBS) \
	$(GTK_LIBS) \
	$(GMLIB_LIBS) \
	$(GIO_LIBS)

endif #HAVE_NAUTILUS

EXTRA_DIST = 
	
indent:
	indent -kr -l120 -i4 -nut *.c
	indent -kr -l120 -i4 -nut *.h
	rm *~
	
cppcheck:
	cppcheck -q -a -v -f -j3 .
	
