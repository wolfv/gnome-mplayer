/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gm_pref_store.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * gm_pref_store.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * gm_pref_store.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with playlist.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "gm_pref_store.h"

#ifdef HAVE_GCONF
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>
#endif

struct _GmPrefStore {
#ifdef HAVE_GCONF
	GConfClient *gconf;
#else
	GKeyFile *keyfile;
#endif
	gchar *context;
};

GmPrefStore *gm_pref_store_new(const gchar * context) {

	GmPrefStore *store = (GmPrefStore *)g_new0(GmPrefStore,1);
	gchar *filename;

	store->context = g_strdup(context);
#ifdef HAVE_GCONF
	store->gconf = gconf_client_get_default();
#else

    filename = g_strdup_printf("%s/%s", g_get_user_config_dir(),context);
    if (!g_file_test(filename, G_FILE_TEST_IS_DIR)) {
        g_mkdir_with_parents(filename, 0775);
    }
    g_free(filename);

    store->keyfile = g_key_file_new();
    filename = g_strdup_printf("%s/%s/%s.conf", g_get_user_config_dir(),context,context);
    g_key_file_load_from_file(store->keyfile,
                              filename,
                              G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

#endif
	return store;
}

void gm_pref_store_free(GmPrefStore *store) {

#ifdef HAVE_GCONF
    if (G_IS_OBJECT(store->gconf))
        g_object_unref(G_OBJECT(store->gconf));
	store->gconf = NULL;
#else
    gchar *filename;
    gchar *data;

    if (store->keyfile != NULL) {
        filename = g_strdup_printf("%s/%s/%s.conf", g_get_user_config_dir(),context,context);
        data = g_key_file_to_data(store->keyfile, NULL, NULL);

        g_file_set_contents(filename, data, -1, NULL);
        g_free(data);
        g_free(filename);
        g_key_file_free(store->keyfile);
        store->keyfile = NULL;
    }
#endif
	g_free(store->context);
	store->context = NULL;

	g_free(store);
	store = NULL;

}

gboolean gm_pref_store_get_boolean(GmPrefStore *store, const gchar *key) {

	gboolean value = FALSE;
#ifdef HAVE_GCONF
	gchar *full_key;
	
	full_key = g_strdup_printf("/apps/%s/preferences/%s",store->context,key);
	value = gconf_client_get_bool(store->gconf, full_key, NULL);
    g_free(full_key);
#else
	
	if (g_key_file_has_key(store->keyfile,store->context,key,NULL))
		value = g_key_file_get_boolean(store->keyfile,store->context,key,NULL);

#endif	
	return value;
}

void gm_pref_store_set_boolean(GmPrefStore *store, const gchar *key, gboolean value) {

#ifdef HAVE_GCONF
	gchar *full_key;
	
	full_key = g_strdup_printf("/apps/%s/preferences/%s",store->context,key);
	gconf_client_set_bool(store->gconf, full_key, value, NULL);
    g_free(full_key);
#else
	
	g_key_file_set_boolean(store->keyfile,store->context,key,value);

#endif	
}

gint gm_pref_store_get_int(GmPrefStore *store, const gchar *key) {

	gint value = 0;
#ifdef HAVE_GCONF
	gchar *full_key;
	
	full_key = g_strdup_printf("/apps/%s/preferences/%s",store->context,key);
	value = gconf_client_get_int(store->gconf, full_key, NULL);
    g_free(full_key);
#else
	
	if (g_key_file_has_key(store->keyfile,store->context,key,NULL))
		value = g_key_file_get_int(store->keyfile,store->context,key,NULL);

#endif	
	return value;
}

gint gm_pref_store_get_int_with_default(GmPrefStore *store, const gchar *key, gint default_value) {

	gint value = 0;
#ifdef HAVE_GCONF
	gchar *full_key;
	
	full_key = g_strdup_printf("/apps/%s/preferences/%s",store->context,key);
	value = gconf_client_get_int(store->gconf, full_key, NULL);
    g_free(full_key);
#else
	
	if (g_key_file_has_key(store->keyfile,store->context,key,NULL)) {
		value = g_key_file_get_int(store->keyfile,store->context,key,NULL);
	} else {
		value = default_value;
	}
#endif	
	return value;
}


void gm_pref_store_set_int(GmPrefStore *store, const gchar *key, gint value) {

#ifdef HAVE_GCONF
	gchar *full_key;
	
	full_key = g_strdup_printf("/apps/%s/preferences/%s",store->context,key);
	gconf_client_set_int(store->gconf, full_key, value, NULL);
    g_free(full_key);
#else
	
	g_key_file_set_int(store->keyfile,store->context,key,value);

#endif	
}

gfloat gm_pref_store_get_float(GmPrefStore *store, const gchar *key) {

	gfloat value = 0.0;
#ifdef HAVE_GCONF
	gchar *full_key;
	
	full_key = g_strdup_printf("/apps/%s/preferences/%s",store->context,key);
	value = gconf_client_get_float(store->gconf, full_key, NULL);
    g_free(full_key);
#else
	
	if (g_key_file_has_key(store->keyfile,store->context,key,NULL))
		value = g_key_file_get_float(store->keyfile,store->context,key,NULL);

#endif	
	return value;
}

void gm_pref_store_set_float(GmPrefStore *store, const gchar *key, gfloat value) {

#ifdef HAVE_GCONF
	gchar *full_key;
	
	full_key = g_strdup_printf("/apps/%s/preferences/%s",store->context,key);
	gconf_client_set_float(store->gconf, full_key, value, NULL);
    g_free(full_key);
#else
	
	g_key_file_set_double(store->keyfile,store->context,key,value);

#endif	
}

gchar * gm_pref_store_get_string(GmPrefStore *store, const gchar *key) {

	gchar * value = NULL;
#ifdef HAVE_GCONF
	gchar *full_key;
	
	full_key = g_strdup_printf("/apps/%s/preferences/%s",store->context,key);
	value = gconf_client_get_string(store->gconf, full_key, NULL);
    g_free(full_key);
#else
	
	if (g_key_file_has_key(store->keyfile,store->context,key,NULL))
		value = g_key_file_get_string(store->keyfile,store->context,key,NULL);

#endif	
	return value;
}

void gm_pref_store_set_string(GmPrefStore *store, const gchar *key, gchar * value) {

#ifdef HAVE_GCONF
	gchar *full_key;
	
	full_key = g_strdup_printf("/apps/%s/preferences/%s",store->context,key);
	gconf_client_unset(store->gconf, full_key, NULL);
	if (value != NULL && strlen(g_strstrip(value)) > 0)
		gconf_client_set_string(store->gconf, full_key, value, NULL);
    g_free(full_key);
#else
    if (value != NULL && strlen(g_strstrip(value)) > 0) {
        g_key_file_set_string(store->keyfile, store->context, key, value);
    } else {
        g_key_file_remove_key(store->keyfile, store->context, key, NULL);
    }	
#endif	
}

void gm_pref_store_unset(GmPrefStore *store, const gchar *key) {

#ifdef HAVE_GCONF
	gchar *full_key;
	
	full_key = g_strdup_printf("/apps/%s/preferences/%s",store->context,key);
	gconf_client_unset(store->gconf, full_key, NULL);
    g_free(full_key);
#else
    g_key_file_remove_key(store->keyfile, store->context, key, NULL);
#endif	

}

