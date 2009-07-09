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

#include <gtk/gtk.h>
#include "libgmlib/gmlib.h"

#ifdef GIO_ENABLED
#include <gio/gio.h>
#endif

#ifndef _COMMON_H
#define _COMMON_H

#define MIXER			"mixer"
#define CACHE_SIZE		"cache_size"
#define OSDLEVEL		"osdlevel"
#define PPLEVEL		"pplevel"
#define SOFTVOL			"softvol"
#define VERBOSE			"verbose"
#define VERTICAL		"vertical"
#define FORCECACHE		"forcecache"
#define LAST_DIR		"last_dir"
#define SHOWPLAYLIST	"showplaylist"
#define SHOWDETAILS	"showdetails"
#define DISABLEASS          "disableass"
#define DISABLEEMBEDDEDFONTS    "disableembeddedfonts"
#define DISABLEDEINTERLACE	"disable_deinterlace"
#define DISABLEFRAMEDROP	"disableframedrop"
#define DISABLEFULLSCREEN	"disablefullscreen"
#define DISABLECONTEXTMENU	"disablecontextmenu"
#define DISABLEPAUSEONCLICK	"disable_pause_on_click"
#define DISABLEANIMATION	"disable_animation"
#define AUTOHIDETIMEOUT	"auto_hide_timeout"
#define METADATACODEPAGE	"metadatacodepage"
#define SUBTITLEFONT	"subtitlefont"
#define SUBTITLESCALE	"subtitlescale"
#define SUBTITLECODEPAGE	"subtitlecodepage"
#define SUBTITLECOLOR	"subtitlecolor"
#define SUBTITLEOUTLINE "subtitleoutline"
#define SUBTITLESHADOW "subtitleshadow"
#define SUBTITLE_MARGIN "subtitle_margin"
#define TRACKER_POSITION "tracker_position"

#define VOLUME	"volume"
#define AUDIO_CHANNELS "audio_channels"
#define USE_MEDIAKEYS		"use_mediakeys"
#define DISABLE_COVER_ART_FETCH "disable_cover_art_fetch"
#define FULLSCREEN	"fullscreen"

#define MPLAYER_BIN		"mplayer_bin"
#define EXTRAOPTS		"extraopts"
#define USE_PULSE_FLAT_VOLUME "use_pulse_flat_volume"

#define DISABLE_QT		"disable_qt"
#define DISABLE_REAL	"disable_real"
#define DISABLE_WMP		"disable_wmp"
#define DISABLE_DVX		"disable_dvx"
#define DISABLE_MIDI    "disable_midi"
#define DISABLE_EMBEDDING		"disable_embedding"

#define REMEMBER_LOC		"remember_loc"
#define WINDOW_X		"window_x"
#define WINDOW_Y		"window_y"
#define WINDOW_HEIGHT	"window_height"
#define WINDOW_WIDTH	"window_width"

#define RESIZE_ON_NEW_MEDIA "resize_on_new_media"
#define KEEP_ON_TOP		"keep_on_top"
#define SINGLE_INSTANCE "single_instance"
#define REPLACE_AND_PLAY "replace_and_play"
#define BRING_TO_FRONT  "bring_to_front"
#define SHOW_NOTIFICATION "show_notification"
#define SHOW_STATUS_ICON "show_status_icon"
#define SHOW_SUBTITLES "show_subtitles"


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

typedef enum {
    NO_ERROR,
    ERROR_RETRY_WITH_PLAYLIST,
    ERROR_RETRY_WITH_HTTP,
    ERROR_RETRY_WITH_MMSHTTP
} PLAYBACK_ERROR;

PLAYSTATE state;
PLAYSTATE guistate;
PLAYSTATE lastguistate;
PLAYBACK_ERROR playback_error;

