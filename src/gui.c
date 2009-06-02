/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gui.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * gui.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * gui.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with gui.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gui.h"
#include "support.h"
#include "common.h"
#include "../pixmaps/gnome_mplayer.xpm"
#include "../pixmaps/media-playback-pause.xpm"
#include "../pixmaps/media-playback-start.xpm"
#include "../pixmaps/media-playback-stop.xpm"
#include "../pixmaps/media-seek-backward.xpm"
#include "../pixmaps/media-seek-forward.xpm"
#include "../pixmaps/media-skip-backward.xpm"
#include "../pixmaps/media-skip-forward.xpm"
#include "../pixmaps/view-fullscreen.xpm"
#include "langlist.h"
#ifdef NOTIFY_ENABLED
#include <libnotify/notify.h>
#include <libnotify/notification.h>
#endif
#ifdef HAVE_ASOUNDLIB
static char *device = "default";
#endif

gint get_player_window()
{
    if (GTK_IS_WIDGET(drawing_area)) {
        while (gtk_events_pending())
            gtk_main_iteration();
        return gtk_socket_get_id(GTK_SOCKET(drawing_area));
    } else {
        return 0;
    }
}

void adjust_paned_rules()
{

    if (GTK_IS_WIDGET(pane)) {
        if (!idledata->videopresent) {
            g_object_ref(vbox);
            gtk_container_remove(GTK_CONTAINER(pane), GTK_PANED(pane)->child1);
            gtk_paned_pack1(GTK_PANED(pane), vbox, FALSE, FALSE);
            if (GTK_IS_WIDGET(plvbox)) {
                if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_info))) {
                    if (vertical_layout) {
                        g_object_set(pane, "position",
                                     window->allocation.height - plvbox->allocation.height, NULL);
                    } else {
                        g_object_set(pane, "position",
                                     window->allocation.width - plvbox->allocation.width, NULL);
                    }
                }
            }
            g_object_unref(vbox);
            if (GTK_IS_WIDGET(plvbox)) {
                g_object_ref(plvbox);
                gtk_container_remove(GTK_CONTAINER(pane), GTK_PANED(pane)->child2);
                gtk_paned_pack2(GTK_PANED(pane), plvbox, TRUE, TRUE);
                g_object_unref(plvbox);
            }
        } else {
            g_object_set(pane, "position-set", FALSE, NULL);
        }
    }
}

void reset_paned_rules()
{
    gint position;

    if (GTK_IS_WIDGET(pane)) {
        g_object_get(pane, "position", &position, NULL);
        g_object_ref(vbox);
        gtk_container_remove(GTK_CONTAINER(pane), vbox);
        gtk_paned_pack1(GTK_PANED(pane), vbox, TRUE, TRUE);
        g_object_unref(vbox);
        if (GTK_IS_WIDGET(plvbox)) {
            g_object_ref(plvbox);
            gtk_container_remove(GTK_CONTAINER(pane), GTK_PANED(pane)->child2);
            gtk_paned_pack2(GTK_PANED(pane), plvbox, TRUE, TRUE);
            g_object_unref(plvbox);
        }
        g_object_set(pane, "position", position, "position-set", TRUE, NULL);
    }
}

gboolean hide_buttons(void *data)
{
    IdleData *idle = (IdleData *) data;

    if (GTK_IS_WIDGET(ff_event_box)) {
        if (idle->streaming) {
            gtk_widget_hide(ff_event_box);
            gtk_widget_hide(rew_event_box);
            if (GTK_IS_WIDGET(menuitem_save))
                gtk_widget_set_sensitive(GTK_WIDGET(menuitem_save), FALSE);
        } else {
            if (embed_window == 0 || window_x > 250) {
                gtk_widget_show_all(ff_event_box);
                gtk_widget_show_all(rew_event_box);
            }
        }
    }
    if (GTK_IS_WIDGET(menuitem_view_details))
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_details), TRUE);

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) < 2 && lastfile != NULL
        && idledata->has_chapters == FALSE) {
        gtk_widget_hide(prev_event_box);
        gtk_widget_hide(next_event_box);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), FALSE);
        gtk_widget_hide(GTK_WIDGET(menuitem_prev));
        gtk_widget_hide(GTK_WIDGET(menuitem_next));
    } else {
        gtk_widget_show_all(prev_event_box);
        gtk_widget_show_all(next_event_box);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), TRUE);
        gtk_widget_show(GTK_WIDGET(menuitem_prev));
        gtk_widget_show(GTK_WIDGET(menuitem_next));
    }

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) > 0) {
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_loop), TRUE);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_loop), FALSE);
    }

    return FALSE;
}

gboolean show_copyurl(void *data)
{
    IdleData *idle = (IdleData *) data;
    gchar *buf;

    gtk_widget_show(GTK_WIDGET(menuitem_copyurl));
    buf = g_strdup_printf(_("%s - GNOME MPlayer"), idle->url);
    gtk_window_set_title(GTK_WINDOW(window), buf);
    g_free(buf);

    return FALSE;

}


gboolean set_media_info(void *data)
{

    IdleData *idle = (IdleData *) data;
    gchar *buf;
    gchar *name;
    GtkTreePath *path;
    gint current = 0, total;


    if (data != NULL && idle != NULL) {
        name = g_strdup(idle->display_name);

        total = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
        path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &iter);
        if (path != NULL) {
            buf = gtk_tree_path_to_string(path);
            current = (gint) g_strtod(buf, NULL);
            g_free(buf);
            gtk_tree_path_free(path);
        }
        if (total > 1) {
            buf = g_strdup_printf(_("%s - (%i/%i) - GNOME MPlayer"), name, current + 1, total);
        } else {
            if (name == NULL || strlen(name) < 1) {
                buf = g_strdup_printf(_("GNOME MPlayer"));
            } else {
                buf = g_strdup_printf(_("%s - GNOME MPlayer"), name);
            }
        }
        gtk_window_set_title(GTK_WINDOW(window), buf);
        g_free(buf);
        g_free(name);
    }
    return FALSE;
}

gboolean set_media_label(void *data)
{

    IdleData *idle = (IdleData *) data;
    gpointer pixbuf;
#ifdef NOTIFY_ENABLED
    NotifyNotification *notification;
#endif
    if (data != NULL && idle != NULL && GTK_IS_WIDGET(media_label)) {
        if (idle->media_info != NULL && strlen(idle->media_info) > 0)
            gtk_label_set_markup(GTK_LABEL(media_label), idle->media_info);
        gtk_label_set_max_width_chars(GTK_LABEL(media_label), 10);

        pixbuf = NULL;
        gtk_image_clear(GTK_IMAGE(cover_art));
        if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, COVERART_COLUMN, &pixbuf, -1);
        }
        if (pixbuf != NULL) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(cover_art), GDK_PIXBUF(pixbuf));
            g_object_unref(pixbuf);
        }
    }

    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_info)) && control_id == 0
        && strlen(idle->media_info) > 0) {
        gtk_widget_show_all(media_hbox);
    } else {
        gtk_widget_hide(media_hbox);
    }
    // gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menuitem_view_info),show_media_label);

    if (strlen(idle->video_format) > 0
        || strlen(idle->video_codec) > 0
        || strlen(idle->video_fps) > 0
        || strlen(idle->video_bitrate) > 0
        || strlen(idle->audio_codec) > 0
        || strlen(idle->audio_bitrate) > 0 || strlen(idle->audio_samplerate) > 0) {

        /*
           printf("details: %i,%i,%i,%i,%i,%i,%i\n",    strlen(idle->video_format),strlen(idle->video_codec),
           strlen(idle->video_fps),strlen(idle->video_bitrate),strlen(idle->audio_codec),
           strlen(idle->audio_bitrate),strlen(idle->audio_samplerate));
           printf("details: %s,%s,%s,%s,%s,%s,%s\n",    idle->video_format,idle->video_codec,
           idle->video_fps,idle->video_bitrate,idle->audio_codec,
           idle->audio_bitrate,idle->audio_samplerate);
         */

        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_details), TRUE);
    }

    if (idle->fromdbus == FALSE) {
        dbus_send_rpsignal_with_string("RP_SetMediaLabel", idle->media_info);

#ifdef NOTIFY_ENABLED
        if (show_notification && control_id == 0 && !gtk_window_is_active((GtkWindow *) window)) {
            notify_init("gnome-mplayer");
            notification =
                notify_notification_new(_("Media Change"), idle->media_notification,
                                        "gnome-mplayer", NULL);
            if (show_status_icon)
                notify_notification_attach_to_status_icon(notification, status_icon);
            notify_notification_show(notification, NULL);
            notify_uninit();
        }
#endif
        if (embed_window == 0
            && gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) != 1) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist),
                                           playlist_visible);
        }

    }
    return FALSE;
}

gboolean set_cover_art(gpointer pixbuf)
{
    if (pixbuf == NULL) {
        if (GTK_IS_IMAGE(cover_art))
            gtk_image_clear(GTK_IMAGE(cover_art));
        if (strlen(idledata->media_info) > 0) {
            gtk_widget_show_all(media_hbox);
        } else {
            gtk_widget_hide_all(media_hbox);
        }
    } else {
        gtk_image_set_from_pixbuf(GTK_IMAGE(cover_art), GDK_PIXBUF(pixbuf));
        g_object_unref(pixbuf);
        gtk_widget_show_all(media_hbox);
    }

    return FALSE;
}

gboolean set_progress_value(void *data)
{

    IdleData *idle = (IdleData *) data;
    gchar *text;
    struct stat buf;
    gchar *iterfilename;
    gchar *iteruri;

    if (idle == NULL)
        return FALSE;

    if (GTK_IS_WIDGET(tracker)) {
        if (state == QUIT && rpcontrols == NULL) {
            js_state = STATE_BUFFERING;
            if (idle->cachepercent >= 0.0 && idle->cachepercent <= 1.0)
                gmtk_media_tracker_set_cache_percentage(tracker, idle->cachepercent);
            gtk_widget_set_sensitive(play_event_box, FALSE);
        } else {
            if (idle->percent >= 0.0 && idle->percent <= 1.0) {
                gmtk_media_tracker_set_percentage(tracker, idle->percent);
                if (autopause == FALSE)
                    gtk_widget_set_sensitive(play_event_box, TRUE);
            } else {
                gmtk_media_tracker_set_thumb_position(tracker, THUMB_HIDDEN);
            }
        }
        if (idle->cachepercent < 1.0 && state == PAUSED) {
            text =
                g_strdup_printf(_("Paused | %2i%% \342\226\274"),
                                (gint) (idle->cachepercent * 100));
            gmtk_media_tracker_set_text(tracker, text);
            gmtk_media_tracker_set_cache_percentage(tracker, idle->cachepercent);
            g_free(text);
        } else {
            gmtk_media_tracker_set_text(tracker, idle->progress_text);
        }
    }

    if (idle->cachepercent > 0.0 && idle->cachepercent < 0.9) {
        if (autopause == FALSE && state == PLAYING) {
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iteruri, -1);
            if (iteruri != NULL) {
                iterfilename = g_filename_from_uri(iteruri, NULL, NULL);
                g_stat(iterfilename, &buf);
                if (verbose > 1) {
                    printf("filename = %s\ndisk size = %li, byte pos = %li\n", iterfilename,
                           (glong) buf.st_size, idle->byte_pos);
                    printf("cachesize = %f, percent = %f\n", idle->cachepercent, idle->percent);
                    printf("will pause = %i\n",
                           ((idle->byte_pos + (cache_size * 512)) > buf.st_size)  && !(playlist));
                }
                // if ((idle->percent + 0.10) > idle->cachepercent && ((idle->byte_pos + (512 * 1024)) > buf.st_size)) {
                // if ((buf.st_size > 0) && (idle->byte_pos + (cache_size * 512)) > buf.st_size) {
                if (((idle->byte_pos + (cache_size * 512)) > buf.st_size) && !(playlist)) {
                    pause_callback(NULL, NULL, NULL);
                    gtk_widget_set_sensitive(play_event_box, FALSE);
                    autopause = TRUE;
                }
                g_free(iterfilename);
                g_free(iteruri);
            }
        } else if (autopause == TRUE && state == PAUSED) {
            if (idle->cachepercent > (idle->percent + 0.20)) {
                gmtk_media_tracker_set_cache_percentage(tracker, idle->cachepercent);
                play_callback(NULL, NULL, NULL);
                gtk_widget_set_sensitive(play_event_box, TRUE);
                autopause = FALSE;
            }
        }
    }

    if (idle->cachepercent > 0.0 && idle->position == 0) {
        gmtk_media_tracker_set_cache_percentage(tracker, idle->cachepercent);
    }

    if (idle->cachepercent > 0.9) {
        if (autopause == TRUE && state == PAUSED) {
            gmtk_media_tracker_set_cache_percentage(tracker, idle->cachepercent);
            play_callback(NULL, NULL, NULL);
            gtk_widget_set_sensitive(play_event_box, TRUE);
            autopause = FALSE;
        }
    }

    if (state == QUIT) {
        gtk_widget_set_sensitive(play_event_box, TRUE);
    }

    update_status_icon();

    if (idle->fromdbus == FALSE) {
        dbus_send_rpsignal_with_double("RP_SetPercent", idle->percent);
        dbus_send_rpsignal_with_int("RP_SetGUIState", state);
    }

    return FALSE;
}

gboolean set_progress_text(void *data)
{

    IdleData *idle = (IdleData *) data;


    if (GTK_IS_WIDGET(tracker)) {
        gmtk_media_tracker_set_text(tracker, idle->progress_text);
    }
    if (idle != NULL && idle->fromdbus == FALSE)
        dbus_send_rpsignal_with_string("RP_SetProgressText", idle->progress_text);
    update_status_icon();
    return FALSE;
}

gboolean set_progress_time(void *data)
{
    gfloat seconds, length_seconds;
    gchar *time_position = NULL;
    gchar *time_length = NULL;

    IdleData *idle = (IdleData *) data;

    if (idle == NULL)
        return FALSE;

    seconds = idle->position;
    length_seconds = idle->length;

    time_position = seconds_to_string(seconds);
    time_length = seconds_to_string(length_seconds);


    if ((int) idle->length == 0 || idle->position > idle->length) {
        if (idle->cachepercent > 0 && idle->cachepercent < 1.0 && !(playlist)) {
            g_snprintf(idle->progress_text, 128,
                       "%s | %2i%% \342\226\274", time_position, (int) (idle->cachepercent * 100));
            gmtk_media_tracker_set_cache_percentage(tracker, idle->cachepercent);
        } else {
            g_snprintf(idle->progress_text, 128, "%s", time_position);
            gmtk_media_tracker_set_cache_percentage(tracker, 0.0);
        }
        gmtk_media_tracker_set_thumb_position(tracker, THUMB_HIDDEN);
    } else {
        if (idle->cachepercent > 0 && idle->cachepercent < 1.0 && !(playlist)) {
            g_snprintf(idle->progress_text, 128,
                       "%s / %s | %2i%% \342\226\274",
                       time_position, time_length, (int) (idle->cachepercent * 100));
            gmtk_media_tracker_set_cache_percentage(tracker, idle->cachepercent);
        } else {
            g_snprintf(idle->progress_text, 128, "%s / %s", time_position, time_length);
            gmtk_media_tracker_set_cache_percentage(tracker, 0.0);
        }
        gmtk_media_tracker_set_thumb_position(tracker, thumb_position);
    }

    g_free(time_position);
    g_free(time_length);

    if (GTK_IS_WIDGET(tracker) && idle->position > 0 && state != PAUSED) {
        gmtk_media_tracker_set_text(tracker, idle->progress_text);
    }

    if (idle->fromdbus == FALSE && state != PAUSED)
        dbus_send_rpsignal_with_string("RP_SetProgressText", idle->progress_text);
    update_status_icon();

    return FALSE;
}

gboolean set_volume_from_slider(gpointer data)
{
    gint vol;
    gchar *cmd;

#ifdef GTK2_12_ENABLED
    vol = (gint) gtk_scale_button_get_value(GTK_SCALE_BUTTON(vol_slider));
#else
    vol = (gint) gtk_range_get_value(GTK_RANGE(vol_slider));
#endif
    cmd = g_strdup_printf("volume %i 1\n", vol);
    send_command(cmd, FALSE);
    g_free(cmd);
    send_command("get_property volume\n", FALSE);
    if (state == PAUSED || state == STOPPED) {
        send_command("pause\n", FALSE);
    }

    return FALSE;
}

gboolean set_volume_tip(void *data)
{

    IdleData *idle = (IdleData *) data;

    if (GTK_IS_WIDGET(vol_slider)) {
        gtk_tooltips_set_tip(volume_tip, vol_slider, idle->vol_tooltip, NULL);
    }
    return FALSE;
}

gboolean set_window_visible(void *data)
{
    //gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist),FALSE);
    if (GTK_IS_WIDGET(fixed)) {
        if (vertical_layout) {
            gtk_widget_show(fixed);
        } else {
            gtk_widget_show(vbox);
        }
    }
    return FALSE;
}

gboolean set_subtitle_visibility(void *data)
{
    if (GTK_IS_WIDGET(menuitem_view_subtitles)) {
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_subtitles))) {
            send_command("set_property sub_visibility 1\n", TRUE);
            idledata->sub_visible = TRUE;
        } else {
            send_command("set_property sub_visibility 0\n", TRUE);
            idledata->sub_visible = FALSE;
        }
    }
    return FALSE;
}

gboolean set_update_gui(void *data)
{
    GtkTreeViewColumn *column;
    gchar *coltitle;
    gint count;
    GList *langs;
    GList *item;

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) < 2
        && idledata->has_chapters == FALSE) {
        gtk_widget_hide(prev_event_box);
        gtk_widget_hide(next_event_box);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), FALSE);
        gtk_widget_hide(GTK_WIDGET(menuitem_prev));
        gtk_widget_hide(GTK_WIDGET(menuitem_next));
    } else {
        gtk_widget_show(prev_event_box);
        gtk_widget_show(next_event_box);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), TRUE);
        gtk_widget_show(GTK_WIDGET(menuitem_prev));
        gtk_widget_show(GTK_WIDGET(menuitem_next));
    }

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) > 0) {
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_loop), TRUE);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_loop), FALSE);
    }

/*
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_subtitles)) !=
        idledata->sub_visible) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_subtitles),
                                       idledata->sub_visible);
    }
*/

    if (GTK_IS_WIDGET(menu_edit_sub_langs)) {
        langs = gtk_container_get_children(GTK_CONTAINER(menu_edit_sub_langs));
        item = g_list_nth(langs, idledata->sub_demux);
        if (item && GTK_IS_WIDGET(item->data))
            gtk_menu_shell_activate_item(GTK_MENU_SHELL(menu_edit_sub_langs),
                                         (GtkWidget *) item->data, FALSE);
    }

    if (idledata->switch_audio >= 0) {
        if (GTK_IS_WIDGET(menu_edit_audio_langs)) {
            langs = gtk_container_get_children(GTK_CONTAINER(menu_edit_audio_langs));
            if (g_list_length(langs) > 1) {
                gtk_widget_show(GTK_WIDGET(menuitem_edit_switch_audio));
            } else {
                gtk_widget_hide(GTK_WIDGET(menuitem_edit_switch_audio));
            }
            item = g_list_nth(langs, idledata->switch_audio);
            if (item && GTK_IS_WIDGET(item->data))
                gtk_menu_shell_activate_item(GTK_MENU_SHELL(menu_edit_audio_langs),
                                             (GtkWidget *) item->data, FALSE);
        }
    } else {
        if (verbose)
            printf("ANS_switch_audio is invalid %i\n", idledata->switch_audio);
    }

    if (GTK_IS_WIDGET(list)) {
        column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
        count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
        if (playlistname != NULL && strlen(playlistname) > 0) {
            coltitle = g_strdup_printf(_("%s items"), playlistname);
        } else {
            coltitle = g_strdup_printf(ngettext("Item to Play", "Items to Play", count));
        }
        gtk_tree_view_column_set_title(column, coltitle);
        g_free(coltitle);
    }
    return FALSE;
}

gboolean set_gui_state(void *data)
{
    if (lastguistate != guistate) {
        if (guistate == PLAYING) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_pause);
            gtk_tooltips_set_tip(tooltip, play_event_box, _("Pause"), NULL);
            gtk_widget_set_sensitive(ff_event_box, TRUE);
            gtk_widget_set_sensitive(rew_event_box, TRUE);
            gtk_container_remove(GTK_CONTAINER(popup_menu), GTK_WIDGET(menuitem_play));
            menuitem_play =
                GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PAUSE, NULL));
            gtk_menu_shell_insert(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menuitem_play), 2);
            gtk_widget_show(GTK_WIDGET(menuitem_play));

        }

        if (guistate == PAUSED) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_play);
            gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
            gtk_widget_set_sensitive(ff_event_box, FALSE);
            gtk_widget_set_sensitive(rew_event_box, FALSE);
            gtk_container_remove(GTK_CONTAINER(popup_menu), GTK_WIDGET(menuitem_play));
            menuitem_play =
                GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL));
            gtk_menu_shell_insert(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menuitem_play), 2);
            gtk_widget_show(GTK_WIDGET(menuitem_play));
        }

        if (guistate == STOPPED) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_play);
            gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
            gtk_widget_set_sensitive(ff_event_box, FALSE);
            gtk_widget_set_sensitive(rew_event_box, FALSE);
            gtk_container_remove(GTK_CONTAINER(popup_menu), GTK_WIDGET(menuitem_play));
            menuitem_play =
                GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL));
            gtk_menu_shell_insert(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menuitem_play), 2);
            gtk_widget_show(GTK_WIDGET(menuitem_play));
        }
        lastguistate = guistate;
    }
    return FALSE;
}

void cancel_clicked(GtkButton * button, gpointer user_data)
{
    cancel_folder_load = TRUE;
}

void create_folder_progress_window()
{
    GtkWidget *vbox;
    GtkWidget *cancel;
    GtkWidget *hbox;

    cancel_folder_load = FALSE;

    folder_progress_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_modal(GTK_WINDOW(folder_progress_window), TRUE);
    gtk_window_set_icon(GTK_WINDOW(folder_progress_window), pb_icon);
    gtk_window_set_resizable(GTK_WINDOW(folder_progress_window), FALSE);
    gtk_widget_set_size_request(folder_progress_window, 400, -1);

    vbox = gtk_vbox_new(FALSE, 10);
    folder_progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(folder_progress_bar), 0.10);
    folder_progress_label = gtk_label_new("");
    gtk_label_set_ellipsize(GTK_LABEL(folder_progress_label), PANGO_ELLIPSIZE_MIDDLE);

    cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(G_OBJECT(cancel), "clicked", G_CALLBACK(cancel_clicked), NULL);
    hbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
    gtk_container_add(GTK_CONTAINER(hbox), cancel);

    gtk_container_add(GTK_CONTAINER(vbox), folder_progress_bar);
    gtk_container_add(GTK_CONTAINER(vbox), folder_progress_label);
    gtk_container_add(GTK_CONTAINER(vbox), hbox);
    gtk_container_add(GTK_CONTAINER(folder_progress_window), vbox);

    gtk_widget_show_all(folder_progress_window);
    while (gtk_events_pending())
        gtk_main_iteration();
}

void destroy_folder_progress_window()
{
    while (gtk_events_pending())
        gtk_main_iteration();
    if (GTK_IS_WIDGET(folder_progress_window))
        gtk_widget_destroy(folder_progress_window);
    folder_progress_window = NULL;
    if (cancel_folder_load)
        clear_playlist(NULL, NULL);
    cancel_folder_load = FALSE;
}


gboolean set_item_add_info(void *data)
{
    gchar *message;

    if (data == NULL)
        return FALSE;

    if (GTK_IS_WIDGET(folder_progress_window)) {
        message = g_strdup_printf(_("Adding %s to playlist"), (gchar *) data);
        gtk_label_set_text(GTK_LABEL(folder_progress_label), message);
        gtk_progress_bar_pulse(GTK_PROGRESS_BAR(folder_progress_bar));
        g_free(message);
        while (gtk_events_pending())
            gtk_main_iteration();
    }

    return FALSE;
}

void remove_langs(GtkWidget * item, gpointer data)
{
    if (GTK_IS_WIDGET(item))
        gtk_widget_destroy(item);
}

void update_status_icon()
{
#ifdef GTK2_12_ENABLED
    gchar *text;

    if (state == PLAYING) {
        text = g_strdup_printf(_("Playing %s"), idledata->display_name);
    } else if (state == PAUSED) {
        text = g_strdup_printf(_("Paused %s"), idledata->display_name);
    } else {
        text = g_strdup_printf(_("Idle"));
    }

    gtk_status_icon_set_tooltip(status_icon, text);

    g_free(text);
#endif
}

void menuitem_lang_callback(GtkMenuItem * menuitem, gpointer sid)
{
    gchar *cmd;

    if (GPOINTER_TO_INT(sid) >= 0) {
        cmd = g_strdup_printf("sub_demux %i\n", GPOINTER_TO_INT(sid));
        send_command(cmd, TRUE);
        g_free(cmd);
    }
}

gboolean set_new_lang_menu(gpointer data)
{
    LangMenu *menu = (LangMenu *) data;
    GList *children;
    GList *sub_children;
    GList *item;
    GList *sub_item;
    gboolean found = FALSE;
    const gchar *text;
    gint value;

    if (menu->value >= 0) {
        children = gtk_container_get_children(GTK_CONTAINER(menu_edit_sub_langs));
        item = g_list_first(children);
        while (item && !found) {
            value = GPOINTER_TO_INT(g_object_get_data(item->data, "id"));
            sub_children = gtk_container_get_children(GTK_CONTAINER(item->data));
            sub_item = g_list_first(sub_children);
            while (sub_item && !found) {
                text = gtk_label_get_text(GTK_LABEL(sub_item->data));
                if (menu->value == value) {
                    if (g_ascii_isdigit(text[0])) {
                        gtk_label_set_text(GTK_LABEL(sub_item->data), menu->label);
                    }
                    found = TRUE;
                }
                sub_item = g_list_next(sub_item);
            }
            item = g_list_next(item);
        }

        if (!found) {
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_select_sub_lang), TRUE);

            menuitem_lang =
                GTK_MENU_ITEM(gtk_radio_menu_item_new_with_label(lang_group, menu->label));
            g_object_set_data(G_OBJECT(menuitem_lang), "id", GINT_TO_POINTER(menu->value));

            lang_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem_lang));

            gtk_menu_append(menu_edit_sub_langs, GTK_WIDGET(menuitem_lang));
            g_signal_connect(GTK_OBJECT(menuitem_lang), "activate",
                             G_CALLBACK(menuitem_lang_callback), GINT_TO_POINTER(menu->value));
        }
    }
    gtk_widget_show(GTK_WIDGET(menuitem_lang));
    g_free(menu->label);
    g_free(menu);
    return FALSE;
}

void menuitem_audio_callback(GtkMenuItem * menuitem, gpointer aid)
{
    gchar *cmd;

    if (GPOINTER_TO_INT(aid) >= 0) {
        cmd = g_strdup_printf("switch_audio %i\n", GPOINTER_TO_INT(aid));
        send_command(cmd, TRUE);
        g_free(cmd);
    }
}

