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
#include <stdlib.h>
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

gint get_bitrate(gchar * name);
void strip_unicode(gchar * data, gsize len);
gint play_file(gchar * filename, gint playlist);
gint detect_playlist(gchar * filename);
gchar *metadata_to_utf8(gchar * string);
gint parse_playlist(gchar * filename);
gint parse_basic(gchar * filename);
gint parse_ram(gchar * filename);
gint parse_cdda(gchar * filename);
gint parse_dvd(gchar * filename);
gboolean save_playlist_pls(gchar * filename);
gboolean save_playlist_m3u(gchar * filename);
gchar *get_path(gchar * filename);
gboolean update_mplayer_config();
gboolean send_command(gchar * command);
gboolean streaming_media(gchar * filename);
gboolean device_name(gchar * filename);
GtkTreeIter add_item_to_playlist(gchar * itemname, gint playlist);
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
#endif                          // _SUPPORT_H
