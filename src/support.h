/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * support.h
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * support.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * support.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with support.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _SUPPORT_H
#define _SUPPORT_H

#include "gui.h"
#include "common.h"
#include <string.h>
#include <unistd.h>

#ifdef HAVE_ASOUNDLIB
#include <asoundlib.h>
#endif

#ifdef HAVE_GCONF
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>
GConfClient *gconf;
#else
GKeyFile *config;
#endif

#ifdef GIO_ENABLED
#include <gio/gio.h>
#endif

#ifdef HAVE_GPOD
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <mntent_compat.h>
#include <gpod/itdb.h>
#endif

#ifdef HAVE_MUSICBRAINZ
#include <musicbrainz3/mb_c.h>
#include <curl/curl.h>
#endif

gint get_bitrate(gchar * name);
void strip_unicode(gchar * data, gsize len);
gint play_iter(GtkTreeIter * playiter);
gint detect_playlist(gchar * filename);
gchar *metadata_to_utf8(gchar * string);
gint parse_playlist(gchar * uri);
gint parse_basic(gchar * filename);
gint parse_ram(gchar * filename);
gint parse_cdda(gchar * filename);
gint parse_dvd(gchar * filename);
gboolean save_playlist_pls(gchar * filename);
gboolean save_playlist_m3u(gchar * filename);
gchar *get_path(gchar * filename);
gboolean update_mplayer_config();
gboolean send_command(gchar * command, gboolean retain_pause);
gboolean streaming_media(gchar * filename);
gboolean device_name(gchar * filename);
gboolean add_item_to_playlist(const gchar * uri, gint playlist);
gboolean next_item_in_playlist(GtkTreeIter * iter);
void copy_playlist(GtkListStore * source, GtkListStore * dest);
void randomize_playlist(GtkListStore * store);
gdouble get_alsa_volume();
gchar *seconds_to_string(gfloat seconds);

void init_preference_store();
gboolean read_preference_bool(gchar * key);
gint read_preference_int(gchar * key);
gfloat read_preference_float(gchar * key);
gchar *read_preference_string(gchar * key);
void write_preference_bool(gchar * key, gboolean value);
void write_preference_int(gchar * key, gint value);
void write_preference_float(gchar * key, gfloat value);
void write_preference_string(gchar * key, gchar * value);
void release_preference_store();

gchar *get_localfile_from_uri(gchar * uri);
gboolean is_uri_dir(gchar * uri);
gboolean uri_exists(gchar * uri);
gchar *switch_protocol(const gchar * uri, gchar * new_protocol);

#ifdef HAVE_GPOD
gchar *find_gpod_mount_point();
gboolean gpod_load_tracks(gchar * mount_point);
#endif

gchar *get_cover_art_url(gchar * artist, gchar * title, gchar * album);
gpointer get_cover_art(gpointer data);

gboolean detect_volume_option();
gboolean map_af_export_file(gpointer data);
gboolean unmap_af_export_file(gpointer data);

#endif                          // _SUPPORT_H