gboolean set_new_audio_menu(gpointer data)
{
    LangMenu *menu = (LangMenu *) data;
    GList *children;
    GList *sub_children;
    GList *item;
    GList *sub_item;
    gboolean found = FALSE;
    const gchar *text;
    gint value;

    if (menu->value >= 0) {
        children = gtk_container_get_children(GTK_CONTAINER(menu_edit_audio_langs));
        item = g_list_first(children);
        while (item && !found) {
            value = GPOINTER_TO_INT(g_object_get_data(item->data, "id"));
            sub_children = gtk_container_get_children(GTK_CONTAINER(item->data));
            sub_item = g_list_first(sub_children);
            while (sub_item && !found) {
                text = gtk_label_get_text(GTK_LABEL(sub_item->data));
                if (menu->value == value) {
                    if (g_ascii_isdigit(text[0])) {
                        gtk_label_set_text(GTK_LABEL(sub_item->data), menu->label);
                    }
                    found = TRUE;
                }
                sub_item = g_list_next(sub_item);
            }
            item = g_list_next(item);
        }

        if (!found) {
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_select_audio_lang), TRUE);

            menuitem_lang =
                GTK_MENU_ITEM(gtk_radio_menu_item_new_with_label(audio_group, menu->label));
            g_object_set_data(G_OBJECT(menuitem_lang), "id", GINT_TO_POINTER(menu->value));
            audio_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem_lang));
            gtk_menu_append(menu_edit_audio_langs, GTK_WIDGET(menuitem_lang));
            g_signal_connect(GTK_OBJECT(menuitem_lang), "activate",
                             G_CALLBACK(menuitem_audio_callback), GINT_TO_POINTER(menu->value));
        }
    }
    gtk_widget_show(GTK_WIDGET(menuitem_lang));
    g_free(menu->label);
    g_free(menu);

    return FALSE;
}

gboolean resize_window(void *data)
{

    IdleData *idle = (IdleData *) data;
    gint total_height = 0;
    gint total_width = 0;
    gint handle_size;
    GTimeVal currenttime;

    if (GTK_IS_WIDGET(window)) {
        if (idle->videopresent) {
            gtk_widget_show(fixed);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), FALSE);
            g_get_current_time(&currenttime);
            last_movement_time = currenttime.tv_sec;
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_info), TRUE);
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_subtitle), TRUE);
            dbus_disable_screensaver();
            if (embed_window == -1) {
                gtk_widget_show_all(window);
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), FALSE);
                hide_buttons(idle);
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols),
                                               showcontrols);
            }
            gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, TRUE);

            if (window_x == 0 && window_y == 0) {
                gtk_widget_show_all(GTK_WIDGET(fixed));
                gtk_widget_set_size_request(fixed, -1, -1);
                gtk_widget_set_size_request(drawing_area, -1, -1);
                if (verbose) {
                    printf("Changing window size to %i x %i visible = %i\n", idle->width,
                           idle->height, GTK_WIDGET_VISIBLE(vbox));
                    //printf("last = %i x %i\n",last_window_width,last_window_height);
                }
                if (last_window_width == 0 && last_window_height == 0) {
                    if (idle->width > 0 && idle->height > 0) {
                        gtk_widget_set_size_request(fixed, idle->width, idle->height);
                        gtk_widget_set_size_request(drawing_area, idle->width, idle->height);
                        total_height = idle->height;
                        total_width = idle->width;
                        total_height += menubar->allocation.height;
                        if (showcontrols) {
                            total_height += controls_box->allocation.height;
                        }
                        if (GTK_IS_WIDGET(media_hbox) && GTK_WIDGET_VISIBLE(media_hbox)) {
                            total_height += media_hbox->allocation.height;
                        }
                        if (GTK_IS_WIDGET(details_table) && GTK_WIDGET_VISIBLE(details_table)) {
                            total_height += details_vbox->allocation.height;
                        }
                        if (GTK_IS_WIDGET(audio_meter) && GTK_WIDGET_VISIBLE(audio_meter)) {
                            total_height += audio_meter->allocation.height;
                        }

                        if (GTK_IS_WIDGET(plvbox) && GTK_WIDGET_VISIBLE(plvbox)) {
                            gtk_widget_style_get(pane, "handle-size", &handle_size, NULL);
                            if (vertical_layout) {
                                total_height += plvbox->allocation.height + handle_size;;
                                gtk_paned_set_position(GTK_PANED(pane), idle->height);
                            } else {
                                total_width += plvbox->allocation.width + handle_size;
                                gtk_paned_set_position(GTK_PANED(pane), idle->width);
                            }
                        }
                        // printf("totals = %i x %i\n",total_width,total_height);
                        gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
                        last_window_width = idle->width;
                        last_window_height = idle->height;
                    }
                } else {
                    if (GTK_IS_WIDGET(plvbox) && GTK_WIDGET_VISIBLE(plvbox)) {
                        gtk_widget_style_get(pane, "handle-size", &handle_size, NULL);

                        if (vertical_layout) {
                            total_height += plvbox->allocation.height + handle_size;;
                            gtk_paned_set_position(GTK_PANED(pane), idle->height);
                        } else {
                            total_width += plvbox->allocation.width + handle_size;
                            gtk_paned_set_position(GTK_PANED(pane), idle->width);
                        }
                    }
                }
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_fullscreen),
                                               fullscreen);
            } else {
                if (window_x > 0 && window_y > 0) {
                    total_height = window_y;
                    gtk_widget_set_size_request(fixed, -1, -1);
                    gtk_widget_set_size_request(drawing_area, -1, -1);
                    gtk_widget_show_all(GTK_WIDGET(fixed));
                    if (showcontrols) {
                        total_height -= controls_box->allocation.height;
                    }

                    if (window_x > 0 && total_height > 0)
                        gtk_widget_set_size_request(fixed, window_x, total_height);
                    gtk_window_resize(GTK_WINDOW(window), window_x, window_y);
                }
            }
        } else {
            gtk_widget_hide(fixed);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_fullscreen), FALSE);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), TRUE);
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_info), FALSE);
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_subtitle), FALSE);
            adjust_paned_rules();
            if (window_x > 0 && window_y > 0) {
                total_height = window_y;
                gtk_widget_set_size_request(fixed, -1, -1);
                gtk_widget_set_size_request(drawing_area, -1, -1);
                gtk_widget_hide_all(GTK_WIDGET(drawing_area));
                if (showcontrols && rpcontrols == NULL) {
                    total_height -= controls_box->allocation.height;
                }
                if (window_x > 0 && total_height > 0) {
                    gtk_widget_set_size_request(media_hbox, window_x, total_height);
                }
                gtk_window_resize(GTK_WINDOW(window), window_x, window_y);
            } else {
                if (embed_window != -1 && GTK_WIDGET_VISIBLE(window)) {
                    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), TRUE);
                    gtk_widget_set_size_request(fixed, -1, -1);
                    gtk_widget_set_size_request(drawing_area, -1, -1);
                    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist))) {
                        gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, TRUE);
                        gtk_widget_style_get(pane, "handle-size", &handle_size, NULL);
                        adjust_paned_rules();
                        if (vertical_layout) {
                            if (window->allocation.height <
                                media_hbox->allocation.height + plvbox->requisition.height) {
                                total_width = window->allocation.width;
                                total_height =
                                    media_hbox->allocation.height +
                                    ((plvbox->allocation.height ==
                                      1) ? plvbox->requisition.height : plvbox->allocation.height);
                                gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
                            }
                            gtk_paned_set_position(GTK_PANED(pane), media_hbox->allocation.height);
                        } else {
                            if (window->allocation.width <
                                media_hbox->allocation.width + plvbox->requisition.width) {
                                total_width =
                                    media_hbox->allocation.width +
                                    ((plvbox->allocation.width ==
                                      1) ? plvbox->requisition.width : plvbox->allocation.width);
                                total_height = window->allocation.height;
                                gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
                            }
                            gtk_paned_set_position(GTK_PANED(pane), media_hbox->allocation.width);
                        }

                    } else {
                        gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);
                        total_height = menubar->allocation.height;
                        if (showcontrols && rpcontrols == NULL) {
                            total_height += controls_box->allocation.height;
                        }
                        if (GTK_WIDGET_VISIBLE(media_hbox)) {
                            total_height += media_hbox->allocation.height;
                        }
                        if (GTK_IS_WIDGET(details_table) && GTK_WIDGET_VISIBLE(details_table)) {
                            total_height += details_vbox->allocation.height;
                        }
                        if (GTK_IS_WIDGET(audio_meter) && GTK_WIDGET_VISIBLE(audio_meter)) {
                            total_height += audio_meter->allocation.height;
                        }

                        if (menubar->allocation.width > 0 && total_height > 0)
                            gtk_window_resize(GTK_WINDOW(window), menubar->allocation.width,
                                              total_height);
                    }
                    last_window_height = 0;
                    last_window_width = 0;
                }
            }
        }
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_fullscreen), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(fs_event_box), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_subtitle), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_take_screenshot), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_fullscreen), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_onetoone), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_onetotwo), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_twotoone), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_advanced), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect_default), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect_four_three), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect_sixteen_nine), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect_sixteen_ten), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect_follow_window),
                                 idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_subtitles), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_smaller_subtitle), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_larger_subtitle), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_angle), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_advanced), idle->videopresent);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_details))) {
            menuitem_details_callback(NULL, NULL);
        }

    }
    if (idle != NULL)
        idle->window_resized = TRUE;

    while (gtk_events_pending())
        gtk_main_iteration();

    menuitem_details_callback(NULL, NULL);
    return FALSE;
}

gboolean set_play(void *data)
{

    play_callback(NULL, NULL, data);
    return FALSE;
}

gboolean set_pause(void *data)
{

    pause_callback(NULL, NULL, data);
    return FALSE;
}

gboolean set_stop(void *data)
{

    stop_callback(NULL, NULL, data);
    return FALSE;
}

gboolean set_ff(void *data)
{

    ff_callback(NULL, NULL, NULL);      // ok is just not NULL which is what we want
    return FALSE;
}

gboolean set_rew(void *data)
{

    rew_callback(NULL, NULL, NULL);     // ok is just not NULL which is what we want
    return FALSE;
}

gboolean set_prev(void *data)
{

    prev_callback(NULL, NULL, NULL);
    return FALSE;
}

gboolean set_next(void *data)
{

    next_callback(NULL, NULL, NULL);
    return FALSE;
}

gboolean set_quit(void *data)
{

    delete_callback(NULL, NULL, NULL);
    return FALSE;
}

gboolean set_position(void *data)
{
    gchar *cmd;
    IdleData *idle = (IdleData *) data;

    if (idle != NULL) {
        cmd = g_strdup_printf("seek %5.0f 2\n", idle->position);
        send_command(cmd, FALSE);
        g_free(cmd);
    }
    return FALSE;
}

gboolean set_volume(void *data)
{
    IdleData *idle = (IdleData *) data;
    gchar *buf = NULL;

    if (GTK_IS_WIDGET(vol_slider) && idle->volume >= 0 && idle->volume <= 100) {
        //printf("setting slider to %f\n", idle->volume);
#ifdef GTK2_12_ENABLED
        if (rpcontrols != NULL && g_strcasecmp(rpcontrols, "volumeslider") == 0) {
            gtk_range_set_value(GTK_RANGE(vol_slider), idle->volume);
            buf = g_strdup_printf(_("Volume %i%%"), (gint) idle->volume);
            g_strlcpy(idledata->vol_tooltip, buf, 128);
            gtk_tooltips_set_tip(volume_tip, vol_slider, idle->vol_tooltip, NULL);
            g_free(buf);
        } else {
            gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider), idle->volume);
        }

#else
        gtk_range_set_value(GTK_RANGE(vol_slider), idle->volume);
        buf = g_strdup_printf(_("Volume %i%%"), (gint) idle->volume);
        g_strlcpy(idledata->vol_tooltip, buf, 128);
        gtk_tooltips_set_tip(volume_tip, vol_slider, idle->vol_tooltip, NULL);
        g_free(buf);
#endif
    }

    return FALSE;
}

gboolean set_fullscreen(void *data)
{

    IdleData *idle = (IdleData *) data;

    // we need to flip the state since the callback reads the value of fullscreen
    // and if fullscreen is 0 it sets it to fullscreen.
    // fullscreen = ! (gint) idle->fullscreen;
    // printf("calling fs_callback with %i\n",fullscreen);
    // fs_callback(NULL, NULL, NULL); // ok is just not NULL which is what we want
    if (idle != NULL && idle->videopresent)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen), idle->fullscreen);
    return FALSE;
}

gboolean set_show_controls(void *data)
{

    IdleData *idle = (IdleData *) data;

    if (idle == NULL)
        return FALSE;

    showcontrols = (gint) idle->showcontrols;

    if (GTK_WIDGET_VISIBLE(menuitem_view_controls)) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_controls), showcontrols);
    } else {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), showcontrols);
    }
    return FALSE;
}

gboolean get_show_controls()
{
    return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_controls));
}

gboolean popup_handler(GtkWidget * widget, GdkEvent * event, void *data)
{
    GtkMenu *menu;
    GdkEventButton *event_button;
    GTimeVal currenttime;

    g_get_current_time(&currenttime);
    last_movement_time = currenttime.tv_sec;
    g_idle_add(make_panel_and_mouse_visible, NULL);

    menu = GTK_MENU(widget);
    gtk_widget_grab_focus(fixed);
    if (event->type == GDK_BUTTON_PRESS) {

        event_button = (GdkEventButton *) event;

        dbus_send_event("MouseDown", event_button->button);
        dbus_send_event("MouseClicked", 0);

        if (event_button->button == 3 && disable_context_menu == 0) {
            gtk_menu_popup(menu, NULL, NULL, NULL, NULL, event_button->button, event_button->time);
            return TRUE;
        }

        if (event_button->button == 1 && idledata->videopresent == TRUE && !disable_pause_on_click) {
            if (event_button->x > fixed->allocation.x
                && event_button->y > fixed->allocation.y
                && event_button->x < fixed->allocation.x + fixed->allocation.width
                && event_button->y < fixed->allocation.y + fixed->allocation.height) {
                play_callback(NULL, NULL, NULL);
            }
        }


    }

    if (event->type == GDK_2BUTTON_PRESS) {
        event_button = (GdkEventButton *) event;
        if (event_button->button == 1 && idledata->videopresent == TRUE) {
            if (event_button->x > fixed->allocation.x
                && event_button->y > fixed->allocation.y
                && event_button->x < fixed->allocation.x + fixed->allocation.width
                && event_button->y < fixed->allocation.y + fixed->allocation.height) {
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen),
                                               !fullscreen);
            }
        }
    }

    if (event->type == GDK_BUTTON_RELEASE) {

        event_button = (GdkEventButton *) event;
        dbus_send_event("MouseUp", event_button->button);

    }

    return FALSE;
}

gboolean drawing_area_scroll_event_callback(GtkWidget * widget, GdkEventScroll * event,
                                            gpointer data)
{

    if (event->direction == GDK_SCROLL_UP) {
        set_ff(NULL);
    }

    if (event->direction == GDK_SCROLL_DOWN) {
        set_rew(NULL);
    }

    return TRUE;
}

gboolean notification_handler(GtkWidget * widget, GdkEventCrossing * event, void *data)
{
    if (event->type == GDK_ENTER_NOTIFY) {
        dbus_send_event("EnterWindow", 0);
    }

    if (event->type == GDK_LEAVE_NOTIFY) {
        dbus_send_event("LeaveWindow", 0);
    }

    return FALSE;
}

gboolean delete_callback(GtkWidget * widget, GdkEvent * event, void *data)
{
    loop = 0;
    ok_to_play = FALSE;
    dontplaynext = TRUE;

    if (remember_loc && !fullscreen) {
        gm_store = gm_pref_store_new("gnome-mplayer");
        gtk_window_get_position(GTK_WINDOW(window), &loc_window_x, &loc_window_y);
        gtk_window_get_size(GTK_WINDOW(window), &loc_window_width, &loc_window_height);
        gm_pref_store_set_int(gm_store, WINDOW_X, loc_window_x);
        gm_pref_store_set_int(gm_store, WINDOW_Y, loc_window_y);
        gm_pref_store_set_int(gm_store, WINDOW_HEIGHT, loc_window_height);
        gm_pref_store_set_int(gm_store, WINDOW_WIDTH, loc_window_width);
        gm_pref_store_free(gm_store);
    }

    mplayer_shutdown();
    while (gtk_events_pending() || thread != NULL) {
        gtk_main_iteration();
    }

    if (control_id != 0)
        dbus_cancel();

    dbus_unhook();

    gtk_main_quit();
    return FALSE;
}

#ifdef GTK2_12_ENABLED
gboolean status_icon_callback(GtkStatusIcon * icon, gpointer data)
{

    if (GTK_WIDGET_VISIBLE(window)) {
        gtk_window_get_position(GTK_WINDOW(window), &loc_window_x, &loc_window_y);
        gtk_window_iconify(GTK_WINDOW(window));
        gtk_widget_hide(GTK_WIDGET(window));
    } else {
        gtk_window_deiconify(GTK_WINDOW(window));
        gtk_window_present(GTK_WINDOW(window));
        gtk_window_move(GTK_WINDOW(window), loc_window_x, loc_window_y);
    }
    return FALSE;
}


void status_icon_context_callback(GtkStatusIcon * status_icon, guint button, guint activate_time,
                                  gpointer data)
{
    gtk_menu_popup(GTK_MENU(popup_menu), NULL, NULL, gtk_status_icon_position_menu, status_icon,
                   button, activate_time);
}

#endif

gboolean motion_notify_callback(GtkWidget * widget, GdkEventMotion * event, gpointer data)
{
    GTimeVal currenttime;

    g_get_current_time(&currenttime);
    last_movement_time = currenttime.tv_sec;
/*
	if (verbose > 1) {
		g_get_current_time(&currenttime);
		printf("motion noticed at %li\n",currenttime.tv_sec);
	}
*/
    g_idle_add(make_panel_and_mouse_visible, NULL);
    return FALSE;
}

gboolean move_window(void *data)
{

    IdleData *idle = (IdleData *) data;

    // printf("moving window to %i x %i\n",idle->x,idle->y);
    if (GTK_IS_WIDGET(drawing_area) && (idle->x != idle->last_x || idle->y != idle->last_y)) {
        gtk_fixed_move(GTK_FIXED(fixed), GTK_WIDGET(drawing_area), idle->x, idle->y);
        idle->last_x = idle->x;
        idle->last_y = idle->y;
    }
    return FALSE;
}

gboolean expose_fixed_callback(GtkWidget * widget, GdkEventExpose * event, gpointer data)
{
    if (GDK_IS_DRAWABLE(fixed->window)) {
        if (videopresent || embed_window != 0) {
            // printf("drawing box %i x %i at %i x %i \n",event->area.width,event->area.height, event->area.x, event->area.y );
            gdk_draw_rectangle(fixed->window, window->style->black_gc, TRUE, event->area.x,
                               event->area.y, event->area.width, event->area.height);
        }
    }
    return FALSE;
}

gboolean allocate_fixed_callback(GtkWidget * widget, GtkAllocation * allocation, gpointer data)
{

    gdouble movie_ratio, window_ratio;
    gint new_width, new_height;

    if (actual_x > 0 && actual_y > 0) {

        movie_ratio = (gdouble) actual_x / (gdouble) actual_y;
        // printf("movie new_width %i new_height %i\n", actual_x, actual_y);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three)))
            movie_ratio = 4.0 / 3.0;
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine)))
            movie_ratio = 16.0 / 9.0;
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten)))
            movie_ratio = 16.0 / 10.0;
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window)))
            movie_ratio = (gdouble) allocation->width / (gdouble) allocation->height;

        window_ratio = (gdouble) allocation->width / (gdouble) allocation->height;
        // printf("window new_width %i new_height %i\n", allocation->width,allocation->height);

        if (allocation->width == idledata->width && allocation->height == idledata->width) {
            new_width = allocation->width;
            new_height = allocation->height;
        } else {
            // printf("last %i x %i\n",last_window_width,last_window_height);
            if (movie_ratio > window_ratio) {
                // printf("movie %lf > window %lf\n",movie_ratio,window_ratio);
                new_width = allocation->width;
                new_height = allocation->width / movie_ratio;
            } else {
                // printf("movie %lf < window %lf\n",movie_ratio,window_ratio);
                new_height = allocation->height;
                new_width = allocation->height * movie_ratio;
            }
        }

        // printf("pre align new_width %i new_height %i\n",new_width, new_height);
        // adjust video to be aligned when playing on video on a smaller screen
        if (new_height < idledata->height || new_width < idledata->width) {
            new_width = new_width - new_width % 16;
            new_height = new_height - new_height % 16;
        }
        // printf("new_width %i new_height %i\n", new_width, new_height);
        gtk_widget_set_size_request(drawing_area, new_width, new_height);
        gtk_widget_set_size_request(fixed, allocation->width, allocation->height);
        idledata->x = (allocation->width - new_width) / 2;
        idledata->y = (allocation->height - new_height) / 2;
        move_window(idledata);

    }

    return TRUE;

}

gboolean window_key_callback(GtkWidget * widget, GdkEventKey * event, gpointer user_data)
{
    GTimeVal currenttime;
    gchar *cmd;

    // printf("key = %i\n",event->keyval);
    // printf("state = %i\n",event->state);
    // printf("other = %i\n", event->state & ~GDK_CONTROL_MASK);

    //printf("key name=%s\n", gdk_keyval_name(event->keyval));
    // We don't want to handle CTRL accelerators here
    // if we pass in items with CTRL then 2 and Ctrl-2 do the same thing
    if (event->state == (event->state & (~GDK_CONTROL_MASK))) {

        g_get_current_time(&currenttime);
        last_movement_time = currenttime.tv_sec;

        g_idle_add(make_panel_and_mouse_visible, NULL);
        switch (event->keyval) {
        case GDK_ISO_Next_Group:
            setup_accelerators();
            return FALSE;
        case GDK_Right:
            if (lastfile != NULL && dvdnav_title_is_menu) {
                send_command("dvdnav 4\n", FALSE);
                return FALSE;
            } else {
                if (state != STOPPED
                    && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)))
                    return ff_callback(NULL, NULL, NULL);
            }
            break;
        case GDK_Left:
            if (lastfile != NULL && dvdnav_title_is_menu) {
                send_command("dvdnav 3\n", FALSE);
                return FALSE;
            } else {
                if (state != STOPPED
                    && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)))
                    return rew_callback(NULL, NULL, NULL);
            }
            break;
        case GDK_Page_Up:
            if (state != STOPPED
                && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)))
                send_command("seek +600 0\n", TRUE);
            if (state == PAUSED) {
                send_command("mute 1\nseek 0 0\npause\n", FALSE);
                send_command("mute 0\n", TRUE);
                idledata->position += 600;
                if (idledata->position > idledata->length)
                    idledata->position = 0;
                gmtk_media_tracker_set_percentage(tracker, idledata->position / idledata->length);
            }
            return FALSE;
        case GDK_Page_Down:
            if (state != STOPPED
                && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)))
                send_command("seek -600 0\n", TRUE);
            if (state == PAUSED) {
                send_command("mute 1\nseek 0 0\npause\n", FALSE);
                send_command("mute 0\n", TRUE);
                idledata->position -= 600;
                if (idledata->position < 0)
                    idledata->position = 0;
                gmtk_media_tracker_set_percentage(tracker, idledata->position / idledata->length);
            }
            return FALSE;
        case GDK_Up:
            if (lastfile != NULL && dvdnav_title_is_menu) {
                send_command("dvdnav 1\n", FALSE);
            } else {
                if (state != STOPPED
                    && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)))
                    send_command("seek +60 0\n", TRUE);
                if (state == PAUSED) {
                    send_command("mute 1\nseek 0 0\npause\n", FALSE);
                    send_command("mute 0\n", TRUE);
                    idledata->position += 60;
                    if (idledata->position > idledata->length)
                        idledata->position = 0;
                    gmtk_media_tracker_set_percentage(tracker,
                                                      idledata->position / idledata->length);
                }
            }

            return FALSE;
        case GDK_Down:
            if (lastfile != NULL && dvdnav_title_is_menu) {
                send_command("dvdnav 2\n", FALSE);
            } else {
                if (state != STOPPED
                    && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)))
                    send_command("seek -60 0\n", TRUE);
                if (state == PAUSED) {
                    send_command("mute 1\nseek 0 0\npause\n", FALSE);
                    send_command("mute 0\n", TRUE);
                    idledata->position -= 60;
                    if (idledata->position < 0)
                        idledata->position = 0;
                    gmtk_media_tracker_set_percentage(tracker,
                                                      idledata->position / idledata->length);
                }
            }
            return FALSE;
        case GDK_Return:
            if (lastfile != NULL && dvdnav_title_is_menu) {
                send_command("dvdnav 6\n", FALSE);
            }
            return FALSE;
        case GDK_less:
            prev_callback(NULL, NULL, NULL);
            return FALSE;
        case GDK_greater:
            next_callback(NULL, NULL, NULL);
            return FALSE;
        case GDK_space:
        case GDK_p:
            return play_callback(NULL, NULL, NULL);
            break;
        case GDK_m:
#ifdef GTK2_12_ENABLED
            if (idledata->mute) {
                gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider), idledata->volume);
            } else {
                gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider), 0);
            }
#else
            if (idledata->mute) {
                gtk_range_set_value(GTK_RANGE(vol_slider), idledata->volume);
            } else {
                gtk_range_set_value(GTK_RANGE(vol_slider), 0);
            }
#endif
            return FALSE;

        case GDK_1:
            send_command("contrast -5\n", TRUE);
            send_command("get_property contrast\n", TRUE);
            return FALSE;
        case GDK_2:
            send_command("contrast 5\n", TRUE);
            send_command("get_property contrast\n", TRUE);
            return FALSE;
        case GDK_3:
            send_command("brightness -5\n", TRUE);
            send_command("get_property brightness\n", TRUE);
            return FALSE;
        case GDK_4:
            send_command("brightness 5\n", TRUE);
            send_command("get_property brightness\n", TRUE);
            return FALSE;
        case GDK_5:
            send_command("hue -5\n", TRUE);
            send_command("get_property hue\n", TRUE);
            return FALSE;
        case GDK_6:
            send_command("hue 5\n", TRUE);
            send_command("get_property hue\n", TRUE);
            return FALSE;
        case GDK_7:
            send_command("saturation -5\n", TRUE);
            send_command("get_property saturation\n", TRUE);
            return FALSE;
        case GDK_8:
            send_command("saturation 5\n", TRUE);
            send_command("get_property saturation\n", TRUE);
            return FALSE;
        case GDK_bracketleft:
            send_command("speed_mult 0.90\n", TRUE);
            return FALSE;
        case GDK_bracketright:
            send_command("speed_mult 1.10\n", TRUE);
            return FALSE;
        case GDK_braceleft:
            send_command("speed_mult 0.50\n", TRUE);
            return FALSE;
        case GDK_braceright:
            send_command("speed_mult 2.0\n", TRUE);
            return FALSE;
        case GDK_BackSpace:
            send_command("speed_set 1.0\n", TRUE);
            return FALSE;
        case GDK_9:
#ifdef GTK2_12_ENABLED
            gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider),
                                       gtk_scale_button_get_value(GTK_SCALE_BUTTON(vol_slider)) -
                                       10);
#else
            gtk_range_set_value(GTK_RANGE(vol_slider),
                                gtk_range_get_value(GTK_RANGE(vol_slider)) - 10);
#endif
            return FALSE;
        case GDK_0:
#ifdef GTK2_12_ENABLED
            gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider),
                                       gtk_scale_button_get_value(GTK_SCALE_BUTTON(vol_slider)) +
                                       10);
#else
            gtk_range_set_value(GTK_RANGE(vol_slider),
                                gtk_range_get_value(GTK_RANGE(vol_slider)) + 10);
#endif
            return FALSE;
        case GDK_numbersign:
            send_command("switch_audio\n", TRUE);
            return FALSE;
        case GDK_j:
            send_command("sub_select\n", TRUE);
            send_command("get_property sub_demux\n", TRUE);
            return FALSE;
        case GDK_q:
            delete_callback(NULL, NULL, NULL);
            return FALSE;
        case GDK_v:
            //send_command("pausing_keep sub_visibility\n");
            return FALSE;
        case GDK_plus:
        case GDK_KP_Add:
            send_command("audio_delay 0.1 0\n", TRUE);
            return FALSE;
        case GDK_minus:
        case GDK_KP_Subtract:
            send_command("audio_delay -0.1 0\n", TRUE);
            return FALSE;
        case GDK_z:
            send_command("sub_delay -0.1 0\n", TRUE);
            return FALSE;
        case GDK_x:
            send_command("sub_delay 0.1 0\n", TRUE);
            return FALSE;
        case GDK_F11:
            if (idledata->videopresent)
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen),
                                               !fullscreen);
            return FALSE;
        case GDK_Escape:
            if (fullscreen) {
                if (idledata->videopresent)
                    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen),
                                                   !fullscreen);
            } else {
                delete_callback(NULL, NULL, NULL);
            }
            return FALSE;
        case GDK_a:
            if (gtk_check_menu_item_get_active
                (GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window))) {
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default),
                                               TRUE);
                return FALSE;
            }
            if (gtk_check_menu_item_get_active
                (GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten)))
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
                                               (menuitem_view_aspect_follow_window), TRUE);
            if (gtk_check_menu_item_get_active
                (GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine)))
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
                                               (menuitem_view_aspect_sixteen_ten), TRUE);
            if (gtk_check_menu_item_get_active
                (GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three)))
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
                                               (menuitem_view_aspect_sixteen_nine), TRUE);
            if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default)))
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three),
                                               TRUE);
            return FALSE;
        case GDK_i:
            if (fullscreen) {
                cmd = g_strdup_printf("osd_show_text '%s' 1500 0", idledata->display_name);
                send_command(cmd, TRUE);
                g_free(cmd);
            }
            return FALSE;
        default:
            if (state == PLAYING) {
                if (!(event->keyval == 0xffe3 || event->keyval == 0xffc6)) {
                    cmd = g_strdup_printf("key_down_event %d\n", event->keyval);
                    send_command(cmd, FALSE);
                    g_free(cmd);
                }
            }
            return FALSE;
        }
    }

    if ((fullscreen == 1) && (event->state & GDK_CONTROL_MASK)) {
        switch (event->keyval) {
        case GDK_f:
            idledata->fullscreen = FALSE;
            set_fullscreen(idledata);
            return TRUE;
        default:
            return FALSE;
        }
    }
    return FALSE;

}

gboolean drop_callback(GtkWidget * widget, GdkDragContext * dc,
                       gint x, gint y, GtkSelectionData * selection_data,
                       guint info, guint t, gpointer data)
{
    gchar **list;
    gint i = 0;
    gint playlist;
    gint itemcount;
    GError *error;
    gboolean added_single = FALSE;

    /* Important, check if we actually got data.  Sometimes errors
     * occure and selection_data will be NULL.
     */
    if (selection_data == NULL)
        return FALSE;
    if (selection_data->length < 0)
        return FALSE;

    if ((info == DRAG_INFO_0) || (info == DRAG_INFO_1) || (info == DRAG_INFO_2)) {
        error = NULL;
        list = g_uri_list_extract_uris((const gchar *) selection_data->data);
        itemcount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);

        while (list[i] != NULL) {
            if (strlen(list[i]) > 0) {
                if (is_uri_dir(list[i])) {
                    create_folder_progress_window();
                    add_folder_to_playlist_callback(list[i], NULL);
                    destroy_folder_progress_window();
                } else {
                    playlist = detect_playlist(list[i]);

                    if (!playlist) {
                        dontplaynext = TRUE;
                        mplayer_shutdown();
                        gtk_list_store_clear(playliststore);
                        gtk_list_store_clear(nonrandomplayliststore);
                        added_single = add_item_to_playlist(list[i], playlist);
                    } else {
                        if (!parse_playlist(list[i])) {
                            dontplaynext = TRUE;
                            mplayer_shutdown();
                            gtk_list_store_clear(playliststore);
                            gtk_list_store_clear(nonrandomplayliststore);
                            added_single = add_item_to_playlist(list[i], playlist);
                        }
                    }
                }
            }
            i++;
        }

        if (itemcount == 0 || added_single) {
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            play_iter(&iter);
        }

        g_strfreev(list);
        update_gui();
    }
    return TRUE;

}

gboolean pause_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    return play_callback(widget, event, data);
}

gboolean play_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    IdleData *idle = (IdleData *) data;

    if (state == PAUSED || state == STOPPED) {
        send_command("seek 0 0\n", FALSE);
        state = PLAYING;
        js_state = STATE_PLAYING;
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_pause);
        gtk_tooltips_set_tip(tooltip, play_event_box, _("Pause"), NULL);
        g_strlcpy(idledata->progress_text, _("Playing"), 1024);
        g_idle_add(set_progress_text, idledata);
        gtk_widget_set_sensitive(ff_event_box, TRUE);
        gtk_widget_set_sensitive(rew_event_box, TRUE);
        gtk_container_remove(GTK_CONTAINER(popup_menu), GTK_WIDGET(menuitem_play));
        menuitem_play =
            GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PAUSE, NULL));
        gtk_menu_shell_insert(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menuitem_play), 2);
        gtk_widget_show(GTK_WIDGET(menuitem_play));
        g_signal_connect(GTK_OBJECT(menuitem_play), "activate",
                         G_CALLBACK(menuitem_pause_callback), NULL);

    } else if (state == PLAYING) {
        send_command("pause\n", FALSE);
        state = PAUSED;
        js_state = STATE_PAUSED;
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_play);
        gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
        g_strlcpy(idledata->progress_text, _("Paused"), 1024);
        g_idle_add(set_progress_text, idledata);
        gtk_widget_set_sensitive(ff_event_box, FALSE);
        gtk_widget_set_sensitive(rew_event_box, FALSE);
        gtk_container_remove(GTK_CONTAINER(popup_menu), GTK_WIDGET(menuitem_play));
        menuitem_play =
            GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL));
        gtk_menu_shell_insert(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menuitem_play), 2);
        gtk_widget_show(GTK_WIDGET(menuitem_play));
        g_signal_connect(GTK_OBJECT(menuitem_play), "activate",
                         G_CALLBACK(menuitem_pause_callback), NULL);

    }

    if (state == QUIT) {
        if (next_item_in_playlist(&iter)) {
            play_iter(&iter);
        } else {
            if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
                play_iter(&iter);
            }
        }
    }

    if (idle == NULL) {
        dbus_send_rpsignal("RP_Play");
    }
    dbus_send_rpsignal_with_int("RP_SetGUIState", state);

    return FALSE;
}

gboolean stop_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    IdleData *idle = (IdleData *) data;

    if (state == PAUSED) {
        send_command("pause\n", FALSE);
        if (idle == NULL)
            dbus_send_rpsignal("RP_Play");
        state = PLAYING;
    }
    if (state == PLAYING) {
        if (idledata != NULL && idledata->streaming) {
            send_command("quit\n", FALSE);
            state = QUIT;
            autopause = FALSE;
        } else {
            send_command("seek 0 2\npause\n", FALSE);
            state = STOPPED;
            autopause = FALSE;
        }
        gmtk_media_tracker_set_percentage(tracker, 0.0);
        gtk_widget_set_sensitive(play_event_box, TRUE);
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_play);
        gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
    }
    if (state == QUIT) {
        gmtk_media_tracker_set_percentage(tracker, 0.0);
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_play);
        gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
        gtk_widget_hide(drawing_area);
    }

    idledata->percent = 0;
    g_idle_add(set_progress_value, idledata);
    g_strlcpy(idledata->progress_text, _("Stopped"), 1024);
    g_idle_add(set_progress_text, idledata);

    if (idle == NULL) {
        dbus_send_rpsignal("RP_Stop");
    }

    dbus_send_rpsignal_with_int("RP_SetGUIState", state);

    return FALSE;
}

gboolean ff_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    if (state != STOPPED) {
        send_command("seek +10 0\n", TRUE);
        if (state == PAUSED) {
            send_command("mute 1\nseek 0 0\npause\n", FALSE);
            send_command("mute 0\n", TRUE);
            idledata->position += 10;
            if (idledata->position > idledata->length)
                idledata->position = 0;
            gmtk_media_tracker_set_percentage(tracker, idledata->position / idledata->length);
        }
    }

    if (rpconsole != NULL && widget != NULL) {
        dbus_send_rpsignal("RP_FastForward");
    }

    return FALSE;
}

gboolean rew_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    if (state != STOPPED) {
        send_command("seek -10 0\n", TRUE);
        if (state == PAUSED) {
            send_command("mute 1\nseek 0 0\npause\n", FALSE);
            send_command("mute 0\n", TRUE);
            idledata->position -= 10;
            if (idledata->position < 0)
                idledata->position = 0;
            gmtk_media_tracker_set_percentage(tracker, idledata->position / idledata->length);
        }
    }

    if (rpconsole != NULL && widget != NULL) {
        dbus_send_rpsignal("RP_FastReverse");
    }

    return FALSE;
}

gboolean prev_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    gchar *iterfilename;
    gchar *localfilename;
    GtkTreeIter localiter;
    GtkTreeIter previter;
    gboolean valid = FALSE;
    GtkTreePath *path;

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iterfilename, -1);
        if (idledata->has_chapters) {
            valid = FALSE;
            send_command("seek_chapter -2 0\n", FALSE);
        } else {
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter);
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN,
                                   &localfilename, -1);
                // printf("iter = %s   local = %s \n",iterfilename,localfilename);
                if (g_ascii_strcasecmp(iterfilename, localfilename) == 0) {
                    // we found the current iter
                    break;
                } else {
                    valid = TRUE;
                    previter = localiter;
                }
                g_free(localfilename);
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
            g_free(iterfilename);
        }
    } else {
        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter);
        do {
            previter = localiter;
            valid = TRUE;
            gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter);
        } while (gtk_list_store_iter_is_valid(playliststore, &localiter));

        if (idledata->has_chapters) {
            valid = FALSE;
            send_command("seek_chapter -1 0\n", FALSE);
        }
    }

    if (valid) {
        dontplaynext = TRUE;
        play_iter(&previter);
        iter = previter;
        if (autopause) {
            autopause = FALSE;
            gtk_widget_set_sensitive(play_event_box, TRUE);
            gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_play);
        }
        gtk_widget_set_sensitive(ff_event_box, TRUE);
        gtk_widget_set_sensitive(rew_event_box, TRUE);

    }

    if (GTK_IS_TREE_SELECTION(selection)) {
        path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &iter);
        gtk_tree_selection_select_path(selection, path);
        if (GTK_IS_WIDGET(list))
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list), path, NULL, FALSE, 0, 0);
        gtk_tree_path_free(path);
    }

    return FALSE;
}

gboolean next_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        if (idledata->has_chapters) {
            send_command("seek_chapter 1 0\n", FALSE);
        } else {
            mplayer_shutdown();
            if (autopause) {
                autopause = FALSE;
                gtk_widget_set_sensitive(play_event_box, TRUE);
                gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_play);
            }
            gtk_widget_set_sensitive(ff_event_box, TRUE);
            gtk_widget_set_sensitive(rew_event_box, TRUE);
        }
    } else {
        if (idledata->has_chapters) {
            send_command("seek_chapter 1 0\n", FALSE);
        }
    }

    return FALSE;
}

gboolean menu_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    send_command("dvdnav 5\n", FALSE);
    return FALSE;
}

void vol_slider_callback(GtkRange * range, gpointer user_data)
{
    gint vol;
    gchar *cmd;
    gchar *buf;

    vol = (gint) gtk_range_get_value(range);
    if (idledata->mute && vol > 0) {
        cmd = g_strdup_printf("mute 0\n");
        send_command(cmd, TRUE);
        g_free(cmd);
        idledata->mute = FALSE;
    }
    if (vol == 0) {
        cmd = g_strdup_printf("mute 1\n");
        send_command(cmd, TRUE);
        g_free(cmd);
        idledata->mute = TRUE;
    } else {
        cmd = g_strdup_printf("volume %i 1\n", vol);
        send_command(cmd, TRUE);
        g_free(cmd);
    }
    if (idledata->volume != vol) {

        buf = g_strdup_printf(_("Volume %i%%"), vol);
        g_strlcpy(idledata->vol_tooltip, buf, 128);
        g_idle_add(set_volume_tip, idledata);
        g_free(buf);
    }
    send_command("get_property volume\n", TRUE);

    dbus_send_rpsignal_with_double("RP_Volume", gtk_range_get_value(GTK_RANGE(vol_slider)));

}

#ifdef GTK2_12_ENABLED
void vol_button_callback(GtkVolumeButton * volume, gpointer user_data)
{
    gint vol;
    gchar *cmd;
    gchar *buf;

    if (rpcontrols != NULL && g_strcasecmp(rpcontrols, "volumeslider") == 0) {
        vol = (gint) gtk_range_get_value(GTK_RANGE(vol_slider));
    } else {
        vol = (gint) gtk_scale_button_get_value(GTK_SCALE_BUTTON(volume));
    }
    if (idledata->mute && vol > 0) {
        cmd = g_strdup_printf("mute 0\n");
        send_command(cmd, TRUE);
        g_free(cmd);
        idledata->mute = FALSE;
    }
    if (!idledata->mute && vol == 0) {
        cmd = g_strdup_printf("mute 1\n");
        send_command(cmd, TRUE);
        g_free(cmd);
        idledata->mute = TRUE;
    } else {
        cmd = g_strdup_printf("volume %i 1\n", vol);
        send_command(cmd, TRUE);
        g_free(cmd);
    }
    if (idledata->volume != vol) {

        buf = g_strdup_printf(_("Volume %i%%"), vol);
        g_strlcpy(idledata->vol_tooltip, buf, 128);
        g_idle_add(set_volume_tip, idledata);
        g_free(buf);
    }
    send_command("get_property volume\n", TRUE);

    dbus_send_rpsignal_with_double("RP_Volume",
                                   gtk_scale_button_get_value(GTK_SCALE_BUTTON(vol_slider)));

}
#endif

gboolean slide_panel_away(gpointer data)
{
    if (!(fullscreen || always_hide_after_timeout)) {
        gtk_widget_set_size_request(controls_box, -1, -1);
        return FALSE;
    }

    if (GTK_IS_WIDGET(controls_box) && GTK_WIDGET_VISIBLE(controls_box)) {
        if (controls_box->allocation.height <= 1) {
            gtk_widget_hide(controls_box);
            g_mutex_unlock(slide_away);
            return FALSE;
        } else {
            if (disable_animation) {
                gtk_widget_set_size_request(controls_box, controls_box->allocation.width, 0);
            } else {
                gtk_widget_set_size_request(controls_box, controls_box->allocation.width,
                                            controls_box->allocation.height - 1);
            }
            return TRUE;
        }
    }
    g_mutex_unlock(slide_away);
    return FALSE;
}

gboolean make_panel_and_mouse_invisible(gpointer data)
{
    GdkColor cursor_color = { 0, 0, 0, 0 };
    GdkPixmap *cursor_source;
    GdkCursor *cursor;
    GTimeVal currenttime;

    if ((fullscreen || always_hide_after_timeout) && auto_hide_timeout > 0
        && GTK_WIDGET_VISIBLE(controls_box)) {
        g_get_current_time(&currenttime);
        g_time_val_add(&currenttime, -auto_hide_timeout * G_USEC_PER_SEC);
        if (last_movement_time > 0 && currenttime.tv_sec > last_movement_time) {
            if (g_mutex_trylock(slide_away)) {
                g_timeout_add(40, slide_panel_away, NULL);
            }
            cursor_source = gdk_pixmap_new(NULL, 1, 1, 1);
            cursor =
                gdk_cursor_new_from_pixmap(cursor_source, cursor_source, &cursor_color,
                                           &cursor_color, 0, 0);
            gdk_pixmap_unref(cursor_source);
            gdk_window_set_cursor(window->window, cursor);
            gdk_cursor_unref(cursor);
/*
			if (verbose > 1) {
				g_get_current_time(&currenttime);
				printf("panel and mouse set invisible at %li\n",currenttime.tv_sec);
			}
*/
        }

    }
    return FALSE;
}

gboolean make_panel_and_mouse_visible(gpointer data)
{
//      GTimeVal currenttime;

    if ((fullscreen || always_hide_after_timeout) && !GTK_WIDGET_VISIBLE(controls_box)) {

        if (showcontrols && GTK_IS_WIDGET(controls_box) && !GTK_WIDGET_VISIBLE(controls_box)) {
            gtk_widget_set_size_request(controls_box, -1, -1);
            gtk_widget_show(controls_box);
        }
        gdk_window_set_cursor(window->window, NULL);
/*
		if (verbose > 1) {
			g_get_current_time(&currenttime);
			printf("panel and mouse set visible at %li\n",currenttime.tv_sec);
		}
*/
    }

    return FALSE;
}

gboolean fs_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen), !fullscreen);

    return FALSE;
}

gboolean enter_button_callback(GtkWidget * widget, GdkEventCrossing * event, gpointer data)
{
    gdk_draw_rectangle(widget->window, widget->style->fg_gc[GTK_STATE_NORMAL], FALSE, 0, 0,
                       widget->allocation.width - 1, widget->allocation.height - 1);

    in_button = TRUE;
    return FALSE;
}

gboolean leave_button_callback(GtkWidget * widget, GdkEventCrossing * event, gpointer data)
{

    gdk_draw_rectangle(widget->window, widget->style->bg_gc[GTK_STATE_NORMAL], FALSE, 0, 0,
                       widget->allocation.width - 1, widget->allocation.height - 1);
    in_button = FALSE;
    return FALSE;
}

void menuitem_open_callback(GtkMenuItem * menuitem, void *data)
{

    GtkWidget *dialog;
    GSList *filename;
    gchar *last_dir;
    gint count;
    GtkTreeViewColumn *column;
    gchar *coltitle;

    dialog = gtk_file_chooser_dialog_new(_("Open File"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    /*allow multiple files to be selected */
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
#ifdef GIO_ENABLED
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), FALSE);
#endif

    gtk_widget_show(dialog);
    gm_store = gm_pref_store_new("gnome-mplayer");
    last_dir = gm_pref_store_get_string(gm_store, LAST_DIR);
    if (last_dir != NULL && is_uri_dir(last_dir)) {
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), last_dir);
        g_free(last_dir);
    }
    gm_pref_store_free(gm_store);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        filename = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));
        last_dir = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
        if (last_dir != NULL && is_uri_dir(last_dir)) {
            gm_store = gm_pref_store_new("gnome-mplayer");
            gm_pref_store_set_string(gm_store, LAST_DIR, last_dir);
            gm_pref_store_free(gm_store);
            g_free(last_dir);
        }

        dontplaynext = TRUE;
        mplayer_shutdown();
        gtk_list_store_clear(playliststore);
        gtk_list_store_clear(nonrandomplayliststore);

        if (filename != NULL) {
            g_slist_foreach(filename, &add_item_to_playlist_callback, NULL);
            g_slist_free(filename);

            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            play_iter(&iter);
            dontplaynext = FALSE;
        }
    }

    if (GTK_IS_WIDGET(list)) {
        column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
        count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
        coltitle = g_strdup_printf(ngettext("Item to Play", "Items to Play", count));
        gtk_tree_view_column_set_title(column, coltitle);
        g_free(coltitle);
    }
    gtk_widget_destroy(dialog);

}

void open_location_callback(GtkWidget * widget, void *data)
{
    gchar *filename;

    filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(open_location)));

    if (filename != NULL && strlen(filename) > 0) {
        dontplaynext = TRUE;
        mplayer_shutdown();
        gtk_list_store_clear(playliststore);
        gtk_list_store_clear(nonrandomplayliststore);

        if (filename != NULL) {

            playlist = detect_playlist(filename);

            if (!playlist) {
                add_item_to_playlist(filename, playlist);
            } else {
                if (!parse_playlist(filename)) {
                    add_item_to_playlist(filename, playlist);
                }

            }

            g_free(filename);
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            play_iter(&iter);
            dontplaynext = FALSE;
        }
    }
    if (GTK_IS_WIDGET(widget))
        gtk_widget_destroy(widget);
}

void menuitem_open_location_callback(GtkMenuItem * menuitem, void *data)
{
    GtkWidget *open_window;
    GtkWidget *vbox;
    GtkWidget *item_box;
    GtkWidget *label;
    GtkWidget *button_box;
    GtkWidget *cancel_button;
    GtkWidget *open_button;

    open_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon(GTK_WINDOW(open_window), pb_icon);

    gtk_window_set_resizable(GTK_WINDOW(open_window), FALSE);
    gtk_window_set_title(GTK_WINDOW(open_window), _("Open Location"));
    vbox = gtk_vbox_new(FALSE, 6);
    label = gtk_label_new(_("Location:"));
    open_location = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(open_location), 50);
    item_box = gtk_hbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(item_box), label, FALSE, FALSE, 12);
    gtk_box_pack_end(GTK_BOX(item_box), open_location, TRUE, TRUE, 0);

    button_box = gtk_hbox_new(FALSE, 6);
    cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    open_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    GTK_WIDGET_SET_FLAGS(open_button, GTK_CAN_DEFAULT);
    gtk_box_pack_end(GTK_BOX(button_box), open_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(button_box), cancel_button, FALSE, FALSE, 0);

    g_signal_connect_swapped(GTK_OBJECT(cancel_button), "clicked",
                             GTK_SIGNAL_FUNC(config_close), open_window);
    g_signal_connect_swapped(GTK_OBJECT(open_button), "clicked",
                             GTK_SIGNAL_FUNC(open_location_callback), open_window);

    gtk_container_add(GTK_CONTAINER(vbox), item_box);
    gtk_container_add(GTK_CONTAINER(vbox), button_box);
    gtk_container_add(GTK_CONTAINER(open_window), vbox);
    gtk_widget_show_all(open_window);
    gtk_window_set_transient_for(GTK_WINDOW(open_window), GTK_WINDOW(window));
    gtk_window_set_keep_above(GTK_WINDOW(open_window), keep_on_top);
    gtk_window_present(GTK_WINDOW(open_window));
    gtk_widget_grab_default(open_button);

}


void menuitem_open_dvd_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_list_store_clear(playliststore);
    gtk_list_store_clear(nonrandomplayliststore);
    if (idledata->device != NULL) {
        g_free(idledata->device);
        idledata->device = NULL;
    }
    parse_dvd("dvd://");

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
        play_iter(&iter);
    }
}

void menuitem_open_dvd_folder_callback(GtkMenuItem * menuitem, void *data)
{
    GtkWidget *dialog;
    gchar *last_dir;

    dialog = gtk_file_chooser_dialog_new(_("Choose Disk Directory"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_widget_show(dialog);
    gm_store = gm_pref_store_new("gnome-mplayer");
    last_dir = gm_pref_store_get_string(gm_store, LAST_DIR);
    if (last_dir != NULL && is_uri_dir(last_dir)) {
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), last_dir);
        g_free(last_dir);
    }
    gm_pref_store_free(gm_store);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        gtk_list_store_clear(playliststore);
        gtk_list_store_clear(nonrandomplayliststore);
        idledata->device = g_strdup(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));

        parse_dvd("dvd://");

        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
            play_iter(&iter);
        }
    }
    if (GTK_IS_WIDGET(dialog))
        gtk_widget_destroy(dialog);

}

void menuitem_open_dvd_iso_callback(GtkMenuItem * menuitem, void *data)
{
    GtkWidget *dialog;
    gchar *last_dir;
    GtkFileFilter *filter;

    dialog = gtk_file_chooser_dialog_new(_("Choose Disk Image"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_widget_show(dialog);
    gm_store = gm_pref_store_new("gnome-mplayer");
    last_dir = gm_pref_store_get_string(gm_store, LAST_DIR);
    if (last_dir != NULL && is_uri_dir(last_dir)) {
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), last_dir);
        g_free(last_dir);
    }
    gm_pref_store_free(gm_store);
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Disk Image (*.iso)"));
    gtk_file_filter_add_pattern(filter, "*.iso");
    gtk_file_filter_add_pattern(filter, "*.ISO");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        gtk_list_store_clear(playliststore);
        gtk_list_store_clear(nonrandomplayliststore);
        idledata->device = g_strdup(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));

        parse_dvd("dvd://");

        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
            play_iter(&iter);
        }
    }
    if (GTK_IS_WIDGET(dialog))
        gtk_widget_destroy(dialog);

}

void menuitem_open_dvdnav_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_list_store_clear(playliststore);
    gtk_list_store_clear(nonrandomplayliststore);
    if (idledata->device != NULL) {
        g_free(idledata->device);
        idledata->device = NULL;
    }
    dvdnav_title_is_menu = TRUE;
    add_item_to_playlist("dvdnav://", 0);
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
    play_iter(&iter);
    gtk_widget_show(menu_event_box);
}

void menuitem_open_dvdnav_folder_callback(GtkMenuItem * menuitem, void *data)
{
    GtkWidget *dialog;
    gchar *last_dir;

    dialog = gtk_file_chooser_dialog_new(_("Choose Disk Directory"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_widget_show(dialog);
    gm_store = gm_pref_store_new("gnome-mplayer");
    last_dir = gm_pref_store_get_string(gm_store, LAST_DIR);
    if (last_dir != NULL && is_uri_dir(last_dir)) {
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), last_dir);
        g_free(last_dir);
    }
    gm_pref_store_free(gm_store);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        gtk_list_store_clear(playliststore);
        gtk_list_store_clear(nonrandomplayliststore);
        idledata->device = g_strdup(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));

        dvdnav_title_is_menu = TRUE;
        add_item_to_playlist("dvdnav://", 0);
        gtk_widget_show(menu_event_box);

        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
            play_iter(&iter);
        }
    }
    if (GTK_IS_WIDGET(dialog))
        gtk_widget_destroy(dialog);

}

