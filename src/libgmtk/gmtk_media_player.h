/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gmtk_media_player.h
 * Copyright (C) Kevin DeKorte 2009 <kdekorte@gmail.com>
 * 
 * gmtk_media_player.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * gmtk_media_tracker.h is distributed in the hope that it will be useful,
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#ifdef X11_ENABLED
#include <gdk/gdkx.h>
#ifdef GTK3_ENABLED
#include <gtk/gtkx.h>
#endif
#endif
#ifdef GTK3_ENABLED
#include <gdk/gdkkeysyms-compat.h>
#else
#include <gdk/gdkkeysyms.h>
#endif
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <math.h>

#ifndef __GMTK_MEDIA_PLAYER_H__
#define __GMTK_MEDIA_PLAYER_H__

#ifndef GSEAL
#ifdef GSEAL_ENABLE
#define GSEAL(ident)      _g_sealed__ ## ident
#else
#define GSEAL(ident)      ident
#endif
#endif

G_BEGIN_DECLS
#define GMTK_TYPE_MEDIA_PLAYER		(gmtk_media_player_get_type ())
#define GMTK_MEDIA_PLAYER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GMTK_TYPE_MEDIA_PLAYER, GmtkMediaPlayer))
#define GMTK_MEDIA_PLAYER_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), GMTK_MEDIA_PLAYER, GmtkMediaPlayerClass))
#define GMTK_IS_MEDIA_PLAYER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMTK_TYPE_MEDIA_PLAYER))
#define GMTK_IS_MEDIA_PLAYER_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), GMTK_TYPE_MEDIA_PLAYER))
#define GMTK_MEDIA_PLAYER_GET_CLASS	(G_TYPE_INSTANCE_GET_CLASS ((obj), GMTK_TYPE_MEDIA_PLAYER, GmtkMediaPlayerClass))
    typedef enum {
    PLAYER_STATE_DEAD,
    PLAYER_STATE_RUNNING
} GmtkMediaPlayerPlayerState;

typedef enum {
    NO_ERROR,
    ERROR_RETRY_WITH_PLAYLIST,
    ERROR_RETRY_WITH_HTTP,
    ERROR_RETRY_WITH_MMSHTTP,
    ERROR_RETRY_WITHOUT_DIVX_VDPAU,
    ERROR_RETRY_WITHOUT_XVMC
} GmtkMediaPlayerPlaybackError;

typedef enum {
    MEDIA_STATE_UNKNOWN,
    MEDIA_STATE_PLAY,
    MEDIA_STATE_PAUSE,
    MEDIA_STATE_STOP,
    MEDIA_STATE_QUIT
} GmtkMediaPlayerMediaState;

typedef enum {
    ASPECT_DEFAULT,
    ASPECT_4X3,
    ASPECT_16X9,
    ASPECT_16X10,
    ASPECT_WINDOW
} GmtkMediaPlayerAspectRatio;

