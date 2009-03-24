## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = \
        libgmlib.la


libgmlib_la_SOURCES = \
   	gmlib.h \
    	gm_file.h \
    	gm_file.c \
	gm_pref_store.h \
	gm_pref_store.c

libgmlib_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(GCONF_DEFINES)

libgmlib_la_LDFLAGS = \
	$(GLIB_LDFLAGS) \
	$(GCONF_LDFLAGS)


 