void menuitem_open_dvdnav_iso_callback(GtkMenuItem * menuitem, void *data)
{
    GtkWidget *dialog;
    gchar *last_dir;
    GtkFileFilter *filter;

    dialog = gtk_file_chooser_dialog_new(_("Choose Disk Image"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_widget_show(dialog);
    gm_store = gm_pref_store_new("gnome-mplayer");
    last_dir = gm_pref_store_get_string(gm_store, LAST_DIR);
    if (last_dir != NULL && is_uri_dir(last_dir)) {
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), last_dir);
        g_free(last_dir);
    }
    gm_pref_store_free(gm_store);
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Disk Image (*.iso)"));
    gtk_file_filter_add_pattern(filter, "*.iso");
    gtk_file_filter_add_pattern(filter, "*.ISO");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        gtk_list_store_clear(playliststore);
        gtk_list_store_clear(nonrandomplayliststore);
        idledata->device = g_strdup(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));

        dvdnav_title_is_menu = TRUE;
        add_item_to_playlist("dvdnav://", 0);
        gtk_widget_show(menu_event_box);

        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
            play_iter(&iter);
        }
    }
    if (GTK_IS_WIDGET(dialog))
        gtk_widget_destroy(dialog);

}

void menuitem_open_acd_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_list_store_clear(playliststore);
    gtk_list_store_clear(nonrandomplayliststore);
    parse_playlist("cdda://");

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
        play_iter(&iter);
    }

}

void menuitem_open_vcd_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_list_store_clear(playliststore);
    gtk_list_store_clear(nonrandomplayliststore);
    parse_playlist("vcd://");

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
        play_iter(&iter);
    }

}

void menuitem_open_atv_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_list_store_clear(playliststore);
    gtk_list_store_clear(nonrandomplayliststore);
    add_item_to_playlist("tv://", 0);

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
        play_iter(&iter);
    }
}

void menuitem_open_recent_callback(GtkRecentChooser * chooser, gpointer data)
{
    gint playlist = 0;
    gchar *uri;
    gint count;
    GtkTreeViewColumn *column;
    gchar *coltitle;

    mplayer_shutdown();
    gtk_list_store_clear(playliststore);
    gtk_list_store_clear(nonrandomplayliststore);

    uri = gtk_recent_chooser_get_current_uri(chooser);
    if (uri != NULL) {
        if (playlist == 0)
            playlist = detect_playlist(uri);

        if (!playlist) {
            add_item_to_playlist(uri, playlist);
        } else {
            if (!parse_playlist(uri)) {
                add_item_to_playlist(uri, playlist);
            }
        }
    }

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        play_iter(&iter);
        dontplaynext = FALSE;
    }

    if (GTK_IS_WIDGET(list)) {
        column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
        count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
        coltitle = g_strdup_printf(ngettext("Item to Play", "Items to Play", count));
        gtk_tree_view_column_set_title(column, coltitle);
        g_free(coltitle);
    }
}

#ifdef GTK2_12_ENABLED
void recent_manager_changed_callback(GtkRecentManager * recent_manager, gpointer data)
{
    GtkRecentFilter *recent_filter;

    if (GTK_IS_WIDGET(menuitem_file_recent_items))
        gtk_widget_destroy(menuitem_file_recent_items);
    menuitem_file_recent_items = gtk_recent_chooser_menu_new();
    recent_filter = gtk_recent_filter_new();
    gtk_recent_filter_add_application(recent_filter, g_get_application_name());
    gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(menuitem_file_recent_items), recent_filter);
    gtk_recent_chooser_set_show_tips(GTK_RECENT_CHOOSER(menuitem_file_recent_items), TRUE);
    gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(menuitem_file_recent_items),
                                     GTK_RECENT_SORT_MRU);
    gtk_menu_item_set_submenu(menuitem_file_recent, menuitem_file_recent_items);
    g_signal_connect(GTK_OBJECT(menuitem_file_recent_items), "item-activated",
                     G_CALLBACK(menuitem_open_recent_callback), NULL);
#ifdef GIO_ENABLED
    gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(menuitem_file_recent_items), FALSE);
#endif
}
#endif

void parseChannels(FILE * f)
{
    gint parsing = 0, i = 0, firstW = 0, firstP = 0;
    gchar ch, s[20];
    gchar *strout;

    while (parsing == 0) {
        ch = (char) fgetc(f);   // Read in the next character
        if ((int) ch != EOF) {
            // If the line is empty or commented, we want to skip it.
            if (((ch == '\n') && (i == 0)) || ((ch == '#') && (i == 0))) {
                firstW++;
                firstP++;
            }
            if ((ch != ':') && (firstW == 0)) {
                s[i] = ch;
                i++;
            } else {
                if ((ch == ':') && (firstP == 0)) {
                    s[i] = '\0';
                    strout = g_strdup_printf("dvb://%s", s);
                    add_item_to_playlist(strout, 0);    //add to playlist
                    g_free(strout);
                    i = 0;
                    firstW++;
                    firstP++;
                }
                if (ch == '\n') {
                    firstW = 0;
                    firstP = 0;
                }
            }
        } else
            parsing++;
    }                           //END while
}                               //END parseChannels

void menuitem_open_dtv_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_list_store_clear(playliststore);
    gtk_list_store_clear(nonrandomplayliststore);
    FILE *fi;                   // FILE pointer to use to open the conf file
    gchar *mpconf;
    mpconf = g_strdup_printf("%s/.mplayer/channels.conf", g_getenv("HOME"));
    fi = fopen(mpconf, "r");    // Make sure this is pointing to
    // the appropriate file
    if (fi != NULL) {
        parseChannels(fi);
        fclose(fi);
    } else {
        printf("Unable to open the config file\n");     //can change this to whatever error message system is used
    }

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        play_iter(&iter);
        dontplaynext = FALSE;
    }
}

#ifdef HAVE_GPOD
void menuitem_open_ipod_callback(GtkMenuItem * menuitem, void *data)
{
    gpod_mount_point = find_gpod_mount_point();
    // printf("mount point is %s\n", gpod_mount_point);
    if (gpod_mount_point != NULL) {
        gpod_load_tracks(gpod_mount_point);
    } else {
        printf("Unable to find gpod mount point\n");
    }
}
#endif

void menuitem_save_callback(GtkMenuItem * menuitem, void *data)
{
    // save dialog
    GtkWidget *file_chooser_save;
    gchar *filename;
    FILE *fin;
    FILE *fout;
    char buffer[1000];
    gint count;
    gchar *default_name;
    GtkWidget *dialog;
    gchar *msg;

    file_chooser_save = gtk_file_chooser_dialog_new(_("Save As..."),
                                                    GTK_WINDOW(window),
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

    default_name = g_strrstr(idledata->url, "/");
    if (default_name == NULL) {
        default_name = idledata->url;
    } else {
        default_name += sizeof(gchar);
    }
    g_strlcpy(buffer, default_name, 1000);
    while (g_strrstr(buffer, "&") != NULL) {
        default_name = g_strrstr(buffer, "&");
        default_name[0] = '\0';
    }

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(file_chooser_save), buffer);

    if (gtk_dialog_run(GTK_DIALOG(file_chooser_save)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser_save));

        printf("Copy %s to %s\n", lastfile, filename);

        fin = g_fopen(lastfile, "rb");
        fout = g_fopen(filename, "wb");
        if (fin != NULL && fout != NULL) {
            while (!feof(fin)) {
                count = fread(buffer, 1, 1000, fin);
                fwrite(buffer, 1, count, fout);
            }
            fclose(fout);
            fclose(fin);
        } else {
            msg = g_strdup_printf(_("Unable to save '%s'"), filename);
            dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_CLOSE, msg);
            gtk_window_set_title(GTK_WINDOW(dialog), _("GNOME MPlayer Error"));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            g_free(msg);
        }

        g_free(filename);
    }

    gtk_widget_destroy(file_chooser_save);

}



void menuitem_quit_callback(GtkMenuItem * menuitem, void *data)
{
    delete_callback(NULL, NULL, NULL);
}

void menuitem_prev_callback(GtkMenuItem * menuitem, void *data)
{
    prev_callback(NULL, NULL, NULL);
}

void menuitem_next_callback(GtkMenuItem * menuitem, void *data)
{
    next_callback(NULL, NULL, NULL);
}

void menuitem_about_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *authors[] = { "Kevin DeKorte", "James Carthew", "Diogo Franco", NULL };
    gtk_show_about_dialog(GTK_WINDOW(window), "name", _("GNOME MPlayer"),
                          "logo", pb_logo,
                          "authors", authors,
                          "copyright", "Copyright © 2007,2008 Kevin DeKorte",
                          "comments", _("A media player for GNOME that uses MPlayer"),
                          "version", VERSION,
                          "license",
                          _
                          ("Gnome MPlayer is free software; you can redistribute it and/or modify it under\nthe terms of the GNU General Public License as published by the Free\nSoftware Foundation; either version 2 of the License, or (at your option)\nany later version."
                           "\n\nGnome MPlayer is distributed in the hope that it will be useful, but WITHOUT\nANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\nFITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for\nmore details."
                           "\n\nYou should have received a copy of the GNU General Public License along with\nGnome MPlayer if not, write to the\n\nFree Software Foundation, Inc.,\n51 Franklin St, Fifth Floor\nBoston, MA 02110-1301 USA")


                          ,
                          "website", "http://code.google.com/p/gnome-mplayer/",
                          "translator-credits",
                          "Bulgarian - Adrian Dimitrov\n"
                          "Chinese (simplified) - Wenzheng Hu\n"
                          "Chinese (Hong Kong) - Hialan Liu\n"
                          "Chinese (Taiwan) - Hailan Liu\n"
                          "German - tim__b\n"
                          "Greek - Georgas\n"
                          "French - Alexandre Bedot\n"
                          "German - Tim Buening\n"
                          "Italian - Cesare Tirabassi\n"
                          "Japanese - Munehiro Yamamoto\n"
                          "Korean - ByeongSik Jeon\n"
                          "Polish - Julian Sikorski\n"
                          "Portugese - LL\n"
                          "Russian - Dmitry Stropaloff\n"
                          "Serbian - Милош Поповић\n"
                          "Spanish - Festor Wailon Dacoba\n"
                          "Swedish - Daniel Nylander\n" "Turkish - Onur Küçük", NULL);
}

void menuitem_play_callback(GtkMenuItem * menuitem, void *data)
{
    if (state != PLAYING)
        play_callback(GTK_WIDGET(menuitem), NULL, NULL);
}

void menuitem_pause_callback(GtkMenuItem * menuitem, void *data)
{
    pause_callback(GTK_WIDGET(menuitem), NULL, NULL);
}

void menuitem_stop_callback(GtkMenuItem * menuitem, void *data)
{
    stop_callback(GTK_WIDGET(menuitem), NULL, NULL);
}

void menuitem_edit_random_callback(GtkMenuItem * menuitem, void *data)
{
    GtkTreePath *path;
    gchar *iterfilename = NULL;
    gchar *localfilename = NULL;

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iterfilename, -1);
    }

    random_order = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_edit_random));
    if (random_order) {
        randomize_playlist(playliststore);
    } else {
        copy_playlist(nonrandomplayliststore, playliststore);
    }

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        if (GTK_IS_TREE_SELECTION(selection)) {
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            if (iterfilename != NULL) {
                do {
                    gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN,
                                       &localfilename, -1);
                    // printf("iter = %s   local = %s \n",iterfilename,localfilename);
                    if (g_ascii_strcasecmp(iterfilename, localfilename) == 0) {
                        // we found the current iter
                        g_free(localfilename);
                        break;
                    }
                    g_free(localfilename);
                } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &iter));
                g_free(iterfilename);
            }
            path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &iter);
            gtk_tree_selection_select_path(selection, path);
            if (GTK_IS_WIDGET(list))
                gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list), path, NULL, FALSE, 0, 0);
            gtk_tree_path_free(path);
        }
    }
}

void menuitem_edit_loop_callback(GtkMenuItem * menuitem, void *data)
{
    loop = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_edit_loop));
}

void menuitem_view_info_callback(GtkMenuItem * menuitem, void *data)
{
    gint width = 0, height = 0;

    //printf("media info = '%s'\n", idledata->media_info);
    if (GTK_IS_WIDGET(media_hbox)) {
        if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_info))) {
            gtk_window_get_size(GTK_WINDOW(window), &width, &height);
            gtk_widget_hide_all(media_hbox);
            gtk_window_resize(GTK_WINDOW(window), width, height - media_hbox->allocation.height);
            while (gtk_events_pending())
                gtk_main_iteration();

        } else {
            if (idledata->window_resized) {
                while (gtk_events_pending())
                    gtk_main_iteration();

                if (idledata->videopresent) {
                    gtk_window_get_size(GTK_WINDOW(window), &width, &height);
                }

                if (strlen(idledata->media_info) > 0)
                    gtk_widget_show_all(media_hbox);

                while (gtk_events_pending())
                    gtk_main_iteration();

                if (idledata->videopresent) {
                    height += media_hbox->allocation.height;
                    gtk_window_resize(GTK_WINDOW(window), width, height);
                }

            } else {
                if (strlen(idledata->media_info) > 0)
                    gtk_widget_show_all(media_hbox);
            }

        }
    }
}

void menuitem_view_fullscreen_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen), !fullscreen);
}

void menuitem_view_onetoone_callback(GtkMenuItem * menuitem, void *data)
{
    gint total_height, total_width;
    IdleData *idle = (IdleData *) data;

    gtk_widget_set_size_request(fixed, -1, -1);
    gtk_widget_set_size_request(drawing_area, -1, -1);
    if (idle->width > 0 && idle->height > 0) {
        gtk_widget_set_size_request(fixed, idle->width, idle->height);
        gtk_widget_set_size_request(drawing_area, idle->width, idle->height);
        total_height = idle->height;
        total_height += menubar->allocation.height;
        if (GTK_WIDGET_VISIBLE(media_hbox)) {
            total_height += media_hbox->allocation.height;
        }

        if (GTK_IS_WIDGET(details_table) && GTK_WIDGET_VISIBLE(details_table)) {
            total_height += details_vbox->allocation.height;
        }
        if (GTK_IS_WIDGET(audio_meter) && GTK_WIDGET_VISIBLE(audio_meter)) {
            total_height += audio_meter->allocation.height;
        }

        total_width = idle->width;
        if (vertical_layout) {
            if (GTK_IS_WIDGET(plvbox) && GTK_WIDGET_VISIBLE(plvbox)) {
                total_height = total_height + plvbox->allocation.height;
            }
        } else {
            if (GTK_IS_WIDGET(plvbox) && GTK_WIDGET_VISIBLE(plvbox)) {
                total_width = total_width + plvbox->allocation.width;
            }
        }
        total_height += controls_box->allocation.height;
        gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
    }
}

void menuitem_view_twotoone_callback(GtkMenuItem * menuitem, void *data)
{
    gint total_height, total_width;
    IdleData *idle = (IdleData *) data;

    gtk_widget_set_size_request(fixed, -1, -1);
    gtk_widget_set_size_request(drawing_area, -1, -1);
    if (idle->width > 0 && idle->height > 0) {
        gtk_widget_set_size_request(fixed, idle->width * 2, idle->height * 2);
        gtk_widget_set_size_request(drawing_area, idle->width * 2, idle->height * 2);
        total_height = idle->height * 2;
        total_height += menubar->allocation.height;
        if (GTK_WIDGET_VISIBLE(media_hbox)) {
            total_height += media_hbox->allocation.height;
        }

        if (GTK_IS_WIDGET(details_table) && GTK_WIDGET_VISIBLE(details_table)) {
            total_height += details_vbox->allocation.height;
        }
        if (GTK_IS_WIDGET(audio_meter) && GTK_WIDGET_VISIBLE(audio_meter)) {
            total_height += audio_meter->allocation.height;
        }

        total_width = idle->width * 2;
        if (vertical_layout) {
            if (GTK_IS_WIDGET(plvbox) && GTK_WIDGET_VISIBLE(plvbox)) {
                total_height = total_height + plvbox->allocation.height;
            }
        } else {
            if (GTK_IS_WIDGET(plvbox) && GTK_WIDGET_VISIBLE(plvbox)) {
                total_width = total_width + plvbox->allocation.width;
            }
        }
        total_height += controls_box->allocation.height;
        gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
    }
}

void menuitem_view_onetotwo_callback(GtkMenuItem * menuitem, void *data)
{
    gint total_height, total_width;
    IdleData *idle = (IdleData *) data;

    gtk_widget_set_size_request(fixed, -1, -1);
    gtk_widget_set_size_request(drawing_area, -1, -1);
    if (idle->width > 0 && idle->height > 0) {
        gtk_widget_set_size_request(fixed, idle->width / 2, idle->height / 2);
        gtk_widget_set_size_request(drawing_area, idle->width / 2, idle->height / 2);
        total_height = idle->height / 2;
        total_height += menubar->allocation.height;
        if (GTK_WIDGET_VISIBLE(media_hbox)) {
            total_height += media_hbox->allocation.height;
        }

        if (GTK_IS_WIDGET(details_table) && GTK_WIDGET_VISIBLE(details_table)) {
            total_height += details_vbox->allocation.height;
        }
        if (GTK_IS_WIDGET(audio_meter) && GTK_WIDGET_VISIBLE(audio_meter)) {
            total_height += audio_meter->allocation.height;
        }

        total_width = idle->width / 2;
        if (vertical_layout) {
            if (GTK_IS_WIDGET(plvbox) && GTK_WIDGET_VISIBLE(plvbox)) {
                total_height = total_height + plvbox->allocation.height;
            }
        } else {
            if (GTK_IS_WIDGET(plvbox) && GTK_WIDGET_VISIBLE(plvbox)) {
                total_width = total_width + plvbox->allocation.width;
            }
        }
        total_height += controls_box->allocation.height;
        gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
    }
}

void menuitem_view_controls_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols),
                                   !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM
                                                                   (menuitem_showcontrols)));
}

void menuitem_view_subtitles_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *cmd;
//      set_sub_visibility
    cmd =
        g_strdup_printf("set_property sub_visibility %i\n",
                        gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM
                                                       (menuitem_view_subtitles)));
    send_command(cmd, TRUE);
    g_free(cmd);
}

//      Switch Audio Streams 
void menuitem_edit_switch_audio_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *cmd;
    cmd = g_strdup_printf("switch_audio\n");
    send_command(cmd, TRUE);
    g_free(cmd);
}

void menuitem_edit_set_subtitle_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *cmd;
    gchar *subtitle = NULL;
    GtkWidget *dialog;
    gchar *path;
    gchar *item;
    gchar *p;

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &item, -1);

        path = g_strdup(item);
        p = g_strrstr(path, "/");
        if (p != NULL)
            p[1] = '\0';

        dialog = gtk_file_chooser_dialog_new(_("Set Subtitle"),
                                             GTK_WINDOW(window),
                                             GTK_FILE_CHOOSER_ACTION_OPEN,
                                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                             GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        gtk_widget_show(dialog);
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), path);
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            subtitle = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            gtk_list_store_set(playliststore, &iter, SUBTITLE_COLUMN, subtitle, -1);
        }
        gtk_widget_destroy(dialog);

        if (subtitle != NULL) {
            cmd = g_strdup_printf("sub_remove\n");
            send_command(cmd, TRUE);
            g_free(cmd);
            cmd = g_strdup_printf("sub_load \"%s\"\n", subtitle);
            send_command(cmd, TRUE);
            g_free(cmd);
            cmd = g_strdup_printf("sub_file 0");
            send_command(cmd, TRUE);
            g_free(cmd);
            g_free(subtitle);
        }
    }
}

//      Take Screenshot 
void menuitem_edit_take_screenshot_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *cmd;
	PLAYSTATE s;
	
    cmd = g_strdup_printf("screenshot 0\n");
	s = state;
    send_command(cmd, FALSE);
	if (s != PLAYING)
		send_command("pause\n", FALSE);
    g_free(cmd);
}

void menuitem_fs_callback(GtkMenuItem * menuitem, void *data)
{
    gint width = 0, height = 0;
    GdkScreen *screen;
    GdkRectangle rect;
    gint wx, wy;
    gint x, y;

    if (GTK_CHECK_MENU_ITEM(menuitem) == GTK_CHECK_MENU_ITEM(menuitem_view_fullscreen)) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen),
                                       gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM
                                                                      (menuitem_view_fullscreen)));
        return;
    } else {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_fullscreen),
                                       gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM
                                                                      (menuitem_fullscreen)));
    }

    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen))) {
        gtk_window_unfullscreen(GTK_WINDOW(window));
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_config), TRUE);

        if (embed_window != 0) {
            while (gtk_events_pending())
                gtk_main_iteration();

            if (GTK_WIDGET_MAPPED(window))
                gtk_widget_unmap(window);

            XReparentWindow(GDK_WINDOW_XDISPLAY(window->window),
                            GDK_WINDOW_XWINDOW(window->window), embed_window, 0, 0);
            gtk_widget_map(window);
            gtk_window_move(GTK_WINDOW(window), 0, 0);
            XResizeWindow(GDK_WINDOW_XDISPLAY(window->window),
                          GDK_WINDOW_XWINDOW(window->window), window_x, window_y - 1);
            if (window_x > 0 && window_y > 0)
                gtk_window_resize(GTK_WINDOW(window), window_x, window_y);
            if (window_x < 250) {
                gtk_widget_hide(fs_event_box);
            }
            if (window_x < 170) {
                gtk_widget_hide(GTK_WIDGET(tracker));
            }
            gtk_widget_destroy(fs_window);
            fs_window = NULL;
        } else {
            gtk_widget_show(menubar);
            if (GDK_IS_DRAWABLE(window_container))
                gdk_drawable_get_size(GDK_DRAWABLE(window_container), &width, &height);
            if (width > 0 && height > 0)
                gtk_window_resize(GTK_WINDOW(window), width, height);
            if (stored_window_width != -1 && stored_window_width > 0) {
                gtk_window_resize(GTK_WINDOW(window), stored_window_width, stored_window_height);
            }
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), restore_info);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_details),
                                           restore_details);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist),
                                           restore_playlist);
        }

        make_panel_and_mouse_visible(NULL);

        fullscreen = 0;
        while (gtk_events_pending())
            gtk_main_iteration();
    } else {
        if (embed_window != 0) {
            fs_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            gtk_window_set_policy(GTK_WINDOW(fs_window), TRUE, TRUE, TRUE);
            gtk_widget_add_events(fs_window, GDK_BUTTON_PRESS_MASK);
            gtk_widget_add_events(fs_window, GDK_BUTTON_RELEASE_MASK);
            gtk_widget_add_events(fs_window, GDK_KEY_PRESS_MASK);
            gtk_widget_add_events(fs_window, GDK_KEY_RELEASE_MASK);
            gtk_widget_add_events(fs_window, GDK_ENTER_NOTIFY_MASK);
            gtk_widget_add_events(fs_window, GDK_LEAVE_NOTIFY_MASK);
            gtk_widget_add_events(fs_window, GDK_KEY_PRESS_MASK);
            gtk_widget_add_events(fs_window, GDK_VISIBILITY_NOTIFY_MASK);
            gtk_widget_add_events(fs_window, GDK_STRUCTURE_MASK);
            gtk_widget_add_events(fs_window, GDK_POINTER_MOTION_MASK);
            g_signal_connect(GTK_OBJECT(fs_window), "key_press_event",
                             G_CALLBACK(window_key_callback), NULL);

            screen = gtk_window_get_screen(GTK_WINDOW(window));
            gtk_window_set_screen(GTK_WINDOW(fs_window), screen);
            gtk_window_set_title(GTK_WINDOW(fs_window), _("Gnome MPlayer Fullscreen"));
            gdk_screen_get_monitor_geometry(screen,
                                            gdk_screen_get_monitor_at_window
                                            (screen, window->window), &rect);

            x = rect.width;
            y = rect.height;
            gtk_widget_realize(fs_window);

            gdk_window_get_root_origin(window->window, &wx, &wy);
            gtk_window_move(GTK_WINDOW(fs_window), wx, wy);

            gtk_widget_show(fs_window);
            gtk_window_fullscreen(GTK_WINDOW(fs_window));
            if (GTK_WIDGET_MAPPED(window))
                gtk_widget_unmap(window);
            XReparentWindow(GDK_WINDOW_XDISPLAY(window->window),
                            GDK_WINDOW_XWINDOW(window->window),
                            GDK_WINDOW_XWINDOW(fs_window->window), 0, 0);

            gtk_widget_map(window);
            XResizeWindow(GDK_WINDOW_XDISPLAY(window->window),
                          GDK_WINDOW_XWINDOW(window->window), rect.width, rect.height - 1);
            gtk_window_resize(GTK_WINDOW(window), rect.width, rect.height);
            if (window_x < 250) {
                gtk_widget_show(fs_event_box);
            }
            if (window_x < 170) {
                gtk_widget_show(GTK_WIDGET(tracker));
            }
            while (gtk_events_pending())
                gtk_main_iteration();
        } else {
            restore_playlist =
                gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist));
            restore_details =
                gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_details));
            restore_info = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_info));
            g_object_get(pane, "position", &restore_pane, NULL);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist), FALSE);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_details), FALSE);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), FALSE);
            gtk_widget_hide(menubar);
            while (gtk_events_pending())
                gtk_main_iteration();
        }

        gtk_window_get_size(GTK_WINDOW(window), &stored_window_width, &stored_window_height);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_config), FALSE);
        if (embed_window == 0) {
            gtk_window_fullscreen(GTK_WINDOW(window));
        }
        fullscreen = 1;
        motion_notify_callback(NULL, NULL, NULL);
    }

}

void menuitem_copyurl_callback(GtkMenuItem * menuitem, void *data)
{
    GtkClipboard *clipboard;
    gchar *url;

    url = g_strdup(idledata->url);
    if (strlen(url) == 0) {
        g_free(url);
        url = g_strdup(lastfile);
    }
    clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text(clipboard, url, -1);
    clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, url, -1);

    g_free(url);
}

void menuitem_showcontrols_callback(GtkCheckMenuItem * menuitem, void *data)
{
    int width, height;

    if (GTK_CHECK_MENU_ITEM(menuitem) == GTK_CHECK_MENU_ITEM(menuitem_view_controls)) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols),
                                       gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM
                                                                      (menuitem_view_controls)));
        return;
    } else {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_controls),
                                       gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM
                                                                      (menuitem_showcontrols)));
    }

    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols))) {
        if (GTK_IS_WIDGET(button_event_box)) {
            gtk_widget_hide_all(button_event_box);
        }
        gtk_widget_set_size_request(controls_box, -1, -1);
        gtk_widget_show(controls_box);
        if (!fullscreen && embed_window == 0) {
            gtk_window_get_size(GTK_WINDOW(window), &width, &height);
            gtk_window_resize(GTK_WINDOW(window), width, height + controls_box->allocation.height);
        }
        showcontrols = TRUE;
    } else {
        gtk_widget_hide(controls_box);
        if (!fullscreen && embed_window == 0) {
            gtk_window_get_size(GTK_WINDOW(window), &width, &height);
            gtk_window_resize(GTK_WINDOW(window), width, height - controls_box->allocation.height);
        }
        showcontrols = FALSE;
    }
}

