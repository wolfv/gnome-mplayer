## Process this file with automake to produce Makefile.in

## Created by Anjuta
SUBDIRS = 

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-I../pixmaps \
	$(GTK_CFLAGS) \
	$(GMLIB_CFLAGS) \
	$(GMTK_CFLAGS) \
    $(GIO_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(ALSA_CFLAGS) \
	$(PULSE_CFLAGS) \
    $(NOTIFY_CFLAGS) \
    $(GPOD_CFLAGS) \
    $(MUSICBRAINZ_CFLAGS) \
	$(XSCRNSAVER_CFLAGS) \
    $(LIBGDA_CFLAGS)   

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
     $(MUSICBRAINZ_DEFINES) \
     $(LIBGDA_DEFINES)
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
	langlist.h \
	mpris-interface.c \
	mpris-interface.h \
	database.c \
	database.h


gnome_mplayer_LDFLAGS = 

gnome_mplayer_LDADD = \
    -lm \
    $(X11_LIBS) \
    $(GTK_LIBS) \
    $(GMLIB_LIBS) \
    $(GMTK_LIBS) \
    $(GIO_LIBS) \
    $(DBUS_LIBS) \
    $(GCONF_LIBS) \
    $(ALSA_LIBS) \
	$(PULSE_LIBS) \
    $(NOTIFY_LIBS) \
    $(GPOD_LIBS) \
    $(MUSICBRAINZ_LIBS) \
    $(XSCRNSAVER_LIBS) \
    $(LIBGDA_LIBS)

if HAVE_NAUTILUS

nautilusdir = $(NAUTILUSDIR)
nautilus_PROGRAMS = libgnome-mplayer-properties-page.so

libgnome_mplayer_properties_page_so_SOURCES = \
    ../config.h \
    mime_types.h \
    nautilus_property_page.c \
    property_page_common.c \
    property_page_common.h

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

if HAVE_NEMO

nemodir = $(NEMODIR)
nemo_PROGRAMS = libgnome-mplayer-nemo-properties-page.so

libgnome_mplayer_nemo_properties_page_so_SOURCES = \
    ../config.h \
    mime_types.h \
    nemo_property_page.c \
    property_page_common.c \
    property_page_common.h

libgnome_mplayer_nemo_properties_page_so_CFLAGS = -fPIC \
    $(NEMO_CFLAGS) \
	$(GTK_CFLAGS) \
    $(GIO_CFLAGS) \
    $(AM_CFLAGS)
    
libgnome_mplayer_nemo_properties_page_so_LDFLAGS = -shared

libgnome_mplayer_nemo_properties_page_so_LDADD = \
	$(NEMO_LIBS) \
	$(GTK_LIBS) \
	$(GMLIB_LIBS) \
	$(GIO_LIBS)

endif #HAVE_NEMO

if HAVE_CAJA

cajadir = $(CAJADIR)
caja_PROGRAMS = libgnome-mplayer-caja-properties-page.so

libgnome_mplayer_caja_properties_page_so_SOURCES = \
    ../config.h \
    mime_types.h \
    caja_property_page.c \
    property_page_common.c \
    property_page_common.h

libgnome_mplayer_caja_properties_page_so_CFLAGS = -fPIC \
    $(CAJA_CFLAGS) \
	$(GTK_CFLAGS) \
    $(GIO_CFLAGS) \
    $(AM_CFLAGS)
    
libgnome_mplayer_caja_properties_page_so_LDFLAGS = -shared

libgnome_mplayer_caja_properties_page_so_LDADD = \
	$(CAJA_LIBS) \
	$(GTK_LIBS) \
	$(GMLIB_LIBS) \
	$(GIO_LIBS)

endif #HAVE_CAJA

EXTRA_DIST = 
	
indent:
	indent -kr -l120 -i4 -nut *.c
	indent -kr -l120 -i4 -nut *.h
	rm *~
	
cppcheck:
	cppcheck -q -a -v -f -j3 .
	
valgrind:
	G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --tool=memcheck --leak-check=full --leak-resolution=high --num-callers=20 --log-file=vgdump ./gnome-mplayer 
valgrind-suppress:
	G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --tool=memcheck --leak-check=full --leak-resolution=high --num-callers=20 --log-file=vgdump --suppressions=$(HOME)/gtk.suppression ./gnome-mplayer 