typedef struct _IdleData {
    gchar info[1024];
    gchar display_name[1024];
    gchar media_info[2048];
    gchar media_notification[2048];
    gchar url[1024];
    gchar af_export[1024];
    GMappedFile *mapped_af_export;
    gint windowid;
    gchar *device;
    gdouble percent;
    gdouble cachepercent;
    gint streaming;
    gchar progress_text[1024];
    gdouble volume;
    gboolean mute;
    gchar vol_tooltip[128];
    gint x;
    gint y;
    gint last_x;
    gint last_y;
    gint width;
    gint height;
    gboolean videopresent;
    gboolean audiopresent;
    gboolean fullscreen;
    gboolean showcontrols;
    gdouble position;
    gdouble length;
    glong byte_pos;
    gint chapters;
    gint brightness;
    gint contrast;
    gint gamma;
    gint hue;
    gint saturation;
    gchar demuxer[64];
    gchar video_format[64];
    gchar video_codec[16];
    gchar video_fps[16];
    gchar video_bitrate[16];
    gchar audio_codec[16];
    gchar audio_bitrate[16];
    gchar audio_samplerate[16];
    gchar audio_channels[16];
    gchar metadata[1024];
    gboolean fromdbus;
    gboolean window_resized;
    gboolean has_chapters;
    gboolean tmpfile;
    gboolean sub_visible;
    gint sub_demux;
    gint switch_audio;
    gboolean retry_on_full_cache;
#ifdef GIO_ENABLED
    GFile *src;
    GFile *tmp;
    GCancellable *cancel;
    GMutex *caching;
    GCond *caching_complete;
#endif
} IdleData;

IdleData *idledata;


typedef struct _ThreadData {
    gchar uri[2048];
    gchar filename[1024];
    gchar subtitle[1024];
    gint streaming;
    gint player_window;
    gint playlist;
    gint start_second;
    gboolean done;
} ThreadData;

enum {
    ITEM_COLUMN,
    DESCRIPTION_COLUMN,
    COUNT_COLUMN,
    PLAYLIST_COLUMN,
    ARTIST_COLUMN,
    ALBUM_COLUMN,
    LENGTH_COLUMN,
    LENGTH_VALUE_COLUMN,
    SUBTITLE_COLUMN,
    COVERART_COLUMN,
    AUDIO_CODEC_COLUMN,
    VIDEO_CODEC_COLUMN,
    DEMUXER_COLUMN,
    VIDEO_WIDTH_COLUMN,
    VIDEO_HEIGHT_COLUMN,
    N_COLUMNS
};

typedef struct _MetaData {
    gchar *uri;
    gchar *title;
    gchar *artist;
    gchar *album;
    gchar *length;
    gfloat length_value;
    gchar *subtitle;
    gchar *audio_codec;
    gchar *video_codec;
    gchar *demuxer;
    gint width;
    gint height;
} MetaData;

typedef struct _LangMenu {
    gchar *label;
    int value;
} LangMenu;

typedef struct _Export {
    int nch;
    int size;
    unsigned long long counter;
    gint16 payload[7][512];
} Export;

#define METER_BARS 		44
gint buckets[METER_BARS];
gint max_buckets[METER_BARS];

//Define MIME for DnD
#define DRAG_NAME_0		"text/plain"
#define DRAG_INFO_0		0
#define DRAG_NAME_1		"text/uri-list"
#define DRAG_INFO_1		1
#define DRAG_NAME_2		"STRING"
#define DRAG_INFO_2		2

gchar *lastfile;
gint cache_size;
gboolean forcecache;
gint osdlevel;
gint pplevel;
gint streaming;
gint showcontrols;
gboolean showsubtitles;
gint fullscreen;
gint videopresent;
gint playlist;
gint embed_window;
gint window_x;
gint window_y;
gint control_id;
gboolean softvol;
gint verbose;
gint reallyverbose;
gint autostart;
gint actual_x, actual_y;
gint play_x, play_y;
gint last_x, last_y;
gint last_window_width, last_window_height;
gint stored_window_width, stored_window_height;
gchar vm[10];
gchar *vo;
gchar *ao;
gchar *mixer;
gint audio_channels;
gboolean disable_deinterlace;
gboolean disable_framedrop;
gboolean disable_context_menu;
gboolean disable_fullscreen;
gboolean disable_pause_on_click;
gint loop;
gint random_order;
gboolean dontplaynext;
gboolean autopause;
gchar *path;
gint js_state;
gchar *rpname;
gchar *rpconsole;
gchar *rpcontrols;
gboolean control_instance;
gchar *playlistname;
gboolean ok_to_play;
gchar *subtitle;
gchar *alang;
gchar *slang;
gchar *metadata_codepage;
gint volume;
gboolean use_volume_option;
gboolean vertical_layout;
gboolean playlist_visible;
gboolean details_visible;
gboolean restore_playlist;
gboolean restore_details;
gboolean restore_info;
gint restore_pane;
gboolean disable_ass;
gboolean disable_embeddedfonts;
gboolean disable_animation;
gint auto_hide_timeout;
gboolean always_hide_after_timeout;
gchar *subtitlefont;
gdouble subtitle_scale;
gdouble subtitle_delay;
gchar *subtitle_codepage;
gchar *subtitle_color;
gboolean subtitle_outline;
gboolean subtitle_shadow;
gint subtitle_margin;
gboolean quit_on_complete;
gchar *mplayer_bin;
gchar *extraopts;
gboolean resize_on_new_media;
gboolean use_pulse_flat_volume;
gboolean single_instance;
gboolean new_instance;
gboolean replace_and_play;
gboolean bring_to_front;
gboolean use_pausing_keep_force;
gboolean show_notification;
gboolean show_status_icon;
gboolean load_tracks_from_gpod;
gchar *gpod_mount_point;
gboolean disable_cover_art_fetch;
gboolean updating_recent;
gboolean large_buttons;
gint button_size;

