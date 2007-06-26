/***************************************************************************
 *            common.h
 *
 *  Sat Feb  3 06:19:18 2007
 *  Copyright  2007  kdekorte
 *  <kdekorte@mini>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef _COMMON_H
#define _COMMON_H

#define CACHE_SIZE		"/apps/gnome-mplayer/preferences/cache_size"
#define OSDLEVEL		"/apps/gnome-mplayer/preferences/osdlevel"
#define LAST_DIR		"/apps/gnome-mplayer/preferences/last_dir"

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
gchar *lastfile;
gint actual_x, actual_y;
gint play_x, play_y;
gint last_x, last_y;
gchar vm[10];
gchar *vo;
gchar *ao;


GtkWidget *create_window(gint windowid);
gint get_player_window();
gboolean hide_buttons(void *data);
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
void dbus_unhook();

void shutdown();
gpointer launch_player(gpointer data);

gboolean update_mplayer_config();
gboolean read_mplayer_config();

#endif                          /* _COMMON_H */
