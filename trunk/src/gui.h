/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gui.h
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * gui.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * gui.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with gui.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>
#include "playlist.h"

GtkWidget *window;
GdkWindow *window_container;

GtkWidget *menubar;
GtkMenuItem *menuitem_file;
GtkMenu *menu_file;
GtkMenuItem *menuitem_file_open;
GtkMenuItem *menuitem_file_open_location;
GtkMenuItem *menuitem_file_dvd;
GtkMenu *menu_file_dvd;
GtkMenuItem *menuitem_file_open_dvd;
GtkMenuItem *menuitem_file_open_dvdnav;
GtkMenuItem *menuitem_file_open_acd;
GtkMenuItem *menuitem_file_tv;
GtkMenu *menu_file_tv;
GtkMenuItem *menuitem_file_open_atv;
GtkMenuItem *menuitem_file_open_dtv;
GtkMenuItem *menuitem_file_open_playlist;
GtkMenuItem *menuitem_file_sep1;
GtkMenuItem *menuitem_file_details;
GtkMenuItem *menuitem_file_sep2;
GtkMenuItem *menuitem_file_quit;
GtkMenuItem *menuitem_edit;
GtkMenu *menu_edit;
GtkMenuItem *menuitem_edit_random;
GtkMenuItem *menuitem_edit_loop;
GtkMenuItem *menuitem_edit_switch_audio;
GtkMenuItem *menuitem_edit_set_subtitle;
GtkMenuItem *menuitem_edit_sep1;
GtkMenuItem *menuitem_edit_config;
GtkMenuItem *menuitem_help;
GtkMenuItem *menuitem_view;
GtkMenu *menu_view;
GtkMenuItem *menuitem_view_playlist;
GtkMenuItem *menuitem_view_info;
GtkMenuItem *menuitem_view_sep0;
GtkMenuItem *menuitem_view_fullscreen;
GtkMenuItem *menuitem_view_sep1;
GtkMenuItem *menuitem_view_onetoone;
GtkMenuItem *menuitem_view_twotoone;
GtkMenuItem *menuitem_view_onetotwo;
GtkMenuItem *menuitem_view_sep4;
GtkMenuItem *menuitem_view_aspect;
GtkMenu *menu_view_aspect;
GtkMenuItem *menuitem_view_aspect_default;
GtkMenuItem *menuitem_view_aspect_four_three;
GtkMenuItem *menuitem_view_aspect_sixteen_nine;
GtkMenuItem *menuitem_view_aspect_sixteen_ten;
GtkMenuItem *menuitem_view_sep2;
GtkMenuItem *menuitem_view_subtitles;
GtkMenuItem *menuitem_view_controls;
GtkMenuItem *menuitem_view_sep3;
GtkMenuItem *menuitem_view_advanced;
GtkMenu *menu_help;
GtkMenuItem *menuitem_help_about;


GtkMenu *popup_menu;
GtkMenuItem *menuitem_open;
GtkMenuItem *menuitem_sep3;
GtkMenuItem *menuitem_play;
GtkMenuItem *menuitem_pause;
GtkMenuItem *menuitem_stop;
GtkMenuItem *menuitem_sep1;
GtkMenuItem *menuitem_copyurl;
GtkMenuItem *menuitem_sep2;
GtkMenuItem *menuitem_sep4;
GtkMenuItem *menuitem_save;
GtkMenuItem *menuitem_showcontrols;
GtkMenuItem *menuitem_fullscreen;
GtkMenuItem *menuitem_config;
GtkMenuItem *menuitem_quit;
gulong delete_signal_id;

GtkWidget *vbox_master;
GtkWidget *pane;
GtkWidget *vbox;
GtkWidget *hbox;
GtkWidget *controls_box;

GtkWidget *fixed;
GtkWidget *media_label;
GtkWidget *details_vbox;
GtkWidget *details_table;