gboolean remember_loc;
gboolean use_remember_loc;
gint loc_window_x;
gint loc_window_y;
gint loc_window_height;
gint loc_window_width;
gboolean keep_on_top;

gboolean cancel_folder_load;
// tv stuff
gchar *tv_device;
gchar *tv_driver;
gchar *tv_input;
gint tv_width;
gint tv_height;
gint tv_fps;

GThread *thread;
GMutex *slide_away;
GCond *mplayer_complete_cond;

gboolean use_mediakeys;
gboolean dvdnav_title_is_menu;

gboolean qt_disabled;
gboolean real_disabled;
gboolean wmp_disabled;
gboolean dvx_disabled;
gboolean midi_disabled;
gboolean embedding_disabled;

GArray *data;
GArray *max_data;
gboolean reading_af_export;

gboolean sub_source_file;

// layout variables
gint non_fs_width;
gint non_fs_height;
gboolean adjusting;

// playlist stuff
GtkListStore *playliststore;
GtkListStore *nonrandomplayliststore;
GtkTreeIter iter;
GtkTreeSelection *selection;
GtkWidget *list;

// preference store
GmPrefStore *gm_store;
GmPrefStore *gmp_store;

GtkWidget *create_window(gint windowid);
void show_window(gint windowid);
void present_main_window();
gint get_player_window();
void adjust_layout();
gboolean hide_buttons(void *data);
gboolean show_copyurl(void *data);
gboolean set_media_info(void *data);
gboolean set_media_label(void *data);
gboolean set_cover_art(gpointer pixbuf);
gboolean set_progress_value(void *data);
gboolean set_progress_text(void *data);
gboolean set_progress_time(void *data);
gboolean set_volume_from_slider(gpointer data);
gboolean set_volume_tip(void *data);
gboolean set_gui_state(void *data);
gboolean resize_window(void *data);
gboolean set_play(void *data);
gboolean set_pause(void *data);
gboolean set_stop(void *data);
gboolean set_ff(void *data);
gboolean set_rew(void *data);
gboolean set_prev(void *data);
gboolean set_next(void *data);
gboolean set_quit(void *data);
gboolean set_position(void *data);
gboolean set_volume(void *data);
gboolean set_fullscreen(void *data);
gboolean set_show_controls(void *data);
gboolean get_show_controls();
gboolean set_window_visible(void *data);
gboolean set_update_gui(void *data);
gboolean set_subtitle_visibility(void *data);
gboolean set_item_add_info(void *data);
void remove_langs(GtkWidget * item, gpointer data);
gboolean set_new_lang_menu(gpointer data);
gboolean set_new_audio_menu(gpointer data);
gboolean make_panel_and_mouse_invisible(gpointer data);
void make_button(gchar * src, gchar * href);
void dbus_open_by_hrefid(gchar * hrefid);
void dbus_open_next();
void dbus_cancel();
void dbus_reload_plugins();
void dbus_send_rpsignal(gchar * signal);
void dbus_send_rpsignal_with_int(gchar * signal, int value);
void dbus_send_rpsignal_with_double(gchar * signal, gdouble value);
void dbus_send_rpsignal_with_string(gchar * signal, gchar * value);
void dbus_send_event(gchar * event, gint button);
void dbus_unhook();
void dbus_enable_screensaver();
void dbus_disable_screensaver();
void menuitem_edit_random_callback(GtkMenuItem * menuitem, void *data);

gboolean update_audio_meter(gpointer data);

void mplayer_shutdown();
gpointer launch_player(gpointer data);

gboolean update_mplayer_config();
gboolean read_mplayer_config();

#ifdef GTK2_12_ENABLED
GtkRecentManager *recent_manager;
void recent_manager_changed_callback(GtkRecentManager * recent_manager, gpointer data);
#endif

#endif                          /* _COMMON_H */