typedef enum {
    ATTRIBUTE_LENGTH,
    ATTRIBUTE_POSITION,
    ATTRIBUTE_POSITION_PERCENT,
    ATTRIBUTE_START_TIME,
    ATTRIBUTE_RUN_TIME,
    ATTRIBUTE_SIZE,
    ATTRIBUTE_WIDTH,
    ATTRIBUTE_HEIGHT,
    ATTRIBUTE_VIDEO_PRESENT,
    ATTRIBUTE_VO,
    ATTRIBUTE_AO,
    ATTRIBUTE_AUDIO_CHANNELS,
    ATTRIBUTE_MEDIA_DEVICE,
    ATTRIBUTE_EXTRA_OPTS,
    ATTRIBUTE_ALSA_MIXER,
    ATTRIBUTE_HARDWARE_AC3,
    ATTRIBUTE_SOFTVOL,
    ATTRIBUTE_VOLUME_GAIN,
    ATTRIBUTE_MUTED,
    ATTRIBUTE_CACHE_SIZE,
    ATTRIBUTE_FORCE_CACHE,
    ATTRIBUTE_SUB_VISIBLE,
    ATTRIBUTE_SUBS_EXIST,
    ATTRIBUTE_SUB_COUNT,
    ATTRIBUTE_AUDIO_TRACK_COUNT,
    ATTRIBUTE_AF_EXPORT_FILENAME,
    ATTRIBUTE_BRIGHTNESS,
    ATTRIBUTE_CONTRAST,
    ATTRIBUTE_GAMMA,
    ATTRIBUTE_HUE,
    ATTRIBUTE_SATURATION,
    ATTRIBUTE_SEEKABLE,
    ATTRIBUTE_CHAPTERS,
    ATTRIBUTE_HAS_CHAPTERS,
    ATTRIBUTE_TITLE_IS_MENU,
    ATTRIBUTE_AUDIO_TRACK,
    ATTRIBUTE_SUBTITLE,
    ATTRIBUTE_VIDEO_FORMAT,
    ATTRIBUTE_VIDEO_CODEC,
    ATTRIBUTE_VIDEO_FPS,
    ATTRIBUTE_VIDEO_BITRATE,
    ATTRIBUTE_AUDIO_FORMAT,
    ATTRIBUTE_AUDIO_CODEC,
    ATTRIBUTE_AUDIO_BITRATE,
    ATTRIBUTE_AUDIO_RATE,
    ATTRIBUTE_AUDIO_NCH,
    ATTRIBUTE_AUDIO_DELAY,
    ATTRIBUTE_DISABLE_UPSCALING,
    ATTRIBUTE_MPLAYER_BINARY,
    ATTRIBUTE_ZOOM,
    ATTRIBUTE_SPEED_MULTIPLIER,
    ATTRIBUTE_DEINTERLACE,
    ATTRIBUTE_OSDLEVEL,
    ATTRIBUTE_POST_PROCESSING_LEVEL,
    ATTRIBUTE_AUDIO_TRACK_FILE,
    ATTRIBUTE_SUBTITLE_FILE,
    ATTRIBUTE_ENABLE_ADVANCED_SUBTITLES,
    ATTRIBUTE_SUBTITLE_MARGIN,
    ATTRIBUTE_ENABLE_EMBEDDED_FONTS,
    ATTRIBUTE_SUBTITLE_FONT,
    ATTRIBUTE_SUBTITLE_OUTLINE,
    ATTRIBUTE_SUBTITLE_SHADOW,
    ATTRIBUTE_SUBTITLE_SCALE,
    ATTRIBUTE_SUBTITLE_COLOR,
    ATTRIBUTE_SUBTITLE_CODEPAGE,
    ATTRIBUTE_SUBTITLE_DELAY,
    ATTRIBUTE_SUBTITLE_POSITION,
    ATTRIBUTE_SUBTITLE_FUZZINESS,
    ATTRIBUTE_PLAYLIST,
    ATTRIBUTE_MESSAGE,
    ATTRIBUTE_ENABLE_DEBUG,
    ATTRIBUTE_ENABLE_FRAME_DROP,
    ATTRIBUTE_TITLE,
    ATTRIBUTE_ARTIST,
    ATTRIBUTE_ALBUM
} GmtkMediaPlayerMediaAttributes;

typedef enum {
    SEEK_RELATIVE,
    SEEK_PERCENT,
    SEEK_ABSOLUTE
} GmtkMediaPlayerSeekType;

typedef enum {
    TYPE_FILE,
    TYPE_CD,
    TYPE_DVD,
    TYPE_VCD,
    TYPE_TV,
    TYPE_PVR,
    TYPE_DVB,
    TYPE_CUE,
    TYPE_NETWORK
} GmtkMediaPlayerMediaType;

typedef enum {
    COMMAND_SHOW_DVD_MENU,
    COMMAND_TAKE_SCREENSHOT,
    COMMAND_SWITCH_ANGLE,
    COMMAND_SWITCH_AUDIO,
    COMMAND_FRAME_STEP,
    COMMAND_SUBTITLE_SELECT,
    COMMAND_SWITCH_FRAME_DROP
} GmtkMediaPlayerCommand;

typedef struct _GmtkMediaPlayer GmtkMediaPlayer;
typedef struct _GmtkMediaPlayerClass GmtkMediaPlayerClass;