void config_apply(GtkWidget * widget, void *data)
{
    gchar *cmd;
    gint oldosd;
    gboolean old_disable_framedrop;
    gchar *filename;
    GdkColor sub_color;

    if (vo != NULL) {
        g_free(vo);
        vo = NULL;
    }
    vo = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(config_vo)->child)));

    if (ao != NULL) {
        g_free(ao);
        ao = NULL;
    }
    ao = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(config_ao)->child)));

#ifdef HAVE_ASOUNDLIB
    if (mixer != NULL) {
        g_free(mixer);
        mixer = NULL;
    }
    mixer = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(config_mixer)->child)));
#endif

    if (alang != NULL) {
        g_free(alang);
        alang = NULL;
    }
    alang = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(config_alang)->child)));

    if (slang != NULL) {
        g_free(slang);
        slang = NULL;
    }
    slang = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(config_slang)->child)));

    if (metadata_codepage != NULL) {
        g_free(metadata_codepage);
        metadata_codepage = NULL;
    }
    metadata_codepage =
        g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(config_metadata_codepage)->child)));

    if (subtitle_codepage != NULL) {
        g_free(subtitle_codepage);
        subtitle_codepage = NULL;
    }
    subtitle_codepage =
        g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(config_subtitle_codepage)->child)));

    cache_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(config_cachesize));
    old_disable_framedrop = disable_framedrop;
    disable_deinterlace =
        !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_deinterlace));
    disable_framedrop =
        !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_framedrop));
    disable_ass = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_ass));
    disable_embeddedfonts =
        !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_embeddedfonts));
    disable_pause_on_click =
        !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_pause_on_click));
    oldosd = osdlevel;
    osdlevel = (gint) gtk_range_get_value(GTK_RANGE(config_osdlevel));
    pplevel = (gint) gtk_range_get_value(GTK_RANGE(config_pplevel));
    softvol = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_softvol));
    verbose = (gint) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_verbose));
    playlist_visible = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_playlist_visible));
    details_visible = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_details_visible));
    vertical_layout = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_vertical_layout));
    single_instance = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_single_instance));
    replace_and_play = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_replace_and_play));
#ifdef NOTIFY_ENABLED
    show_notification = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_show_notification));
#endif
    thumb_position = gtk_combo_box_get_active(GTK_COMBO_BOX(config_thumb_position));
#ifdef GTK2_12_ENABLED
    show_status_icon = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_show_status_icon));
    gtk_status_icon_set_visible(status_icon, show_status_icon);
#endif
    forcecache = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_forcecache));
    remember_loc = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_remember_loc));
    keep_on_top = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_keep_on_top));
    gtk_window_set_keep_above(GTK_WINDOW(window), keep_on_top);

    if (subtitlefont != NULL) {
        g_free(subtitlefont);
        subtitlefont = NULL;
    }
    subtitlefont = g_strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(config_subtitle_font)));
    subtitle_scale = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(config_subtitle_scale));
    gtk_color_button_get_color(GTK_COLOR_BUTTON(config_subtitle_color), &sub_color);
    if (subtitle_color != NULL) {
        g_free(subtitle_color);
        subtitle_color = NULL;
    }
    subtitle_color =
        g_strdup_printf("%02x%02x%02x00", sub_color.red >> 8, sub_color.green >> 8,
                        sub_color.blue >> 8);
    subtitle_outline = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_subtitle_outline));
    subtitle_shadow = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_subtitle_shadow));
    showsubtitles = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_show_subtitles));

    if (old_disable_framedrop != disable_framedrop) {
        cmd = g_strdup_printf("frame_drop %d\n", !disable_framedrop);
        send_command(cmd, TRUE);
        g_free(cmd);
    }

    if (oldosd != osdlevel) {
        cmd = g_strdup_printf("osd %i\n", osdlevel);
        send_command(cmd, TRUE);
        g_free(cmd);
    }

    qt_disabled = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_qt));
    real_disabled = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_real));
    wmp_disabled = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_wmp));
    dvx_disabled = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_dvx));
    midi_disabled = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_midi));
    embedding_disabled = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_noembed));

    mplayer_bin = g_strdup(gtk_entry_get_text(GTK_ENTRY(config_mplayer_bin)));
    if (!g_file_test(mplayer_bin, G_FILE_TEST_EXISTS)) {
        g_free(mplayer_bin);
        mplayer_bin = NULL;
    }
    extraopts = g_strdup(gtk_entry_get_text(GTK_ENTRY(config_extraopts)));

    update_mplayer_config();

    gm_store = gm_pref_store_new("gnome-mplayer");
#ifndef HAVE_ASOUNDLIB
    gm_pref_store_set_int(gm_store, VOLUME,
                          gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(config_volume)));
#endif
    gm_pref_store_set_int(gm_store, CACHE_SIZE, cache_size);
    gm_pref_store_set_string(gm_store, MIXER, mixer);
    gm_pref_store_set_int(gm_store, OSDLEVEL, osdlevel);
    gm_pref_store_set_int(gm_store, PPLEVEL, pplevel);
    gm_pref_store_set_boolean(gm_store, SOFTVOL, softvol);
    gm_pref_store_set_boolean(gm_store, FORCECACHE, forcecache);
    gm_pref_store_set_boolean(gm_store, DISABLEASS, disable_ass);
    gm_pref_store_set_boolean(gm_store, DISABLEEMBEDDEDFONTS, disable_embeddedfonts);
    gm_pref_store_set_boolean(gm_store, DISABLEDEINTERLACE, disable_deinterlace);
    gm_pref_store_set_boolean(gm_store, DISABLEFRAMEDROP, disable_framedrop);
    gm_pref_store_set_boolean(gm_store, DISABLEPAUSEONCLICK, disable_pause_on_click);
    gm_pref_store_set_boolean(gm_store, SHOWPLAYLIST, playlist_visible);
    gm_pref_store_set_boolean(gm_store, SHOWDETAILS, details_visible);
    gm_pref_store_set_boolean(gm_store, SHOW_NOTIFICATION, show_notification);
    gm_pref_store_set_boolean(gm_store, SHOW_STATUS_ICON, show_status_icon);
    gm_pref_store_set_boolean(gm_store, VERTICAL, vertical_layout);
    gm_pref_store_set_boolean(gm_store, SINGLE_INSTANCE, single_instance);
    gm_pref_store_set_boolean(gm_store, REPLACE_AND_PLAY, replace_and_play);
    gm_pref_store_set_boolean(gm_store, REMEMBER_LOC, remember_loc);
    gm_pref_store_set_boolean(gm_store, KEEP_ON_TOP, keep_on_top);
    gm_pref_store_set_int(gm_store, TRACKER_POSITION, thumb_position);
    gm_pref_store_set_int(gm_store, VERBOSE, verbose);
    gm_pref_store_set_string(gm_store, METADATACODEPAGE, metadata_codepage);
    gm_pref_store_set_string(gm_store, SUBTITLEFONT, subtitlefont);
    gm_pref_store_set_float(gm_store, SUBTITLESCALE, subtitle_scale);
    gm_pref_store_set_string(gm_store, SUBTITLECODEPAGE, subtitle_codepage);
    gm_pref_store_set_string(gm_store, SUBTITLECOLOR, subtitle_color);
    gm_pref_store_set_boolean(gm_store, SUBTITLEOUTLINE, subtitle_outline);
    gm_pref_store_set_boolean(gm_store, SUBTITLESHADOW, subtitle_shadow);
    gm_pref_store_set_boolean(gm_store, SHOW_SUBTITLES, showsubtitles);

    gm_pref_store_set_string(gm_store, MPLAYER_BIN, mplayer_bin);
    gm_pref_store_set_string(gm_store, EXTRAOPTS, extraopts);
    gm_pref_store_free(gm_store);

    gmp_store = gm_pref_store_new("gecko-mediaplayer");
    gm_pref_store_set_boolean(gmp_store, DISABLE_QT, qt_disabled);
    gm_pref_store_set_boolean(gmp_store, DISABLE_REAL, real_disabled);
    gm_pref_store_set_boolean(gmp_store, DISABLE_WMP, wmp_disabled);
    gm_pref_store_set_boolean(gmp_store, DISABLE_DVX, dvx_disabled);
    gm_pref_store_set_boolean(gmp_store, DISABLE_MIDI, midi_disabled);
    gm_pref_store_set_boolean(gmp_store, DISABLE_EMBEDDING, embedding_disabled);
    gm_pref_store_free(gmp_store);

    filename = g_strdup_printf("%s/.mozilla/pluginreg.dat", g_getenv("HOME"));
    g_remove(filename);
    g_free(filename);
    filename = g_strdup_printf("%s/.firefox/pluginreg.dat", g_getenv("HOME"));
    g_remove(filename);
    g_free(filename);
    filename = g_strdup_printf("%s/.mozilla/firefox/pluginreg.dat", g_getenv("HOME"));
    g_remove(filename);
    g_free(filename);

    dbus_reload_plugins();

    gtk_widget_destroy(widget);
}

void adv_reset_values(GtkWidget * widget, void *data)
{
    gtk_range_set_value(GTK_RANGE(adv_brightness), 0);
    gtk_range_set_value(GTK_RANGE(adv_contrast), 0);
    gtk_range_set_value(GTK_RANGE(adv_hue), 0);
    gtk_range_set_value(GTK_RANGE(adv_gamma), 0);
    gtk_range_set_value(GTK_RANGE(adv_saturation), 0);
}


void config_close(GtkWidget * widget, void *data)
{
    selection = NULL;
    if (GTK_IS_WIDGET(widget))
        gtk_widget_destroy(widget);
}

void brightness_callback(GtkRange * range, gpointer data)
{
    gint brightness;
    gchar *cmd;
    IdleData *idle = (IdleData *) data;

    brightness = (gint) gtk_range_get_value(range);
    cmd = g_strdup_printf("brightness %i 1\n", brightness);
    send_command(cmd, TRUE);
    g_free(cmd);
    send_command("get_property brightness\n", TRUE);
    idle->brightness = brightness;
}

void contrast_callback(GtkRange * range, gpointer data)
{
    gint contrast;
    gchar *cmd;
    IdleData *idle = (IdleData *) data;

    contrast = (gint) gtk_range_get_value(range);
    cmd = g_strdup_printf("contrast %i 1\n", contrast);
    send_command(cmd, TRUE);
    g_free(cmd);
    send_command("get_property contrast\n", TRUE);
    idle->contrast = contrast;
}

void gamma_callback(GtkRange * range, gpointer data)
{
    gint gamma;
    gchar *cmd;
    IdleData *idle = (IdleData *) data;

    gamma = (gint) gtk_range_get_value(range);
    cmd = g_strdup_printf("gamma %i 1\n", gamma);
    send_command(cmd, TRUE);
    g_free(cmd);
    send_command("get_property gamma\n", TRUE);
    idle->hue = gamma;
}

void hue_callback(GtkRange * range, gpointer data)
{
    gint hue;
    gchar *cmd;
    IdleData *idle = (IdleData *) data;

    hue = (gint) gtk_range_get_value(range);
    cmd = g_strdup_printf("hue %i 1\n", hue);
    send_command(cmd, TRUE);
    g_free(cmd);
    send_command("get_property hue\n", TRUE);
    idle->hue = hue;
}

void saturation_callback(GtkRange * range, gpointer data)
{
    gint saturation;
    gchar *cmd;
    IdleData *idle = (IdleData *) data;

    saturation = (gint) gtk_range_get_value(range);
    cmd = g_strdup_printf("saturation %i 1\n", saturation);
    send_command(cmd, TRUE);
    g_free(cmd);
    send_command("get_property saturation\n", TRUE);
    idle->saturation = saturation;
}

void menuitem_meter_callback(GtkMenuItem * menuitem, void *data)
{
    gint width = 0, height = 0;

    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_meter))) {
        gtk_window_get_size(GTK_WINDOW(window), &width, &height);
        gtk_widget_hide(audio_meter);
        gtk_window_resize(GTK_WINDOW(window), width, height - audio_meter->allocation.height);

    } else {
        gtk_window_get_size(GTK_WINDOW(window), &width, &height);
        gtk_widget_show(audio_meter);
        gtk_window_resize(GTK_WINDOW(window), width, height + audio_meter->allocation.height);
    }
}

void menuitem_details_callback(GtkMenuItem * menuitem, void *data)
{
    GtkWidget *label;
    gchar *buf;
    gint i = 0;
    IdleData *idle = idledata;
    gint width = 0, height = 0;
	gboolean dontresize = FALSE;

    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_details))) {
        if (GTK_IS_WIDGET(details_table)) {
            gtk_window_get_size(GTK_WINDOW(window), &width, &height);
            gtk_widget_hide_all(details_vbox);
            gtk_window_resize(GTK_WINDOW(window), width, height - details_vbox->allocation.height);
            gtk_widget_destroy(details_table);
            details_table = NULL;
        }
    } else {
        if (GTK_IS_WIDGET(details_table)) {
            gtk_widget_destroy(details_table);
            details_table = NULL;
			dontresize = TRUE;
        }
        details_table = gtk_table_new(20, 2, FALSE);
        gtk_container_add(GTK_CONTAINER(details_vbox), details_table);

        if (idle != NULL && idle->videopresent) {
            label = gtk_label_new(_("<span weight=\"bold\">Video Details</span>"));
            gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 0, 6);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
            i++;

            label = gtk_label_new(_("Video Size:"));
            gtk_widget_set_size_request(label, 150, -1);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
            buf = g_strdup_printf("%i x %i", idle->width, idle->height);
            label = gtk_label_new(buf);
            gtk_widget_set_size_request(label, 100, -1);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 1, 2, i, i + 1);
            g_free(buf);
            i++;

            label = gtk_label_new(_("Video Format:"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
            buf = g_ascii_strup(idle->video_format, -1);
            label = gtk_label_new(buf);
            g_free(buf);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 1, 2, i, i + 1);
            i++;

            label = gtk_label_new(_("Video Codec:"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
            buf = g_ascii_strup(idle->video_codec, -1);
            label = gtk_label_new(buf);
            g_free(buf);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 1, 2, i, i + 1);
            i++;

            label = gtk_label_new(_("Video FPS:"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
            label = gtk_label_new(idle->video_fps);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 1, 2, i, i + 1);
            i++;

            label = gtk_label_new(_("Video Bitrate:"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
            buf = g_strdup_printf("%i Kb/s", (gint) (g_strtod(idle->video_bitrate, NULL) / 1000));
            label = gtk_label_new(buf);
            g_free(buf);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 1, 2, i, i + 1);
            i++;

            if (idle->chapters > 0) {
                label = gtk_label_new(_("Video Chapters:"));
                gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
                gtk_misc_set_padding(GTK_MISC(label), 12, 0);
                gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
                buf = g_strdup_printf("%i", idle->chapters);
                label = gtk_label_new(buf);
                g_free(buf);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
                gtk_table_attach_defaults(GTK_TABLE(details_table), label, 1, 2, i, i + 1);
                i++;
            }

            label = gtk_label_new("");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
            i++;
        }

        if (idle != NULL && idle->audiopresent) {
            label = gtk_label_new(_("<span weight=\"bold\">Audio Details</span>"));
            gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 0, 6);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
            i++;

            label = gtk_label_new(_("Audio Codec:"));
            gtk_widget_set_size_request(label, 150, -1);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
            if (idle != NULL) {
                buf = g_ascii_strup(idle->audio_codec, -1);
                label = gtk_label_new(buf);
                g_free(buf);
                gtk_widget_set_size_request(label, 100, -1);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
                gtk_table_attach_defaults(GTK_TABLE(details_table), label, 1, 2, i, i + 1);
            }
            i++;

            if (idle->audio_channels != NULL && strlen(idle->audio_channels) > 0
                && g_strtod(idle->audio_channels, NULL) >= 1) {
                label = gtk_label_new(_("Audio Channels:"));
                gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
                gtk_misc_set_padding(GTK_MISC(label), 12, 0);
                gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
                if (idle != NULL) {
                    buf = g_ascii_strup(idle->audio_channels, -1);
                    label = gtk_label_new(buf);
                    g_free(buf);
                    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
                    gtk_table_attach_defaults(GTK_TABLE(details_table), label, 1, 2, i, i + 1);
                }
                i++;
            }

            label = gtk_label_new(_("Audio Bitrate:"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
            if (idle != NULL) {
                buf =
                    g_strdup_printf("%i Kb/s", (gint) (g_strtod(idle->audio_bitrate, NULL) / 1000));
                label = gtk_label_new(buf);
                g_free(buf);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
                gtk_table_attach_defaults(GTK_TABLE(details_table), label, 1, 2, i, i + 1);
            }
            i++;

            label = gtk_label_new(_("Audio Sample Rate:"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
            if (idle != NULL) {
                buf =
                    g_strdup_printf("%i Kb/s",
                                    (gint) (g_strtod(idle->audio_samplerate, NULL) / 1000));
                label = gtk_label_new(buf);
                g_free(buf);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
                gtk_table_attach_defaults(GTK_TABLE(details_table), label, 1, 2, i, i + 1);
            }
            i++;

        }
        if (idledata->window_resized) {
            while (gtk_events_pending())
                gtk_main_iteration();

            if (idledata->videopresent) {
                gtk_window_get_size(GTK_WINDOW(window), &width, &height);
            }

            if (i > 0)
                gtk_widget_show_all(details_vbox);

            while (gtk_events_pending())
                gtk_main_iteration();

            if (idledata->videopresent) {
                height += details_vbox->allocation.height;
				if (!dontresize)
	                gtk_window_resize(GTK_WINDOW(window), width, height);
            }

        } else {
            if (i > 0)
                gtk_widget_show_all(details_vbox);
        }

    }

    if (idle != NULL && !idle->videopresent && !idle->audiopresent) {
        if (GTK_IS_WIDGET(details_table))
            gtk_widget_destroy(details_table);
        details_table = NULL;
    }

}


void menuitem_advanced_callback(GtkMenuItem * menuitem, void *data)
{
    GtkWidget *adv_window;
    GtkWidget *adv_vbox;
    GtkWidget *adv_hbutton_box;
    GtkWidget *adv_table;
    GtkWidget *adv_reset;
    GtkWidget *adv_close;
    GtkWidget *label;
    gint i = 0;

    adv_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(adv_window), GDK_WINDOW_TYPE_HINT_UTILITY);
    gtk_window_set_resizable(GTK_WINDOW(adv_window), FALSE);
    gtk_window_set_title(GTK_WINDOW(adv_window), _("Video Picture Adjustments"));

    adv_vbox = gtk_vbox_new(FALSE, 10);
    adv_hbutton_box = gtk_hbutton_box_new();
    gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
    adv_table = gtk_table_new(20, 2, FALSE);

    gtk_container_add(GTK_CONTAINER(adv_vbox), adv_table);
    gtk_container_add(GTK_CONTAINER(adv_vbox), adv_hbutton_box);
    gtk_container_add(GTK_CONTAINER(adv_window), adv_vbox);

    gtk_container_set_border_width(GTK_CONTAINER(adv_window), 5);

    label = gtk_label_new(_("<span weight=\"bold\">Video Picture Adjustments</span>"));
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(label), 0, 6);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), label, 0, 1, i, i + 1);
    i++;

    label = gtk_label_new(_("Brightness"));
    adv_brightness = gtk_hscale_new_with_range(-100.0, 100.0, 1.0);
    gtk_widget_set_size_request(adv_brightness, 200, -1);
    gtk_range_set_value(GTK_RANGE(adv_brightness), idledata->brightness);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), label, 0, 1, i, i + 1);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), adv_brightness, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Contrast"));
    adv_contrast = gtk_hscale_new_with_range(-100.0, 100.0, 1.0);
    gtk_widget_set_size_request(adv_contrast, 200, -1);
    gtk_range_set_value(GTK_RANGE(adv_contrast), idledata->contrast);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), label, 0, 1, i, i + 1);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), adv_contrast, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Gamma"));
    adv_gamma = gtk_hscale_new_with_range(-100.0, 100.0, 1.0);
    gtk_widget_set_size_request(adv_gamma, 200, -1);
    gtk_range_set_value(GTK_RANGE(adv_gamma), idledata->gamma);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), label, 0, 1, i, i + 1);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), adv_gamma, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Hue"));
    adv_hue = gtk_hscale_new_with_range(-100.0, 100.0, 1.0);
    gtk_widget_set_size_request(adv_hue, 200, -1);
    gtk_range_set_value(GTK_RANGE(adv_hue), idledata->hue);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), label, 0, 1, i, i + 1);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), adv_hue, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Saturation"));
    adv_saturation = gtk_hscale_new_with_range(-100.0, 100.0, 1.0);
    gtk_widget_set_size_request(adv_saturation, 200, -1);
    gtk_range_set_value(GTK_RANGE(adv_saturation), idledata->saturation);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), label, 0, 1, i, i + 1);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), adv_saturation, 1, 2, i, i + 1);
    i++;

    g_signal_connect(G_OBJECT(adv_brightness), "value_changed", G_CALLBACK(brightness_callback),
                     idledata);
    g_signal_connect(G_OBJECT(adv_contrast), "value_changed", G_CALLBACK(contrast_callback),
                     idledata);
    g_signal_connect(G_OBJECT(adv_gamma), "value_changed", G_CALLBACK(gamma_callback), idledata);
    g_signal_connect(G_OBJECT(adv_hue), "value_changed", G_CALLBACK(hue_callback), idledata);
    g_signal_connect(G_OBJECT(adv_saturation), "value_changed", G_CALLBACK(saturation_callback),
                     idledata);

    adv_reset = gtk_button_new_with_mnemonic(_("_Reset"));
    g_signal_connect(GTK_OBJECT(adv_reset), "clicked", GTK_SIGNAL_FUNC(adv_reset_values), NULL);
    gtk_container_add(GTK_CONTAINER(adv_hbutton_box), adv_reset);

    adv_close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(GTK_OBJECT(adv_close), "clicked",
                             GTK_SIGNAL_FUNC(config_close), adv_window);

    gtk_container_add(GTK_CONTAINER(adv_hbutton_box), adv_close);
    gtk_widget_show_all(adv_window);
    gtk_window_set_transient_for(GTK_WINDOW(adv_window), GTK_WINDOW(window));
    gtk_window_set_keep_above(GTK_WINDOW(adv_window), keep_on_top);
    gtk_window_present(GTK_WINDOW(adv_window));
}

void menuitem_view_angle_callback(GtkMenuItem * menuitem, gpointer data)
{
    gchar *cmd;

    cmd = g_strdup_printf("switch_angle\n");
    send_command(cmd, TRUE);
    g_free(cmd);

    return;
}

void menuitem_view_smaller_subtitle_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *cmd;

    if (subtitle_scale > 0.2) {
        subtitle_scale -= 0.2;
    } else {
        subtitle_scale = 0.2;
    }

    cmd = g_strdup_printf("sub_scale %f 1\n", subtitle_scale);
    send_command(cmd, TRUE);
    g_free(cmd);

    return;

}

void menuitem_view_larger_subtitle_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *cmd;

    subtitle_scale += 0.2;

    cmd = g_strdup_printf("sub_scale %f 1\n", subtitle_scale);
    send_command(cmd, TRUE);
    g_free(cmd);

    return;

}

void menuitem_view_aspect_callback(GtkMenuItem * menuitem, void *data)
{
    static gint i = 0;

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_default) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine),
                                       FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten),
                                       FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window),
                                       FALSE);
        i--;
    }

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_four_three) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine),
                                       FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten),
                                       FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window),
                                       FALSE);
        i--;
    }

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_sixteen_nine) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten),
                                       FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window),
                                       FALSE);
        i--;
    }

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_sixteen_ten) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine),
                                       FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window),
                                       FALSE);
        i--;
    }

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_follow_window) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine),
                                       FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten),
                                       FALSE);
        i--;
    }


    if (i == 0) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
        allocate_fixed_callback(fixed, &fixed->allocation, NULL);
    }


}

static gchar *osdlevel_format_callback(GtkScale * scale, gdouble value)
{
    gchar *text;

    switch ((gint) value) {

    case 0:
        text = g_strdup(_("No Display"));
        break;
    case 1:
        text = g_strdup(_("Minimal"));
        break;
    case 2:
        text = g_strdup(_("Timer"));
        break;
    case 3:
        text = g_strdup(_("Timer/Total"));
        break;
    default:
        text = g_strdup("How did we get here?");
    }

    return text;
}

static gchar *pplevel_format_callback(GtkScale * scale, gdouble value)
{
    gchar *text;

    switch ((gint) value) {

    case 0:
        text = g_strdup(_("No Postprocessing"));
        break;
    case 1:
    case 2:
        text = g_strdup(_("Minimal Postprocessing"));
        break;
    case 3:
    case 4:
        text = g_strdup(_("More Postprocessing"));
        break;
    case 5:
    case 6:
        text = g_strdup(_("Maximum Postprocessing"));
        break;
    default:
        text = g_strdup("How did we get here?");
    }

    return text;
}

void osdlevel_change_callback(GtkRange * range, gpointer data)
{
    gchar *cmd;
    gint value;

    value = gtk_range_get_value(range);
    cmd = g_strdup_printf("osd %i\n", (gint) value);
    send_command(cmd, TRUE);
    g_free(cmd);

    return;
}

void config_single_instance_callback(GtkWidget * button, gpointer data)
{
    gtk_widget_set_sensitive(config_replace_and_play,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                          (config_single_instance)));
}

void ass_toggle_callback(GtkToggleButton * source, gpointer user_data)
{
    gtk_widget_set_sensitive(config_subtitle_color, gtk_toggle_button_get_active(source));
    gtk_widget_set_sensitive(config_embeddedfonts, gtk_toggle_button_get_active(source));
}

void embedded_fonts_toggle_callback(GtkToggleButton * source, gpointer user_data)
{
    gtk_widget_set_sensitive(config_subtitle_font, !gtk_toggle_button_get_active(source));
    gtk_widget_set_sensitive(config_subtitle_shadow, !gtk_toggle_button_get_active(source));
    gtk_widget_set_sensitive(config_subtitle_outline, !gtk_toggle_button_get_active(source));
}


void ao_change_callback(GtkComboBox widget, gpointer data)
{

#ifdef HAVE_ASOUNDLIB
    if (g_strncasecmp(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(config_ao)->child)), "alsa", 4) == 0) {
        gtk_widget_set_sensitive(config_mixer, TRUE);
    } else {
        gtk_widget_set_sensitive(config_mixer, FALSE);
    }
#endif

}


