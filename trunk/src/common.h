/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * common.h
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * common.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * common.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with common.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */


#ifndef _COMMON_H
#define _COMMON_H

#define CACHE_SIZE		"/apps/gnome-mplayer/preferences/cache_size"
#define OSDLEVEL		"/apps/gnome-mplayer/preferences/osdlevel"
#define VERBOSE			"/apps/gnome-mplayer/preferences/verbose"
#define LAST_DIR		"/apps/gnome-mplayer/preferences/last_dir"

#define DISABLE_QT		"/apps/gecko-mediaplayer/preferences/disable_qt"
#define DISABLE_REAL	"/apps/gecko-mediaplayer/preferences/disable_real"
#define DISABLE_WMP		"/apps/gecko-mediaplayer/preferences/disable_wmp"
#define DISABLE_DVX		"/apps/gecko-mediaplayer/preferences/disable_dvx"


// JavaScript Playstates
#define STATE_UNDEFINED     0
#define STATE_STOPPED       1
#define STATE_PAUSED        2
#define STATE_PLAYING       3
#define STATE_SCANFORWARD   4
#define STATE_SCANREVERSE   5
#define STATE_BUFFERING	    6
#define STATE_WAITING       7
#define STATE_MEDIAENDED    8
#define STATE_TRANSITIONING 9
#define STATE_READY	        10
#define STATE_RECONNECTING  11


typedef enum {
    PLAYING,
    PAUSED,
    STOPPED,
    QUIT
} PLAYSTATE;

PLAYSTATE state;

typedef struct _IdleData {
    gchar info[1024];
	gchar url[1024];
    gdouble percent;
    gdouble cachepercent;
    gint streaming;
    gchar progress_text[1024];
    gdouble volume;
    gint mute;
    gchar vol_tooltip[128];
    gint x;
    gint y;
    gint last_x;
    gint last_y;
    gint width;
    gint height;
    gint videopresent;
    gboolean fullscreen;
    gboolean showcontrols;
    gdouble position;
    gdouble length;
    gint brightness;
    gint contrast;
    gint gamma;
    gint hue;
    gint saturation;
    gchar video_format[64];
    gchar video_codec[16];
    gchar video_fps[16];
    gchar video_bitrate[16];
    gchar audio_codec[16];
    gchar audio_bitrate[16];
    gchar audio_samplerate[16];
} IdleData;

IdleData *idledata;


typedef struct _ThreadData {
    gchar filename[1024];
    gint streaming;
    gint player_window;
    gint playlist;
} ThreadData;

enum {
	ITEM_COLUMN,
	DESCRIPTION_COLUMN,
	COUNT_COLUMN,
	PLAYLIST_COLUMN,
	N_COLUMNS
};

gchar *lastfile;
gint cache_size;
gint osdlevel;
gint streaming;
gint showcontrols;
gint fullscreen;
gint videopresent;
gint playlist;
gint embed_window;
gint window_x;
gint window_y;
gint control_id;
gint verbose;
gint autostart;
gint actual_x, actual_y;
gint play_x, play_y;
gint last_x, last_y;
gint last_window_width, last_window_height;
gchar vm[10];
gchar *vo;
gchar *ao;
gint disable_context_menu;
gint loop;
gint random_order;
gboolean dontplaynext;
gboolean autopause;
gchar *path;
gint js_state;
gchar *rpname;
gchar *rptarget;


gboolean qt_disabled;
gboolean real_disabled;
gboolean wmp_disabled;
gboolean dvx_disabled;

// playlist stuff
GtkListStore *playliststore;
GtkListStore *nonrandomplayliststore;
GtkTreeIter iter;
GtkTreeSelection *selection;

GtkWidget *create_window(gint windowid);
gint get_player_window();
gboolean hide_buttons(void *data);
gboolean show_copyurl(void *data);
gboolean set_media_info(void *data);
gboolean set_progress_value(void *data);
gboolean set_progress_text(void *data);
gboolean set_progress_time(void *data);
gboolean set_volume_from_slider(gpointer data);
gboolean set_volume_tip(void *data);
gboolean resize_window(void *data);
gboolean set_play(void *data);
gboolean set_pause(void *data);
gboolean set_stop(void *data);
gboolean set_ff(void *data);
gboolean set_rew(void *data);
gboolean set_position(void *data);
gboolean set_volume(void *data);
gboolean set_fullscreen(void *data);
gboolean set_show_controls(void *data);
gboolean set_window_visible(void *data);
void make_button(gchar * src, gchar * href);
void dbus_open_by_hrefid(gchar * hrefid);
void dbus_open_next();
void dbus_cancel();
void dbus_reload_plugins();
void dbus_send_rpsignal(gchar * signal);
void dbus_send_event(gchar *event, gint button);
void dbus_unhook();
void dbus_disable_screensaver();

void shutdown();
gpointer launch_player(gpointer data);

gboolean update_mplayer_config();
gboolean read_mplayer_config();

#endif                          /* _COMMON_H */
