## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = \
        libgmlib.a


libgmlib_a_SOURCES = \
   	gmlib.h \
   	gm_file.h \
   	gm_file.c \
   	gm_parse.h \
   	gm_parse.c \
	gm_pref_store.h \
	gm_pref_store.c \
	gm_strfuncs.h \
	gm_strfuncs.c

libgmlib_a_CFLAGS = -fPIC \
	$(GLIB_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GCONF_DEFINES) \
	$(GIO_DEFINES) \
	$(GLIB2_26_DEFINES)