GtkWidget *drawing_area;
GdkPixbuf *pb_play;
GdkPixbuf *pb_pause;
GdkPixbuf *pb_stop;
GdkPixbuf *pb_ff;
GdkPixbuf *pb_rew;
GdkPixbuf *pb_fs;
GdkPixbuf *pb_next;
GdkPixbuf *pb_prev;
GdkPixbuf *pb_menu;
GdkPixbuf *pb_icon;
GdkPixbuf *pb_logo;
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
GtkWidget *prev_event_box;
GtkWidget *next_event_box;
GtkWidget *menu_event_box;

GtkWidget *fs_event_box;
GtkProgressBar *progress;
GtkWidget *vol_slider;

gboolean in_button;

GtkWidget *image_play;
GtkWidget *image_pause;
GtkWidget *image_stop;
GtkWidget *image_ff;
GtkWidget *image_rew;
GtkWidget *image_next;
GtkWidget *image_prev;
GtkWidget *image_menu;
GtkWidget *image_fs;
GtkWidget *image_icon;
GtkTooltips *tooltip;
GtkTooltips *volume_tip;

GtkWidget *config_vo;
GtkWidget *config_ao;
GtkWidget *config_cachesize;
GtkWidget *config_osdlevel;
GtkWidget *config_deinterlace;
GtkWidget *config_framedrop;
GtkWidget *config_pplevel;

GtkWidget *config_playlist_visible;
GtkWidget *config_vertical_layout;
GtkWidget *config_pause_on_click;
GtkWidget *config_softvol;
GtkWidget *config_forcecache;
GtkWidget *config_verbose;

GtkWidget *config_alang;
GtkWidget *config_slang;
GtkWidget *config_metadata_codepage;

GtkWidget *config_ass;
GtkWidget *config_embeddedfonts;
GtkWidget *config_subtitle_font;
GtkWidget *config_subtitle_scale;
GtkWidget *config_subtitle_codepage;
GtkWidget *config_subtitle_color;

GtkWidget *config_qt;
GtkWidget *config_real;
GtkWidget *config_wmp;
GtkWidget *config_dvx;
GtkWidget *config_noembed;

GtkWidget *config_extraopts;

GtkWidget *open_location;

// Playlist container
GtkWidget *plvbox;


GtkAccelGroup *accel_group;

glong last_movement_time;

gboolean popup_handler(GtkWidget * widget, GdkEvent * event, void *data);
gboolean delete_callback(GtkWidget * widget, GdkEvent * event, void *data);
void config_close(GtkWidget * widget, void *data);

gboolean rew_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
gboolean play_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
gboolean pause_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
gboolean stop_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
gboolean ff_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
gboolean prev_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
gboolean next_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
void vol_slider_callback(GtkRange * range, gpointer user_data);
gboolean fs_callback(GtkWidget * widget, GdkEventExpose * event, void *data);
gboolean make_panel_and_mouse_visible(gpointer data);
void menuitem_open_callback(GtkMenuItem * menuitem, void *data);
void menuitem_quit_callback(GtkMenuItem * menuitem, void *data);
void menuitem_about_callback(GtkMenuItem * menuitem, void *data);
void menuitem_play_callback(GtkMenuItem * menuitem, void *data);
void menuitem_pause_callback(GtkMenuItem * menuitem, void *data);
void menuitem_stop_callback(GtkMenuItem * menuitem, void *data);
void menuitem_fs_callback(GtkMenuItem * menuitem, void *data);
void menuitem_showcontrols_callback(GtkCheckMenuItem * menuitem, void *data);
void menuitem_quit_callback(GtkMenuItem * menuitem, void *data);
void menuitem_details_callback(GtkMenuItem * menuitem, void *data);

gboolean playlist_drop_callback(GtkWidget * widget, GdkDragContext * dc,
                                gint x, gint y, GtkSelectionData * selection_data,
                                guint info, guint t, gpointer data);
