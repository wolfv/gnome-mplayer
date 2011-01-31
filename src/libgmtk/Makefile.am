## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = \
        libgmtk.a


libgmtk_a_SOURCES = \
	gmtk.h \
    	gmtk_audio_meter.h \
    	gmtk_audio_meter.c \
    	gmtk_media_tracker.h \
    	gmtk_media_tracker.c \
    	gmtk_media_player.h \
    	gmtk_media_player.c \
    	gmtk_output_combo_box.h \
    	gmtk_output_combo_box.c

libgmtk_a_CFLAGS = -fPIC \
	$(GTK_CFLAGS) \
	$(ALSA_CFLAGS)
	
#libgmtk_a_LDADD = \
#	$(ALSA_LIBS)