void menuitem_config_callback(GtkMenuItem * menuitem, void *data)
{
    GtkWidget *config_window;
    GtkWidget *conf_vbox;
    GtkWidget *conf_hbutton_box;
    GtkWidget *conf_ok;
    GtkWidget *conf_cancel;
    GtkWidget *conf_table;
    GtkWidget *conf_label;
    GtkWidget *conf_page1;
    GtkWidget *conf_page2;
    GtkWidget *conf_page3;
    GtkWidget *conf_page4;
    GtkWidget *conf_page5;
    GtkWidget *conf_page6;
    GtkWidget *notebook;
    GdkColor sub_color;
    gint i = 0;
    gint j = -1;

#ifdef HAVE_ASOUNDLIB
    snd_mixer_t *mhandle;
    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *sid;
    gint err;
    gchar *mix;
#endif

    read_mplayer_config();

    config_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon(GTK_WINDOW(config_window), pb_icon);

    gtk_window_set_resizable(GTK_WINDOW(config_window), FALSE);
    conf_vbox = gtk_vbox_new(FALSE, 10);
    conf_page1 = gtk_vbox_new(FALSE, 10);
    conf_page2 = gtk_vbox_new(FALSE, 10);
    conf_page3 = gtk_vbox_new(FALSE, 10);
    conf_page4 = gtk_vbox_new(FALSE, 10);
    conf_page5 = gtk_vbox_new(FALSE, 10);
    conf_page6 = gtk_vbox_new(FALSE, 10);
    conf_hbutton_box = gtk_hbutton_box_new();
    gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
    conf_table = gtk_table_new(20, 2, FALSE);

    notebook = gtk_notebook_new();
    conf_label = gtk_label_new(_("Player"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), conf_page1, conf_label);
    conf_label = gtk_label_new(_("Language Settings"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), conf_page3, conf_label);
    conf_label = gtk_label_new(_("Subtitles"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), conf_page4, conf_label);
    conf_label = gtk_label_new(_("Interface"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), conf_page5, conf_label);
    conf_label = gtk_label_new(_("MPlayer"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), conf_page6, conf_label);
    conf_label = gtk_label_new(_("Plugin"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), conf_page2, conf_label);

    gtk_container_add(GTK_CONTAINER(conf_vbox), notebook);
    gtk_container_add(GTK_CONTAINER(config_window), conf_vbox);

    gtk_window_set_title(GTK_WINDOW(config_window), _("GNOME MPlayer Configuration"));
    gtk_container_set_border_width(GTK_CONTAINER(config_window), 5);
    gtk_window_set_default_size(GTK_WINDOW(config_window), 300, 300);
    conf_ok = gtk_button_new_from_stock(GTK_STOCK_OK);
    g_signal_connect_swapped(GTK_OBJECT(conf_ok), "clicked",
                             GTK_SIGNAL_FUNC(config_apply), config_window);

    conf_cancel = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(GTK_OBJECT(conf_cancel), "clicked",
                             GTK_SIGNAL_FUNC(config_apply), config_window);

    config_vo = gtk_combo_box_entry_new_text();
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_vo,
                         _
                         ("mplayer video output device\nx11 should always work, try xv or gl for better performance"),
                         NULL);
    if (config_vo != NULL) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "gl");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "x11");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "xv");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "xvmc");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "vdpau");
        if (vo != NULL) {
            if (strcmp(vo, "gl") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 0);
            if (strcmp(vo, "x11") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 1);
            if (strcmp(vo, "xv") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 2);
            if (strcmp(vo, "xvmc") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 3);
            if (strcmp(vo, "vdpau") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 4);
            if (gtk_combo_box_get_active(GTK_COMBO_BOX(config_vo))
                == -1) {
                gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), vo);
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 5);
            }
        }
    }
#ifdef HAVE_ASOUNDLIB
    config_mixer = gtk_combo_box_entry_new_text();
    if (config_mixer != NULL) {
        if ((err = snd_mixer_open(&mhandle, 0)) < 0) {
            if (verbose)
                printf("Mixer open error %s\n", snd_strerror(err));
        }

        if ((err = snd_mixer_attach(mhandle, device)) < 0) {
            if (verbose)
                printf("Mixer attach error %s\n", snd_strerror(err));
        }

        if ((err = snd_mixer_selem_register(mhandle, NULL, NULL)) < 0) {
            if (verbose)
                printf("Mixer register error %s\n", snd_strerror(err));
        }

        if ((err = snd_mixer_load(mhandle)) < 0) {
            if (verbose)
                printf("Mixer load error %s\n", snd_strerror(err));
        }
        i = 1;
        j = -1;
        snd_mixer_selem_id_alloca(&sid);
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_mixer), "");
        for (elem = snd_mixer_first_elem(mhandle); elem; elem = snd_mixer_elem_next(elem)) {
            snd_mixer_selem_get_id(elem, sid);
            if (!snd_mixer_selem_is_active(elem))
                continue;
            if (snd_mixer_selem_has_capture_volume(elem)
                || snd_mixer_selem_has_capture_switch(elem))
                continue;
            if (!snd_mixer_selem_has_playback_volume(elem))
                continue;
            mix =
                g_strdup_printf("%s,%i", snd_mixer_selem_id_get_name(sid),
                                snd_mixer_selem_id_get_index(sid));
            //mix = g_strdup_printf("%s", snd_mixer_selem_id_get_name(sid));
            gtk_combo_box_append_text(GTK_COMBO_BOX(config_mixer), mix);
            if (mixer != NULL && g_ascii_strcasecmp(mix, mixer) == 0)
                j = i;
            i++;
        }
        if (j != -1)
            gtk_combo_box_set_active(GTK_COMBO_BOX(config_mixer), j);
        if (mixer != NULL && strlen(mixer) > 0 && j == -1) {
            gtk_combo_box_append_text(GTK_COMBO_BOX(config_mixer), mixer);
            gtk_combo_box_set_active(GTK_COMBO_BOX(config_mixer), i);
        }
        snd_mixer_close(mhandle);

    }
#endif

    config_ao = gtk_combo_box_entry_new_text();
    g_signal_connect(GTK_WIDGET(config_ao), "changed", GTK_SIGNAL_FUNC(ao_change_callback), NULL);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_ao,
                         _
                         ("mplayer audio output device\nalsa or oss should always work, try esd in gnome, arts in kde, or pulse on newer distributions"),
                         NULL);
    if (config_ao != NULL) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_ao), "alsa");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_ao), "arts");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_ao), "esd");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_ao), "jack");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_ao), "oss");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_ao), "pulse");
        if (ao != NULL) {
            if (strcmp(ao, "alsa") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_ao), 0);
            if (strcmp(ao, "arts") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_ao), 1);
            if (strcmp(ao, "esd") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_ao), 2);
            if (strcmp(ao, "jack") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_ao), 3);
            if (strcmp(ao, "oss") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_ao), 4);
            if (strcmp(ao, "pulse") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_ao), 5);
            if (gtk_combo_box_get_active(GTK_COMBO_BOX(config_ao))
                == -1) {
                gtk_combo_box_append_text(GTK_COMBO_BOX(config_ao), ao);
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_ao), 6);
            }
        }
    }

    config_alang = gtk_combo_box_entry_new_text();
    if (config_alang != NULL) {
        i = 0;
        j = -1;
        while (i < 464) {
            if (alang != NULL && g_strncasecmp(alang, langlist[i], strlen(alang)) == 0)
                j = i;
            gtk_combo_box_append_text(GTK_COMBO_BOX(config_alang), langlist[i++]);
            if (j != -1) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_alang), j);
            }
        }
        if (alang != NULL && j == -1) {
            gtk_combo_box_append_text(GTK_COMBO_BOX(config_alang), alang);
            gtk_combo_box_set_active(GTK_COMBO_BOX(config_alang), i);
        }
    }
    config_slang = gtk_combo_box_entry_new_text();
    if (config_slang != NULL) {
        i = 0;
        j = -1;
        while (i < 464) {
            if (slang != NULL && g_strncasecmp(slang, langlist[i], strlen(slang)) == 0)
                j = i;
            gtk_combo_box_append_text(GTK_COMBO_BOX(config_slang), langlist[i++]);
            if (j != -1)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_slang), j);
        }
        if (slang != NULL && j == -1) {
            gtk_combo_box_append_text(GTK_COMBO_BOX(config_slang), slang);
            gtk_combo_box_set_active(GTK_COMBO_BOX(config_slang), i);
        }
    }
    config_metadata_codepage = gtk_combo_box_entry_new_text();
    if (config_metadata_codepage != NULL) {
        i = 0;
        j = -1;
        while (i < 25) {
            if (metadata_codepage != NULL && strlen(metadata_codepage) > 1
                && g_strncasecmp(metadata_codepage, codepagelist[i],
                                 strlen(metadata_codepage)) == 0)
                j = i;
            gtk_combo_box_append_text(GTK_COMBO_BOX(config_metadata_codepage), codepagelist[i++]);
            if (j != -1)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_metadata_codepage), j);
        }
    }
    config_subtitle_codepage = gtk_combo_box_entry_new_text();
    if (config_subtitle_codepage != NULL) {
        i = 0;
        j = -1;
        while (i < 25) {
            if (subtitle_codepage != NULL && strlen(subtitle_codepage) > 1
                && g_strncasecmp(subtitle_codepage, codepagelist[i],
                                 strlen(subtitle_codepage)) == 0)
                j = i;
            gtk_combo_box_append_text(GTK_COMBO_BOX(config_subtitle_codepage), codepagelist[i++]);
            if (j != -1)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_subtitle_codepage), j);
        }
    }

    conf_label = gtk_label_new(_("<span weight=\"bold\">Adjust Output Settings</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    i++;

    conf_label = gtk_label_new(_("Video Output:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_widget_set_size_request(GTK_WIDGET(config_vo), 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_vo, 1, 2, i, i + 1, GTK_SHRINK, GTK_SHRINK, 0,
                     0);
    i++;

    conf_label = gtk_label_new(_("Audio Output:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_widget_set_size_request(GTK_WIDGET(config_ao), 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_ao, 1, 2, i, i + 1, GTK_SHRINK, GTK_SHRINK, 0,
                     0);
    i++;

#ifdef HAVE_ASOUNDLIB
    conf_label = gtk_label_new(_("Default Mixer:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_widget_set_size_request(GTK_WIDGET(config_mixer), 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_mixer, 1, 2, i, i + 1, GTK_SHRINK, GTK_SHRINK, 0,
                     0);
    i++;
#endif
    conf_label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    i++;

    gtk_container_add(GTK_CONTAINER(conf_page1), conf_table);

    conf_table = gtk_table_new(20, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(conf_page1), conf_table);
    i = 0;
    conf_label = gtk_label_new(_("<span weight=\"bold\">Adjust Configuration Settings</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    i++;

#ifndef HAVE_ASOUNDLIB
    conf_label = gtk_label_new(_("Default Volume Level:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    config_volume = gtk_spin_button_new_with_range(0, 100, 1);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_volume, _("Default volume for playback"), NULL);
    gtk_widget_set_size_request(config_volume, 100, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_volume, 1, 2, i, i + 1, GTK_SHRINK,
                     GTK_SHRINK, 0, 0);
    gm_store = gm_pref_store_new("gnome-mplayer");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_volume),
                              gm_pref_store_get_int(gm_store, VOLUME));
    gm_pref_store_free(gm_store);
    gtk_entry_set_width_chars(GTK_ENTRY(config_volume), 6);
    gtk_entry_set_editable(GTK_ENTRY(config_volume), FALSE);
    gtk_entry_set_alignment(GTK_ENTRY(config_volume), 1);
    gtk_widget_show(config_volume);
    i++;
#endif

    conf_label = gtk_label_new(_("Minimum Cache Size (KB):"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    config_cachesize = gtk_spin_button_new_with_range(32, 32767, 512);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_cachesize,
                         _
                         ("Amount of data to cache when playing media from network, use higher values for slow networks."),
                         NULL);
    gtk_widget_set_size_request(config_cachesize, 100, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_cachesize, 1, 2, i, i + 1, GTK_SHRINK,
                     GTK_SHRINK, 0, 0);
    //gtk_range_set_value(GTK_RANGE(config_cachesize), cache_size);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_cachesize), cache_size);
    gtk_entry_set_width_chars(GTK_ENTRY(config_cachesize), 6);
    gtk_entry_set_editable(GTK_ENTRY(config_cachesize), FALSE);
    gtk_entry_set_alignment(GTK_ENTRY(config_cachesize), 1);
    gtk_widget_show(config_cachesize);
    i++;

    conf_label = gtk_label_new(_("On Screen Display Level:"));
    config_osdlevel = gtk_hscale_new_with_range(0.0, 3.0, 1.0);
    gtk_range_set_value(GTK_RANGE(config_osdlevel), osdlevel);
    g_signal_connect(GTK_OBJECT(config_osdlevel), "format-value",
                     GTK_SIGNAL_FUNC(osdlevel_format_callback), NULL);
    g_signal_connect(GTK_OBJECT(config_osdlevel), "value-changed",
                     GTK_SIGNAL_FUNC(osdlevel_change_callback), NULL);
    gtk_widget_set_size_request(config_osdlevel, 150, -1);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_osdlevel, 1, 2, i, i + 1);
    i++;

    conf_label = gtk_label_new(_("Post-processing level:"));
    config_pplevel = gtk_hscale_new_with_range(0.0, 6.0, 1.0);
    g_signal_connect(GTK_OBJECT(config_pplevel), "format-value",
                     GTK_SIGNAL_FUNC(pplevel_format_callback), NULL);
    gtk_widget_set_size_request(config_pplevel, 150, -1);
    gtk_range_set_value(GTK_RANGE(config_pplevel), pplevel);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_pplevel, 1, 2, i, i + 1);
    i++;

    conf_table = gtk_table_new(20, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(conf_page2), conf_table);
    i = 0;
    conf_label = gtk_label_new(_("<span weight=\"bold\">Adjust Plugin Emulation Settings</span>\n\n"
                                 "These options affect the gecko-mediaplayer plugin when it is installed.\n"
                                 "Gecko-mediaplayer is a Firefox plugin that will emulate various\n"
                                 "media players and allow playback of various web content within\n"
                                 "NPRuntime compatible browsers (Firefox, Konqueror, etc)."));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    i++;

    config_qt = gtk_check_button_new_with_label(_("QuickTime Emulation"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_qt), !qt_disabled);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_qt, 0, 1, i, i + 1);
    i++;

    config_real = gtk_check_button_new_with_label(_("RealPlayer Emulation"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_real), !real_disabled);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_real, 0, 1, i, i + 1);
    i++;

    config_wmp = gtk_check_button_new_with_label(_("Windows Media Player Emulation"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_wmp), !wmp_disabled);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_wmp, 0, 1, i, i + 1);
    i++;

    config_dvx = gtk_check_button_new_with_label(_("DiVX Player Emulation"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_dvx), !dvx_disabled);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_dvx, 0, 1, i, i + 1);
    i++;

    config_midi = gtk_check_button_new_with_label(_("MIDI Support (requires MPlayer support)"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_midi), !midi_disabled);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_midi, 0, 1, i, i + 1);
    i++;

    config_noembed = gtk_check_button_new_with_label(_("Disable Player Embedding"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_noembed), embedding_disabled);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_noembed, 0, 1, i, i + 1);
    i++;

    conf_table = gtk_table_new(20, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(conf_page3), conf_table);
    i = 0;
    conf_label = gtk_label_new(_("<span weight=\"bold\">Adjust Language Settings</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    i++;

    conf_label = gtk_label_new(_("Default Audio Language"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_widget_set_size_request(GTK_WIDGET(config_alang), 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_alang, 1, 2, i, i + 1, GTK_SHRINK, GTK_SHRINK, 0,
                     0);
    i++;

    conf_label = gtk_label_new(_("Default Subtitle Language:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_widget_set_size_request(GTK_WIDGET(config_slang), 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_slang, 1, 2, i, i + 1, GTK_SHRINK, GTK_SHRINK, 0,
                     0);
    i++;

    conf_label = gtk_label_new(_("File Metadata Encoding:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_widget_set_size_request(GTK_WIDGET(config_metadata_codepage), 200, -1);

    gtk_table_attach(GTK_TABLE(conf_table), config_metadata_codepage, 1, 2, i, i + 1, GTK_SHRINK,
                     GTK_SHRINK, 0, 0);
    i++;

    // Page 4
    conf_table = gtk_table_new(20, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(conf_page4), conf_table);
    i = 0;
    conf_label = gtk_label_new(_("<span weight=\"bold\">Subtitle Settings</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    i++;

    config_ass =
        gtk_check_button_new_with_mnemonic(_
                                           ("Enable _Advanced Substation Alpha (ASS) Subtitle Support"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_ass), !disable_ass);
    g_signal_connect(GTK_OBJECT(config_ass), "toggled", GTK_SIGNAL_FUNC(ass_toggle_callback), NULL);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_ass, 0, 2, i, i + 1);
    gtk_widget_show(config_ass);
    i++;

    config_embeddedfonts = gtk_check_button_new_with_mnemonic(_("Use _Embedded Fonts"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_embeddedfonts), !disable_embeddedfonts);
    gtk_widget_set_sensitive(config_embeddedfonts, !disable_ass);
    g_signal_connect(GTK_OBJECT(config_embeddedfonts), "toggled",
                     GTK_SIGNAL_FUNC(embedded_fonts_toggle_callback), NULL);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_embeddedfonts, 0, 2, i, i + 1);
    gtk_widget_show(config_embeddedfonts);
    i++;

    conf_label = gtk_label_new(_("Subtitle Font:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);

    config_subtitle_font = gtk_font_button_new();
    if (subtitlefont != NULL) {
        gtk_font_button_set_font_name(GTK_FONT_BUTTON(config_subtitle_font), subtitlefont);
    }
    gtk_widget_set_sensitive(config_subtitle_font, disable_embeddedfonts);
    gtk_font_button_set_show_size(GTK_FONT_BUTTON(config_subtitle_font), FALSE);
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(config_subtitle_font), FALSE);

    gtk_font_button_set_use_size(GTK_FONT_BUTTON(config_subtitle_font), FALSE);
    gtk_font_button_set_use_font(GTK_FONT_BUTTON(config_subtitle_font), TRUE);

    gtk_font_button_set_title(GTK_FONT_BUTTON(config_subtitle_font), _("Subtitle Font Selection"));
    gtk_table_attach(GTK_TABLE(conf_table), config_subtitle_font, 1, 2, i, i + 1, GTK_SHRINK,
                     GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Subtitle Color:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    config_subtitle_color = gtk_color_button_new();
    if (subtitle_color != NULL && strlen(subtitle_color) > 5) {
        sub_color.red = g_ascii_xdigit_value(subtitle_color[0]) << 4;
        sub_color.red += g_ascii_xdigit_value(subtitle_color[1]);
        sub_color.red = sub_color.red << 8;
        sub_color.green = g_ascii_xdigit_value(subtitle_color[2]) << 4;
        sub_color.green += g_ascii_xdigit_value(subtitle_color[3]);
        sub_color.green = sub_color.green << 8;
        sub_color.blue = g_ascii_xdigit_value(subtitle_color[4]) << 4;
        sub_color.blue += g_ascii_xdigit_value(subtitle_color[5]);
        sub_color.blue = sub_color.blue << 8;
        gtk_color_button_set_color(GTK_COLOR_BUTTON(config_subtitle_color), &sub_color);
    } else {
        sub_color.red = 0xFF << 8;
        sub_color.green = 0xFF << 8;
        sub_color.blue = 0xFF << 8;
        gtk_color_button_set_color(GTK_COLOR_BUTTON(config_subtitle_color), &sub_color);
    }
    gtk_color_button_set_title(GTK_COLOR_BUTTON(config_subtitle_color),
                               _("Subtitle Color Selection"));
    gtk_widget_set_sensitive(config_subtitle_color, !disable_ass);
    gtk_table_attach(GTK_TABLE(conf_table), config_subtitle_color, 1, 2, i, i + 1, GTK_SHRINK,
                     GTK_SHRINK, 0, 0);
    i++;

    config_subtitle_outline = gtk_check_button_new_with_label(_("Outline Subtitle Font"));
    gtk_widget_set_sensitive(config_subtitle_outline, disable_embeddedfonts);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_subtitle_outline), subtitle_outline);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_subtitle_outline, 0, 2, i, i + 1);
    i++;

    config_subtitle_shadow = gtk_check_button_new_with_label(_("Shadow Subtitle Font"));
    gtk_widget_set_sensitive(config_subtitle_shadow, disable_embeddedfonts);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_subtitle_shadow), subtitle_shadow);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_subtitle_shadow, 0, 2, i, i + 1);
    i++;

    conf_label = gtk_label_new(_("Subtitle Font Scaling:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    config_subtitle_scale = gtk_spin_button_new_with_range(0.25, 10, 0.05);
    gtk_widget_set_size_request(config_subtitle_scale, -1, -1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_subtitle_scale), subtitle_scale);
    gtk_table_attach(GTK_TABLE(conf_table), config_subtitle_scale, 1, 2, i, i + 1, GTK_SHRINK,
                     GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Subtitle File Encoding:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_widget_set_size_request(GTK_WIDGET(config_subtitle_codepage), 200, -1);

    gtk_table_attach(GTK_TABLE(conf_table), config_subtitle_codepage, 1, 2, i, i + 1, GTK_SHRINK,
                     GTK_SHRINK, 0, 0);
    i++;

    config_show_subtitles = gtk_check_button_new_with_label(_("Show Subtitles by Default"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_show_subtitles), showsubtitles);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_show_subtitles, 0, 2, i, i + 1);
    i++;

    // Page 5
    conf_table = gtk_table_new(20, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(conf_page5), conf_table);
    i = 0;
    conf_label = gtk_label_new(_("<span weight=\"bold\">Application Preferences</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 2, i, i + 1);
    i++;

    config_playlist_visible = gtk_check_button_new_with_label(_("Start with playlist visible"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_playlist_visible), playlist_visible);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_playlist_visible, 0, 2, i, i + 1);
    i++;

    config_details_visible = gtk_check_button_new_with_label(_("Start with details visible"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_details_visible), details_visible);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_details_visible, 0, 2, i, i + 1);
    i++;

#ifdef NOTIFY_ENABLED
    config_show_notification = gtk_check_button_new_with_label(_("Show notification popup"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_show_notification), show_notification);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_show_notification, 0, 2, i, i + 1);
    i++;
#endif

#ifdef GTK2_12_ENABLED
    config_show_status_icon = gtk_check_button_new_with_label(_("Show status icon"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_show_status_icon), show_status_icon);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_show_status_icon, 0, 2, i, i + 1);
    i++;
#endif

    config_vertical_layout =
        gtk_check_button_new_with_label(_
                                        ("Place playlist below media (requires application restart)"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_vertical_layout), vertical_layout);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_vertical_layout, 0, 2, i, i + 1);
    i++;

    config_single_instance =
        gtk_check_button_new_with_label(_("Only allow one instance of Gnome MPlayer"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_single_instance), single_instance);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_single_instance, 0, 2, i, i + 1);
    g_signal_connect(G_OBJECT(config_single_instance), "toggled",
                     G_CALLBACK(config_single_instance_callback), NULL);
    i++;

    conf_label = gtk_label_new("");
    gtk_label_set_width_chars(GTK_LABEL(conf_label), 3);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    config_replace_and_play =
        gtk_check_button_new_with_label(_
                                        ("When opening in single instance mode, replace existing file"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_replace_and_play), replace_and_play);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_replace_and_play, 1, 2, i, i + 1);
    gtk_widget_set_sensitive(config_replace_and_play,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                          (config_single_instance)));
    i++;

    config_remember_loc = gtk_check_button_new_with_label(_("Remember Window Location and Size"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_remember_loc), remember_loc);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_remember_loc, 0, 2, i, i + 1);
    i++;

    config_keep_on_top = gtk_check_button_new_with_label(_("Keep window above other windows"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_keep_on_top), keep_on_top);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_keep_on_top, 0, 2, i, i + 1);
    i++;

    config_pause_on_click = gtk_check_button_new_with_label(_("Pause playback on mouse click"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_pause_on_click), !disable_pause_on_click);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_pause_on_click, 0, 2, i, i + 1);
    i++;

    conf_label = gtk_label_new(_("Tracker Thumb Position:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    config_thumb_position = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(config_thumb_position), _("Hidden"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(config_thumb_position), _("Bottom"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(config_thumb_position), _("Top"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(config_thumb_position), _("Top and Bottom"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(config_thumb_position), thumb_position);

    gtk_widget_set_size_request(GTK_WIDGET(config_thumb_position), 200, -1);

    gtk_table_attach(GTK_TABLE(conf_table), config_thumb_position, 1, 2, i, i + 1, GTK_SHRINK,
                     GTK_SHRINK, 0, 0);
    i++;

    config_verbose = gtk_check_button_new_with_label(_("Verbose Debug Enabled"));
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_verbose,
                         _
                         ("When this option is set, extra debug information is sent to the terminal or into ~/.xsession-errors"),
                         NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_verbose), verbose);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_verbose, 0, 2, i, i + 1);
    i++;

    // Page 6
    conf_table = gtk_table_new(20, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(conf_page6), conf_table);
    i = 0;
    conf_label = gtk_label_new(_("<span weight=\"bold\">Advanced Settings for MPlayer</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 2, i, i + 1);
    i++;

    config_softvol = gtk_check_button_new_with_label(_("Mplayer Software Volume Control Enabled"));
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_softvol,
                         _
                         ("Set this option if changing the volume in Gnome MPlayer changes the master volume"),
                         NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_softvol), softvol);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_softvol, 0, 2, i, i + 1);
    i++;

    config_deinterlace = gtk_check_button_new_with_mnemonic(_("De_interlace Video"));
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_deinterlace, _("Set this option if video looks striped"),
                         NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_deinterlace), !disable_deinterlace);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_deinterlace, 0, 2, i, i + 1);
    i++;

    config_framedrop = gtk_check_button_new_with_mnemonic(_("_Drop frames"));
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_framedrop,
                         _("Set this option if video is well behind the audio"), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_framedrop), !disable_framedrop);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_framedrop, 0, 2, i, i + 1);
    i++;

    config_forcecache =
        gtk_check_button_new_with_label(_("Force the use of cache setting on streaming media"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_forcecache), forcecache);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_forcecache, 0, 2, i, i + 1);
    i++;

    conf_label = gtk_label_new(_("MPlayer Executable:"));
    config_mplayer_bin = gtk_entry_new();
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_mplayer_bin,
                         _
                         ("Use this option to specify a mplayer application that is not in the path"),
                         NULL);
    gtk_entry_set_text(GTK_ENTRY(config_mplayer_bin), ((mplayer_bin) ? mplayer_bin : ""));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_entry_set_width_chars(GTK_ENTRY(config_mplayer_bin), 40);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    i++;
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_mplayer_bin, 0, 1, i, i + 1);
    i++;

    conf_label = gtk_label_new(_("Extra Options to MPlayer:"));
    config_extraopts = gtk_entry_new();
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_extraopts,
                         _("Add any extra mplayer options here (filters etc)"), NULL);
    gtk_entry_set_text(GTK_ENTRY(config_extraopts), ((extraopts) ? extraopts : ""));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_entry_set_width_chars(GTK_ENTRY(config_extraopts), 40);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    i++;
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_extraopts, 0, 1, i, i + 1);
    i++;

    gtk_container_add(GTK_CONTAINER(conf_hbutton_box), conf_cancel);
    gtk_container_add(GTK_CONTAINER(conf_vbox), conf_hbutton_box);

    gtk_widget_show_all(config_window);
    gtk_window_set_transient_for(GTK_WINDOW(config_window), GTK_WINDOW(window));
    gtk_window_set_keep_above(GTK_WINDOW(config_window), keep_on_top);
    gtk_window_present(GTK_WINDOW(config_window));

}

