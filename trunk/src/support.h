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
MetaData *get_metadata(gchar * uri);
MetaData *get_basic_metadata(gchar * uri);
void strip_unicode(gchar * data, gsize len);
gint play_iter(GtkTreeIter * playiter, gint restart_second);
gint detect_playlist(gchar * filename);
gchar *metadata_to_utf8(gchar * string);
gint parse_playlist(gchar * uri);
gint parse_asx(gchar * uri);
gint parse_basic(gchar * filename);
gint parse_ram(gchar * filename);
gint parse_cdda(gchar * filename);
gint parse_dvd(gchar * filename);
gint parse_vcd(gchar * filename);
gboolean save_playlist_pls(gchar * filename);
gboolean save_playlist_m3u(gchar * filename);
gchar *get_path(gchar * filename);
gboolean update_mplayer_config();
gboolean send_command(gchar * command, gboolean retain_pause);
gboolean streaming_media(gchar * filename);
gboolean device_name(gchar * filename);
gboolean add_item_to_playlist(const gchar * uri, gint playlist);
gboolean first_item_in_playlist(GtkTreeIter * iter);
gboolean prev_item_in_playlist(GtkTreeIter * iter);
gboolean next_item_in_playlist(GtkTreeIter * iter);
void randomize_playlist(GtkListStore * store);
void reset_playlist_order(GtkListStore * store);
gdouble get_alsa_volume(gboolean show_details);
gboolean set_alsa_volume(gboolean show_details, gint volume);
gchar *seconds_to_string(gfloat seconds);

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

gboolean add_to_playlist_and_play(gpointer data);
gboolean clear_playlist_and_play(gpointer data);

#endif                          // _SUPPORT_H
