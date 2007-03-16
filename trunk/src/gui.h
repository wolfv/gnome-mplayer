/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.h
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * callbacks.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * callbacks.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with callbacks.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <gnome.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>

/*
#include "../pixmaps/play_down_small.xpm"
#include "../pixmaps/play_up_small.xpm"
#include "../pixmaps/pause_down_small.xpm"
#include "../pixmaps/pause_up_small.xpm"
#include "../pixmaps/stop_down_small.xpm"
#include "../pixmaps/stop_up_small.xpm"
#include "../pixmaps/ff_down_small.xpm"
#include "../pixmaps/ff_up_small.xpm"
#include "../pixmaps/rew_down_small.xpm"
#include "../pixmaps/rew_up_small.xpm"
#include "../pixmaps/fs_down_small.xpm"
#include "../pixmaps/fs_up_small.xpm"
*/
#include "../pixmaps/gnome_mplayer.xpm"
#include "../pixmaps/media-playback-pause.xpm"
#include "../pixmaps/media-playback-start.xpm"
#include "../pixmaps/media-playback-stop.xpm"
#include "../pixmaps/media-seek-backward.xpm"
#include "../pixmaps/media-seek-forward.xpm"
// #include "../pixmaps/media-skip-backward.xpm"
// #include "../pixmaps/media-skip-forward.xpm"
#include "../pixmaps/view-fullscreen.xpm"

GtkWidget *window;
GdkWindow *window_container;

GtkWidget *menubar;
GtkMenuItem *menuitem_file;
GtkMenu *menu_file;
GtkMenuItem *menuitem_file_open;
GtkMenuItem *menuitem_file_open_dvd;
GtkMenuItem *menuitem_file_open_acd;
GtkMenuItem *menuitem_file_sep1;
GtkMenuItem *menuitem_file_quit;
GtkMenuItem *menuitem_edit;
GtkMenu *menu_edit;
GtkMenuItem *menuitem_edit_config;
GtkMenuItem *menuitem_help;
GtkMenu *menu_help;
GtkMenuItem *menuitem_help_about;


GtkMenu *popup_menu;
GtkMenuItem *menuitem_open;
GtkMenuItem *menuitem_sep3;
GtkMenuItem *menuitem_play;
GtkMenuItem *menuitem_pause;
GtkMenuItem *menuitem_stop;
GtkMenuItem *menuitem_sep1;
GtkMenuItem *menuitem_sep2;
GtkMenuItem *menuitem_sep3;
GtkMenuItem *menuitem_showcontrols;
GtkMenuItem *menuitem_fullscreen;
GtkMenuItem *menuitem_config;
GtkMenuItem *menuitem_quit;
gulong delete_signal_id;


GtkWidget *vbox;
GtkWidget *hbox;
GtkWidget *controls_box;

GtkWidget *fixed;

GtkWidget *drawing_area;
GdkPixbuf *pb_play;
GdkPixbuf *pb_pause;
GdkPixbuf *pb_stop;
GdkPixbuf *pb_ff;
GdkPixbuf *pb_rew;
GdkPixbuf *pb_fs;
GdkPixbuf *pb_icon;
GdkPixbuf *pb_button;
GtkWidget *button_event_box;
GtkWidget *image_button;

GtkWidget *play_button;
GtkWidget *stop_button;
//GtkWidget *pause_button;
GtkWidget *ff_button;
GtkWidget *rew_button;

GtkWidget *play_event_box;
//GtkWidget *pause_event_box;
GtkWidget *stop_event_box;
GtkWidget *ff_event_box;
GtkWidget *rew_event_box;
GtkWidget *fs_event_box;
GtkProgressBar *progress;
GtkWidget *vol_slider;

GtkWidget *image_play;
GtkWidget *image_pause;
GtkWidget *image_stop;
GtkWidget *image_ff;
GtkWidget *image_rew;
GtkWidget *image_fs;
GtkWidget *image_icon;
GtkTooltips *tooltip;
GtkTooltips *volume_tip;
GtkWidget *song_title;

GtkWidget *config_vo;
GtkWidget *config_ao;
GtkWidget *config_cachesize;

GtkAccelGroup *accel_group;

gboolean popup_handler(GtkWidget * widget, GdkEvent * event, void *data);
gboolean delete_callback(GtkWidget * widget, GdkEvent * event, void *data);

gboolean rew_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
gboolean play_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
gboolean pause_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
gboolean stop_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
gboolean ff_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
void vol_slider_callback(GtkRange * range, gpointer user_data);
gboolean fs_callback(GtkWidget * widget, GdkEventExpose * event, void *data);


void menuitem_open_callback(GtkMenuItem * menuitem, void *data);
void menuitem_quit_callback(GtkMenuItem * menuitem, void *data);
void menuitem_about_callback(GtkMenuItem * menuitem, void *data);
void menuitem_play_callback(GtkMenuItem * menuitem, void *data);
void menuitem_pause_callback(GtkMenuItem * menuitem, void *data);
void menuitem_stop_callback(GtkMenuItem * menuitem, void *data);
void menuitem_fs_callback(GtkMenuItem * menuitem, void *data);
void menuitem_showcontrols_callback(GtkCheckMenuItem * menuitem, void *data);
void menuitem_quit_callback(GtkMenuItem * menuitem, void *data);