gboolean progress_callback(GtkWidget * widget, GdkEventButton * event, void *data)
{
    gchar *cmd;
    gdouble percent;
    gint width;
    gint height;
    GdkEventButton *event_button;

    if (event->type == GDK_BUTTON_PRESS) {
        event_button = (GdkEventButton *) event;
        if (event_button->button == 3) {
            gtk_menu_popup(popup_menu, NULL, NULL, NULL, NULL, event_button->button,
                           event_button->time);
            return TRUE;
        } else {
            gdk_drawable_get_size(GDK_DRAWABLE(widget->window), &width, &height);

            percent = (gdouble) event->x / (gdouble) width;

            if (idledata->cachepercent > 0.0 && percent > idledata->cachepercent) {
                percent = idledata->cachepercent - 0.10;
            }

            if (!idledata->streaming) {
                if (!autopause) {
                    if (state != STOPPED) {
                        cmd = g_strdup_printf("seek %i 1\n", (gint) (percent * 100));
                        send_command(cmd, TRUE);
                        if (state == PAUSED) {
                            send_command("mute 1\nseek 0 0\npause\n", FALSE);
                            send_command("mute 0\n", TRUE);
                            idledata->position = idledata->length * percent;
                            gmtk_media_tracker_set_percentage(tracker, percent);
                        }
                        g_free(cmd);
                        //state = PLAYING;
                    }
                }
            }

        }
        mouse_down_in_progress = TRUE;

    } else {
        mouse_down_in_progress = FALSE;
    }

    return TRUE;
}

gboolean progress_leave_callback(GtkWidget * widget, GdkEventCrossing * event, gpointer data)
{
    mouse_down_in_progress = FALSE;
    return TRUE;
}

gboolean progress_motion_callback(GtkWidget * widget, GdkEventMotion * event, gpointer data)
{
    gchar *cmd;
    gchar *tip;
    gchar *time;
    gdouble percent;
    gint width;
    gint height;
    static gdouble last_percent;

    if (mouse_down_in_progress) {

        gdk_drawable_get_size(GDK_DRAWABLE(widget->window), &width, &height);

        percent = (gdouble) event->x / (gdouble) width;

        if (idledata->cachepercent > 0.0 && percent > idledata->cachepercent) {
            percent = idledata->cachepercent - 0.10;
        }

        if (!idledata->streaming) {
            if (!autopause) {
                if (state != STOPPED && (fabs(last_percent - percent) > 0.05)) {
                    if (idledata->mute == 0) {
                        cmd =
                            g_strdup_printf("mute 1\nseek %i 1\nmute 0\n", (gint) (percent * 100));
                    } else {
                        cmd = g_strdup_printf("seek %f 2\n", percent * idledata->length);
                    }
                    time = seconds_to_string(percent * idledata->length);
                    tip = g_strdup_printf(_("Seeking to %s"), time);
                    gtk_tooltips_set_tip(progress_tip, GTK_WIDGET(tracker), tip, NULL);
                    g_free(time);
                    g_free(tip);
                    send_command(cmd, TRUE);
                    g_free(cmd);
                    if (state == PAUSED) {
                        send_command("mute 1\nseek 0 0\npause\n", FALSE);
                        send_command("mute 0\n", TRUE);
                        idledata->position = idledata->length * percent;
                        gmtk_media_tracker_set_percentage(tracker, percent);
                    }
                    //state = PLAYING;
                    last_percent = percent;
                }
            }
        }

    } else {
        gdk_drawable_get_size(GDK_DRAWABLE(widget->window), &width, &height);
        percent = (gdouble) event->x / (gdouble) width;
        time = seconds_to_string(percent * idledata->length);
        tip = g_strdup_printf("%s", time);
        gtk_tooltips_set_tip(progress_tip, GTK_WIDGET(tracker), tip, NULL);
        g_free(time);
        g_free(tip);
    }
    return TRUE;
}

gboolean load_href_callback(GtkWidget * widget, GdkEventExpose * event, gchar * hrefid)
{
    GdkEventButton *event_button;

    if (event->type == GDK_BUTTON_PRESS) {
        event_button = (GdkEventButton *) event;

        if (event_button->button == 3) {
            gtk_menu_popup(popup_menu, NULL, NULL, NULL,
                           NULL, event_button->button, event_button->time);
            return TRUE;
        }

        if (event_button->button != 3) {
            dbus_open_by_hrefid(hrefid);
            // do this in the plugin when we should
            // gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), TRUE);
            return TRUE;
        }
    }

    return FALSE;

}

void make_button(gchar * src, gchar * hrefid)
{

    GError *error;
    gchar *dirname = NULL;
    gchar *filename = NULL;
    gchar *basepath = NULL;
    gint exit_status;
    gchar *out = NULL;
    gchar *err = NULL;
    gchar *av[255];
    gint ac = 0;

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), FALSE);

    error = NULL;
    // only try if src ne NULL
    if (src != NULL) {
        pb_button = gdk_pixbuf_new_from_file(src, &error);
    } else {
        return;
    }

    // if we can't directly load the file into a pixbuf it might be a media file
    if (error != NULL) {
        g_error_free(error);
        error = NULL;

        basepath = g_strdup_printf("%s/gnome-mplayer/plugin", g_get_user_cache_dir());
        dirname = gm_tempname(basepath, "gnome-mplayerXXXXXX");
        filename = g_strdup_printf("%s/00000001.jpg", dirname);
        g_free(basepath);

        // run mplayer and try to get the first frame and convert it to a jpeg
        av[ac++] = g_strdup_printf("mplayer");
        av[ac++] = g_strdup_printf("-vo");
        av[ac++] = g_strdup_printf("jpeg:outdir=%s", dirname);
        av[ac++] = g_strdup_printf("-ao");
        av[ac++] = g_strdup_printf("null");
        av[ac++] = g_strdup_printf("-x");
        av[ac++] = g_strdup_printf("%i", window_x);
        av[ac++] = g_strdup_printf("-y");
        av[ac++] = g_strdup_printf("%i", window_y);
        av[ac++] = g_strdup_printf("-frames");
        av[ac++] = g_strdup_printf("1");
        av[ac++] = g_strdup_printf("%s", src);
        av[ac] = NULL;

        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err,
                     &exit_status, &error);
        if (error != NULL) {
            printf("Error when running: %s\n", error->message);
            g_error_free(error);
            error = NULL;
        }

        if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
            pb_button = gdk_pixbuf_new_from_file(filename, &error);
            if (error != NULL) {
                g_error_free(error);
                error = NULL;
            }
        }

        if (filename != NULL) {
            if (g_file_test(filename, G_FILE_TEST_EXISTS))
                g_remove(filename);
            g_free(filename);
        }

        if (dirname != NULL) {
            g_remove(dirname);
            g_free(dirname);
        }

        if (err != NULL)
            g_free(err);

        if (out != NULL)
            g_free(out);
    }

    if (pb_button != NULL && GDK_IS_PIXBUF(pb_button)) {

        button_event_box = gtk_event_box_new();
        image_button = gtk_image_new_from_pixbuf(pb_button);
        gtk_container_add(GTK_CONTAINER(button_event_box), image_button);
        gtk_box_pack_start(GTK_BOX(vbox), button_event_box, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(button_event_box), "button_press_event",
                         G_CALLBACK(load_href_callback), hrefid);
        gtk_widget_show_all(button_event_box);

    } else {
        if (verbose)
            printf("unable to make button from media, using default\n");
        button_event_box = gtk_event_box_new();
        image_button = gtk_image_new_from_pixbuf(pb_icon);
        gtk_container_add(GTK_CONTAINER(button_event_box), image_button);
        gtk_box_pack_start(GTK_BOX(vbox), button_event_box, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(button_event_box), "button_press_event",
                         G_CALLBACK(load_href_callback), hrefid);
        gtk_widget_show_all(button_event_box);
    }


}

void setup_accelerators()
{
    if (gtk_accel_group_query(accel_group, GDK_c, 0, NULL) != NULL) {
        // printf("flushing accelerators\n");
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_edit_config),
                                      accel_group, GDK_p, GDK_CONTROL_MASK);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_edit_take_screenshot),
                                      accel_group, GDK_t, GDK_CONTROL_MASK);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_playlist), accel_group, GDK_F9, 0);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_file_open_location),
                                      accel_group, GDK_l, GDK_CONTROL_MASK);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_info), accel_group, GDK_i, 0);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_subtitles), accel_group, GDK_v, 0);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_details),
                                      accel_group, GDK_d, GDK_CONTROL_MASK);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_meter),
                                      accel_group, GDK_m, GDK_CONTROL_MASK);

        if (!disable_fullscreen) {
            gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_fullscreen), accel_group, GDK_f, 0);

            gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_fullscreen),
                                          accel_group, GDK_f, GDK_CONTROL_MASK);

        }
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_onetoone),
                                      accel_group, GDK_1, GDK_CONTROL_MASK);

        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_twotoone),
                                      accel_group, GDK_2, GDK_CONTROL_MASK);

        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_showcontrols), accel_group, GDK_c, 0);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_controls), accel_group, GDK_c, 0);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_angle), accel_group, GDK_a,
                                      GDK_CONTROL_MASK);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_aspect), accel_group, GDK_a, 0);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_smaller_subtitle), accel_group,
                                      GDK_r, GDK_SHIFT_MASK);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_larger_subtitle), accel_group, GDK_t,
                                      GDK_SHIFT_MASK);

    }


    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_edit_config), "activate",
                               accel_group, GDK_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_edit_take_screenshot), "activate",
                               accel_group, GDK_t, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_playlist), "activate",
                               accel_group, GDK_F9, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_file_open_location), "activate",
                               accel_group, GDK_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_info), "activate",
                               accel_group, GDK_i, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_subtitles), "activate",
                               accel_group, GDK_v, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_details), "activate",
                               accel_group, GDK_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_meter), "activate",
                               accel_group, GDK_m, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    if (!disable_fullscreen) {
        gtk_widget_add_accelerator(GTK_WIDGET(menuitem_fullscreen), "activate",
                                   accel_group, GDK_f, 0, GTK_ACCEL_VISIBLE);

        gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_fullscreen), "activate",
                                   accel_group, GDK_f, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    }

    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_aspect), "activate",
                               accel_group, GDK_a, 0, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_smaller_subtitle), "activate",
                               accel_group, GDK_r, GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_larger_subtitle), "activate",
                               accel_group, GDK_t, GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_onetoone), "activate",
                               accel_group, GDK_1, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_twotoone), "activate",
                               accel_group, GDK_2, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_showcontrols), "activate",
                               accel_group, GDK_c, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_controls), "activate",
                               accel_group, GDK_c, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_angle), "activate",
                               accel_group, GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

}

GtkWidget *create_window(gint windowid)
{
    GError *error = NULL;
    GtkIconTheme *icon_theme;
    GtkTargetEntry target_entry[3];
    gint i = 0;

#ifdef GTK2_12_ENABLED
    GtkRecentFilter *recent_filter;
    GtkAdjustment *adj;
#endif

    in_button = FALSE;
    last_movement_time = -1;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), _("GNOME MPlayer"));

    if (windowid > 0 && embedding_disabled == FALSE) {
        gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
        GTK_WIDGET_SET_FLAGS(window, GTK_CAN_FOCUS);
    }
    gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, TRUE);

    gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(window, GDK_BUTTON_RELEASE_MASK);
    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    gtk_widget_add_events(window, GDK_KEY_RELEASE_MASK);
    gtk_widget_add_events(window, GDK_ENTER_NOTIFY_MASK);
    gtk_widget_add_events(window, GDK_LEAVE_NOTIFY_MASK);
    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    gtk_widget_add_events(window, GDK_VISIBILITY_NOTIFY_MASK);
    gtk_widget_add_events(window, GDK_STRUCTURE_MASK);
    gtk_widget_add_events(window, GDK_POINTER_MOTION_MASK);

    delete_signal_id =
        g_signal_connect(GTK_OBJECT(window), "delete_event", G_CALLBACK(delete_callback), NULL);
    g_signal_connect(GTK_OBJECT(window), "motion_notify_event", G_CALLBACK(motion_notify_callback),
                     NULL);

    accel_group = gtk_accel_group_new();

    // right click menu
    popup_menu = GTK_MENU(gtk_menu_new());
    menubar = gtk_menu_bar_new();
    menuitem_open = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, accel_group));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_open));
    gtk_widget_show(GTK_WIDGET(menuitem_open));
    menuitem_sep3 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_sep3));
    gtk_widget_show(GTK_WIDGET(menuitem_sep3));
    menuitem_play = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_play));
    gtk_widget_show(GTK_WIDGET(menuitem_play));
    menuitem_stop = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_STOP, NULL));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_stop));
    gtk_widget_show(GTK_WIDGET(menuitem_stop));
    menuitem_prev =
        GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PREVIOUS, NULL));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_prev));
    menuitem_next = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_NEXT, NULL));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_next));
    menuitem_sep1 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_sep1));
    gtk_widget_show(GTK_WIDGET(menuitem_sep1));
    menuitem_showcontrols =
        GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("S_how Controls")));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_showcontrols));
    gtk_widget_show(GTK_WIDGET(menuitem_showcontrols));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), TRUE);
    menuitem_fullscreen = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Full Screen")));
    if (!disable_fullscreen) {
        gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_fullscreen));
        gtk_widget_show(GTK_WIDGET(menuitem_fullscreen));
    }
    menuitem_copyurl = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Copy Location")));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_copyurl));
    gtk_widget_show(GTK_WIDGET(menuitem_copyurl));
    menuitem_sep2 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_sep2));
    gtk_widget_show(GTK_WIDGET(menuitem_sep2));
    menuitem_config =
        GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_config));
    gtk_widget_show(GTK_WIDGET(menuitem_config));

    menuitem_sep4 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_sep4));
    menuitem_save = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE, accel_group));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_save));
    // we only want to show the save option when under control of gecko-mediaplayer
    if (control_id != 0) {
        gtk_widget_show(GTK_WIDGET(menuitem_sep4));
        gtk_widget_show(GTK_WIDGET(menuitem_save));
    }

    menuitem_sep3 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_sep3));
    gtk_widget_show(GTK_WIDGET(menuitem_sep3));
    menuitem_quit = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_quit));
    gtk_widget_show(GTK_WIDGET(menuitem_quit));


    g_signal_connect(GTK_OBJECT(menuitem_open), "activate",
                     G_CALLBACK(menuitem_open_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_play), "activate",
                     G_CALLBACK(menuitem_pause_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_stop), "activate",
                     G_CALLBACK(menuitem_stop_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_prev), "activate",
                     G_CALLBACK(menuitem_prev_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_next), "activate",
                     G_CALLBACK(menuitem_next_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_showcontrols), "toggled",
                     G_CALLBACK(menuitem_showcontrols_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_fullscreen), "toggled",
                     G_CALLBACK(menuitem_fs_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_copyurl), "activate",
                     G_CALLBACK(menuitem_copyurl_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_config), "activate",
                     G_CALLBACK(menuitem_config_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_save), "activate",
                     G_CALLBACK(menuitem_save_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_quit), "activate",
                     G_CALLBACK(menuitem_quit_callback), NULL);


    g_signal_connect_swapped(G_OBJECT(window),
                             "button_press_event",
                             G_CALLBACK(popup_handler), GTK_OBJECT(popup_menu));
    g_signal_connect_swapped(G_OBJECT(window),
                             "button_release_event",
                             G_CALLBACK(popup_handler), GTK_OBJECT(popup_menu));
    g_signal_connect_swapped(G_OBJECT(window),
                             "scroll_event",
                             G_CALLBACK(drawing_area_scroll_event_callback),
                             GTK_OBJECT(drawing_area));
    g_signal_connect_swapped(G_OBJECT(window), "enter_notify_event",
                             G_CALLBACK(notification_handler), NULL);
    g_signal_connect_swapped(G_OBJECT(window), "leave_notify_event",
                             G_CALLBACK(notification_handler), NULL);


    // File Menu
    menuitem_file = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_File")));
    menu_file = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_file));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), GTK_WIDGET(menuitem_file));
    gtk_menu_item_set_submenu(menuitem_file, GTK_WIDGET(menu_file));
    menuitem_file_open =
        GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, accel_group));
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_open));
    menuitem_file_open_location =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _Location")));
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_open_location));

    menuitem_file_disc = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Disc")));
    menu_file_disc = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_file_disc));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_file), GTK_WIDGET(menuitem_file_disc));
    gtk_menu_item_set_submenu(menuitem_file_disc, GTK_WIDGET(menu_file_disc));

    menuitem_file_open_acd =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _Audio CD")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_acd));
    menuitem_file_open_sep1 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_sep1));

    menuitem_file_open_dvd = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _DVD")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_dvd));
    menuitem_file_open_dvdnav =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open DVD with _Menus")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_dvdnav));
    menuitem_file_open_dvd_folder =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open DVD from _Folder")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_dvd_folder));
    menuitem_file_open_dvdnav_folder =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open DVD from Folder with M_enus")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_dvdnav_folder));
    menuitem_file_open_dvd_iso =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open DVD from _ISO")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_dvd_iso));
    menuitem_file_open_dvdnav_iso =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open DVD from ISO with Me_nus")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_dvdnav_iso));

    menuitem_file_open_sep2 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_sep2));
    menuitem_file_open_vcd = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _VCD")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_vcd));

    menuitem_file_tv = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_TV")));
    menu_file_tv = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_file_tv));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_file), GTK_WIDGET(menuitem_file_tv));
    gtk_menu_item_set_submenu(menuitem_file_tv, GTK_WIDGET(menu_file_tv));

    menuitem_file_open_atv =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _Analog TV")));
    gtk_menu_append(menu_file_tv, GTK_WIDGET(menuitem_file_open_atv));
    menuitem_file_open_dtv =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _Digital TV")));
    gtk_menu_append(menu_file_tv, GTK_WIDGET(menuitem_file_open_dtv));
#ifdef HAVE_GPOD
    menuitem_file_open_ipod =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _iPod™")));
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_open_ipod));
#endif
#ifdef GTK2_12_ENABLED
#ifdef GIO_ENABLED
    recent_manager = gtk_recent_manager_get_default();
    menuitem_file_recent = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("Open _Recent")));
    // g_signal_connect(recent_manager, "changed", G_CALLBACK(recent_manager_changed_callback), NULL);
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_recent));
    menuitem_file_recent_items = gtk_recent_chooser_menu_new();
    recent_filter = gtk_recent_filter_new();
    gtk_recent_filter_add_application(recent_filter, g_get_application_name());
    gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(menuitem_file_recent_items), recent_filter);
    gtk_recent_chooser_set_show_tips(GTK_RECENT_CHOOSER(menuitem_file_recent_items), TRUE);
    gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(menuitem_file_recent_items),
                                     GTK_RECENT_SORT_MRU);
    gtk_menu_item_set_submenu(menuitem_file_recent, menuitem_file_recent_items);
    gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(menuitem_file_recent_items), FALSE);
#endif
#endif

    menuitem_file_sep2 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_sep2));

    menuitem_file_quit =
        GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group));
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_quit));

    g_signal_connect(GTK_OBJECT(menuitem_file_open), "activate",
                     G_CALLBACK(menuitem_open_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_location), "activate",
                     G_CALLBACK(menuitem_open_location_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_dvd), "activate",
                     G_CALLBACK(menuitem_open_dvd_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_dvdnav), "activate",
                     G_CALLBACK(menuitem_open_dvdnav_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_dvd_folder), "activate",
                     G_CALLBACK(menuitem_open_dvd_folder_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_dvdnav_folder), "activate",
                     G_CALLBACK(menuitem_open_dvdnav_folder_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_dvd_iso), "activate",
                     G_CALLBACK(menuitem_open_dvd_iso_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_dvdnav_iso), "activate",
                     G_CALLBACK(menuitem_open_dvdnav_iso_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_acd), "activate",
                     G_CALLBACK(menuitem_open_acd_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_vcd), "activate",
                     G_CALLBACK(menuitem_open_vcd_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_atv), "activate",
                     G_CALLBACK(menuitem_open_atv_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_dtv), "activate",
                     G_CALLBACK(menuitem_open_dtv_callback), NULL);
#ifdef HAVE_GPOD
    g_signal_connect(GTK_OBJECT(menuitem_file_open_ipod), "activate",
                     G_CALLBACK(menuitem_open_ipod_callback), NULL);
#endif
#ifdef GTK2_12_ENABLED
    g_signal_connect(GTK_OBJECT(menuitem_file_recent_items), "item-activated",
                     G_CALLBACK(menuitem_open_recent_callback), NULL);