typedef struct _GmtkMediaPlayerSubtitle {
    gint id;
    gboolean is_file;
    gchar *lang;
    gchar *name;
    gchar *label;
} GmtkMediaPlayerSubtitle;

typedef struct _GmtkMediaPlayerAudioTrack {
    gint id;
    gboolean is_file;
    gchar *lang;
    gchar *name;
    gchar *label;
} GmtkMediaPlayerAudioTrack;

typedef struct _GmtkMediaPlayerEvent {
    GmtkMediaPlayer *player;
    gchar *event_name;
    gint event_data_int;
    gdouble event_data_double;
    GtkAllocation *event_allocation;
} GmtkMediaPlayerEvent;

struct _GmtkMediaPlayer {
    GtkEventBox parent;

    /*
       GtkWidget *GSEAL(scale);
       GtkWidget *GSEAL(hbox);
       GtkWidget *GSEAL(message);
       GtkWidget *GSEAL(timer);
       GtkTooltips *GSEAL(progress_tip);
     */
    GtkWidget *alignment;
    GtkWidget *socket;
    gint socket_id;
    gchar *uri;
    gchar *message;
    gdouble position;
    gint video_width;
    gint video_height;
    gboolean video_present;
    gint top, left;
    gdouble length;
    gdouble start_time;
    gdouble run_time;
    gdouble volume;
    gboolean muted;
    gchar *media_device;
    gchar *extra_opts;
    gboolean title_is_menu;
    gchar *vo;
    gchar *ao;
    gchar *alsa_mixer;
    gint audio_channels;
    gboolean softvol;
    gdouble volume_gain;
    gdouble cache_size;
    gdouble cache_percent;
    gboolean force_cache;
    gboolean sub_visible;
    GList *subtitles;
    GList *audio_tracks;
    gint audio_track_id;
    gint subtitle_source;
    gint subtitle_id;
    gint subtitle_is_file;
    gchar *af_export_filename;
    gchar *audio_track_file;
    gchar *subtitle_file;
    gint brightness;
    gint contrast;
    gint hue;
    gint gamma;
    gint saturation;
    gint osdlevel;
    gint post_processing_level;
    gboolean seekable;
    gint chapters;
    gboolean has_chapters;
    gchar *video_format;
    gchar *video_codec;
    gint video_bitrate;
    gdouble video_fps;
    gchar *audio_format;
    gchar *audio_codec;
    gint audio_bitrate;
    gint audio_rate;
    gint audio_nch;
    gdouble audio_delay;
    gboolean disable_upscaling;
    gdouble zoom;
    gdouble speed_multiplier;
    gboolean playlist;
    gchar *artist;
    gchar *title;
    gchar *album;
    gboolean enable_divx;
    gboolean disable_xvmc;

    gboolean deinterlace;
    gboolean frame_drop;
    gboolean debug;
    gboolean hardware_ac3;

    gboolean enable_advanced_subtitles;
    gboolean subtitle_outline;
    gboolean subtitle_shadow;
    gboolean enable_embedded_fonts;
    gdouble subtitle_scale;
    gdouble subtitle_delay;
    gint subtitle_margin;
    gint subtitle_position;
    gchar *subtitle_color;
    gchar *subtitle_codepage;
    gchar *subtitle_font;
    gint subtitle_fuzziness;

    gchar *tv_device;
    gchar *tv_driver;
    gchar *tv_input;
    gint tv_width;
    gint tv_height;
    gint tv_fps;

    GmtkMediaPlayerPlaybackError playback_error;
    GmtkMediaPlayerPlayerState player_state;
    GmtkMediaPlayerMediaState media_state;
    GThread *mplayer_thread;
    GmtkMediaPlayerAspectRatio aspect_ratio;
    GmtkMediaPlayerMediaType type;

    GMutex *thread_running;
    GCond *mplayer_complete_cond;
    gchar *mplayer_binary;
    gboolean use_mplayer2;
    gboolean features_detected;
    gboolean minimum_mplayer;

    gint std_in;
    gint std_out;
    gint std_err;

    GIOChannel *channel_out;
    GIOChannel *channel_in;
    GIOChannel *channel_err;

