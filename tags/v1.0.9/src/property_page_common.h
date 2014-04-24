/*
 * property_page_common.h
 *
 * Copyright (C) 2013 - Kevin DeKorte
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <gmlib.h>

typedef struct _MetaData {
    gchar *title;
    gchar *artist;
    gchar *album;
    gchar *length;
    gfloat length_value;
    gchar *subtitle;
    gchar *audio_codec;
    gchar *video_codec;
    gchar *audio_bitrate;
    gchar *video_bitrate;
    gchar *video_fps;
    gchar *audio_nch;
    gchar *demuxer;
    gint width;
    gint height;
    gboolean video_present;
    gboolean audio_present;
} MetaData;

gint verbose;
gboolean get_properties(GtkWidget * page, gchar * uri);