#endif
    g_signal_connect(GTK_OBJECT(menuitem_file_quit), "activate",
                     G_CALLBACK(menuitem_quit_callback), NULL);
    // Edit Menu
    menuitem_edit = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Edit")));
    menu_edit = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_edit));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), GTK_WIDGET(menuitem_edit));
    gtk_menu_item_set_submenu(menuitem_edit, GTK_WIDGET(menu_edit));

    menuitem_edit_random =
        GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Shuffle Playlist")));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_edit_random), random_order);
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_random));

    menuitem_edit_loop = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Loop Playlist")));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_edit_loop), loop);
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_loop));

    menuitem_edit_switch_audio =
        GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("S_witch Audio Track")));
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_switch_audio));

    menuitem_edit_select_audio_lang =
        GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("Select _Audio Language")));
    menu_edit_audio_langs = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_edit_select_audio_lang));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_edit), GTK_WIDGET(menuitem_edit_select_audio_lang));
    gtk_menu_item_set_submenu(menuitem_edit_select_audio_lang, GTK_WIDGET(menu_edit_audio_langs));

    menuitem_edit_set_subtitle = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("Set Sub_title")));
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_set_subtitle));

    menuitem_edit_select_sub_lang =
        GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("S_elect Subtitle Language")));
    menu_edit_sub_langs = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_edit_select_sub_lang));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_edit), GTK_WIDGET(menuitem_edit_select_sub_lang));
    gtk_menu_item_set_submenu(menuitem_edit_select_sub_lang, GTK_WIDGET(menu_edit_sub_langs));

    menuitem_edit_take_screenshot =
        GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Take Screenshot")));
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_take_screenshot));
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, GTK_WIDGET(menuitem_edit_take_screenshot),
                         _("Files named ’shotNNNN.png’ will be saved in the working directory"),
                         NULL);

    menuitem_edit_sep1 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_sep1));

    menuitem_edit_config =
        GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL));
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_config));
    g_signal_connect(GTK_OBJECT(menuitem_edit_random), "activate",
                     G_CALLBACK(menuitem_edit_random_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_edit_loop), "activate",
                     G_CALLBACK(menuitem_edit_loop_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_edit_switch_audio), "activate",
                     G_CALLBACK(menuitem_edit_switch_audio_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_edit_set_subtitle), "activate",
                     G_CALLBACK(menuitem_edit_set_subtitle_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_edit_take_screenshot), "activate",
                     G_CALLBACK(menuitem_edit_take_screenshot_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_edit_config), "activate",
                     G_CALLBACK(menuitem_config_callback), NULL);



    // View Menu
    menuitem_view = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_View")));
    menu_view = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_view));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), GTK_WIDGET(menuitem_view));
    gtk_menu_item_set_submenu(menuitem_view, GTK_WIDGET(menu_view));
    menuitem_view_playlist = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Playlist")));
    // gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist), playlist_visible);
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_playlist));
    menuitem_view_info = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("Media _Info")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_info));
    menuitem_view_details = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("D_etails")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_details));
    menuitem_view_meter = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("Audio _Meter")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_meter));
    menuitem_view_sep0 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_sep0));

    menuitem_view_fullscreen =
        GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Full Screen")));
    menuitem_view_sep1 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    if (!disable_fullscreen) {
        gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_fullscreen));
        gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_sep1));
    }
    menuitem_view_onetoone =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Normal (1:1)")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_onetoone));
    menuitem_view_twotoone =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Double Size (2:1)")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_twotoone));
    menuitem_view_onetotwo =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Half Size (1:2)")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_onetotwo));

    menuitem_view_aspect = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Aspect")));
    menu_view_aspect = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_view_aspect));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_view), GTK_WIDGET(menuitem_view_aspect));
    gtk_menu_item_set_submenu(menuitem_view_aspect, GTK_WIDGET(menu_view_aspect));

    menuitem_view_aspect_default =
        GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("D_efault Aspect")));
    gtk_menu_append(menu_view_aspect, GTK_WIDGET(menuitem_view_aspect_default));
    menuitem_view_aspect_four_three =
        GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_4:3 Aspect")));
    gtk_menu_append(menu_view_aspect, GTK_WIDGET(menuitem_view_aspect_four_three));
    menuitem_view_aspect_sixteen_nine =
        GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_16:9 Aspect")));
    gtk_menu_append(menu_view_aspect, GTK_WIDGET(menuitem_view_aspect_sixteen_nine));
    menuitem_view_aspect_sixteen_ten =
        GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("1_6:10 Aspect")));
    gtk_menu_append(menu_view_aspect, GTK_WIDGET(menuitem_view_aspect_sixteen_ten));
    menuitem_view_aspect_follow_window =
        GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Follow Window")));
    gtk_menu_append(menu_view_aspect, GTK_WIDGET(menuitem_view_aspect_follow_window));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), TRUE);

    menuitem_view_sep2 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    menuitem_view_sep5 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    menuitem_view_subtitles =
        GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("Show _Subtitles")));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_subtitles), showsubtitles);
    menuitem_view_smaller_subtitle =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Decrease Subtitle Size")));
    menuitem_view_larger_subtitle =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Increase Subtitle Size")));
    menuitem_view_angle = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Switch An_gle")));
    menuitem_view_controls = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Controls")));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_controls), TRUE);
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_sep2));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_subtitles));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_smaller_subtitle));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_larger_subtitle));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_sep5));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_angle));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_controls));
    menuitem_view_sep3 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_sep3));
    menuitem_view_advanced =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Video Picture Adjustments")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_advanced));

    g_signal_connect(GTK_OBJECT(menuitem_view_playlist), "toggled",
                     G_CALLBACK(menuitem_view_playlist_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_info), "activate",
                     G_CALLBACK(menuitem_view_info_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_details), "activate",
                     G_CALLBACK(menuitem_details_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_meter), "activate",
                     G_CALLBACK(menuitem_meter_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_fullscreen), "toggled",
                     G_CALLBACK(menuitem_fs_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_onetoone), "activate",
                     G_CALLBACK(menuitem_view_onetoone_callback), idledata);
    g_signal_connect(GTK_OBJECT(menuitem_view_twotoone), "activate",
                     G_CALLBACK(menuitem_view_twotoone_callback), idledata);
    g_signal_connect(GTK_OBJECT(menuitem_view_onetotwo), "activate",
                     G_CALLBACK(menuitem_view_onetotwo_callback), idledata);
    g_signal_connect(GTK_OBJECT(menuitem_view_aspect_default), "activate",
                     G_CALLBACK(menuitem_view_aspect_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_aspect_four_three), "activate",
                     G_CALLBACK(menuitem_view_aspect_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_aspect_sixteen_nine), "activate",
                     G_CALLBACK(menuitem_view_aspect_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_aspect_sixteen_ten), "activate",
                     G_CALLBACK(menuitem_view_aspect_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_aspect_follow_window), "activate",
                     G_CALLBACK(menuitem_view_aspect_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_subtitles), "toggled",
                     G_CALLBACK(menuitem_view_subtitles_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_smaller_subtitle), "activate",
                     G_CALLBACK(menuitem_view_smaller_subtitle_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_larger_subtitle), "activate",
                     G_CALLBACK(menuitem_view_larger_subtitle_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_angle), "activate",
                     G_CALLBACK(menuitem_view_angle_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_controls), "toggled",
                     G_CALLBACK(menuitem_showcontrols_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_advanced), "activate",
                     G_CALLBACK(menuitem_advanced_callback), idledata);

    // Help Menu
    menuitem_help = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Help")));
    menu_help = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_help));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), GTK_WIDGET(menuitem_help));
    gtk_menu_item_set_submenu(menuitem_help, GTK_WIDGET(menu_help));
    menuitem_help_about = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL));
    gtk_menu_append(menu_help, GTK_WIDGET(menuitem_help_about));
    g_signal_connect(GTK_OBJECT(menuitem_help_about), "activate",
                     G_CALLBACK(menuitem_about_callback), NULL);

    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
    setup_accelerators();
    g_signal_connect(GTK_OBJECT(window), "key_press_event", G_CALLBACK(window_key_callback), NULL);

    // Give the window the property to accept DnD
    target_entry[i].target = DRAG_NAME_0;
    target_entry[i].flags = 0;
    target_entry[i++].info = DRAG_INFO_0;
    target_entry[i].target = DRAG_NAME_1;
    target_entry[i].flags = 0;
    target_entry[i++].info = DRAG_INFO_1;
    target_entry[i].target = DRAG_NAME_2;
    target_entry[i].flags = 0;
    target_entry[i++].info = DRAG_INFO_2;

    gtk_drag_dest_set(window,
                      GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
                      GTK_DEST_DEFAULT_DROP, target_entry, i, GDK_ACTION_LINK);
    gtk_drag_dest_add_uri_targets(window);

    //Connect the signal for DnD
    g_signal_connect(GTK_OBJECT(window), "drag_data_received", GTK_SIGNAL_FUNC(drop_callback),
                     NULL);


    vbox = gtk_vbox_new(FALSE, 0);
    hbox = gtk_hbox_new(FALSE, 0);
    controls_box = gtk_vbox_new(FALSE, 0);
    fixed = gtk_fixed_new();
    drawing_area = gtk_socket_new();

    cover_art = gtk_image_new();
    media_label = gtk_label_new("");
    gtk_widget_set_size_request(media_label, 300, -1);
    media_hbox = gtk_hbox_new(FALSE, 10);
    details_vbox = gtk_vbox_new(FALSE, 10);
    gtk_misc_set_alignment(GTK_MISC(media_label), 0, 0);
    audio_meter = gmtk_audio_meter_new(METER_BARS);
    gmtk_audio_meter_set_max_division_width(GMTK_AUDIO_METER(audio_meter), 10);
    gtk_widget_set_size_request(audio_meter, -1, 100);
    g_timeout_add_full(G_PRIORITY_HIGH, 40, update_audio_meter, NULL, NULL);

    gtk_fixed_put(GTK_FIXED(fixed), drawing_area, 0, 0);
    gtk_box_pack_start(GTK_BOX(vbox), fixed, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(media_hbox), cover_art, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(media_hbox), media_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), media_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), details_vbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), audio_meter, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_box), hbox, FALSE, FALSE, 1);

    gtk_widget_add_events(drawing_area, GDK_POINTER_MOTION_MASK);

    g_signal_connect(GTK_OBJECT(drawing_area), "motion_notify_event",
                     G_CALLBACK(motion_notify_callback), NULL);
    gtk_widget_show(drawing_area);

    if (vertical_layout) {
        pane = gtk_vpaned_new();
    } else {
        pane = gtk_hpaned_new();
    }
    gtk_paned_pack1(GTK_PANED(pane), vbox, TRUE, TRUE);

    vbox_master = gtk_vbox_new(FALSE, 0);
    if (windowid == 0)
        gtk_box_pack_start(GTK_BOX(vbox_master), menubar, FALSE, FALSE, 0);
    gtk_widget_show(menubar);

    gtk_box_pack_start(GTK_BOX(vbox_master), pane, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox_master), controls_box, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), vbox_master);

    error = NULL;
    icon_theme = gtk_icon_theme_get_default();


    // ok if the theme has all the icons we need, use them, otherwise use the default GNOME ones

    if (gtk_icon_theme_has_icon(icon_theme, "media-playback-start")
        && gtk_icon_theme_has_icon(icon_theme, "media-playback-pause")
        && gtk_icon_theme_has_icon(icon_theme, "media-playback-stop")
        && gtk_icon_theme_has_icon(icon_theme, "media-seek-forward")
        && gtk_icon_theme_has_icon(icon_theme, "media-seek-backward")
        && gtk_icon_theme_has_icon(icon_theme, "media-skip-forward")
        && gtk_icon_theme_has_icon(icon_theme, "media-skip-backward")
        && gtk_icon_theme_has_icon(icon_theme, GTK_STOCK_INDEX)
        && gtk_icon_theme_has_icon(icon_theme, "view-fullscreen")) {

        pb_play =
            gtk_icon_theme_load_icon(icon_theme, "media-playback-start", button_size, 0, &error);
        pb_pause =
            gtk_icon_theme_load_icon(icon_theme, "media-playback-pause", button_size, 0, &error);
        pb_stop =
            gtk_icon_theme_load_icon(icon_theme, "media-playback-stop", button_size, 0, &error);
        pb_ff = gtk_icon_theme_load_icon(icon_theme, "media-seek-forward", button_size, 0, &error);
        pb_rew =
            gtk_icon_theme_load_icon(icon_theme, "media-seek-backward", button_size, 0, &error);
        pb_next =
            gtk_icon_theme_load_icon(icon_theme, "media-skip-forward", button_size, 0, &error);
        pb_prev =
            gtk_icon_theme_load_icon(icon_theme, "media-skip-backward", button_size, 0, &error);
        pb_menu = gtk_icon_theme_load_icon(icon_theme, GTK_STOCK_INDEX, button_size, 0, &error);
        pb_fs = gtk_icon_theme_load_icon(icon_theme, "view-fullscreen", button_size, 0, &error);

    } else if (gtk_icon_theme_has_icon(icon_theme, "stock_media-play")
               && gtk_icon_theme_has_icon(icon_theme, "stock_media-pause")
               && gtk_icon_theme_has_icon(icon_theme, "stock_media-stop")
               && gtk_icon_theme_has_icon(icon_theme, "stock_media-fwd")
               && gtk_icon_theme_has_icon(icon_theme, "stock_media-rew")
               && gtk_icon_theme_has_icon(icon_theme, "stock_media-prev")
               && gtk_icon_theme_has_icon(icon_theme, "stock_media-next")
               && gtk_icon_theme_has_icon(icon_theme, GTK_STOCK_INDEX)
               && gtk_icon_theme_has_icon(icon_theme, "view-fullscreen")) {

        pb_play = gtk_icon_theme_load_icon(icon_theme, "stock_media-play", button_size, 0, &error);
        pb_pause =
            gtk_icon_theme_load_icon(icon_theme, "stock_media-pause", button_size, 0, &error);
        pb_stop = gtk_icon_theme_load_icon(icon_theme, "stock_media-stop", button_size, 0, &error);
        pb_ff = gtk_icon_theme_load_icon(icon_theme, "stock_media-fwd", button_size, 0, &error);
        pb_rew = gtk_icon_theme_load_icon(icon_theme, "stock_media-rew", button_size, 0, &error);
        pb_next = gtk_icon_theme_load_icon(icon_theme, "stock_media-next", button_size, 0, &error);
        pb_prev = gtk_icon_theme_load_icon(icon_theme, "stock_media-prev", button_size, 0, &error);
        pb_menu = gtk_icon_theme_load_icon(icon_theme, GTK_STOCK_INDEX, button_size, 0, &error);
        pb_fs = gtk_icon_theme_load_icon(icon_theme, "view-fullscreen", button_size, 0, &error);

    } else {

        pb_play = gdk_pixbuf_new_from_xpm_data((const char **) media_playback_start_xpm);
        pb_pause = gdk_pixbuf_new_from_xpm_data((const char **) media_playback_pause_xpm);
        pb_stop = gdk_pixbuf_new_from_xpm_data((const char **) media_playback_stop_xpm);
        pb_ff = gdk_pixbuf_new_from_xpm_data((const char **) media_seek_forward_xpm);
        pb_rew = gdk_pixbuf_new_from_xpm_data((const char **) media_seek_backward_xpm);
        pb_next = gdk_pixbuf_new_from_xpm_data((const char **) media_skip_forward_xpm);
        pb_prev = gdk_pixbuf_new_from_xpm_data((const char **) media_skip_backward_xpm);
        pb_menu = gtk_icon_theme_load_icon(icon_theme, GTK_STOCK_INDEX, button_size, 0, &error);
        pb_fs = gdk_pixbuf_new_from_xpm_data((const char **) view_fullscreen_xpm);

    }

    image_play = gtk_image_new_from_pixbuf(pb_play);
    image_stop = gtk_image_new_from_pixbuf(pb_stop);
    image_pause = gtk_image_new_from_pixbuf(pb_pause);

    image_ff = gtk_image_new_from_pixbuf(pb_ff);
    image_rew = gtk_image_new_from_pixbuf(pb_rew);

    image_prev = gtk_image_new_from_pixbuf(pb_prev);
    image_next = gtk_image_new_from_pixbuf(pb_next);
    image_menu = gtk_image_new_from_pixbuf(pb_menu);

    image_fs = gtk_image_new_from_pixbuf(pb_fs);

    if (gtk_icon_theme_has_icon(icon_theme, "gnome-mplayer")) {
        pb_icon = gtk_icon_theme_load_icon(icon_theme, "gnome-mplayer", 64, 0, NULL);
        pb_logo = gtk_icon_theme_load_icon(icon_theme, "gnome-mplayer", 64, 0, NULL);
    } else {
        pb_icon = gdk_pixbuf_new_from_xpm_data((const char **) gnome_mplayer_xpm);
        pb_logo = gdk_pixbuf_new_from_xpm_data((const char **) gnome_mplayer_xpm);
    }
    gtk_window_set_icon(GTK_WINDOW(window), pb_icon);

#ifdef GTK2_12_ENABLED
    status_icon = gtk_status_icon_new_from_pixbuf(pb_icon);
    if (control_id != 0) {
        gtk_status_icon_set_visible(status_icon, FALSE);
    } else {
        gtk_status_icon_set_visible(status_icon, show_status_icon);
    }
    g_signal_connect(status_icon, "activate", G_CALLBACK(status_icon_callback), NULL);
    g_signal_connect(status_icon, "popup_menu", G_CALLBACK(status_icon_context_callback), NULL);
#endif

    menu_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(menu_event_box), image_menu);
    gtk_button_set_relief(GTK_BUTTON(menu_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, menu_event_box, _("Menu"), NULL);
    gtk_widget_set_events(menu_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(menu_event_box), "button_press_event", G_CALLBACK(menu_callback),
                     NULL);

    gtk_box_pack_start(GTK_BOX(hbox), menu_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_menu);
    gtk_widget_show(menu_event_box);

    prev_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(prev_event_box), image_prev);
    gtk_button_set_relief(GTK_BUTTON(prev_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, prev_event_box, _("Previous"), NULL);
    gtk_widget_set_events(prev_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(prev_event_box), "button_press_event", G_CALLBACK(prev_callback),
                     NULL);

    gtk_box_pack_start(GTK_BOX(hbox), prev_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_prev);
    gtk_widget_show(prev_event_box);

    rew_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(rew_event_box), image_rew);
    gtk_button_set_relief(GTK_BUTTON(rew_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, rew_event_box, _("Rewind"), NULL);
    gtk_widget_set_events(rew_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(rew_event_box), "button_press_event", G_CALLBACK(rew_callback), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), rew_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_rew);
    gtk_widget_show(rew_event_box);

    play_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(play_event_box), image_play);
    gtk_button_set_relief(GTK_BUTTON(play_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
    gtk_widget_set_events(play_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(play_event_box),
                     "button_press_event", G_CALLBACK(play_callback), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), play_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_play);
    gtk_widget_show(play_event_box);

    stop_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(stop_event_box), image_stop);
    gtk_button_set_relief(GTK_BUTTON(stop_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, stop_event_box, _("Stop"), NULL);
    gtk_widget_set_events(stop_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(stop_event_box),
                     "button_press_event", G_CALLBACK(stop_callback), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), stop_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_stop);
    gtk_widget_show(stop_event_box);

    ff_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(ff_event_box), image_ff);
    gtk_button_set_relief(GTK_BUTTON(ff_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, ff_event_box, _("Fast Forward"), NULL);
    gtk_widget_set_events(ff_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(ff_event_box), "button_press_event", G_CALLBACK(ff_callback), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), ff_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_ff);
    gtk_widget_show(ff_event_box);

    next_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(next_event_box), image_next);
    gtk_button_set_relief(GTK_BUTTON(next_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, next_event_box, _("Next"), NULL);
    gtk_widget_set_events(next_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(next_event_box), "button_press_event", G_CALLBACK(next_callback),
                     NULL);
    gtk_box_pack_start(GTK_BOX(hbox), next_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_next);
    gtk_widget_show(next_event_box);

    // progress bar
    tracker = GMTK_MEDIA_TRACKER(gmtk_media_tracker_new());
    gmtk_media_tracker_set_thumb_position(tracker, THUMB_HIDDEN);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(tracker), TRUE, TRUE, 2);
    g_signal_connect(G_OBJECT(tracker), "button_press_event", G_CALLBACK(progress_callback), NULL);
    g_signal_connect(G_OBJECT(tracker), "button_release_event", G_CALLBACK(progress_callback),
                     NULL);
    g_signal_connect(G_OBJECT(tracker), "motion_notify_event",
                     G_CALLBACK(progress_motion_callback), NULL);
    g_signal_connect(G_OBJECT(tracker), "leave_notify_event", G_CALLBACK(progress_leave_callback),
                     NULL);
    gtk_widget_show(GTK_WIDGET(tracker));
    progress_tip = gtk_tooltips_new();
    gtk_tooltips_set_tip(progress_tip, GTK_WIDGET(tracker), _("No Information"), NULL);

    // fullscreen button, pack from end for this button and the vol slider
    fs_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(fs_event_box), image_fs);
    gtk_button_set_relief(GTK_BUTTON(fs_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, fs_event_box, _("Full Screen"), NULL);
    gtk_widget_set_events(fs_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(fs_event_box), "button_press_event", G_CALLBACK(fs_callback), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), fs_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_fs);
    if (!disable_fullscreen)
        gtk_widget_show(fs_event_box);


    // volume control
    if ((window_y > window_x)
        && (rpcontrols != NULL && g_strcasecmp(rpcontrols, "volumeslider") == 0)) {
        vol_slider = gtk_vscale_new_with_range(0.0, 100.0, 1.0);
        gtk_widget_set_size_request(vol_slider, -1, window_y);
        gtk_range_set_inverted(GTK_RANGE(vol_slider), TRUE);
    } else {
#ifdef GTK2_12_ENABLED
        vol_slider = gtk_volume_button_new();
        adj = gtk_scale_button_get_adjustment(GTK_SCALE_BUTTON(vol_slider));
        adj->lower = 0.0;
        adj->upper = 100.0;
        adj->step_increment = 1.0;
        gtk_scale_button_set_adjustment(GTK_SCALE_BUTTON(vol_slider), adj);
        gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider), idledata->volume);
        if (large_buttons)
            gtk_object_set(GTK_OBJECT(vol_slider), "size", GTK_ICON_SIZE_BUTTON, NULL);
        else
            gtk_object_set(GTK_OBJECT(vol_slider), "size", GTK_ICON_SIZE_MENU, NULL);

        g_signal_connect(G_OBJECT(vol_slider), "value_changed", G_CALLBACK(vol_button_callback),
                         NULL);
        gtk_button_set_relief(GTK_BUTTON(vol_slider), GTK_RELIEF_NONE);
#else
        vol_slider = gtk_hscale_new_with_range(0.0, 100.0, 1.0);
        gtk_widget_set_size_request(vol_slider, 44, button_size);
        gtk_scale_set_draw_value(GTK_SCALE(vol_slider), FALSE);
        gtk_range_set_value(GTK_RANGE(vol_slider), idledata->volume);
        g_signal_connect(G_OBJECT(vol_slider), "value_changed", G_CALLBACK(vol_slider_callback),
                         NULL);
#endif
    }
    volume_tip = gtk_tooltips_new();
    gtk_tooltips_set_tip(volume_tip, vol_slider, _("Volume 100%"), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), vol_slider, FALSE, FALSE, 0);
    GTK_WIDGET_UNSET_FLAGS(vol_slider, GTK_CAN_FOCUS);
    gtk_widget_show(vol_slider);

    gtk_widget_realize(window);

    return window;
}

void show_window(gint windowid)
{

    gint i;
    gchar **visuals;

    if (windowid != 0 && embedding_disabled == FALSE) {
        while (gtk_events_pending())
            gtk_main_iteration();

        window_container = gdk_window_foreign_new(windowid);
        if (GTK_WIDGET_MAPPED(window))
            gtk_widget_unmap(window);

        gdk_window_reparent(window->window, window_container, 0, 0);
        gmtk_media_tracker_set_allow_expand(tracker, FALSE);
    } else {
        if (remember_loc) {
            gtk_window_move(GTK_WINDOW(window), loc_window_x, loc_window_y);
            gtk_window_resize(GTK_WINDOW(window), loc_window_width, loc_window_height);
        }
    }

    if (rpcontrols == NULL || (rpcontrols != NULL && g_strcasecmp(rpcontrols, "all") == 0)) {
        if (windowid != -1)
            gtk_widget_show_all(window);
        gtk_widget_hide(media_hbox);
        gtk_widget_hide(menu_event_box);
        gtk_widget_hide(audio_meter);
        if (disable_fullscreen)
            gtk_widget_hide(fs_event_box);

        if (windowid == 0 && control_id == 0)
            gtk_widget_hide(fixed);

        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), showcontrols);

        if (windowid > 0) {
            if (window_x < 250) {
                gtk_widget_hide(rew_event_box);
                gtk_widget_hide(ff_event_box);
                gtk_widget_hide(fs_event_box);
            }

            if (window_x < 170) {
                gtk_widget_hide(GTK_WIDGET(tracker));
            }
            if (window_x > 0 && window_y > 0) {
                gtk_window_resize(GTK_WINDOW(window), window_x, window_y);
            }
        }

    } else {

        if (windowid != -1)
            gtk_widget_show_all(window);

        gtk_widget_hide(fixed);
        gtk_widget_hide(menubar);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), FALSE);
        gtk_widget_hide(menu_event_box);
        gtk_widget_hide(audio_meter);

        gtk_widget_hide_all(controls_box);

        printf("showing the following controls = %s\n", rpcontrols);
        visuals = g_strsplit(rpcontrols, ",", 0);
        i = 0;
        while (visuals[i] != NULL) {
            if (g_strcasecmp(visuals[i], "statusbar") == 0
                || g_strcasecmp(visuals[i], "statusfield") == 0
                || g_strcasecmp(visuals[i], "positionfield") == 0
                || g_strcasecmp(visuals[i], "positionslider") == 0) {
                gtk_widget_show(GTK_WIDGET(tracker));
                gtk_widget_show(controls_box);
                gtk_widget_show(hbox);
                gtk_widget_hide(menu_event_box);
                gtk_widget_hide(fixed);
                control_instance = FALSE;
            }
            if (g_strcasecmp(visuals[i], "infovolumepanel") == 0) {
                gtk_widget_show(GTK_WIDGET(vol_slider));
                gtk_widget_show(controls_box);
                gtk_widget_show(hbox);
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), TRUE);
                control_instance = FALSE;
            }
            if (g_strcasecmp(visuals[i], "infopanel") == 0) {
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), TRUE);
                control_instance = FALSE;
            }

            if (g_strcasecmp(visuals[i], "volumeslider") == 0) {
                gtk_widget_show(GTK_WIDGET(vol_slider));
                gtk_widget_show(controls_box);
                gtk_widget_show(hbox);
            }
            if (g_strcasecmp(visuals[i], "playbutton") == 0) {
                gtk_widget_show_all(GTK_WIDGET(play_event_box));
                gtk_widget_show(controls_box);
                gtk_widget_show(hbox);
            }
            if (g_strcasecmp(visuals[i], "stopbutton") == 0) {
                gtk_widget_show_all(GTK_WIDGET(stop_event_box));
                gtk_widget_show(controls_box);
                gtk_widget_show(hbox);
            }
            if (g_strcasecmp(visuals[i], "pausebutton") == 0) {
                gtk_widget_show_all(GTK_WIDGET(play_event_box));
                gtk_widget_show(controls_box);
                gtk_widget_show(hbox);
            }
            if (g_strcasecmp(visuals[i], "ffctrl") == 0) {
                gtk_widget_show_all(GTK_WIDGET(ff_event_box));
                gtk_widget_show(controls_box);
                gtk_widget_show(hbox);
            }
            if (g_strcasecmp(visuals[i], "rwctrl") == 0) {
                gtk_widget_show_all(GTK_WIDGET(rew_event_box));
                gtk_widget_show(controls_box);
                gtk_widget_show(hbox);
            }

            if (g_strcasecmp(visuals[i], "imagewindow") == 0) {
                gtk_widget_show_all(GTK_WIDGET(fixed));
                control_instance = FALSE;
            }

            if (g_strcasecmp(visuals[i], "controlpanel") == 0) {
                gtk_widget_show(controls_box);
                gtk_widget_show_all(hbox);
                gtk_widget_hide(GTK_WIDGET(tracker));
                gtk_widget_hide(menu_event_box);
                gtk_widget_hide(fixed);
            }

            i++;
        }


    }
    g_signal_connect(G_OBJECT(fixed), "size_allocate", G_CALLBACK(allocate_fixed_callback), NULL);
    g_signal_connect(G_OBJECT(fixed), "expose_event", G_CALLBACK(expose_fixed_callback), NULL);

    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_fullscreen), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_details), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(fs_event_box), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_loop), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_subtitle), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_take_screenshot), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_select_audio_lang), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_select_sub_lang), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_info), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_fullscreen), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_onetoone), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_onetotwo), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_twotoone), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_twotoone), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect_default), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect_four_three), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect_sixteen_nine), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect_sixteen_ten), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect_follow_window), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_subtitles), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_smaller_subtitle), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_larger_subtitle), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_angle), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_details), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_advanced), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), FALSE);
    gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);
    gtk_widget_hide(prev_event_box);
    gtk_widget_hide(next_event_box);
    gtk_widget_hide(GTK_WIDGET(menuitem_prev));
    gtk_widget_hide(GTK_WIDGET(menuitem_next));
    gtk_widget_hide(media_hbox);
    gtk_widget_hide(audio_meter);

    gtk_widget_hide(GTK_WIDGET(menuitem_edit_switch_audio));
    gtk_window_set_keep_above(GTK_WINDOW(window), keep_on_top);
    update_status_icon();

}

gboolean update_audio_meter(gpointer data)
{
    gint i, j;
    gfloat f;
    gint max;
    gfloat freq;
    Export *export;
    gint bucketid;

    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_meter)))
        return TRUE;

    if (idledata->mapped_af_export == NULL)
        return TRUE;

    if (state != PLAYING)
        return TRUE;


    if (audio_meter != NULL && GTK_WIDGET_VISIBLE(audio_meter)) {

        if (data)
            g_array_free(data, TRUE);
        if (max_data)
            g_array_free(max_data, TRUE);
        data = g_array_new(FALSE, TRUE, sizeof(gfloat));
        max_data = g_array_new(FALSE, TRUE, sizeof(gfloat));

        for (i = 0; i < METER_BARS; i++) {
            buckets[i] = 0;
        }

        reading_af_export = TRUE;
        export = NULL;
        if (idledata->mapped_af_export != NULL)
            export = (Export *) g_mapped_file_get_contents(idledata->mapped_af_export);
        if (export != NULL) {
            for (i = 0; export != NULL && i < 512; i++) {
                freq = 0;
                for (j = 0; j < export->nch; j++) {
                    // scale SB16 data to 0 - 22000 range, believe this is Hz now
                    freq += (export->payload[j][i]) * 22000 / (32768 * export->nch);
                }
                // ignore values below 20, as this is unhearable and may skew data
                if (freq > 20) {
                    bucketid = (gint) (freq / (gfloat) (22000.0 / (gfloat) METER_BARS));
                    if (bucketid >= METER_BARS) {
                        printf("bucketid = %i freq = %f\n", bucketid, freq);
                        bucketid = METER_BARS - 1;
                    }
                    buckets[bucketid]++;
                }
            }
            // g_free(export);
        }
        reading_af_export = FALSE;

        max = 0;
        for (i = 0; i < METER_BARS; i++) {
            if (buckets[i] > max_buckets[i])
                max_buckets[i] = buckets[i];
            if (max_buckets[i] > max)
                max = max_buckets[i];
        }

        for (i = 0; i < METER_BARS; i++) {
            if (max == 0) {
                f = 0.0;
                g_array_append_val(data, f);
                g_array_append_val(max_data, f);
            } else {
                f = logf((gfloat) buckets[i]) / logf((gfloat) max);
                // f = ((gfloat) buckets[i]) / ((gfloat) max);
                g_array_append_val(data, f);
                f = logf((gfloat) max_buckets[i]) / logf((gfloat) max);
                // f = ((gfloat) max_buckets[i]) / ((gfloat) max);
                g_array_append_val(max_data, f);
            }
        }

        gmtk_audio_meter_set_data_full(GMTK_AUDIO_METER(audio_meter), data, max_data);

    }
    return TRUE;

}

gint get_height()
{
    gint total_height;

    total_height = idledata->height;
    total_height += menubar->allocation.height;
    if (showcontrols) {
        total_height += controls_box->allocation.height;
    }
    if (GTK_IS_WIDGET(media_hbox) && GTK_WIDGET_VISIBLE(media_hbox)) {
        total_height += media_hbox->allocation.height;
    }
    if (GTK_IS_WIDGET(details_table) && GTK_WIDGET_VISIBLE(details_table)) {
        total_height += details_vbox->allocation.height;
    }
    if (GTK_IS_WIDGET(audio_meter) && GTK_WIDGET_VISIBLE(audio_meter)) {
        total_height += audio_meter->allocation.height;
    }

    return total_height;
}

gint get_width()
{
    gint total_width;

    total_width = media_hbox->allocation.width;
    return total_width;
}