    guint watch_in_id;
    guint watch_err_id;
    guint watch_in_hup_id;

    gboolean restart;
    gdouble restart_position;
    GmtkMediaPlayerMediaState restart_state;

    GdkColor *default_background;
};

struct _GmtkMediaPlayerClass {
    GtkEventBoxClass parent_class;
    void (*position_changed) (GmtkMediaPlayer * player);
    void (*cache_percent_changed) (GmtkMediaPlayer * player);
    void (*attribute_changed) (GmtkMediaPlayer * player);
    void (*player_state_changed) (GmtkMediaPlayer * player);
    void (*media_state_changed) (GmtkMediaPlayer * player);
    void (*subtitles_changed) (GmtkMediaPlayer * player);
    void (*audio_tracks_changed) (GmtkMediaPlayer * player);
    void (*restart_shutdown_complete) (GmtkMediaPlayer * player);
    void (*restart_complete) (GmtkMediaPlayer * player);
};

GType gmtk_media_player_get_type(void);
GtkWidget *gmtk_media_player_new();

void gmtk_media_player_set_uri(GmtkMediaPlayer * player, const gchar * uri);
const gchar *gmtk_media_player_get_uri(GmtkMediaPlayer * player);

void gmtk_media_player_set_state(GmtkMediaPlayer * player, const GmtkMediaPlayerMediaState new_state);
GmtkMediaPlayerMediaState gmtk_media_player_get_state(GmtkMediaPlayer * player);

void gmtk_media_player_set_attribute_boolean(GmtkMediaPlayer * player,
                                             GmtkMediaPlayerMediaAttributes attribute, gboolean value);
gboolean gmtk_media_player_get_attribute_boolean(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute);

void gmtk_media_player_set_attribute_double(GmtkMediaPlayer * player,
                                            GmtkMediaPlayerMediaAttributes attribute, gdouble value);
gdouble gmtk_media_player_get_attribute_double(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute);

void gmtk_media_player_set_attribute_string(GmtkMediaPlayer * player,
                                            GmtkMediaPlayerMediaAttributes attribute, const gchar * value);
const gchar *gmtk_media_player_get_attribute_string(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute);

void gmtk_media_player_set_attribute_integer(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute,
                                             gint value);
void gmtk_media_player_set_attribute_integer_delta(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute,
                                                   gint delta);
gint gmtk_media_player_get_attribute_integer(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute);

void gmtk_media_player_seek(GmtkMediaPlayer * player, gdouble value, GmtkMediaPlayerSeekType seek_type);
void gmtk_media_player_seek_chapter(GmtkMediaPlayer * player, int value, GmtkMediaPlayerSeekType seek_type);

void gmtk_media_player_set_volume(GmtkMediaPlayer * player, gdouble value);
gdouble gmtk_media_player_get_volume(GmtkMediaPlayer * player);

void gmtk_media_player_set_media_device(GmtkMediaPlayer * player, gchar * media_device);
void gmtk_media_player_set_media_type(GmtkMediaPlayer * player, GmtkMediaPlayerMediaType type);
GmtkMediaPlayerMediaType gmtk_media_player_get_media_type(GmtkMediaPlayer * player);

gboolean gmtk_media_player_send_key_press_event(GmtkMediaPlayer * widget, GdkEventKey * event, gpointer data);

void gmtk_media_player_select_subtitle(GmtkMediaPlayer * player, const gchar * label);
void gmtk_media_player_select_audio_track(GmtkMediaPlayer * player, const gchar * label);
void gmtk_media_player_select_subtitle_by_id(GmtkMediaPlayer * player, gint id);
void gmtk_media_player_select_audio_track_by_id(GmtkMediaPlayer * player, gint id);

void gmtk_media_player_restart(GmtkMediaPlayer * player);
void gmtk_media_player_send_command(GmtkMediaPlayer * player, GmtkMediaPlayerCommand command);

void gmtk_media_player_set_aspect(GmtkMediaPlayer * player, GmtkMediaPlayerAspectRatio aspect);
GmtkMediaPlayerAspectRatio gmtk_media_player_get_aspect(GmtkMediaPlayer * player);


G_END_DECLS
#endif
