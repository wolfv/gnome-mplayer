/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * playlist.h
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * playlist.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * playlist.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with playlist.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>

GtkTooltips *playlisttip;
GtkMenu *playlist_popup_menu;
GtkMenuItem *playlist_set_subtitle;
gint window_width, window_height;
gint filecount;
GtkWidget *up;
GtkWidget *down;

void update_gui();
void menuitem_view_playlist_callback(GtkMenuItem * menuitem, void *data);
void add_item_to_playlist_callback(gpointer data, gpointer user_data);
void add_folder_to_playlist_callback(gpointer data, gpointer user_data);

// these are in gui.c
gint get_height();
gint get_width();
