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
/*
#include "../pixmaps/media-playback-pause.xpm"
#include "../pixmaps/media-playback-start.xpm"
#include "../pixmaps/media-playback-stop.xpm"
#include "../pixmaps/media-seek-backward.xpm"
#include "../pixmaps/media-seek-forward.xpm"
#include "../pixmaps/media-skip-backward.xpm"
#include "../pixmaps/media-skip-forward.xpm"
#include "../pixmaps/view-fullscreen.xpm"
*/
#include "langlist.h"
#ifdef NOTIFY_ENABLED
#include <libnotify/notify.h>
#include <libnotify/notification.h>
#endif

void get_allocation(GtkWidget * widget, GtkAllocation * allocation)
{
#ifdef GTK2_18_ENABLED
    gtk_widget_get_allocation(widget, allocation);
#else
    allocation = &(widget->allocation);
#endif
}

GdkWindow *get_window(GtkWidget * widget)
{
#ifdef GTK2_14_ENABLED
    return gtk_widget_get_window(widget);
#else
    return widget->window;
#endif
}

gboolean get_visible(GtkWidget * widget)
{
#ifdef GTK2_18_ENABLED
    return gtk_widget_get_visible(widget);
#else
    return GTK_WIDGET_VISIBLE(widget);
#endif
}


gboolean add_to_playlist_and_play(gpointer data)
{
    gchar *s = (gchar *) data;
    gboolean playlist;
    gchar *buf = NULL;

    selection = NULL;
    if (!uri_exists(s) && !streaming_media(s)) {
        buf = g_filename_to_uri(s, NULL, NULL);
    } else {
        buf = g_strdup(s);
    }
    if (buf != NULL) {
        playlist = detect_playlist(buf);
        if (!playlist) {
            add_item_to_playlist(buf, playlist);
        } else {
            if (!parse_playlist(buf)) {
                add_item_to_playlist(buf, playlist);
            }
        }
        g_free(buf);
    }
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) == 1
        || !gtk_list_store_iter_is_valid(playliststore, &iter)) {
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
            dontplaynext = TRUE;
            play_iter(&iter, 0);
            if (embed_window == 0 && bring_to_front)
                present_main_window();
        }
    }
    if (s != NULL)
        g_free(s);
    g_idle_add(set_update_gui, NULL);
    return FALSE;
}

gboolean clear_playlist_and_play(gpointer data)
{
    gtk_list_store_clear(playliststore);
    add_to_playlist_and_play(data);
    return FALSE;
}


static void drawing_area_realized(GtkWidget * widget, gpointer user_data)
{
    /* requesting the XID forces the GdkWindow to be native in GTK+ 2.18
     * onwards, requesting the native window in a thread causes a BadWindowID,
     * so we need to request it now. We could call gdk_window_ensure_native(),
     * but that would mean we require GTK+ 2.18, so instead we call this */
#ifdef GTK2_18_ENABLED
    gdk_window_ensure_native(gtk_widget_get_window(widget));
#else
#ifdef GTK2_14_ENABLED
#ifdef X11_ENABLED
    GDK_WINDOW_XID(get_window(GTK_WIDGET(widget)));
#endif
#endif
#endif

}

gint get_player_window()
{
    if (GTK_IS_WIDGET(drawing_area)) {
#ifdef GTK2_20_ENABLED
        if (!gtk_widget_get_realized(drawing_area)) {
            gtk_widget_realize(GTK_WIDGET(drawing_area));
        }
#else
        if (!GTK_WIDGET_REALIZED(drawing_area)) {
            gtk_widget_realize(GTK_WIDGET(drawing_area));
        }
#endif
        return (gint) gtk_socket_get_id(GTK_SOCKET(drawing_area));
    } else {
        return 0;
    }
}

void view_option_show_callback(GtkWidget * widget, gpointer data)
{
    skip_fixed_allocation_on_show = TRUE;
}

void view_option_hide_callback(GtkWidget * widget, gpointer data)
{
    skip_fixed_allocation_on_hide = TRUE;
    g_idle_add(set_adjust_layout, NULL);
}

void view_option_size_allocate_callback(GtkWidget * widget, GtkAllocation * allocation, gpointer data)
{

    //if (widget == plvbox)
    //printf("plvbox size_allocate_callback\n");
    //else
    //printf("size_allocate_callback\n");


    g_idle_add(set_adjust_layout, NULL);
}

gboolean set_adjust_layout(gpointer data)
{
    adjusting = FALSE;
    adjust_layout();
    return FALSE;
}

void adjust_layout()
{
    gint total_height;
    gint total_width;
    gint handle_size;
    GtkAllocation alloc;

    //while (gtk_events_pending())
    //      gtk_main_iteration(); 

    //printf("media size = %i x %i\n", non_fs_width, non_fs_height);
    //printf("fixed = %i x %i\n", fixed->allocation.width, fixed->allocation.height);
    if (GTK_IS_WIDGET(fixed)) {
        gtk_widget_set_size_request(fixed, -1, -1);
    } else {
        printf("fixed is not a widget\n");
        return;
    }

    if (GTK_IS_WIDGET(drawing_area)) {
        gtk_widget_set_size_request(drawing_area, -1, -1);
    } else {
        printf("drawing area is not a widget\n");
        return;
    }

    total_height = non_fs_height;
    total_width = non_fs_width;

    if (playlist_visible && remember_loc && !vertical_layout) {
        total_width = gtk_paned_get_position(GTK_PANED(pane));
    }

    if (playlist_visible && remember_loc && vertical_layout) {
        total_height = gtk_paned_get_position(GTK_PANED(pane));
    }

    if (total_width == 0) {
        if (playlist_visible && !vertical_layout) {
            total_width = gtk_paned_get_position(GTK_PANED(pane));
        } else {
            if (showcontrols) {
                get_allocation(controls_box, &alloc);
                total_width = alloc.width;
            }
        }
    }

    if (total_height == 0) {
        if (playlist_visible && vertical_layout) {
            total_height = gtk_paned_get_position(GTK_PANED(pane));
        }
    }

    if (GTK_IS_WIDGET(media_hbox)
        && gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_info))) {
        if (get_visible(media_hbox) == 0) {
            gtk_widget_show_all(media_hbox);
            return;
        }
        get_allocation(media_hbox, &alloc);
        total_height += alloc.height;
    } else {
        gtk_widget_hide_all(media_hbox);
    }

    if (GTK_IS_WIDGET(details_table)
        && gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_details))) {
        if (get_visible(details_vbox) == 0) {
            gtk_widget_show_all(details_vbox);
            return;
        }
        get_allocation(details_vbox, &alloc);
        total_height += alloc.height;
    } else {
        gtk_widget_hide_all(details_vbox);
    }

    if (GTK_IS_WIDGET(audio_meter)
        && gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_meter))) {
        if (get_visible(audio_meter) == 0) {
            gtk_widget_show_all(audio_meter);
            return;
        }
        get_allocation(audio_meter, &alloc);
        total_height += alloc.height;
    } else {
        gtk_widget_hide_all(audio_meter);
    }

    if (GTK_IS_WIDGET(plvbox)
        && gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist))) {
        //gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
        //g_object_set_property(G_OBJECT(window), "allow-shrink", &ALLOW_SHRINK_TRUE);
        if (get_visible(plvbox) == 0) {
            gtk_widget_show_all(plvbox);
            return;
        }
        gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
        g_object_set_property(G_OBJECT(window), "allow-shrink", &ALLOW_SHRINK_TRUE);
        gtk_widget_grab_default(plclose);
        gtk_widget_style_get(pane, "handle-size", &handle_size, NULL);
        //printf("handle size = %i\n",handle_size);
        //printf("plvbox req = %i x %i\n",plvbox->requisition.width,plvbox->requisition.height);
        //printf("plvbox alloc = %i x %i\n",plvbox->allocation.width,plvbox->allocation.height);

        get_allocation(plvbox, &alloc);
        if (vertical_layout) {
            // printf("totals = %i x %i\n",total_width,total_height);
            //gtk_paned_set_position(GTK_PANED(pane), total_height);
            total_height += alloc.height + handle_size;
        } else {
            total_width += alloc.width + handle_size;
        }

        if (non_fs_height == 0) {
            // printf("height = %i\n",plvbox->allocation.height);
            if (alloc.height < 16) {
                total_height = 200;
            } else {
                total_height = MAX(total_height, alloc.height);
            }
        }

    } else {
        if (!idledata->videopresent) {
            gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
            //gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);
            gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
            g_object_set_property(G_OBJECT(window), "allow-shrink", &ALLOW_SHRINK_FALSE);
        }
        gtk_paned_set_position(GTK_PANED(pane), -1);
        if (get_visible(plvbox) == 1) {
            gtk_widget_hide_all(plvbox);
            return;
        }
    }

    if (!fullscreen) {
        //printf("menubar = %i\n",menubar->allocation.height);
        get_allocation(menubar, &alloc);
        total_height += alloc.height;
    }

    if (showcontrols) {
        get_allocation(controls_box, &alloc);
        total_height += alloc.height;
    }

    if (non_fs_height > 32 && non_fs_width > 32) {
        gtk_widget_modify_bg(drawing_area, GTK_STATE_NORMAL, &(gtk_widget_get_style(window)->black));
    }
    //printf("controls = %i\n",menubar->allocation.height + controls_box->allocation.height);
    //printf("totals = %i x %i\n",total_width,total_height);

    if (use_remember_loc) {
        // printf("setting size to %i x %i\n", loc_window_width, loc_window_height);
        gtk_window_resize(GTK_WINDOW(window), loc_window_width, loc_window_height);
        use_remember_loc = FALSE;
    } else {
        if (total_height > 0 && total_width > 0 && idledata->videopresent) {
            gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
        }
        if (total_height > 0 && total_width > 0 && !idledata->window_resized && !remember_loc) {
            gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
        }
        //if (total_height > 0 && total_width > 0 && !idledata->videopresent
        //    && gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist))) {
        //    gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
        //}
    }

    if (idledata->fullscreen) {
        get_allocation(fixed, &alloc);
        allocate_fixed_callback(fixed, &alloc, NULL);
    }

    return;
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
    if (control_id == 0) {
        buf = g_strdup_printf(_("%s - GNOME MPlayer"), idle->url);
        gtk_window_set_title(GTK_WINDOW(window), buf);
        g_free(buf);
    }

    return FALSE;

}


gboolean set_media_info(void *data)
{

    IdleData *idle = (IdleData *) data;
    gchar *buf;
    gchar *name = NULL;
    GtkTreePath *path;
    gint current = 0, total;


    if (data != NULL && idle != NULL) {
        name = g_strdup(idle->display_name);

        total = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
        if (total > 0 && gtk_list_store_iter_is_valid(playliststore, &iter)) {
            path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &iter);
            if (path != NULL) {
                buf = gtk_tree_path_to_string(path);
                current = (gint) g_strtod(buf, NULL);
                g_free(buf);
                gtk_tree_path_free(path);
            }
        }
        if (total > 1) {
            if (name != NULL)
                buf = g_strdup_printf(_("%s - (%i/%i) - GNOME MPlayer"), name, current + 1, total);
            else
                buf = g_strdup_printf(_("(%i/%i) - GNOME MPlayer"), current + 1, total);

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
        if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) > 0
            && gtk_list_store_iter_is_valid(playliststore, &iter)) {
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, COVERART_COLUMN, &pixbuf, -1);
        }
        if (pixbuf != NULL) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(cover_art), GDK_PIXBUF(pixbuf));
            g_object_unref(pixbuf);
        }
    } else {
        if (GTK_IS_IMAGE(cover_art))
            gtk_image_clear(GTK_IMAGE(cover_art));
        if (GTK_IS_WIDGET(media_label)) {
            gtk_label_set_markup(GTK_LABEL(media_label), "");
        }
        return FALSE;
    }

    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_info)) && control_id == 0
        && strlen(idle->media_info) > 0) {
        gtk_widget_show_all(media_hbox);
    } else {
        gtk_widget_hide(media_hbox);
    }

    if (idle->fromdbus == FALSE) {
        dbus_send_rpsignal_with_string("RP_SetMediaLabel", idle->media_info);

#ifdef NOTIFY_ENABLED
        if (show_notification && control_id == 0 && !gtk_window_is_active((GtkWindow *) window)) {
            notify_init("gnome-mplayer");
#ifdef NOTIFY0_7_ENABLED
            notification = notify_notification_new(idle->display_name, idle->media_notification, "gnome-mplayer");
#else
            notification = notify_notification_new(idle->display_name, idle->media_notification, "gnome-mplayer", NULL);
#endif
#ifdef GTK2_12_ENABLED
#if !NOTIFY0_7_ENABLED
            if (show_status_icon)
                notify_notification_attach_to_status_icon(notification, status_icon);
#endif
#endif
            notify_notification_show(notification, NULL);
            notify_uninit();
        }
#endif
        if (embed_window == 0 && gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) != 1) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist), playlist_visible);
        }

    }

    g_idle_add(set_adjust_layout, NULL);

    return FALSE;
}

gboolean set_cover_art(gpointer pixbuf)
{
    gint width, height;
    gfloat aspect;
    GdkPixbuf *scaled;

    if (pixbuf == NULL) {
        if (GTK_IS_IMAGE(cover_art))
            gtk_image_clear(GTK_IMAGE(cover_art));
        if (strlen(idledata->media_info) > 0) {
            //gtk_widget_show_all(media_hbox);
        } else {
            // gtk_widget_hide_all(media_hbox);
        }
    } else {
        width = gdk_pixbuf_get_width(GDK_PIXBUF(pixbuf));
        height = gdk_pixbuf_get_height(GDK_PIXBUF(pixbuf));

        if (width > 200) {
            aspect = (gfloat) width / (gfloat) height;
            scaled = gdk_pixbuf_scale_simple(GDK_PIXBUF(pixbuf), 200, 200 / aspect, GDK_INTERP_BILINEAR);
            gtk_image_set_from_pixbuf(GTK_IMAGE(cover_art), GDK_PIXBUF(scaled));
            g_object_unref(pixbuf);
            g_object_unref(scaled);
        } else {
            gtk_image_set_from_pixbuf(GTK_IMAGE(cover_art), GDK_PIXBUF(pixbuf));
            g_object_unref(pixbuf);
        }
        //gtk_widget_show_all(media_hbox);
    }

    g_idle_add(set_adjust_layout, NULL);

    //adjust_layout();
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
            text = g_strdup_printf(_("Buffering: %2i%%"), (gint) (idle->cachepercent * 100));
            gmtk_media_tracker_set_text(tracker, text);
            g_free(text);
            gtk_widget_set_sensitive(play_event_box, FALSE);
        } else {
            if (idle->percent >= 0.0 && idle->percent <= 1.0) {
                gmtk_media_tracker_set_percentage(tracker, idle->percent);
                if (autopause == FALSE)
                    gtk_widget_set_sensitive(play_event_box, TRUE);
            }
        }
        if (idle->cachepercent < 1.0 && state == PAUSED) {
            text = g_strdup_printf(_("Paused | %2i%% \342\226\274"), (gint) (idle->cachepercent * 100));
            gmtk_media_tracker_set_text(tracker, text);
            g_free(text);
        } else {
            //gmtk_media_tracker_set_text(tracker, idle->progress_text);
        }
        gmtk_media_tracker_set_cache_percentage(tracker, idle->cachepercent);
    }

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) > 0
        && gtk_list_store_iter_is_valid(playliststore, &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iteruri, -1);
    }

    if (idle->cachepercent > 0.0 && idle->cachepercent < 0.9 && !forcecache && !streaming_media(iteruri)) {
        if (autopause == FALSE && state == PLAYING) {
            if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
                g_free(iteruri);
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iteruri, -1);
                if (iteruri != NULL) {
                    iterfilename = g_filename_from_uri(iteruri, NULL, NULL);
                    g_stat(iterfilename, &buf);
                    if (verbose > 1) {
                        printf("filename = %s\ndisk size = %li, byte pos = %li\n", iterfilename,
                               (glong) buf.st_size, idle->byte_pos);
                        printf("cachesize = %f, percent = %f\n", idle->cachepercent, idle->percent);
                        printf("will pause = %i\n", ((idle->byte_pos + (cache_size * 512)) > buf.st_size)
                               && !(playlist));
                    }
                    // if ((idle->percent + 0.10) > idle->cachepercent && ((idle->byte_pos + (512 * 1024)) > buf.st_size)) {
                    // if ((buf.st_size > 0) && (idle->byte_pos + (cache_size * 512)) > buf.st_size) {
                    if (((idle->byte_pos + (cache_size * 1024)) > buf.st_size) && !(playlist)) {
                        pause_callback(NULL, NULL, NULL);
                        gtk_widget_set_sensitive(play_event_box, FALSE);
                        autopause = TRUE;
                    }
                    g_free(iterfilename);
                    g_free(iteruri);
                }
            }
        } else if (autopause == TRUE && state == PAUSED) {
            if (idle->cachepercent > (idle->percent + 0.20) || idle->cachepercent >= 0.99) {
                play_callback(NULL, NULL, NULL);
                gtk_widget_set_sensitive(play_event_box, TRUE);
                autopause = FALSE;
            }
        }
    }

    if (idle->cachepercent > 0.95) {
        if (autopause == TRUE && state == PAUSED) {
            play_callback(NULL, NULL, NULL);
            gtk_widget_set_sensitive(play_event_box, TRUE);
            autopause = FALSE;
        }
    }

    if (idle->cachepercent >= 1.0) {
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_save), TRUE);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_save), FALSE);
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
    gchar *text;

    IdleData *idle = (IdleData *) data;

    if (idle == NULL)
        return FALSE;

    seconds = idle->position;
    length_seconds = idle->length;
/*
	if (control_id == 0) {
		if (state == PLAYING) {
		    text = g_strdup_printf(_("Playing %s"), idledata->display_name);
		} else if (state == PAUSED) {
		    text = g_strdup_printf(_("Paused %s"), idledata->display_name);
		} else {
		    text = g_strdup_printf(_("Idle"));
		}
	} else { */
    if (state == PLAYING) {
        text = g_strdup_printf(_("Playing"));
    } else if (state == PAUSED) {
        text = g_strdup_printf(_("Paused"));
    } else {
        text = g_strdup_printf(_("Idle"));
    }
//      }

    if (idle->cachepercent > 0 && idle->cachepercent < 1.0 && !(playlist) && !forcecache && !idle->streaming) {
        g_snprintf(idle->progress_text, 128, "%s | %2i%% \342\226\274", text, (int) (idle->cachepercent * 100));
    } else {
        g_snprintf(idle->progress_text, 128, "%s", text);
    }

    gmtk_media_tracker_set_text(tracker, text);
    g_free(text);

    if (GTK_IS_WIDGET(tracker) && idle->position > 0 && state != PAUSED) {
        gmtk_media_tracker_set_length(tracker, idle->length);
        gmtk_media_tracker_set_position(tracker, idle->position);
    }

    if (idle->fromdbus == FALSE && state != PAUSED)
        dbus_send_rpsignal_with_string("RP_SetProgressText", idle->progress_text);
    update_status_icon();

    dbus_send_event("TimeChanged", 0);

    return FALSE;
}

gboolean set_volume_from_slider(gpointer data)
{
    gdouble vol;
    gchar *cmd;

#ifdef GTK2_12_ENABLED
    vol = gtk_scale_button_get_value(GTK_SCALE_BUTTON(vol_slider));
#else
    vol = gtk_range_get_value(GTK_RANGE(vol_slider));
#endif
    if (!idledata->mute) {
        cmd = g_strdup_printf("volume %i 1\n", (gint) (vol * 100.0));
        send_command(cmd, FALSE);
        g_free(cmd);

        if (remember_softvol) {
            volume_softvol = vol;
            set_software_volume(&volume_softvol);
        }
    }

    return FALSE;
}

gboolean set_volume_tip(void *data)
{
#ifndef GTK2_12_ENABLED
    gchar *tip_text = NULL;
    IdleData *idle = (IdleData *) data;
#endif

    if (GTK_IS_WIDGET(vol_slider)) {
#ifdef GTK2_12_ENABLED
        // should be automatic
        /*
           tip_text = gtk_widget_get_tooltip_text(vol_slider);
           if (tip_text == NULL || g_ascii_strcasecmp(tip_text, idle->vol_tooltip) != 0)
           gtk_widget_set_tooltip_text(vol_slider, idle->vol_tooltip);
           g_free(tip_text);
         */
#else
        gtk_tooltips_set_tip(volume_tip, vol_slider, idle->vol_tooltip, NULL);
#endif
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
    gchar **split;
    gchar *joined;

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) < 2 && idledata->has_chapters == FALSE) {
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
        if (sub_source_file)
            item = g_list_last(langs);
        else
            item = g_list_nth(langs, idledata->sub_demux);
        if (item && GTK_IS_WIDGET(item->data))
            gtk_menu_shell_activate_item(GTK_MENU_SHELL(menu_edit_sub_langs), (GtkWidget *) item->data, FALSE);
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
                gtk_menu_shell_activate_item(GTK_MENU_SHELL(menu_edit_audio_langs), (GtkWidget *) item->data, FALSE);
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

        split = g_strsplit(coltitle, "_", 0);
        joined = g_strjoinv("__", split);
        gtk_tree_view_column_set_title(column, joined);
        g_free(coltitle);
        g_free(joined);
        g_strfreev(split);
    }
    return FALSE;
}

gboolean set_gui_state(void *data)
{
    gchar *tip_text = NULL;

    if (lastguistate != guistate) {
        if (guistate == PLAYING) {
            gtk_image_set_from_stock(GTK_IMAGE(image_play), GTK_STOCK_MEDIA_PAUSE, button_size);
#ifdef GTK2_12_ENABLED
            tip_text = gtk_widget_get_tooltip_text(play_event_box);
            if (tip_text == NULL || g_ascii_strcasecmp(tip_text, _("Pause")) != 0)
                gtk_widget_set_tooltip_text(play_event_box, _("Pause"));
            g_free(tip_text);
#else
            gtk_tooltips_set_tip(tooltip, play_event_box, _("Pause"), NULL);
#endif
            gtk_widget_set_sensitive(ff_event_box, TRUE);
            gtk_widget_set_sensitive(rew_event_box, TRUE);
            gtk_container_remove(GTK_CONTAINER(popup_menu), GTK_WIDGET(menuitem_play));
            menuitem_play = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PAUSE, NULL));
            g_signal_connect(GTK_OBJECT(menuitem_play), "activate", G_CALLBACK(menuitem_pause_callback), NULL);
            gtk_menu_shell_insert(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menuitem_play), 0);
            gtk_widget_show(GTK_WIDGET(menuitem_play));
            if (idledata->videopresent)
                dbus_disable_screensaver();
        }

        if (guistate == PAUSED) {
            gtk_image_set_from_stock(GTK_IMAGE(image_play), GTK_STOCK_MEDIA_PLAY, button_size);
#ifdef GTK2_12_ENABLED
            tip_text = gtk_widget_get_tooltip_text(play_event_box);
            if (tip_text == NULL || g_ascii_strcasecmp(tip_text, _("Play")) != 0)
                gtk_widget_set_tooltip_text(play_event_box, _("Play"));
            g_free(tip_text);
#else
            gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
#endif
            gtk_widget_set_sensitive(ff_event_box, FALSE);
            gtk_widget_set_sensitive(rew_event_box, FALSE);
            gtk_container_remove(GTK_CONTAINER(popup_menu), GTK_WIDGET(menuitem_play));
            menuitem_play = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL));
            g_signal_connect(GTK_OBJECT(menuitem_play), "activate", G_CALLBACK(menuitem_play_callback), NULL);
            gtk_menu_shell_insert(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menuitem_play), 0);
            gtk_widget_show(GTK_WIDGET(menuitem_play));
            dbus_enable_screensaver();
        }

        if (guistate == STOPPED) {
            gtk_image_set_from_stock(GTK_IMAGE(image_play), GTK_STOCK_MEDIA_PLAY, button_size);
#ifdef GTK2_12_ENABLED
            tip_text = gtk_widget_get_tooltip_text(play_event_box);
            if (tip_text == NULL || g_ascii_strcasecmp(tip_text, _("Play")) != 0)
                gtk_widget_set_tooltip_text(play_event_box, _("Play"));
            g_free(tip_text);
#else
            gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
#endif
            gtk_widget_set_sensitive(ff_event_box, FALSE);
            gtk_widget_set_sensitive(rew_event_box, FALSE);
            gtk_container_remove(GTK_CONTAINER(popup_menu), GTK_WIDGET(menuitem_play));
            menuitem_play = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL));
            g_signal_connect(GTK_OBJECT(menuitem_play), "activate", G_CALLBACK(menuitem_play_callback), NULL);
            gtk_menu_shell_insert(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menuitem_play), 0);
            gtk_widget_show(GTK_WIDGET(menuitem_play));
            dbus_enable_screensaver();
        }
        lastguistate = guistate;
    }
    return FALSE;
}

gboolean set_metadata(gpointer data)
{
    MetaData *mdata = (MetaData *) data;
    gchar *uri;
    GtkTreeIter riter;

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &riter)) {
        do {
            if (gtk_list_store_iter_is_valid(playliststore, &riter)) {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &riter, ITEM_COLUMN, &uri, -1);
                if (strcmp(mdata->uri, uri) == 0) {
                    g_free(uri);
                    break;
                }
                g_free(uri);
            }
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &riter));

        if (gtk_list_store_iter_is_valid(playliststore, &riter)) {
            if (mdata != NULL) {

                gtk_list_store_set(playliststore, &riter,
                                   DESCRIPTION_COLUMN, mdata->title,
                                   ARTIST_COLUMN, mdata->artist,
                                   ALBUM_COLUMN, mdata->album,
                                   SUBTITLE_COLUMN, mdata->subtitle,
                                   AUDIO_CODEC_COLUMN, mdata->audio_codec,
                                   VIDEO_CODEC_COLUMN, mdata->video_codec,
                                   LENGTH_COLUMN, mdata->length,
                                   DEMUXER_COLUMN, mdata->demuxer,
                                   LENGTH_VALUE_COLUMN, mdata->length_value,
                                   VIDEO_WIDTH_COLUMN, mdata->width, VIDEO_HEIGHT_COLUMN,
                                   mdata->height, PLAYABLE_COLUMN, mdata->playable, -1);

                if (mdata != NULL && mdata->playable == FALSE) {
                    gtk_list_store_remove(playliststore, &riter);
                    g_idle_add(set_media_info, idledata);
                }
            }
        }
    }

    if (mdata != NULL) {
        g_free(mdata->uri);
        g_free(mdata->demuxer);
        g_free(mdata->title);
        g_free(mdata->artist);
        g_free(mdata->album);
        g_free(mdata->length);
        g_free(mdata->subtitle);
        g_free(mdata->audio_codec);
        g_free(mdata->video_codec);
        g_free(mdata);
    }

    return FALSE;
}

gboolean set_show_seek_buttons(gpointer data)
{
    IdleData *idle = (IdleData *) data;

    if (idle->seekable) {
        gtk_widget_show_all(ff_event_box);
        gtk_widget_show_all(rew_event_box);
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

    if (GTK_IS_WIDGET(status_icon)) {
        gtk_status_icon_set_tooltip(status_icon, text);
    }

    g_free(text);
#endif
}

void menuitem_lang_callback(GtkMenuItem * menuitem, gpointer sid)
{
    gchar *cmd;

    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) {

        if (GPOINTER_TO_INT(sid) >= 9000) {
            cmd = g_strdup_printf("sub_file %i\n", GPOINTER_TO_INT(sid - 9000));
            send_command(cmd, TRUE);
            g_free(cmd);
        } else if (GPOINTER_TO_INT(sid) >= 0) {
            cmd = g_strdup_printf("sub_demux %i\n", GPOINTER_TO_INT(sid));
            send_command(cmd, TRUE);
            g_free(cmd);
        }

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

            menuitem_lang = GTK_MENU_ITEM(gtk_radio_menu_item_new_with_label(lang_group, menu->label));
            g_object_set_data(G_OBJECT(menuitem_lang), "id", GINT_TO_POINTER(menu->value));

            lang_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem_lang));

            gtk_menu_shell_append(GTK_MENU_SHELL(menu_edit_sub_langs), GTK_WIDGET(menuitem_lang));
            g_signal_connect(GTK_OBJECT(menuitem_lang), "toggled",
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

    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) {

        if (GPOINTER_TO_INT(aid) >= 0) {
            cmd = g_strdup_printf("switch_audio %i\n", GPOINTER_TO_INT(aid));
            send_command(cmd, TRUE);
            g_free(cmd);
        }

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
                    if (verbose)
                        printf("Updating audio track, id = %i, label = %s\n", menu->value, menu->label);
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
            if (verbose)
                printf("Adding audio track, id = %i, label = %s\n", menu->value, menu->label);
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_select_audio_lang), TRUE);

            menuitem_lang = GTK_MENU_ITEM(gtk_radio_menu_item_new_with_label(audio_group, menu->label));
            g_object_set_data(G_OBJECT(menuitem_lang), "id", GINT_TO_POINTER(menu->value));
            audio_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem_lang));
            gtk_menu_shell_append(GTK_MENU_SHELL(menu_edit_audio_langs), GTK_WIDGET(menuitem_lang));
            g_signal_connect(GTK_OBJECT(menuitem_lang), "toggled",
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
    gint total_height;
    IdleData *idle = (IdleData *) data;
    GTimeVal currenttime;
    GValue resize_value = { 0 };
    GtkAllocation alloc;

    g_value_init(&resize_value, G_TYPE_BOOLEAN);

    if (GTK_IS_WIDGET(window)) {
        if (idle->videopresent) {
            g_value_set_boolean(&resize_value, TRUE);
            gtk_container_child_set_property(GTK_CONTAINER(pane), vbox, "resize", &resize_value);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), FALSE);
            g_get_current_time(&currenttime);
            last_movement_time = currenttime.tv_sec;
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_info), TRUE);
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_subtitle), TRUE);
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_audiofile), TRUE);
            dbus_disable_screensaver();
            if (embed_window == -1) {
                gtk_widget_show_all(window);
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), FALSE);
                hide_buttons(idle);
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), showcontrols);
            }
            // gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, TRUE);
            gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
            g_object_set_property(G_OBJECT(window), "allow-shrink", &ALLOW_SHRINK_TRUE);
            gtk_widget_show_all(GTK_WIDGET(fixed));

            if (window_x == 0 && window_y == 0) {
                if (verbose)
                    printf("current size = %i x %i \n", non_fs_width, non_fs_height);
                if ((non_fs_width < 16 || non_fs_height < 16) || resize_on_new_media == TRUE || idle->streaming) {
                    if (idle->width > 1 && idle->height > 1) {
                        if (verbose) {
                            printf("Changing window size to %i x %i visible = %i\n", idle->width,
                                   idle->height, get_visible(vbox));
                        }
                        last_window_width = idle->width;
                        last_window_height = idle->height;
                        non_fs_width = idle->width;
                        non_fs_height = idle->height;
                        //gtk_widget_set_size_request(fixed, idle->width, idle->height);
                        adjusting = TRUE;
                        g_idle_add(set_adjust_layout, &adjusting);
                    }
                } else {
                    if (verbose) {
                        printf("old aspect is %f new aspect is %f\n",
                               (gfloat) non_fs_width / (gfloat) non_fs_height,
                               (gfloat) (idle->width) / (gfloat) (idle->height));
                    }
                    // adjust the drawing area, new media may have different aspect
                    get_allocation(fixed, &alloc);
                    allocate_fixed_callback(fixed, &alloc, NULL);
                }
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_fullscreen), fullscreen);
                if (init_fullscreen) {
                    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_fullscreen), init_fullscreen);
                    init_fullscreen = 0;
                }

            } else {
                if (window_x > 0 && window_y > 0) {
                    total_height = window_y;
                    if (showcontrols) {
                        get_allocation(controls_box, &alloc);
                        total_height -= alloc.height;
                    }

                    if (window_x > 0 && total_height > 0) {
                        gtk_widget_set_size_request(fixed, window_x, total_height);
                        g_idle_add(set_adjust_layout, NULL);
                        // adjust_layout();
                    }
                }
            }
        } else {
            // audio only file
            gtk_widget_modify_bg(drawing_area, GTK_STATE_NORMAL, &(gtk_widget_get_style(window)->bg[0]));

            g_value_set_boolean(&resize_value, FALSE);
            gtk_container_child_set_property(GTK_CONTAINER(pane), vbox, "resize", &resize_value);
            gtk_widget_hide(fixed);
            non_fs_height = 0;
            non_fs_width = 0;
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_fullscreen), FALSE);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), TRUE);
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_info), FALSE);
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_subtitle), FALSE);
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_audiofile), FALSE);
            last_window_height = 0;
            last_window_width = 0;
            g_idle_add(set_adjust_layout, NULL);
            if (window_x > 0) {
                gtk_widget_set_size_request(media_label, window_x, -1);
            } else {
                gtk_widget_set_size_request(media_label, 300, -1);
            }
            // adjust_layout();
        }
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_fullscreen), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(fs_event_box), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_subtitle), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_audiofile), idle->videopresent);
        if (vo == NULL
            || !(g_ascii_strncasecmp(vo, "xvmc", strlen("xvmc")) == 0
                 || g_ascii_strncasecmp(vo, "vdpau", strlen("vdpau")) == 0)) {
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_take_screenshot), idle->videopresent);
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_take_screenshot), FALSE);
        }
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
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_aspect_follow_window), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_subtitles), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_smaller_subtitle), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_larger_subtitle), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_decrease_subtitle_delay), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_increase_subtitle_delay), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_angle), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_advanced), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_details), TRUE);
    }
    if (idle != NULL)
        idle->window_resized = TRUE;

    update_details_table();


    return FALSE;
}

gboolean set_play(void *data)
{
    if (embed_window == 0 && bring_to_front)
        gtk_window_present(GTK_WINDOW(window));

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

gboolean set_kill_mplayer(void *data)
{
    mplayer_shutdown();
    return FALSE;
}

gboolean set_pane_position(void *data)
{
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist))) {
        gtk_paned_set_position(GTK_PANED(pane), loc_panel_position);
    }
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

gboolean set_software_volume(int *data)
{

    gm_store = gm_pref_store_new("gnome-mplayer");
    gm_pref_store_set_int(gm_store, VOLUME_SOFTVOL, *data);
    gm_pref_store_free(gm_store);
    return FALSE;
}

gboolean set_volume(void *data)
{
    IdleData *idle = (IdleData *) data;
    gchar *buf = NULL;

    if (data == NULL) {
        // printf("in set_volume without data\n");
        gm_audio_get_volume(&audio_device);
        // printf("new volume is %f\n",audio_device.volume); 
#ifdef GTK2_12_ENABLED
        gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider), audio_device.volume);
#else
        gtk_range_set_value(GTK_RANGE(vol_slider), audio_device.volume);
#endif
        return FALSE;
    }

    if (GTK_IS_WIDGET(vol_slider) && audio_device.volume >= 0.0 && audio_device.volume <= 1.0) {
        //printf("setting slider to %f\n", idle->volume);
        if (remember_softvol) {
            volume_softvol = audio_device.volume;
            set_software_volume(&volume_softvol);
        }
#ifdef GTK2_12_ENABLED
        if (rpcontrols != NULL && g_strcasecmp(rpcontrols, "volumeslider") == 0) {
            gtk_range_set_value(GTK_RANGE(vol_slider), audio_device.volume);
            buf = g_strdup_printf(_("Volume %i%%"), (gint) (audio_device.volume * 100.0));
            g_strlcpy(idledata->vol_tooltip, buf, 128);
            g_free(buf);
        } else {
            gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider), audio_device.volume);
        }

#else
        gtk_range_set_value(GTK_RANGE(vol_slider), audio_device.volume);
        buf = g_strdup_printf(_("Volume %i%%"), (gint) (audio_device.volume * 100.0));
        g_strlcpy(idledata->vol_tooltip, buf, 128);
        g_free(buf);
#endif
        g_idle_add(set_volume_tip, idle);
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

    if (get_visible(GTK_WIDGET(menuitem_view_controls))) {
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
    GtkAllocation alloc;

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

        if (event_button->button == 2) {

#ifdef GTK2_12_ENABLED
            if (idledata->mute) {
                gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider), audio_device.volume);
            } else {
                gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider), 0.0);
            }
#else
            if (idledata->mute) {
                gtk_range_set_value(GTK_RANGE(vol_slider), audio_device.volume);
            } else {
                gtk_range_set_value(GTK_RANGE(vol_slider), 0.0);
            }
#endif
        }

        if (event_button->button == 1 && idledata->videopresent == TRUE && !disable_pause_on_click) {
            get_allocation(fixed, &alloc);
            if (event_button->x > alloc.x
                && event_button->y > alloc.y
                && event_button->x < alloc.x + alloc.width && event_button->y < alloc.y + alloc.height) {
                play_callback(NULL, NULL, NULL);
            }
        }


    }

    if (event->type == GDK_2BUTTON_PRESS) {
        event_button = (GdkEventButton *) event;
        if (event_button->button == 1 && idledata->videopresent == TRUE) {
            get_allocation(fixed, &alloc);
            if (event_button->x > alloc.x
                && event_button->y > alloc.y
                && event_button->x < alloc.x + alloc.width && event_button->y < alloc.y + alloc.height) {
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen), !fullscreen);

            }
        }
    }

    if (event->type == GDK_BUTTON_RELEASE) {

        event_button = (GdkEventButton *) event;
        dbus_send_event("MouseUp", event_button->button);

    }

    return FALSE;
}

gboolean drawing_area_scroll_event_callback(GtkWidget * widget, GdkEventScroll * event, gpointer data)
{

    if (mouse_wheel_changes_volume == FALSE) {
        if (event->direction == GDK_SCROLL_UP) {
            set_ff(NULL);
        }

        if (event->direction == GDK_SCROLL_DOWN) {
            set_rew(NULL);
        }
    } else {
        if (event->direction == GDK_SCROLL_UP) {
            audio_device.volume += 0.01;
            g_idle_add(set_volume, idledata);
        }

        if (event->direction == GDK_SCROLL_DOWN) {
            audio_device.volume -= 0.01;
            g_idle_add(set_volume, idledata);
        }
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

    g_idle_remove_by_data(idledata);

    if (remember_loc && !fullscreen && embed_window == 0) {
        gm_store = gm_pref_store_new("gnome-mplayer");
        gtk_window_get_position(GTK_WINDOW(window), &loc_window_x, &loc_window_y);
        gtk_window_get_size(GTK_WINDOW(window), &loc_window_width, &loc_window_height);
        loc_panel_position = gtk_paned_get_position(GTK_PANED(pane));

        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist))) {

        }
        //if (save_loc) {
        gm_pref_store_set_int(gm_store, WINDOW_X, loc_window_x);
        gm_pref_store_set_int(gm_store, WINDOW_Y, loc_window_y);
        gm_pref_store_set_int(gm_store, WINDOW_HEIGHT, loc_window_height);
        gm_pref_store_set_int(gm_store, WINDOW_WIDTH, loc_window_width);
        gm_pref_store_set_int(gm_store, PANEL_POSITION, loc_panel_position);
        //}
        gm_pref_store_free(gm_store);
    }

    mplayer_shutdown();

    if (control_id == 0) {
        g_thread_pool_stop_unused_threads();
        if (retrieve_metadata_pool != NULL) {
            while (gtk_events_pending() || thread != NULL || g_thread_pool_unprocessed(retrieve_metadata_pool)) {
                gtk_main_iteration();
            }
            g_thread_pool_free(retrieve_metadata_pool, TRUE, TRUE);
        }
    } else {
        while (gtk_events_pending() || thread != NULL) {
            gtk_main_iteration();
        }
        dbus_cancel();
    }
    dbus_enable_screensaver();
    dbus_unhook();

    if (use_defaultpl && embed_window == 0)
        save_playlist_pls(default_playlist);

    gtk_main_quit();
    return FALSE;
}

#ifdef GTK2_12_ENABLED
gboolean status_icon_callback(GtkStatusIcon * icon, gpointer data)
{

    if (get_visible(window)) {
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


void status_icon_context_callback(GtkStatusIcon * status_icon, guint button, guint activate_time, gpointer data)
{
    gtk_menu_popup(GTK_MENU(popup_menu), NULL, NULL, gtk_status_icon_position_menu, status_icon, button, activate_time);
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

    //printf("moving window to %i x %i\n",idle->x,idle->y);
    if (GTK_IS_WIDGET(drawing_area) && (idle->x != idle->last_x || idle->y != idle->last_y)) {
        gtk_fixed_move(GTK_FIXED(fixed), GTK_WIDGET(drawing_area), idle->x, idle->y);
        idle->last_x = idle->x;
        idle->last_y = idle->y;
    }
    return FALSE;
}

gboolean window_state_callback(GtkWidget * widget, GdkEventWindowState * event, gpointer user_data)
{
    //printf("fullscreen = %i\nState = %i mask = %i flag = %i\n",(event->new_window_state == GDK_WINDOW_STATE_FULLSCREEN),event->new_window_state, event->changed_mask, GDK_WINDOW_STATE_FULLSCREEN);
    //if (embed_window == 0) {
    //update_control_flag = TRUE;
    //printf("restore controls = %i showcontrols = %i\n", restore_controls, showcontrols);
    if (fullscreen == 1 && (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)) {
        // fullscreen, but window hidden
        hide_fs_controls();
    }

    if (fullscreen == 1 && (event->new_window_state == GDK_WINDOW_STATE_FULLSCREEN)) {
        if (showcontrols) {
            show_fs_controls();
        }
    }

    return FALSE;

    fullscreen = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN);
    if (fullscreen) {
        gtk_widget_hide(menubar);
    } else {
        gtk_widget_show(menubar);
    }
    if (event->changed_mask == GDK_WINDOW_STATE_FULLSCREEN) {
        idledata->showcontrols = restore_controls;
        set_show_controls(idledata);
    }
    //}

    return FALSE;
}

gboolean configure_callback(GtkWidget * widget, GdkEventConfigure * event, gpointer user_data)
{
    if (use_remember_loc && event->width == loc_window_width && event->height == loc_window_height) {
        // this might be useful
    }
    return FALSE;
}

gboolean expose_fixed_callback(GtkWidget * widget, GdkEventExpose * event, gpointer data)
{
    if (GDK_IS_DRAWABLE(get_window(fixed))) {
        if (videopresent || embed_window != 0) {
            // printf("drawing box %i x %i at %i x %i \n",event->area.width,event->area.height, event->area.x, event->area.y );
            gdk_draw_rectangle(get_window(fixed), gtk_widget_get_style(window)->black_gc, TRUE,
                               event->area.x, event->area.y, event->area.width, event->area.height);
        }
    }
    return FALSE;
}

gboolean allocate_fixed_callback(GtkWidget * widget, GtkAllocation * allocation, gpointer data)
{
    gdouble movie_ratio, window_ratio;
    gint new_width = 0, new_height;

    //printf("allocate_fixed_callback %i\n", skip_fixed_allocation_on_show);
    if (skip_fixed_allocation_on_show == TRUE) {
        skip_fixed_allocation_on_show = FALSE;
        return TRUE;
    }
    if (skip_fixed_allocation_on_hide == TRUE) {
        skip_fixed_allocation_on_hide = FALSE;
        return TRUE;
    }

    if (verbose > 1) {
        printf("video present = %i\n", idledata->videopresent);
        printf("movie size = %i x %i\n", non_fs_width, non_fs_height);
        printf("movie allocation new_width %i new_height %i\n", allocation->width, allocation->height);
        printf("actual movie new_width %i new_height %i\n", actual_x, actual_y);
        printf("original movie width %i height %i\n", idledata->original_w, idledata->original_h);
    }
    if (actual_x == 0 && actual_y == 0) {
        actual_x = allocation->width;
        actual_y = allocation->height;
    }

    if (actual_x > 0 && actual_y > 0) {


        movie_ratio = (gdouble) idledata->original_w / (gdouble) idledata->original_h;
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

        if (allocation->width == idledata->width && allocation->height == idledata->height) {
            new_width = allocation->width;
            new_height = allocation->height;
        } else {
            // printf("last %i x %i\n",last_window_width,last_window_height);
            if (movie_ratio > window_ratio) {
                // printf("movie %lf > window %lf\n",movie_ratio,window_ratio);
                new_width = allocation->width;
                new_height = floorf((allocation->width / movie_ratio) + 0.5);
            } else {
                // printf("movie %lf < window %lf\n",movie_ratio,window_ratio);
                new_height = allocation->height;
                new_width = floorf((allocation->height * movie_ratio) + 0.5);
            }
        }


        // printf("pre align new_width %i new_height %i\n",new_width, new_height);
        // adjust video to be aligned when playing on video on a smaller screen
        if (new_height < idledata->height || new_width < idledata->width) {
            new_width = new_width - (new_width % 16);
            new_height = new_height - (new_height % 16);
        }
        if (verbose > 1)
            printf("new_width %i new_height %i\n", new_width, new_height);

#ifdef ENABLE_PANSCAN
        gtk_widget_set_size_request(drawing_area, allocation->width, allocation->height);
        idledata->x = 0;        // (allocation->width - new_width) / 2;
        idledata->y = 0;        // (allocation->height - new_height) / 2;
#else
        if (new_height > 0 && new_width > 0) {
            gtk_widget_set_size_request(drawing_area, new_width, new_height);
            idledata->x = (allocation->width - new_width) / 2;
            idledata->y = (allocation->height - new_height) / 2;
        }
#endif
        g_idle_add(move_window, idledata);
    }

    if (idledata->videopresent) {
        gtk_widget_set_size_request(media_label, allocation->width * 0.8, -1);
    } else {
        gtk_widget_set_size_request(media_label, 300, -1);
    }

    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen))) {
//      if (!fullscreen) {
        if (idledata->videopresent) {
            //printf("Adjusting = %i\n",adjusting);
            //printf("fixed resized to %i x %i\n",allocation->width,allocation->height);
            if (!adjusting) {
                non_fs_width = allocation->width;
                non_fs_height = allocation->height;
            }
        } else {
            non_fs_width = 0;
            non_fs_height = 0;
        }
    }

    if (non_fs_width > 0) {
        if (non_fs_width < 250) {
            gtk_widget_hide(rew_event_box);
            gtk_widget_hide(ff_event_box);
            gtk_widget_hide(fs_event_box);
        } else {
            gtk_widget_show(rew_event_box);
            gtk_widget_show(ff_event_box);
            gtk_widget_show(fs_event_box);
        }
        if (non_fs_width < 170) {
            gtk_widget_hide(GTK_WIDGET(tracker));
        } else {
            gtk_widget_show(GTK_WIDGET(tracker));
        }
    }

    /*
       if (GDK_IS_DRAWABLE(fixed->window)) {
       if (videopresent || embed_window != 0) {
       // printf("drawing box %i x %i at %i x %i \n",event->area.width,event->area.height, event->area.x, event->area.y );
       gdk_draw_rectangle(fixed->window, window->style->black_gc, TRUE, allocation->x,
       allocation->y, allocation->width, allocation->height);
       }
       }
     */
    if (update_control_flag && restore_controls == showcontrols) {
        idledata->showcontrols = !showcontrols;
        set_show_controls(idledata);
        idledata->showcontrols = restore_controls;
        set_show_controls(idledata);
        update_control_flag = FALSE;
    }

    return FALSE;

}

gboolean window_key_callback(GtkWidget * widget, GdkEventKey * event, gpointer user_data)
{
    GTimeVal currenttime;
    gchar *cmd;

    // printf("key = %i\n",event->keyval);
    // printf("state = %i\n",event->state);
    // printf("other = %i\n", event->state & ~GDK_CONTROL_MASK);

    // printf("key name=%s\n", gdk_keyval_name(event->keyval));
    // We don't want to handle CTRL accelerators here
    // if we pass in items with CTRL then 2 and Ctrl-2 do the same thing
    if (get_visible(plvbox) && gtk_tree_view_get_enable_search(GTK_TREE_VIEW(list)))
        return FALSE;

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
                if (state != STOPPED && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)))
                    return ff_callback(NULL, NULL, NULL);
            }
            break;
        case GDK_Left:
            if (lastfile != NULL && dvdnav_title_is_menu) {
                send_command("dvdnav 3\n", FALSE);
                return FALSE;
            } else {
                if (state != STOPPED && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)))
                    return rew_callback(NULL, NULL, NULL);
            }
            break;
        case GDK_Page_Up:
            if (state != STOPPED && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)))
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
            if (state != STOPPED && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)))
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
                if (state != STOPPED && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)))
                    send_command("seek +60 0\n", TRUE);
                if (state == PAUSED) {
                    send_command("mute 1\nseek 0 0\npause\n", FALSE);
                    send_command("mute 0\n", TRUE);
                    idledata->position += 60;
                    if (idledata->position > idledata->length)
                        idledata->position = 0;
                    gmtk_media_tracker_set_percentage(tracker, idledata->position / idledata->length);
                }
            }

            return FALSE;
        case GDK_Down:
            if (lastfile != NULL && dvdnav_title_is_menu) {
                send_command("dvdnav 2\n", FALSE);
            } else {
                if (state != STOPPED && !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)))
                    send_command("seek -60 0\n", TRUE);
                if (state == PAUSED) {
                    send_command("mute 1\nseek 0 0\npause\n", FALSE);
                    send_command("mute 0\n", TRUE);
                    idledata->position -= 60;
                    if (idledata->position < 0)
                        idledata->position = 0;
                    gmtk_media_tracker_set_percentage(tracker, idledata->position / idledata->length);
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
                gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider), audio_device.volume);
            } else {
                gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider), 0.0);
            }
#else
            if (idledata->mute) {
                gtk_range_set_value(GTK_RANGE(vol_slider), audio_device.volume);
            } else {
                gtk_range_set_value(GTK_RANGE(vol_slider), 0.0);
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
                                       gtk_scale_button_get_value(GTK_SCALE_BUTTON(vol_slider)) - 10);
#else
            gtk_range_set_value(GTK_RANGE(vol_slider), gtk_range_get_value(GTK_RANGE(vol_slider)) - 10);
#endif
            return FALSE;
        case GDK_0:
#ifdef GTK2_12_ENABLED
            gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider),
                                       gtk_scale_button_get_value(GTK_SCALE_BUTTON(vol_slider)) + 10);
#else
            gtk_range_set_value(GTK_RANGE(vol_slider), gtk_range_get_value(GTK_RANGE(vol_slider)) + 10);
#endif
            return FALSE;
        case GDK_numbersign:
            send_command("switch_audio\n", TRUE);
            return FALSE;
        case GDK_period:
            if (state == PAUSED)
                send_command("frame_step\n", FALSE);
            return FALSE;
        case GDK_j:
            send_command("sub_select\n", TRUE);
            send_command("get_property sub_demux\n", TRUE);
            return FALSE;
        case GDK_q:
            delete_callback(NULL, NULL, NULL);
            return FALSE;
        case GDK_v:
            if (fullscreen) {
                send_command("sub_visibility\n", TRUE);
            }
            return FALSE;
        case GDK_plus:
        case GDK_KP_Add:
            send_command("audio_delay 0.1 0\n", TRUE);
            return FALSE;
        case GDK_minus:
        case GDK_KP_Subtract:
            send_command("audio_delay -0.1 0\n", TRUE);
            return FALSE;
            //case GDK_z:
            //    menuitem_view_decrease_subtitle_delay_callback(NULL, NULL);
            //    return FALSE;
            //case GDK_x:
            //    menuitem_view_increase_subtitle_delay_callback(NULL, NULL);
            //    return FALSE;
        case GDK_F11:
            if (idledata->videopresent)
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen), !fullscreen);
            return FALSE;
        case GDK_Escape:
            if (fullscreen) {
                if (idledata->videopresent)
                    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen), !fullscreen);
            } else {
                delete_callback(NULL, NULL, NULL);
            }
            return FALSE;
        case GDK_a:
            if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window))) {
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), TRUE);
                return FALSE;
            }
            if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten)))
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window), TRUE);
            if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine)))
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten), TRUE);
            if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three)))
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine), TRUE);
            if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default)))
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three), TRUE);
            return FALSE;
        case GDK_d:
            send_command("frame_drop\nosd_show_property_text \"framedropping: ${framedropping}\"\n", TRUE);
            return FALSE;
        case GDK_i:
            if (fullscreen) {
                cmd = g_strdup_printf("osd_show_text '%s' 1500 0\n", idledata->display_name);
                send_command(cmd, TRUE);
                g_free(cmd);
            }
            return FALSE;
        case GDK_b:
            send_command("sub_pos -1 0\n", TRUE);
            return FALSE;
        case GDK_B:
            send_command("sub_pos 1 0\n", TRUE);
            return FALSE;
        case GDK_s:
        case GDK_S:
            cmd = g_strdup_printf("screenshot 0\n");
            send_command(cmd, TRUE);
            g_free(cmd);
            return FALSE;
#ifdef ENABLE_PANSCAN
        case GDK_w:
            send_command("panscan -0.05 0\n", TRUE);
            send_command("get_property panscan\n", TRUE);
            return FALSE;
        case GDK_e:
            send_command("panscan 0.05 0\n", TRUE);
            send_command("get_property panscan\n", TRUE);
            return FALSE;
#endif
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
                       gint x, gint y, GtkSelectionData * selection_data, guint info, guint t, gpointer data)
{
    gchar **list;
    gint i = 0;
    gint playlist;
    gint itemcount;
    GError *error;
    gboolean added_single = FALSE;
    gchar *cmd;
    gchar *filename;
    /* Important, check if we actually got data.  Sometimes errors
     * occure and selection_data will be NULL.
     */
    if (selection_data == NULL)
        return FALSE;
#ifdef GTK2_14_ENABLED
    if (gtk_selection_data_get_length(selection_data) < 0)
        return FALSE;
#else
    if (selection_data->length < 0)
        return FALSE;
#endif

    if ((info == DRAG_INFO_0) || (info == DRAG_INFO_1) || (info == DRAG_INFO_2)) {
        error = NULL;
#ifdef GTK2_14_ENABLED
        list = g_uri_list_extract_uris((const gchar *) gtk_selection_data_get_data(selection_data));
#else
        list = g_uri_list_extract_uris((const gchar *) selection_data->data);
#endif
        itemcount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);

        while (list[i] != NULL) {
            if (strlen(list[i]) > 0) {
                if (is_uri_dir(list[i])) {
                    create_folder_progress_window();
                    add_folder_to_playlist_callback(list[i], NULL);
                    destroy_folder_progress_window();
                } else {
                    // subtitle?
                    if (g_strrstr(list[i], ".ass") != NULL || g_strrstr(list[i], ".ssa") != NULL
                        || g_strrstr(list[i], ".srt") != NULL) {
                        send_command("sub_remove\n", TRUE);
                        filename = g_filename_from_uri(list[i], NULL, NULL);
                        if (filename != NULL) {
                            cmd = g_strdup_printf("sub_load '%s'\n", filename);
                            send_command(cmd, TRUE);
                            g_free(cmd);
                            gtk_list_store_set(playliststore, &iter, SUBTITLE_COLUMN, filename, -1);
                            menuitem_lang_callback(menuitem_lang, GINT_TO_POINTER(9000));
                            g_free(filename);
                        }
                    } else {
                        playlist = detect_playlist(list[i]);

                        if (!playlist) {
                            if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist))) {
                                dontplaynext = TRUE;
                                mplayer_shutdown();
                                set_media_label(NULL);
                                gtk_list_store_clear(playliststore);
                                added_single = add_item_to_playlist(list[i], playlist);
                            } else {
                                add_item_to_playlist(list[i], playlist);
                            }
                        } else {
                            if (!parse_playlist(list[i])) {
                                if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist))) {
                                    dontplaynext = TRUE;
                                    mplayer_shutdown();
                                    set_media_label(NULL);
                                    gtk_list_store_clear(playliststore);
                                    added_single = add_item_to_playlist(list[i], playlist);
                                } else {
                                    add_item_to_playlist(list[i], playlist);
                                }
                            }
                        }
                    }
                }
            }
            i++;
        }

        if (itemcount == 0 || added_single) {
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            play_iter(&iter, 0);
            dontplaynext = FALSE;
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

    if (verbose > 1) {
        if (state == PAUSED)
            printf("play_callback with state = PAUSED\n");
        if (state == PLAYING)
            printf("play_callback with state = PLAYING\n");
        if (state == QUIT)
            printf("play_callback with state = QUIT\n");
    }

    if (state == PAUSED || state == STOPPED) {
        send_command("pause\n", TRUE);
        send_command("seek 0 0\n", FALSE);
        state = PLAYING;
        js_state = STATE_PLAYING;
        gtk_image_set_from_stock(GTK_IMAGE(image_play), GTK_STOCK_MEDIA_PAUSE, button_size);
#ifdef GTK2_12_ENABLED
        gtk_widget_set_tooltip_text(play_event_box, _("Pause"));
#else
        gtk_tooltips_set_tip(tooltip, play_event_box, _("Pause"), NULL);
#endif
        g_strlcpy(idledata->progress_text, _("Playing"), 1024);
        g_idle_add(set_progress_text, idledata);
        gtk_widget_set_sensitive(ff_event_box, TRUE);
        gtk_widget_set_sensitive(rew_event_box, TRUE);
        gtk_container_remove(GTK_CONTAINER(popup_menu), GTK_WIDGET(menuitem_play));
        menuitem_play = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PAUSE, NULL));
        gtk_menu_shell_insert(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menuitem_play), 0);
        gtk_widget_show(GTK_WIDGET(menuitem_play));
        g_signal_connect(GTK_OBJECT(menuitem_play), "activate", G_CALLBACK(menuitem_pause_callback), NULL);

        if (idle && idle->videopresent)
            dbus_disable_screensaver();

    } else if (state == PLAYING) {
        send_command("pause\n", FALSE);
        state = PAUSED;
        js_state = STATE_PAUSED;
        gtk_image_set_from_stock(GTK_IMAGE(image_play), GTK_STOCK_MEDIA_PLAY, button_size);
#ifdef GTK2_12_ENABLED
        gtk_widget_set_tooltip_text(play_event_box, _("Play"));
#else
        gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
#endif
        g_strlcpy(idledata->progress_text, _("Paused"), 1024);
        g_idle_add(set_progress_text, idledata);
        gtk_widget_set_sensitive(ff_event_box, FALSE);
        gtk_widget_set_sensitive(rew_event_box, FALSE);
        gtk_container_remove(GTK_CONTAINER(popup_menu), GTK_WIDGET(menuitem_play));
        menuitem_play = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL));
        gtk_menu_shell_insert(GTK_MENU_SHELL(popup_menu), GTK_WIDGET(menuitem_play), 0);
        gtk_widget_show(GTK_WIDGET(menuitem_play));
        g_signal_connect(GTK_OBJECT(menuitem_play), "activate", G_CALLBACK(menuitem_pause_callback), NULL);
        if (idledata->videopresent)
            dbus_enable_screensaver();
    }

    if (state == QUIT) {
        if (next_item_in_playlist(&iter)) {
            play_iter(&iter, 0);
        } else {
            if (first_item_in_playlist(&iter)) {
                play_iter(&iter, 0);
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
        gtk_image_set_from_stock(GTK_IMAGE(image_play), GTK_STOCK_MEDIA_PLAY, button_size);
#ifdef GTK2_12_ENABLED
        gtk_widget_set_tooltip_text(play_event_box, _("Play"));
#else
        gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
#endif
    }

    if (state == QUIT) {
        gmtk_media_tracker_set_percentage(tracker, 0.0);
        gtk_image_set_from_stock(GTK_IMAGE(image_play), GTK_STOCK_MEDIA_PLAY, button_size);
#ifdef GTK2_12_ENABLED
        gtk_widget_set_tooltip_text(play_event_box, _("Play"));
#else
        gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
#endif
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
    dbus_enable_screensaver();

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
    gboolean valid = FALSE;
    GtkTreePath *path;
    GtkTreeIter previter;
    GtkTreeIter localiter;

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        if (idledata->has_chapters) {
            valid = FALSE;
            send_command("seek_chapter -2 0\n", FALSE);
        } else {
            valid = prev_item_in_playlist(&iter);
        }
    } else {
        // for the case where we have rolled off the end of the list, allow prev to work
        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter);
        do {
            previter = localiter;
            valid = TRUE;
            gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter);
        } while (gtk_list_store_iter_is_valid(playliststore, &localiter));
        iter = previter;

        if (idledata->has_chapters) {
            valid = FALSE;
            send_command("seek_chapter -1 0\n", FALSE);
        }
    }

    if (valid) {
        dontplaynext = TRUE;
        play_iter(&iter, 0);
        if (autopause) {
            autopause = FALSE;
            gtk_widget_set_sensitive(play_event_box, TRUE);
            gtk_image_set_from_stock(GTK_IMAGE(image_play), GTK_STOCK_MEDIA_PLAY, button_size);
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
                gtk_image_set_from_stock(GTK_IMAGE(image_play), GTK_STOCK_MEDIA_PLAY, button_size);
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

    if (softvol || audio_device.type == AUDIO_TYPE_SOFTVOL) {
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
            cmd = g_strdup_printf("volume %i 1\n", (gint) (vol * 100.0));
            send_command(cmd, TRUE);
            g_free(cmd);
        }

        if (remember_softvol) {
            volume_softvol = vol;
            set_software_volume(&volume_softvol);
        }
    } else {
        gm_audio_set_volume(&audio_device, gtk_range_get_value(range));
    }

    dbus_send_rpsignal_with_double("RP_Volume", gtk_range_get_value(GTK_RANGE(vol_slider)));

}

#ifdef GTK2_12_ENABLED
void vol_button_value_changed_callback(GtkScaleButton * volume, gdouble value, gpointer data)
{
    gchar *cmd;

    if (softvol || audio_device.type == AUDIO_TYPE_SOFTVOL) {
        if (idledata->mute && value > 0.0) {
            cmd = g_strdup_printf("mute 0\n");
            send_command(cmd, TRUE);
            g_free(cmd);
            idledata->mute = FALSE;
        }
        if (!idledata->mute && value == 0.0) {
            cmd = g_strdup_printf("mute 1\n");
            send_command(cmd, TRUE);
            g_free(cmd);
            idledata->mute = TRUE;
        } else {
            cmd = g_strdup_printf("volume %i 1\n", (gint) (value * 100.0));
            send_command(cmd, TRUE);
            g_free(cmd);
            gm_audio_set_volume(&audio_device, value);
        }

        if (remember_softvol) {
            volume_softvol = value;
            set_software_volume(&volume_softvol);
        }
    } else {
        gm_audio_set_volume(&audio_device, value);
    }

    dbus_send_rpsignal_with_double("RP_Volume", value * 100.0);

}
#endif

gboolean slide_panel_away(gpointer data)
{
    if (!showcontrols)
        return FALSE;

    if (fs_controls == NULL) {
        return FALSE;
    }

    if (!(fullscreen || always_hide_after_timeout)) {
        //gtk_widget_set_size_request(fs_controls, -1, -1);
        return FALSE;
    }

    if (g_mutex_trylock(slide_away)) {
        // mutex is locked now, unlock it and exit function
        g_mutex_unlock(slide_away);
        return FALSE;
    }
    // mutex was already locked, this is good since we only want to do the animation if locked

    if (GTK_IS_WIDGET(fs_controls) && get_visible(fs_controls) && mouse_over_controls == FALSE) {
        gtk_widget_hide(fs_controls);
        g_mutex_unlock(slide_away);
        return FALSE;
        /*
           if (fs_controls->allocation.height <= 1) {
           gtk_widget_hide(fs_controls);
           g_mutex_unlock(slide_away);
           return FALSE;
           } else {
           //if (disable_animation) {
           gtk_widget_set_size_request(fs_controls, fs_controls->allocation.width, 0);
           //} else {
           //    gtk_widget_set_size_request(fs_controls, fs_controls->allocation.width,
           //                                fs_controls->allocation.height - 1);
           //}
           return TRUE;
           }
         */
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
        && (get_visible(controls_box) || fs_controls != NULL)
        && mouse_over_controls == FALSE) {
        g_get_current_time(&currenttime);
        g_time_val_add(&currenttime, -auto_hide_timeout * G_USEC_PER_SEC);
        if (last_movement_time > 0 && currenttime.tv_sec > last_movement_time) {
            //if (g_mutex_trylock(slide_away)) {
            //    g_timeout_add(40, slide_panel_away, NULL);
            //}
            // gtk_widget_hide(controls_box);
            hide_fs_controls();
        }

    }

    g_get_current_time(&currenttime);
    g_time_val_add(&currenttime, -auto_hide_timeout * G_USEC_PER_SEC);
    /*
       printf("%i, %i, %i, %i, %i, %i\n",   currenttime.tv_sec,
       get_visible(menu_file),
       get_visible(menu_edit),
       get_visible(menu_view),
       get_visible(menu_help),
       gtk_tree_view_get_enable_search(GTK_TREE_VIEW(list)));
     */
    if (get_visible(GTK_WIDGET(menu_file))
        || get_visible(GTK_WIDGET(menu_edit))
        || get_visible(GTK_WIDGET(menu_view))
        || get_visible(GTK_WIDGET(menu_help))
        || gtk_tree_view_get_enable_search(GTK_TREE_VIEW(list))) {

        gdk_window_set_cursor(get_window(window), NULL);

    } else {

        if (last_movement_time > 0 && currenttime.tv_sec > last_movement_time) {
            cursor_source = gdk_pixmap_new(NULL, 1, 1, 1);
            cursor = gdk_cursor_new_from_pixmap(cursor_source, cursor_source, &cursor_color, &cursor_color, 0, 0);
            gdk_pixmap_unref(cursor_source);
            gdk_window_set_cursor(get_window(window), cursor);
            gdk_cursor_unref(cursor);
        }
    }

    return FALSE;
}

gboolean make_panel_and_mouse_visible(gpointer data)
{

    g_mutex_unlock(slide_away);
    if (showcontrols && GTK_IS_WIDGET(controls_box)) {
        //gtk_widget_set_size_request(controls_box, -1, -1);
        //gtk_widget_show(controls_box);
        show_fs_controls();

    }
    gdk_window_set_cursor(get_window(window), NULL);

    return FALSE;
}

gboolean fs_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen), !fullscreen);

    return FALSE;
}

gboolean enter_button_callback(GtkWidget * widget, GdkEventCrossing * event, gpointer data)
{
    GtkAllocation alloc;

    get_allocation(widget, &alloc);
    gdk_draw_rectangle(get_window(widget), gtk_widget_get_style(widget)->fg_gc[GTK_STATE_NORMAL],
                       FALSE, 0, 0, alloc.width - 1, alloc.height - 1);

    in_button = TRUE;
    return FALSE;
}

gboolean leave_button_callback(GtkWidget * widget, GdkEventCrossing * event, gpointer data)
{
    GtkAllocation alloc;

    get_allocation(widget, &alloc);
    gdk_draw_rectangle(get_window(widget), gtk_widget_get_style(widget)->bg_gc[GTK_STATE_NORMAL],
                       FALSE, 0, 0, alloc.width - 1, alloc.height - 1);
    in_button = FALSE;
    return FALSE;
}

gboolean fs_controls_entered(GtkWidget * widget, GdkEventCrossing * event, gpointer data)
{
    mouse_over_controls = TRUE;
    return FALSE;
}

gboolean fs_controls_left(GtkWidget * widget, GdkEventCrossing * event, gpointer data)
{
    mouse_over_controls = FALSE;
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

        if (filename != NULL) {
            g_slist_foreach(filename, &add_item_to_playlist_callback, NULL);
            g_slist_free(filename);

            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            play_iter(&iter, 0);
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
            play_iter(&iter, 0);
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
    gtk_entry_set_activates_default(GTK_ENTRY(open_location), TRUE);
    item_box = gtk_hbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(item_box), label, FALSE, FALSE, 12);
    gtk_box_pack_end(GTK_BOX(item_box), open_location, TRUE, TRUE, 0);

    button_box = gtk_hbox_new(FALSE, 6);
    cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    open_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
#ifdef GTK2_22_ENABLED
    gtk_widget_set_can_default(open_button, TRUE);
#else
    GTK_WIDGET_SET_FLAGS(open_button, GTK_CAN_DEFAULT);
#endif
    gtk_box_pack_end(GTK_BOX(button_box), open_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(button_box), cancel_button, FALSE, FALSE, 0);

    g_signal_connect_swapped(GTK_OBJECT(cancel_button), "clicked", G_CALLBACK(config_close), open_window);
    g_signal_connect_swapped(GTK_OBJECT(open_button), "clicked", G_CALLBACK(open_location_callback), open_window);

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
    if (idledata->device != NULL) {
        g_free(idledata->device);
        idledata->device = NULL;
    }
    parse_dvd("dvd://");

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
        play_iter(&iter, 0);
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
        idledata->device = g_strdup(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));

        parse_dvd("dvd://");

        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
            play_iter(&iter, 0);
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
        idledata->device = g_strdup(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));

        parse_dvd("dvd://");

        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
            play_iter(&iter, 0);
        }
    }
    if (GTK_IS_WIDGET(dialog))
        gtk_widget_destroy(dialog);

}

void menuitem_open_dvdnav_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_list_store_clear(playliststore);
    if (idledata->device != NULL) {
        g_free(idledata->device);
        idledata->device = NULL;
    }
    dvdnav_title_is_menu = TRUE;
    add_item_to_playlist("dvdnav://", 0);
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
    play_iter(&iter, 0);
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
        idledata->device = g_strdup(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));

        dvdnav_title_is_menu = TRUE;
        add_item_to_playlist("dvdnav://", 0);
        gtk_widget_show(menu_event_box);

        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
            play_iter(&iter, 0);
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
        idledata->device = g_strdup(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));

        dvdnav_title_is_menu = TRUE;
        add_item_to_playlist("dvdnav://", 0);
        gtk_widget_show(menu_event_box);

        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
            play_iter(&iter, 0);
        }
    }
    if (GTK_IS_WIDGET(dialog))
        gtk_widget_destroy(dialog);

}

void menuitem_open_acd_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_list_store_clear(playliststore);
    parse_playlist("cdda://");

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
        play_iter(&iter, 0);
    }

}

void menuitem_open_vcd_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_list_store_clear(playliststore);
    parse_playlist("vcd://");

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
        play_iter(&iter, 0);
    }

}

void menuitem_open_atv_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_list_store_clear(playliststore);
    add_item_to_playlist("tv://", 0);

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
        play_iter(&iter, 0);
    }
}

void menuitem_open_recent_callback(GtkRecentChooser * chooser, gpointer data)
{
    gint playlist = 0;
    gchar *uri;
    gint count;
    GtkTreeViewColumn *column;
    gchar *coltitle;

    dontplaynext = TRUE;
    mplayer_shutdown();
    gtk_list_store_clear(playliststore);

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
        play_iter(&iter, 0);
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
    gtk_recent_filter_add_application(recent_filter, "gnome-mplayer");
    gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(menuitem_file_recent_items), recent_filter);
    gtk_recent_chooser_set_show_tips(GTK_RECENT_CHOOSER(menuitem_file_recent_items), TRUE);
    gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(menuitem_file_recent_items), GTK_RECENT_SORT_MRU);
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
    FILE *fi;                   // FILE pointer to use to open the conf file
    gchar *mpconf;

    gtk_list_store_clear(playliststore);

    mpconf = g_strdup_printf("%s/.mplayer/channels.conf", g_getenv("HOME"));
    fi = fopen(mpconf, "r");    // Make sure this is pointing to
    // the appropriate file
    if (fi != NULL) {
        parseChannels(fi);
        fclose(fi);
    } else {
        printf("Unable to open the config file\n");     //can change this to whatever error message system is used
    }
    g_free(mpconf);

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        play_iter(&iter, 0);
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
                                            GTK_BUTTONS_CLOSE, "%s", msg);
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

void about_url_hook(GtkAboutDialog * about, const char *link, gpointer data)
{
#ifdef GTK2_14_ENABLED
    GError *error = NULL;

    if (!gtk_show_uri(gtk_widget_get_screen(GTK_WIDGET(about)), link, gtk_get_current_event_time(), &error)) {
        g_error_free(error);
    }
#endif
}

void menuitem_about_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *authors[] = { "Kevin DeKorte", "James Carthew", "Diogo Franco", "Icons provided by Victor Castillejo",
        NULL
    };
#ifdef GTK2_14_ENABLED
    gtk_about_dialog_set_url_hook(about_url_hook, NULL, NULL);
#endif
    gtk_show_about_dialog(GTK_WINDOW(window), "name", _("GNOME MPlayer"), "authors", authors,
                          "copyright", "Copyright © 2007-2011 Kevin DeKorte", "comments",
                          _("A media player for GNOME that uses MPlayer"), "version", VERSION,
                          "license",
                          _
                          ("Gnome MPlayer is free software; you can redistribute it and/or modify it under\nthe terms of the GNU General Public License as published by the Free\nSoftware Foundation; either version 2 of the License, or (at your option)\nany later version."
                           "\n\nGnome MPlayer is distributed in the hope that it will be useful, but WITHOUT\nANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\nFITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for\nmore details."
                           "\n\nYou should have received a copy of the GNU General Public License along with\nGnome MPlayer if not, write to the\n\nFree Software Foundation, Inc.,\n51 Franklin St, Fifth Floor\nBoston, MA 02110-1301 USA")


                          ,
                          "website", "http://code.google.com/p/gnome-mplayer/",
                          "translator-credits",
                          "Bulgarian - Adrian Dimitrov\n"
                          "Czech - Petr Pisar\n"
                          "Chinese (simplified) - Wenzheng Hu\n"
                          "Chinese (Hong Kong) - Hialan Liu\n"
                          "Chinese (Taiwan) - Hailan Liu\n"
                          "Dutch - Mark Huijgen\n"
                          "Finnish - Kristian Polso & Tuomas Lähteenmäki\n"
                          "French - Alexandre Bedot\n"
                          "German - Tim Buening\n"
                          "Greek - Γεώργιος Γεωργάς\n"
                          "Hungarian - Kulcsár Kázmér\n"
                          "Italian - Cesare Tirabassi\n"
                          "Japanese - Munehiro Yamamoto\n"
                          "Korean - ByeongSik Jeon\n"
                          "Lithuanian - Mindaugas B.\n"
                          "Polish - Julian Sikorski\n"
                          "Portugese - LL\n"
                          "Russian - Dmitry Stropaloff and Denis Koryavov\n"
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

    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(playliststore), -2, GTK_SORT_ASCENDING);

    random_order = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_edit_random));
    if (random_order) {
        randomize_playlist(playliststore);
    } else {
        reset_playlist_order(playliststore);
    }

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        if (GTK_IS_TREE_SELECTION(selection)) {
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            if (iterfilename != NULL) {
                do {
                    gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &localfilename, -1);
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

    if (GTK_IS_WIDGET(media_hbox)) {
#ifdef GTK2_20_ENABLED
        if (!gtk_widget_get_realized(media_hbox))
            gtk_widget_realize(media_hbox);
#else
        if (!GTK_WIDGET_REALIZED(media_hbox))
            gtk_widget_realize(media_hbox);
#endif
        g_idle_add(set_adjust_layout, NULL);
        // adjust_layout();
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
    GtkAllocation alloc;

    gtk_widget_set_size_request(fixed, -1, -1);
    gtk_widget_set_size_request(drawing_area, -1, -1);
    if (idle->width > 0 && idle->height > 0) {
        gtk_widget_set_size_request(fixed, idle->width, idle->height);
        gtk_widget_set_size_request(drawing_area, idle->width, idle->height);
        total_height = idle->height;
        get_allocation(menubar, &alloc);
        total_height += alloc.height;
        if (get_visible(media_hbox)) {
            get_allocation(media_hbox, &alloc);
            total_height += alloc.height;
        }

        if (GTK_IS_WIDGET(details_table) && get_visible(details_table)) {
            get_allocation(details_vbox, &alloc);
            total_height += alloc.height;
        }
        if (GTK_IS_WIDGET(audio_meter) && get_visible(audio_meter)) {
            get_allocation(audio_meter, &alloc);
            total_height += alloc.height;
        }

        total_width = idle->width;
        if (vertical_layout) {
            if (GTK_IS_WIDGET(plvbox) && get_visible(plvbox)) {
                get_allocation(plvbox, &alloc);
                total_height = total_height + alloc.height;
            }
        } else {
            if (GTK_IS_WIDGET(plvbox) && get_visible(plvbox)) {
                get_allocation(plvbox, &alloc);
                total_width = total_width + alloc.width;
            }
        }
        if (GTK_IS_WIDGET(controls_box) && get_visible(controls_box)) {
            get_allocation(controls_box, &alloc);
            total_height += alloc.height;
        }
        gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
    }
}

void menuitem_view_twotoone_callback(GtkMenuItem * menuitem, void *data)
{
    gint total_height, total_width;
    IdleData *idle = (IdleData *) data;
    GtkAllocation alloc;

    gtk_widget_set_size_request(fixed, -1, -1);
    gtk_widget_set_size_request(drawing_area, -1, -1);
    if (idle->width > 0 && idle->height > 0) {
        gtk_widget_set_size_request(fixed, idle->width * 2, idle->height * 2);
        gtk_widget_set_size_request(drawing_area, idle->width * 2, idle->height * 2);
        total_height = idle->height * 2;
        get_allocation(menubar, &alloc);
        total_height += alloc.height;
        if (get_visible(media_hbox)) {
            get_allocation(media_hbox, &alloc);
            total_height += alloc.height;
        }

        if (GTK_IS_WIDGET(details_table) && get_visible(details_table)) {
            get_allocation(details_vbox, &alloc);
            total_height += alloc.height;
        }
        if (GTK_IS_WIDGET(audio_meter) && get_visible(audio_meter)) {
            get_allocation(audio_meter, &alloc);
            total_height += alloc.height;
        }

        total_width = idle->width * 2;
        if (vertical_layout) {
            if (GTK_IS_WIDGET(plvbox) && get_visible(plvbox)) {
                get_allocation(plvbox, &alloc);
                total_height = total_height + alloc.height;
            }
        } else {
            if (GTK_IS_WIDGET(plvbox) && get_visible(plvbox)) {
                get_allocation(plvbox, &alloc);
                total_width = total_width + alloc.width;
            }
        }
        if (GTK_IS_WIDGET(controls_box) && get_visible(controls_box)) {
            get_allocation(controls_box, &alloc);
            total_height += alloc.height;
        }
        gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
    }
}

void menuitem_view_onetotwo_callback(GtkMenuItem * menuitem, void *data)
{
    gint total_height, total_width;
    IdleData *idle = (IdleData *) data;
    GtkAllocation alloc;

    gtk_widget_set_size_request(fixed, -1, -1);
    gtk_widget_set_size_request(drawing_area, -1, -1);
    if (idle->width > 0 && idle->height > 0) {
        gtk_widget_set_size_request(fixed, idle->width / 2, idle->height / 2);
        gtk_widget_set_size_request(drawing_area, idle->width / 2, idle->height / 2);
        total_height = idle->height / 2;
        get_allocation(menubar, &alloc);
        total_height += alloc.height;
        if (get_visible(media_hbox)) {
            get_allocation(media_hbox, &alloc);
            total_height += alloc.height;
        }

        if (GTK_IS_WIDGET(details_table) && get_visible(details_table)) {
            get_allocation(details_vbox, &alloc);
            total_height += alloc.height;
        }
        if (GTK_IS_WIDGET(audio_meter) && get_visible(audio_meter)) {
            get_allocation(audio_meter, &alloc);
            total_height += alloc.height;
        }

        total_width = idle->width / 2;
        if (vertical_layout) {
            if (GTK_IS_WIDGET(plvbox) && get_visible(plvbox)) {
                get_allocation(plvbox, &alloc);
                total_height = total_height + alloc.height;
            }
        } else {
            if (GTK_IS_WIDGET(plvbox) && get_visible(plvbox)) {
                get_allocation(plvbox, &alloc);
                total_width = total_width + alloc.width;
            }
        }
        if (GTK_IS_WIDGET(controls_box) && get_visible(controls_box)) {
            get_allocation(controls_box, &alloc);
            total_height += alloc.height;
        }
        gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
    }
}

void menuitem_view_controls_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols),
                                   !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols)));
}

void menuitem_view_subtitles_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *cmd;
//      set_sub_visibility
    cmd =
        g_strdup_printf("set_property sub_visibility %i\n",
                        gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_subtitles)));
    send_command(cmd, TRUE);
    g_free(cmd);

    cmd =
        g_strdup_printf("set_property sub_forced_only %i\n",
                        !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_subtitles)));
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

void menuitem_edit_set_audiofile_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *audiofile = NULL;
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

        dialog = gtk_file_chooser_dialog_new(_("Set AudioFile"),
                                             GTK_WINDOW(window),
                                             GTK_FILE_CHOOSER_ACTION_OPEN,
                                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                             GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        gtk_widget_show(dialog);
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), path);
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            audiofile = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            gtk_list_store_set(playliststore, &iter, AUDIOFILE_COLUMN, audiofile, -1);
        }
        gtk_widget_destroy(dialog);

        if (audiofile != NULL) {
            dontplaynext = TRUE;

            if (idledata->streaming)
                play_iter(&iter, 0);
            else
                play_iter(&iter, idledata->position);
        }
    }
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
    GdkScreen *screen;
    GdkRectangle rect;
    gint wx, wy;
    gint x, y;

    if (GTK_CHECK_MENU_ITEM(menuitem) == GTK_CHECK_MENU_ITEM(menuitem_view_fullscreen)) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen),
                                       gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_fullscreen)));
        return;
    } else {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_fullscreen),
                                       gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen)));
    }

    if (fullscreen) {
        hide_fs_controls();

        if (embed_window == 0) {
            skip_fixed_allocation_on_show = TRUE;
            gtk_window_unfullscreen(GTK_WINDOW(window));
        } else {
#ifdef GTK2_20_ENABLED
            if (gtk_widget_get_mapped(window))
                gtk_widget_unmap(window);
#else
            if (GTK_WIDGET_MAPPED(window))
                gtk_widget_unmap(window);
#endif
            gdk_window_reparent(get_window(window), gdk_window_lookup(embed_window), 0, 0);
            gtk_widget_map(window);
            gtk_window_move(GTK_WINDOW(window), 0, 0);
            gtk_widget_set_size_request(fixed, window_x, window_y - 1);
            gdk_window_resize(get_window(window), window_x, window_y - 1);
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

        }
        gtk_widget_show(menubar);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_controls))) {
            gtk_widget_show(controls_box);
        }
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_details))) {
            gtk_widget_show(details_vbox);
        }
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_meter))) {
            gtk_widget_show(audio_meter);
        }
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_info))) {
            gtk_widget_show(media_hbox);
        }
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist))) {
            gtk_widget_show(plvbox);
        }
        skip_fixed_allocation_on_show = FALSE;
        gtk_window_resize(GTK_WINDOW(window), last_window_width, last_window_height);

    } else {
        gtk_window_get_size(GTK_WINDOW(window), &last_window_width, &last_window_height);
        gtk_widget_hide(menubar);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_controls))) {
            gtk_widget_hide(controls_box);
        }
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_details))) {
            gtk_widget_hide(details_vbox);
        }
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_meter))) {
            gtk_widget_hide(audio_meter);
        }
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_info))) {
            gtk_widget_hide(media_hbox);
        }
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist))) {
            gtk_widget_hide(plvbox);
        }
        if (embed_window == 0) {
            // --fullscreen option doesn't work without this event flush
            while (gtk_events_pending())
                gtk_main_iteration();

            gtk_window_fullscreen(GTK_WINDOW(window));
        } else {
            fs_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#ifdef GTK2_18_ENABLED
            gdk_window_ensure_native(gtk_widget_get_window(fs_window));
#else
#ifdef GTK2_14_ENABLED
#ifdef X11_ENABLED
            GDK_WINDOW_XID(get_window(GTK_WIDGET(fs_window)));
#endif
#endif
#endif
            gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
            g_object_set_property(G_OBJECT(window), "allow-shrink", &ALLOW_SHRINK_TRUE);
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
            g_signal_connect(GTK_OBJECT(fs_window), "key_press_event", G_CALLBACK(window_key_callback), NULL);
            g_signal_connect(GTK_OBJECT(fs_window), "motion_notify_event", G_CALLBACK(motion_notify_callback), NULL);

            screen = gtk_window_get_screen(GTK_WINDOW(window));
            gtk_window_set_screen(GTK_WINDOW(fs_window), screen);
            gtk_window_set_title(GTK_WINDOW(fs_window), _("Gnome MPlayer Fullscreen"));
            gdk_screen_get_monitor_geometry(screen,
                                            gdk_screen_get_monitor_at_window(screen, get_window(window)), &rect);

            x = rect.width;
            y = rect.height;
            gtk_widget_realize(fs_window);

            gdk_window_get_root_origin(get_window(window), &wx, &wy);
            gtk_window_move(GTK_WINDOW(fs_window), wx, wy);

            gtk_widget_show(fs_window);
#ifdef X11_ENABLED
            XReparentWindow(GDK_WINDOW_XDISPLAY(get_window(window)),
                            GDK_WINDOW_XWINDOW(get_window(window)), GDK_WINDOW_XWINDOW(get_window(fs_window)), 0, 0);
#else
            gdk_window_reparent(get_window(window), get_window(fs_window), 0, 0);
#endif
            gtk_widget_map(window);
            gtk_window_fullscreen(GTK_WINDOW(fs_window));
            gtk_window_resize(GTK_WINDOW(window), rect.width, rect.height);
            if (window_x < 250) {
                gtk_widget_show(fs_event_box);
            }
            if (window_x < 170) {
                gtk_widget_show(GTK_WIDGET(tracker));
            }
        }
    }

    fullscreen = !fullscreen;



/*
    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen))) {
        if (embed_window == 0) {
            gtk_window_unfullscreen(GTK_WINDOW(window));
        }
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_config), TRUE);

        make_panel_and_mouse_visible(NULL);
        hide_fs_controls();

        if (embed_window != 0) {
            while (gtk_events_pending())
                gtk_main_iteration();

            if (GTK_WIDGET_MAPPED(window))
                gtk_widget_unmap(window);

            //XReparentWindow(GDK_WINDOW_XDISPLAY(get_window(window)),
            //                GDK_WINDOW_XWINDOW(get_window(window)), embed_window, 0, 0);
            gdk_window_reparent(get_window(window), gdk_window_lookup(embed_window), 0, 0);
            gtk_widget_map(window);
            gtk_window_move(GTK_WINDOW(window), 0, 0);
            gtk_widget_set_size_request(fixed, window_x, window_y - 1);
            //XResizeWindow(GDK_WINDOW_XDISPLAY(get_window(window)),
            //              GDK_WINDOW_XWINDOW(get_window(window)), window_x, window_y - 1);
            gdk_window_resize(get_window(window), window_x, window_y - 1);
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
            fullscreen = 0;
            g_idle_add(set_show_controls, idledata);
        } else {
            restore_controls =
                gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols));
            if (restore_controls)
            	gtk_widget_show(controls_box);
            //while (gtk_events_pending())
            //    gtk_main_iteration();
            //gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), restore_info);
            //gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_details),
            //                               restore_details);
            //gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist),
            //                               restore_playlist);
        }

        //while (gtk_events_pending())
        //    gtk_main_iteration();
    } else {
        if (embed_window != 0) {

            restore_controls =
                gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols));
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), FALSE);

            fs_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#ifdef GTK2_14_ENABLED
            GDK_WINDOW_XID(get_window(GTK_WIDGET(fs_window)));
#endif
            //gtk_window_set_policy(GTK_WINDOW(fs_window), TRUE, TRUE, TRUE);
            gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
            g_object_set_property(G_OBJECT(window), "allow-shrink", &ALLOW_SHRINK_TRUE);
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
            g_signal_connect(GTK_OBJECT(fs_window), "motion_notify_event",
                             G_CALLBACK(motion_notify_callback), NULL);

            screen = gtk_window_get_screen(GTK_WINDOW(window));
            gtk_window_set_screen(GTK_WINDOW(fs_window), screen);
            gtk_window_set_title(GTK_WINDOW(fs_window), _("Gnome MPlayer Fullscreen"));
            gdk_screen_get_monitor_geometry(screen,
                                            gdk_screen_get_monitor_at_window
                                            (screen, get_window(window)), &rect);

            x = rect.width;
            y = rect.height;
            gtk_widget_realize(fs_window);

            gdk_window_get_root_origin(get_window(window), &wx, &wy);
            gtk_window_move(GTK_WINDOW(fs_window), wx, wy);

            gtk_widget_show(fs_window);
            //gtk_window_fullscreen(GTK_WINDOW(fs_window));
            //if (GTK_WIDGET_MAPPED(window))
            //    gtk_widget_unmap(window);
#ifdef X11_ENABLED
            XReparentWindow(GDK_WINDOW_XDISPLAY(get_window(window)),
                            GDK_WINDOW_XWINDOW(get_window(window)),
                            GDK_WINDOW_XWINDOW(get_window(fs_window)), 0, 0);
#else
            gdk_window_reparent(get_window(window), get_window(fs_window), 0, 0);
#endif
            gtk_widget_map(window);
            gtk_window_fullscreen(GTK_WINDOW(fs_window));
            //XResizeWindow(GDK_WINDOW_XDISPLAY(get_window(window)),
            //              GDK_WINDOW_XWINDOW(get_window(window)), rect.width, rect.height - 1);
            //gdk_window_resize(get_window(window), rect.width, rect.height - 1);
            gtk_window_resize(GTK_WINDOW(window), rect.width, rect.height);
            //gtk_widget_set_size_request(fixed, rect.width, rect.height);
            if (window_x < 250) {
                gtk_widget_show(fs_event_box);
            }
            if (window_x < 170) {
                gtk_widget_show(GTK_WIDGET(tracker));
            }
            //while (gtk_events_pending())
            //    gtk_main_iteration();

            if (restore_controls) {
                fullscreen = 1;
                idledata->showcontrols = 1;
                show_fs_controls();
                g_idle_add(set_show_controls, idledata);
            }
        } else {
            restore_playlist =
                gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist));
            restore_details =
                gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_details));
            restore_info = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_info));
            restore_controls =
                gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols));
            g_object_get(pane, "position", &restore_pane, NULL);
            //gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist), FALSE);
            //gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_details), FALSE);
            //gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), FALSE);
            //gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), FALSE);
            //while (gtk_events_pending())
            //    gtk_main_iteration();
        }
        fullscreen = 1;

        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_config), FALSE);
        if (embed_window == 0) {
            gtk_window_fullscreen(GTK_WINDOW(window));
        }
        motion_notify_callback(NULL, NULL, NULL);
        //if (embed_window == 0)
        //    g_idle_add(set_adjust_layout, NULL);
        //adjust_layout();
    }
    */
#ifdef ENABLE_PANSCAN
    send_command("vo_fullscreen 1\n", TRUE);
    send_command("panscan 0 1\n", TRUE);
#endif

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
    GtkAllocation alloc;

    if (GTK_CHECK_MENU_ITEM(menuitem) == GTK_CHECK_MENU_ITEM(menuitem_view_controls)) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols),
                                       gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_controls)));
        return;
    } else {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_controls),
                                       gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols)));
    }

    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols))) {
        if (GTK_IS_WIDGET(button_event_box)) {
            gtk_widget_hide_all(button_event_box);
        }

        if (fullscreen) {
            show_fs_controls();
        } else {
            gtk_widget_set_size_request(controls_box, -1, -1);
            gtk_widget_show(controls_box);
            if (!fullscreen && embed_window == 0) {
                gtk_window_get_size(GTK_WINDOW(window), &width, &height);
                get_allocation(controls_box, &alloc);
                gtk_window_resize(GTK_WINDOW(window), width, height + alloc.height);
            }
        }

        showcontrols = TRUE;
    } else {
        if (fullscreen) {
            hide_fs_controls();
        } else {
            gtk_widget_hide(controls_box);
            if (!fullscreen && embed_window == 0) {
                gtk_window_get_size(GTK_WINDOW(window), &width, &height);
                get_allocation(controls_box, &alloc);
                gtk_window_resize(GTK_WINDOW(window), width, height - alloc.height);
            }
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
    vo = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(config_vo)))));

    audio_device_name = g_strdup(gmtk_output_combo_box_get_active_description(GMTK_OUTPUT_COMBO_BOX(config_ao)));

    if (audio_device.description != NULL) {
        g_free(audio_device.description);
        audio_device.description = NULL;
    }
    audio_device.description = g_strdup(audio_device_name);
    gm_audio_update_device(&audio_device);
    gm_audio_get_volume(&audio_device);
    gm_audio_set_server_volume_update_callback(&audio_device, set_volume);

#ifdef HAVE_ASOUNDLIB
    if (audio_device.alsa_mixer != NULL) {
        g_free(audio_device.alsa_mixer);
        audio_device.alsa_mixer = NULL;
    }
    audio_device.alsa_mixer = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(config_mixer)))));
#endif

    if (alang != NULL) {
        g_free(alang);
        alang = NULL;
    }
    alang = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(config_alang)))));

    if (slang != NULL) {
        g_free(slang);
        slang = NULL;
    }
    slang = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(config_slang)))));

    if (metadata_codepage != NULL) {
        g_free(metadata_codepage);
        metadata_codepage = NULL;
    }
    metadata_codepage = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(config_metadata_codepage)))));

    if (subtitle_codepage != NULL) {
        g_free(subtitle_codepage);
        subtitle_codepage = NULL;
    }
    subtitle_codepage = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(config_subtitle_codepage)))));

    if (mplayer_dvd_device != NULL) {
        g_free(mplayer_dvd_device);
        mplayer_dvd_device = NULL;
    }
    mplayer_dvd_device = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(config_mplayer_dvd_device)))));

    audio_channels = gtk_combo_box_get_active(GTK_COMBO_BOX(config_audio_channels));
    use_hw_audio = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_use_hw_audio));
    cache_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(config_cachesize));
    plugin_audio_cache_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(config_plugin_audio_cache_size));
    plugin_video_cache_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(config_plugin_video_cache_size));
    old_disable_framedrop = disable_framedrop;
    disable_deinterlace = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_deinterlace));
    disable_framedrop = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_framedrop));
    disable_ass = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_ass));
    disable_embeddedfonts = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_embeddedfonts));
    disable_pause_on_click = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_pause_on_click));
    disable_animation = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_disable_animation));
    oldosd = osdlevel;
    osdlevel = (gint) gtk_range_get_value(GTK_RANGE(config_osdlevel));
    pplevel = (gint) gtk_range_get_value(GTK_RANGE(config_pplevel));
    softvol = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_softvol));
    remember_softvol = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_remember_softvol));
    volume_gain = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(config_volume_gain));
    verbose = (gint) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_verbose));
    mouse_wheel_changes_volume = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_mouse_wheel));
    playlist_visible = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_playlist_visible));
    details_visible = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_details_visible));
    use_mediakeys = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_use_mediakeys));
    use_defaultpl = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_use_defaultpl));
    use_xscrnsaver = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_use_xscrnsaver));
    vertical_layout = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_vertical_layout));
    single_instance = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_single_instance));
    replace_and_play = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_replace_and_play));
    bring_to_front = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_bring_to_front));
#ifdef NOTIFY_ENABLED
    show_notification = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_show_notification));
#endif
#ifdef GTK2_12_ENABLED
    show_status_icon = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_show_status_icon));
    if (GTK_IS_STATUS_ICON(status_icon)) {
        gtk_status_icon_set_visible(status_icon, show_status_icon);
    } else {
        if (show_status_icon) {
            GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
            if (gtk_icon_theme_has_icon(icon_theme, "gnome-mplayer")) {
                status_icon = gtk_status_icon_new_from_icon_name("gnome-mplayer");
            } else {
                status_icon = gtk_status_icon_new_from_pixbuf(pb_icon);
            }
            gtk_status_icon_set_visible(status_icon, show_status_icon);
            g_signal_connect(status_icon, "activate", G_CALLBACK(status_icon_callback), NULL);
            g_signal_connect(status_icon, "popup_menu", G_CALLBACK(status_icon_context_callback), NULL);

        } else {
            gtk_status_icon_set_visible(status_icon, show_status_icon);
        }
    }

#endif
    forcecache = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_forcecache));
    remember_loc = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_remember_loc));
    resize_on_new_media = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_resize_on_new_media));
    keep_on_top = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_keep_on_top));
    gtk_window_set_keep_above(GTK_WINDOW(window), keep_on_top);

    if (subtitlefont != NULL) {
        g_free(subtitlefont);
        subtitlefont = NULL;
    }
    subtitlefont = g_strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(config_subtitle_font)));
    subtitle_scale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(config_subtitle_scale));
    gtk_color_button_get_color(GTK_COLOR_BUTTON(config_subtitle_color), &sub_color);
    if (subtitle_color != NULL) {
        g_free(subtitle_color);
        subtitle_color = NULL;
    }
    subtitle_color = g_strdup_printf("%02x%02x%02x00", sub_color.red >> 8, sub_color.green >> 8, sub_color.blue >> 8);
    subtitle_outline = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_subtitle_outline));
    subtitle_shadow = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_subtitle_shadow));
    subtitle_margin = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(config_subtitle_margin));
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

    gm_pref_store_set_string(gm_store, AUDIO_DEVICE_NAME, audio_device_name);

#ifndef HAVE_ASOUNDLIB
    gm_pref_store_set_int(gm_store, VOLUME, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(config_volume)));
#endif
    gm_pref_store_set_int(gm_store, AUDIO_CHANNELS, audio_channels);
    gm_pref_store_set_boolean(gm_store, USE_HW_AUDIO, use_hw_audio);
    gm_pref_store_set_int(gm_store, CACHE_SIZE, cache_size);
    gm_pref_store_set_int(gm_store, PLUGIN_AUDIO_CACHE_SIZE, plugin_audio_cache_size);
    gm_pref_store_set_int(gm_store, PLUGIN_VIDEO_CACHE_SIZE, plugin_video_cache_size);
    gm_pref_store_set_string(gm_store, ALSA_MIXER, audio_device.alsa_mixer);
    gm_pref_store_set_int(gm_store, OSDLEVEL, osdlevel);
    gm_pref_store_set_int(gm_store, PPLEVEL, pplevel);
    gm_pref_store_set_boolean(gm_store, SOFTVOL, softvol);
    gm_pref_store_set_boolean(gm_store, REMEMBER_SOFTVOL, remember_softvol);
    gm_pref_store_set_int(gm_store, VOLUME_GAIN, volume_gain);
    gm_pref_store_set_boolean(gm_store, FORCECACHE, forcecache);
    gm_pref_store_set_boolean(gm_store, DISABLEASS, disable_ass);
    gm_pref_store_set_boolean(gm_store, DISABLEEMBEDDEDFONTS, disable_embeddedfonts);
    gm_pref_store_set_boolean(gm_store, DISABLEDEINTERLACE, disable_deinterlace);
    gm_pref_store_set_boolean(gm_store, DISABLEFRAMEDROP, disable_framedrop);
    gm_pref_store_set_boolean(gm_store, DISABLEPAUSEONCLICK, disable_pause_on_click);
    gm_pref_store_set_boolean(gm_store, DISABLEANIMATION, disable_animation);
    gm_pref_store_set_boolean(gm_store, SHOWPLAYLIST, playlist_visible);
    gm_pref_store_set_boolean(gm_store, SHOWDETAILS, details_visible);
    gm_pref_store_set_boolean(gm_store, USE_MEDIAKEYS, use_mediakeys);
    gm_pref_store_set_boolean(gm_store, USE_DEFAULTPL, use_defaultpl);
    gm_pref_store_set_boolean(gm_store, USE_XSCRNSAVER, use_xscrnsaver);
    gm_pref_store_set_boolean(gm_store, MOUSE_WHEEL_CHANGES_VOLUME, mouse_wheel_changes_volume);
    gm_pref_store_set_boolean(gm_store, SHOW_NOTIFICATION, show_notification);
    gm_pref_store_set_boolean(gm_store, SHOW_STATUS_ICON, show_status_icon);
    gm_pref_store_set_boolean(gm_store, VERTICAL, vertical_layout);
    gm_pref_store_set_boolean(gm_store, SINGLE_INSTANCE, single_instance);
    gm_pref_store_set_boolean(gm_store, REPLACE_AND_PLAY, replace_and_play);
    gm_pref_store_set_boolean(gm_store, BRING_TO_FRONT, bring_to_front);
    gm_pref_store_set_boolean(gm_store, REMEMBER_LOC, remember_loc);
    gm_pref_store_set_boolean(gm_store, KEEP_ON_TOP, keep_on_top);
    gm_pref_store_set_boolean(gm_store, RESIZE_ON_NEW_MEDIA, resize_on_new_media);
    gm_pref_store_set_int(gm_store, VERBOSE, verbose);
    gm_pref_store_set_string(gm_store, METADATACODEPAGE, metadata_codepage);
    gm_pref_store_set_string(gm_store, SUBTITLEFONT, subtitlefont);
    gm_pref_store_set_float(gm_store, SUBTITLESCALE, subtitle_scale);
    gm_pref_store_set_string(gm_store, SUBTITLECODEPAGE, subtitle_codepage);
    gm_pref_store_set_string(gm_store, SUBTITLECOLOR, subtitle_color);
    gm_pref_store_set_boolean(gm_store, SUBTITLEOUTLINE, subtitle_outline);
    gm_pref_store_set_boolean(gm_store, SUBTITLESHADOW, subtitle_shadow);
    gm_pref_store_set_int(gm_store, SUBTITLE_MARGIN, subtitle_margin);
    gm_pref_store_set_boolean(gm_store, SHOW_SUBTITLES, showsubtitles);

    gm_pref_store_set_string(gm_store, MPLAYER_BIN, mplayer_bin);
    gm_pref_store_set_string(gm_store, MPLAYER_DVD_DEVICE, mplayer_dvd_device);
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

    // don't reload plugins when running in plugin mode
    if (embed_window == 0 && control_id == 0)
        dbus_reload_plugins();

    dontplaynext = TRUE;

    if (idledata->streaming)
        play_iter(&iter, 0);
    else
        play_iter(&iter, idledata->position);

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
    idle->gamma = gamma;
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
    g_idle_add(set_adjust_layout, NULL);
    //adjust_layout();
}

void update_details_table()
{
    gchar *buf;
    IdleData *idle = idledata;

    if (idle->videopresent) {
        buf = g_strdup_printf("%i x %i", idle->width, idle->height);
    } else {
        buf = g_strdup_printf("0 x 0");
    }
    gtk_label_set_text(GTK_LABEL(details_video_size), buf);
    g_free(buf);

    buf = g_ascii_strup(idle->video_format, -1);
    gtk_label_set_text(GTK_LABEL(details_video_format), buf);
    g_free(buf);

    buf = g_ascii_strup(idle->video_codec, -1);
    gtk_label_set_text(GTK_LABEL(details_video_codec), buf);
    g_free(buf);

    gtk_label_set_text(GTK_LABEL(details_video_fps), idle->video_fps);

    buf = g_strdup_printf("%i Kb/s", (gint) (g_strtod(idle->video_bitrate, NULL) / 1000));
    gtk_label_set_text(GTK_LABEL(details_video_bitrate), buf);
    g_free(buf);

    buf = g_strdup_printf("%i", idle->chapters);
    gtk_label_set_text(GTK_LABEL(details_video_chapters), buf);
    g_free(buf);

    buf = g_ascii_strup(idle->audio_codec, -1);
    gtk_label_set_text(GTK_LABEL(details_audio_codec), buf);
    g_free(buf);

    buf = g_ascii_strup(idle->audio_channels, -1);
    gtk_label_set_text(GTK_LABEL(details_audio_channels), buf);
    g_free(buf);

    buf = g_strdup_printf("%i Kb/s", (gint) (g_strtod(idle->audio_bitrate, NULL) / 1000));
    gtk_label_set_text(GTK_LABEL(details_audio_bitrate), buf);
    g_free(buf);

    buf = g_strdup_printf("%i Kb/s", (gint) (g_strtod(idle->audio_samplerate, NULL) / 1000));
    gtk_label_set_text(GTK_LABEL(details_audio_samplerate), buf);
    g_free(buf);
}


void menuitem_details_callback(GtkMenuItem * menuitem, void *data)
{
    update_details_table();
    g_idle_add(set_adjust_layout, NULL);

}

void create_details_table()
{
    GtkWidget *label;
    gchar *buf;
    gint i = 0;
    IdleData *idle = idledata;

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
    details_video_size = gtk_label_new(buf);
    gtk_widget_set_size_request(details_video_size, 100, -1);
    gtk_misc_set_alignment(GTK_MISC(details_video_size), 0.0, 0.0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), details_video_size, 1, 2, i, i + 1);
    g_free(buf);
    i++;

    label = gtk_label_new(_("Video Format:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
    buf = g_ascii_strup(idle->video_format, -1);
    details_video_format = gtk_label_new(buf);
    g_free(buf);
    gtk_misc_set_alignment(GTK_MISC(details_video_format), 0.0, 0.0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), details_video_format, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Video Codec:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
    buf = g_ascii_strup(idle->video_codec, -1);
    details_video_codec = gtk_label_new(buf);
    g_free(buf);
    gtk_misc_set_alignment(GTK_MISC(details_video_codec), 0.0, 0.0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), details_video_codec, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Video FPS:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
    details_video_fps = gtk_label_new(idle->video_fps);
    gtk_misc_set_alignment(GTK_MISC(details_video_fps), 0.0, 0.0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), details_video_fps, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Video Bitrate:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
    buf = g_strdup_printf("%i Kb/s", (gint) (g_strtod(idle->video_bitrate, NULL) / 1000));
    details_video_bitrate = gtk_label_new(buf);
    g_free(buf);
    gtk_misc_set_alignment(GTK_MISC(details_video_bitrate), 0.0, 0.0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), details_video_bitrate, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Video Chapters:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
    buf = g_strdup_printf("%i", idle->chapters);
    details_video_chapters = gtk_label_new(buf);
    g_free(buf);
    gtk_misc_set_alignment(GTK_MISC(details_video_chapters), 0.0, 0.0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), details_video_chapters, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
    i++;

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
    buf = g_ascii_strup(idle->audio_codec, -1);
    details_audio_codec = gtk_label_new(buf);
    g_free(buf);
    gtk_widget_set_size_request(details_audio_codec, 100, -1);
    gtk_misc_set_alignment(GTK_MISC(details_audio_codec), 0.0, 0.0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), details_audio_codec, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Audio Channels:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
    if (idle != NULL) {
        buf = g_ascii_strup(idle->audio_channels, -1);
        details_audio_channels = gtk_label_new(buf);
        g_free(buf);
        gtk_misc_set_alignment(GTK_MISC(details_audio_channels), 0.0, 0.0);
        gtk_table_attach_defaults(GTK_TABLE(details_table), details_audio_channels, 1, 2, i, i + 1);
    }
    i++;

    label = gtk_label_new(_("Audio Bitrate:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
    if (idle != NULL) {
        buf = g_strdup_printf("%i Kb/s", (gint) (g_strtod(idle->audio_bitrate, NULL) / 1000));
        details_audio_bitrate = gtk_label_new(buf);
        g_free(buf);
        gtk_misc_set_alignment(GTK_MISC(details_audio_bitrate), 0.0, 0.0);
        gtk_table_attach_defaults(GTK_TABLE(details_table), details_audio_bitrate, 1, 2, i, i + 1);
    }
    i++;

    label = gtk_label_new(_("Audio Sample Rate:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
    if (idle != NULL) {
        buf = g_strdup_printf("%i KHz", (gint) (g_strtod(idle->audio_samplerate, NULL) / 1000));
        details_audio_samplerate = gtk_label_new(buf);
        g_free(buf);
        gtk_misc_set_alignment(GTK_MISC(details_audio_samplerate), 0.0, 0.0);
        gtk_table_attach_defaults(GTK_TABLE(details_table), details_audio_samplerate, 1, 2, i, i + 1);
    }
    i++;

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

    g_signal_connect(G_OBJECT(adv_brightness), "value_changed", G_CALLBACK(brightness_callback), idledata);
    g_signal_connect(G_OBJECT(adv_contrast), "value_changed", G_CALLBACK(contrast_callback), idledata);
    g_signal_connect(G_OBJECT(adv_gamma), "value_changed", G_CALLBACK(gamma_callback), idledata);
    g_signal_connect(G_OBJECT(adv_hue), "value_changed", G_CALLBACK(hue_callback), idledata);
    g_signal_connect(G_OBJECT(adv_saturation), "value_changed", G_CALLBACK(saturation_callback), idledata);

    adv_reset = gtk_button_new_with_mnemonic(_("_Reset"));
    g_signal_connect(GTK_OBJECT(adv_reset), "clicked", G_CALLBACK(adv_reset_values), NULL);
    gtk_container_add(GTK_CONTAINER(adv_hbutton_box), adv_reset);

    adv_close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(GTK_OBJECT(adv_close), "clicked", G_CALLBACK(config_close), adv_window);

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

void menuitem_view_decrease_subtitle_delay_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *cmd;

    subtitle_delay -= 0.1;
    cmd = g_strdup_printf("sub_delay %f 1\n", subtitle_delay);
    send_command(cmd, TRUE);
    g_free(cmd);

    return;

}

void menuitem_view_increase_subtitle_delay_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *cmd;

    subtitle_delay += 0.1;
    cmd = g_strdup_printf("sub_delay %f 1\n", subtitle_delay);
    send_command(cmd, TRUE);
    g_free(cmd);

    return;

}

void menuitem_view_aspect_callback(GtkMenuItem * menuitem, void *data)
{
    static gint i = 0;
    GtkAllocation alloc;
#ifdef ENABLE_PANSCAN
    gchar *cmd;
    gdouble movie_ratio;
#endif

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_default) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window), FALSE);
        i--;
    }

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_four_three) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window), FALSE);
        i--;
    }

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_sixteen_nine) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window), FALSE);
        i--;
    }

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_sixteen_ten) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window), FALSE);
        i--;
    }

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_follow_window) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten), FALSE);
        i--;
    }

    if (i == 0) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
#ifdef ENABLE_PANSCAN

        movie_ratio = (gdouble) idledata->original_w / (gdouble) idledata->original_h;
        // printf("movie new_width %i new_height %i\n", actual_x, actual_y);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three)))
            movie_ratio = 4.0 / 3.0;
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine)))
            movie_ratio = 16.0 / 9.0;
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten)))
            movie_ratio = 16.0 / 10.0;
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_follow_window))) {
            get_allocation(fixed, &alloc);
            movie_ratio = (gdouble) alloc.width / (gdouble) alloc.height;
        }

        cmd = g_strdup_printf("switch_ratio %f\n", movie_ratio);
        send_command(cmd, TRUE);
        g_free(cmd);
#else
        get_allocation(fixed, &alloc);
        allocate_fixed_callback(fixed, &alloc, NULL);
#endif

    }


}

gchar *osdlevel_format_callback(GtkScale * scale, gdouble value)
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

gchar *pplevel_format_callback(GtkScale * scale, gdouble value)
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
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_single_instance)));
    gtk_widget_set_sensitive(config_bring_to_front,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_single_instance)));
}

void config_softvol_callback(GtkWidget * button, gpointer data)
{
    gtk_widget_set_sensitive(config_remember_softvol, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_softvol)));
    gtk_widget_set_sensitive(config_volume_gain, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_softvol)));

}

void config_forcecache_callback(GtkWidget * button, gpointer data)
{
    gtk_widget_set_sensitive(config_cachesize, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_forcecache)));
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

void hw_audio_toggle_callback(GtkToggleButton * source, gpointer user_data)
{
    if (gtk_toggle_button_get_active(source)) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_meter), FALSE);
        gtk_widget_hide(GTK_WIDGET(menuitem_view_meter));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_softvol), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(config_softvol), FALSE);
    } else {
        gtk_widget_show(GTK_WIDGET(menuitem_view_meter));
        gtk_widget_set_sensitive(GTK_WIDGET(config_softvol), TRUE);
    }
}

void ao_change_callback(GtkComboBox widget, gpointer data)
{

#ifdef HAVE_ASOUNDLIB
    if (g_ascii_strncasecmp(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(config_ao)))), "alsa", 4) == 0) {
        gtk_widget_set_sensitive(config_mixer, TRUE);
        gtk_widget_set_sensitive(config_use_hw_audio, TRUE);
    } else {
        gtk_widget_set_sensitive(config_mixer, FALSE);
        gtk_widget_set_sensitive(config_use_hw_audio, FALSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_use_hw_audio), FALSE);
    }
#endif

}

void output_combobox_changed_callback(GtkComboBox * config_ao, gpointer data)
{
    GtkComboBox *config_mixer = GTK_COMBO_BOX(data);
    gchar *device;
    gint card;

#ifdef HAVE_ASOUNDLIB
    snd_mixer_t *mhandle;
    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *sid;
    gint err;
    gchar *mix;
    gint i, j, master, pcm;
#endif

    if (gmtk_output_combo_box_get_active_type(GMTK_OUTPUT_COMBO_BOX(config_ao)) == OUTPUT_TYPE_ALSA) {
        gtk_widget_set_sensitive(GTK_WIDGET(config_mixer), TRUE);
        card = gmtk_output_combo_box_get_active_card(GMTK_OUTPUT_COMBO_BOX(config_ao));
        if (card == -1) {
            device = g_strdup_printf("default");
        } else {
            device = g_strdup_printf("hw:%i", card);
        }

        // this might be wrong, so commenting out
        //softvol = FALSE;
        //gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_softvol), softvol);

        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(config_mixer)));
        gtk_combo_box_set_active(config_mixer, -1);

#ifdef HAVE_ASOUNDLIB
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
            i = 0;
            j = -1;
            master = -1;
            pcm = -1;
            snd_mixer_selem_id_alloca(&sid);
            //gtk_combo_box_append_text(GTK_COMBO_BOX(config_mixer), "");
            for (elem = snd_mixer_first_elem(mhandle); elem; elem = snd_mixer_elem_next(elem)) {
                snd_mixer_selem_get_id(elem, sid);
                if (!snd_mixer_selem_is_active(elem))
                    continue;
                if (snd_mixer_selem_has_capture_volume(elem)
                    || snd_mixer_selem_has_capture_switch(elem))
                    continue;
                if (!snd_mixer_selem_has_playback_volume(elem))
                    continue;
                mix = g_strdup_printf("%s,%i", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
                //mix = g_strdup_printf("%s", snd_mixer_selem_id_get_name(sid));
                gtk_combo_box_append_text(GTK_COMBO_BOX(config_mixer), mix);
                if (audio_device.alsa_mixer != NULL && g_ascii_strcasecmp(mix, audio_device.alsa_mixer) == 0)
                    j = i;
                if (g_ascii_strcasecmp(snd_mixer_selem_id_get_name(sid), "Master") == 0)
                    master = i;
                if (g_ascii_strcasecmp(snd_mixer_selem_id_get_name(sid), "PCM") == 0)
                    pcm = i;

                i++;
            }
            if (j != -1)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_mixer), j);
            //if (mixer != NULL && strlen(mixer) > 0 && j == -1) {
            //    gtk_combo_box_append_text(GTK_COMBO_BOX(config_mixer), mixer);
            //    gtk_combo_box_set_active(GTK_COMBO_BOX(config_mixer), i);
            //}
            if (j == -1 && pcm != -1)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_mixer), pcm);
            if (j == -1 && master != -1)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_mixer), master);

            snd_mixer_close(mhandle);

        }
        g_free(device);
#endif
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(config_mixer), FALSE);
    }
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
    GtkTreeIter ao_iter;
    gchar *desc;

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
    g_signal_connect_swapped(GTK_OBJECT(conf_ok), "clicked", G_CALLBACK(config_apply), config_window);

    conf_cancel = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(GTK_OBJECT(conf_cancel), "clicked", G_CALLBACK(config_apply), config_window);

    config_vo = gtk_combo_box_entry_new_text();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(config_vo,
                                _
                                ("mplayer video output device\nx11 should always work, try xv or gl for better performance"));

#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_vo,
                         _
                         ("mplayer video output device\nx11 should always work, try xv or gl for better performance"),
                         NULL);
#endif
    if (config_vo != NULL) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "gl");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "gl2");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "x11");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "xv");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "xvmc");
#ifndef OPENBSD		
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "vaapi");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "vdpau");
#endif
        if (vo != NULL) {
            if (strcmp(vo, "gl") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 0);
            if (strcmp(vo, "gl2") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 1);
            if (strcmp(vo, "x11") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 2);
            if (strcmp(vo, "xv") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 3);
            if (strcmp(vo, "xvmc") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 4);
            if (strcmp(vo, "vaapi") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 5);
            if (strcmp(vo, "vdpau") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 6);
            if (gtk_combo_box_get_active(GTK_COMBO_BOX(config_vo))
                == -1) {
                gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), vo);
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 7);
            }
        }
    }

    config_use_hw_audio = gtk_check_button_new_with_mnemonic(_("Enable AC3/DTS pass-through to S/PDIF"));
    g_signal_connect(GTK_WIDGET(config_use_hw_audio), "toggled", G_CALLBACK(hw_audio_toggle_callback), NULL);

    config_mixer = gtk_combo_box_entry_new_text();
    config_softvol = gtk_check_button_new_with_label(_("Mplayer Software Volume Control Enabled"));

    config_ao = gmtk_output_combo_box_new();
    g_signal_connect(GTK_WIDGET(config_ao), "changed", G_CALLBACK(output_combobox_changed_callback), config_mixer);

    if (gtk_tree_model_get_iter_first(gmtk_output_combo_box_get_tree_model(GMTK_OUTPUT_COMBO_BOX(config_ao)), &ao_iter)) {
        do {
            if (gtk_list_store_iter_is_valid
                (GTK_LIST_STORE(gmtk_output_combo_box_get_tree_model(GMTK_OUTPUT_COMBO_BOX(config_ao))), &ao_iter)) {
                gtk_tree_model_get(gmtk_output_combo_box_get_tree_model
                                   (GMTK_OUTPUT_COMBO_BOX(config_ao)), &ao_iter, OUTPUT_DESCRIPTION_COLUMN, &desc, -1);

                if (audio_device_name != NULL && strcmp(audio_device_name, desc) == 0) {
                    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(config_ao), &ao_iter);
                    g_free(desc);
                    break;
                }

                if (audio_device_name == NULL && strcmp(desc, _("Default")) == 0) {
                    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(config_ao), &ao_iter);
                    g_free(desc);
                    break;
                }
                g_free(desc);

            }
        } while (gtk_tree_model_iter_next
                 (gmtk_output_combo_box_get_tree_model(GMTK_OUTPUT_COMBO_BOX(config_ao)), &ao_iter));
    }

    config_alang = gtk_combo_box_entry_new_text();
    if (config_alang != NULL) {
        i = 0;
        j = -1;
        while (i < 464) {
            if (alang != NULL && g_ascii_strncasecmp(alang, langlist[i], strlen(alang)) == 0)
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
            if (slang != NULL && g_ascii_strncasecmp(slang, langlist[i], strlen(slang)) == 0)
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
        while (i < 28) {
            if (metadata_codepage != NULL && strlen(metadata_codepage) > 1
                && g_ascii_strncasecmp(metadata_codepage, codepagelist[i], strlen(metadata_codepage)) == 0)
                j = i;
            gtk_combo_box_append_text(GTK_COMBO_BOX(config_metadata_codepage), codepagelist[i++]);
            if (j != -1)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_metadata_codepage), j);
        }
        if (metadata_codepage != NULL && j == -1) {
            gtk_combo_box_append_text(GTK_COMBO_BOX(config_metadata_codepage), metadata_codepage);
            gtk_combo_box_set_active(GTK_COMBO_BOX(config_metadata_codepage), i);
        }
    }
    config_subtitle_codepage = gtk_combo_box_entry_new_text();
    if (config_subtitle_codepage != NULL) {
        i = 0;
        j = -1;
        while (i < 28) {
            if (subtitle_codepage != NULL && strlen(subtitle_codepage) > 1
                && g_ascii_strncasecmp(subtitle_codepage, codepagelist[i], strlen(subtitle_codepage)) == 0)
                j = i;
            gtk_combo_box_append_text(GTK_COMBO_BOX(config_subtitle_codepage), codepagelist[i++]);
            if (j != -1)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_subtitle_codepage), j);
        }
        if (subtitle_codepage != NULL && j == -1) {
            gtk_combo_box_append_text(GTK_COMBO_BOX(config_subtitle_codepage), subtitle_codepage);
            gtk_combo_box_set_active(GTK_COMBO_BOX(config_subtitle_codepage), i);
        }
    }

    config_audio_channels = gtk_combo_box_entry_new_text();
    if (config_audio_channels != NULL) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_audio_channels), "Stereo");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_audio_channels), "Surround");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_audio_channels), "5.1 Surround");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_audio_channels), "7.1 Surround");
        gtk_combo_box_set_active(GTK_COMBO_BOX(config_audio_channels), audio_channels);
    }

    i = 0;
    j = -1;

    config_mplayer_dvd_device = gtk_combo_box_entry_new_text();
#ifdef OPENBSD
	gtk_combo_box_append_text(GTK_COMBO_BOX(config_mplayer_dvd_device), "/dev/rcd0c");
    if (mplayer_dvd_device == NULL || g_ascii_strcasecmp("/dev/rcd0c", mplayer_dvd_device) == 0) {
        j = i;
    }
#else
    gtk_combo_box_append_text(GTK_COMBO_BOX(config_mplayer_dvd_device), "/dev/dvd");
    if (mplayer_dvd_device == NULL || g_ascii_strcasecmp("/dev/dvd", mplayer_dvd_device) == 0) {
        j = i;
    }
#endif
    i++;

#ifdef GIO_ENABLED
    GVolumeMonitor *volumemonitor;
    GList *d, *drives;
    GDrive *drive;
    gchar *unix_device;


    volumemonitor = g_volume_monitor_get();
    if (volumemonitor != NULL) {
        drives = g_volume_monitor_get_connected_drives(volumemonitor);

        for (d = drives; d != NULL; d = d->next) {
            drive = G_DRIVE(d->data);
            if (g_drive_can_poll_for_media(drive)) {
                unix_device = g_drive_get_identifier(drive, "unix-device");
                if (unix_device != NULL) {
                    gtk_combo_box_append_text(GTK_COMBO_BOX(config_mplayer_dvd_device), unix_device);
                    if (mplayer_dvd_device != NULL && g_ascii_strcasecmp(unix_device, mplayer_dvd_device) == 0) {
                        j = i;
                    }
                    g_free(unix_device);
                    i++;
                }
            }
        }

    }
#endif
    if (j != -1)
        gtk_combo_box_set_active(GTK_COMBO_BOX(config_mplayer_dvd_device), j);



    conf_label = gtk_label_new(_("<span weight=\"bold\">Adjust Output Settings</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Video Output:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    gtk_widget_set_size_request(GTK_WIDGET(config_vo), 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_vo, 1, 2, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Audio Output:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_widget_set_size_request(GTK_WIDGET(config_ao), 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_ao, 1, 2, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

#ifdef HAVE_ASOUNDLIB
    conf_label = gtk_label_new(_("Default Mixer:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_widget_set_size_request(GTK_WIDGET(config_mixer), 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_mixer, 1, 2, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;
#endif
    conf_label = gtk_label_new(_("Audio Channels to Output"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_widget_set_size_request(GTK_WIDGET(config_audio_channels), 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_audio_channels, 1, 2, i, i + 1,
                     GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_use_hw_audio), use_hw_audio);
    gtk_table_attach(GTK_TABLE(conf_table), config_use_hw_audio, 1, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(config_use_hw_audio);
    i++;

    conf_label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_EXPAND, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    i++;

    gtk_container_add(GTK_CONTAINER(conf_page1), conf_table);

    //conf_table = gtk_table_new(20, 2, FALSE);
    //gtk_container_add(GTK_CONTAINER(conf_page1), conf_table);
    //i = 0;
    conf_label = gtk_label_new(_("<span weight=\"bold\">Adjust Configuration Settings</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

#ifndef HAVE_ASOUNDLIB
    conf_label = gtk_label_new(_("Default Volume Level:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    config_volume = gtk_spin_button_new_with_range(0, 100, 1);
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(config_volume, _("Default volume for playback"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_volume, _("Default volume for playback"), NULL);
#endif
    gtk_widget_set_size_request(config_volume, 100, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_volume, 1, 2, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    gm_store = gm_pref_store_new("gnome-mplayer");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_volume), gm_pref_store_get_int(gm_store, VOLUME));
    gm_pref_store_free(gm_store);
    gtk_entry_set_width_chars(GTK_ENTRY(config_volume), 6);
    gtk_entry_set_editable(GTK_ENTRY(config_volume), FALSE);
    gtk_entry_set_alignment(GTK_ENTRY(config_volume), 1);
    gtk_widget_show(config_volume);
    i++;
#endif

    conf_label = gtk_label_new(_("On Screen Display Level:"));
    config_osdlevel = gtk_hscale_new_with_range(0.0, 3.0, 1.0);
    gtk_range_set_value(GTK_RANGE(config_osdlevel), osdlevel);
    g_signal_connect(GTK_OBJECT(config_osdlevel), "format-value", G_CALLBACK(osdlevel_format_callback), NULL);
    g_signal_connect(GTK_OBJECT(config_osdlevel), "value-changed", G_CALLBACK(osdlevel_change_callback), NULL);
    gtk_widget_set_size_request(config_osdlevel, 150, -1);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(conf_table), config_osdlevel, 1, 2, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Post-processing level:"));
    config_pplevel = gtk_hscale_new_with_range(0.0, 6.0, 1.0);
    g_signal_connect(GTK_OBJECT(config_pplevel), "format-value", G_CALLBACK(pplevel_format_callback), NULL);
    gtk_widget_set_size_request(config_pplevel, 150, -1);
    gtk_range_set_value(GTK_RANGE(config_pplevel), pplevel);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(conf_table), config_pplevel, 1, 2, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
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
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 2, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    config_qt = gtk_check_button_new_with_label(_("QuickTime Emulation"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_qt), !qt_disabled);
    gtk_table_attach(GTK_TABLE(conf_table), config_qt, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_real = gtk_check_button_new_with_label(_("RealPlayer Emulation"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_real), !real_disabled);
    gtk_table_attach(GTK_TABLE(conf_table), config_real, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_wmp = gtk_check_button_new_with_label(_("Windows Media Player Emulation"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_wmp), !wmp_disabled);
    gtk_table_attach(GTK_TABLE(conf_table), config_wmp, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_dvx = gtk_check_button_new_with_label(_("DiVX Player Emulation"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_dvx), !dvx_disabled);
    gtk_table_attach(GTK_TABLE(conf_table), config_dvx, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_midi = gtk_check_button_new_with_label(_("MIDI Support (requires MPlayer support)"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_midi), !midi_disabled);
    gtk_table_attach(GTK_TABLE(conf_table), config_midi, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_noembed = gtk_check_button_new_with_label(_("Disable Player Embedding"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_noembed), embedding_disabled);
    gtk_table_attach(GTK_TABLE(conf_table), config_noembed, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Audio Cache Size (KB):"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    config_plugin_audio_cache_size = gtk_spin_button_new_with_range(64, 256 * 1024, 64);
    config_plugin_video_cache_size = gtk_spin_button_new_with_range(256, 256 * 1024, 256);
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(config_plugin_audio_cache_size,
                                _
                                ("Amount of data to cache when playing media from network, use higher values for slow networks."));
    gtk_widget_set_tooltip_text(config_plugin_video_cache_size,
                                _
                                ("Amount of data to cache when playing media from network, use higher values for slow networks."));

#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_plugin_audio_cache_size,
                         _
                         ("Amount of data to cache when playing media from network, use higher values for slow networks."),
                         NULL);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_plugin_video_cache_size,
                         _
                         ("Amount of data to cache when playing media from network, use higher values for slow networks."),
                         NULL);
#endif
    //gtk_widget_set_size_request(config_plugin_cache_size, 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_plugin_audio_cache_size, 1, 2, i, i + 1,
                     GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    //gtk_range_set_value(GTK_RANGE(config_cachesize), cache_size);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_plugin_audio_cache_size), plugin_audio_cache_size);
    gtk_entry_set_width_chars(GTK_ENTRY(config_plugin_audio_cache_size), 6);
    gtk_entry_set_editable(GTK_ENTRY(config_plugin_audio_cache_size), FALSE);
    gtk_entry_set_alignment(GTK_ENTRY(config_plugin_audio_cache_size), 1);
    gtk_widget_show(config_plugin_audio_cache_size);
    i++;

    conf_label = gtk_label_new(_("Video Cache Size (KB):"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);

    gtk_table_attach(GTK_TABLE(conf_table), config_plugin_video_cache_size, 1, 2, i, i + 1,
                     GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_plugin_video_cache_size), plugin_video_cache_size);
    gtk_entry_set_width_chars(GTK_ENTRY(config_plugin_video_cache_size), 6);
    gtk_entry_set_editable(GTK_ENTRY(config_plugin_video_cache_size), FALSE);
    gtk_entry_set_alignment(GTK_ENTRY(config_plugin_video_cache_size), 1);
    gtk_widget_show(config_plugin_video_cache_size);
    i++;


    // Language 
    conf_table = gtk_table_new(20, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(conf_page3), conf_table);
    i = 0;
    conf_label = gtk_label_new(_("<span weight=\"bold\">Adjust Language Settings</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Default Audio Language"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    gtk_widget_set_size_request(GTK_WIDGET(config_alang), 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_alang, 1, 2, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Default Subtitle Language:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);;
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_widget_set_size_request(GTK_WIDGET(config_slang), 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_slang, 1, 2, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("File Metadata Encoding:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_widget_set_size_request(GTK_WIDGET(config_metadata_codepage), 200, -1);

    gtk_table_attach(GTK_TABLE(conf_table), config_metadata_codepage, 1, 2, i, i + 1,
                     GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    // Page 4
    conf_table = gtk_table_new(20, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(conf_page4), conf_table);
    i = 0;
    conf_label = gtk_label_new(_("<span weight=\"bold\">Subtitle Settings</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    config_ass = gtk_check_button_new_with_mnemonic(_("Enable _Advanced Substation Alpha (ASS) Subtitle Support"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_ass), !disable_ass);
    g_signal_connect(GTK_OBJECT(config_ass), "toggled", G_CALLBACK(ass_toggle_callback), NULL);
    gtk_table_attach(GTK_TABLE(conf_table), config_ass, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(config_ass);
    i++;

    config_embeddedfonts = gtk_check_button_new_with_mnemonic(_("Use _Embedded Fonts (MKV only)"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_embeddedfonts), !disable_embeddedfonts);
    gtk_widget_set_sensitive(config_embeddedfonts, !disable_ass);
//    g_signal_connect(GTK_OBJECT(config_embeddedfonts), "toggled",
//                     G_CALLBACK(embedded_fonts_toggle_callback), NULL);
    gtk_table_attach(GTK_TABLE(conf_table), config_embeddedfonts, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(config_embeddedfonts);
    i++;

    conf_label = gtk_label_new(_("Subtitle Font:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);

    config_subtitle_font = gtk_font_button_new();
    if (subtitlefont != NULL) {
        gtk_font_button_set_font_name(GTK_FONT_BUTTON(config_subtitle_font), subtitlefont);
    }
//    gtk_widget_set_sensitive(config_subtitle_font, disable_embeddedfonts);
    gtk_font_button_set_show_size(GTK_FONT_BUTTON(config_subtitle_font), TRUE);
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(config_subtitle_font), TRUE);

    gtk_font_button_set_use_size(GTK_FONT_BUTTON(config_subtitle_font), FALSE);
    gtk_font_button_set_use_font(GTK_FONT_BUTTON(config_subtitle_font), TRUE);

    gtk_font_button_set_title(GTK_FONT_BUTTON(config_subtitle_font), _("Subtitle Font Selection"));
    gtk_table_attach(GTK_TABLE(conf_table), config_subtitle_font, 1, 2, i, i + 1,
                     GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Subtitle Color:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
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
    gtk_color_button_set_title(GTK_COLOR_BUTTON(config_subtitle_color), _("Subtitle Color Selection"));
    gtk_widget_set_sensitive(config_subtitle_color, !disable_ass);
    gtk_table_attach(GTK_TABLE(conf_table), config_subtitle_color, 1, 2, i, i + 1,
                     GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    config_subtitle_outline = gtk_check_button_new_with_label(_("Outline Subtitle Font"));
//    gtk_widget_set_sensitive(config_subtitle_outline, disable_embeddedfonts);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_subtitle_outline), subtitle_outline);
    gtk_table_attach(GTK_TABLE(conf_table), config_subtitle_outline, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_subtitle_shadow = gtk_check_button_new_with_label(_("Shadow Subtitle Font"));
//    gtk_widget_set_sensitive(config_subtitle_shadow, disable_embeddedfonts);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_subtitle_shadow), subtitle_shadow);
    gtk_table_attach(GTK_TABLE(conf_table), config_subtitle_shadow, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Subtitle Font Scaling:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    config_subtitle_scale = gtk_spin_button_new_with_range(0.25, 10, 0.05);
    gtk_widget_set_size_request(config_subtitle_scale, -1, -1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_subtitle_scale), subtitle_scale);
    gtk_table_attach(GTK_TABLE(conf_table), config_subtitle_scale, 1, 2, i, i + 1,
                     GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Subtitle File Encoding:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_widget_set_size_request(GTK_WIDGET(config_subtitle_codepage), 200, -1);

    gtk_table_attach(GTK_TABLE(conf_table), config_subtitle_codepage, 1, 2, i, i + 1,
                     GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Subtitle Lower Margin (X11/XV Only):"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    config_subtitle_margin = gtk_spin_button_new_with_range(0, 200, 1);
    gtk_widget_set_size_request(config_subtitle_scale, -1, -1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_subtitle_margin), subtitle_margin);
    gtk_table_attach(GTK_TABLE(conf_table), config_subtitle_margin, 1, 2, i, i + 1,
                     GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    config_show_subtitles = gtk_check_button_new_with_label(_("Show Subtitles by Default"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_show_subtitles), showsubtitles);
    gtk_table_attach(GTK_TABLE(conf_table), config_show_subtitles, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    // Page 5
    conf_table = gtk_table_new(20, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(conf_page5), conf_table);
    i = 0;
    conf_label = gtk_label_new(_("<span weight=\"bold\">Application Preferences</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 2, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    config_playlist_visible = gtk_check_button_new_with_label(_("Start with playlist visible"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_playlist_visible), playlist_visible);
    gtk_table_attach(GTK_TABLE(conf_table), config_playlist_visible, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_details_visible = gtk_check_button_new_with_label(_("Start with details visible"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_details_visible), details_visible);
    gtk_table_attach(GTK_TABLE(conf_table), config_details_visible, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_use_mediakeys = gtk_check_button_new_with_label(_("Respond to Keyboard Media Keys"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_use_mediakeys), use_mediakeys);
    gtk_table_attach(GTK_TABLE(conf_table), config_use_mediakeys, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_use_defaultpl = gtk_check_button_new_with_label(_("Use default playlist"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_use_defaultpl), use_defaultpl);
    gtk_table_attach(GTK_TABLE(conf_table), config_use_defaultpl, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

#ifdef NOTIFY_ENABLED
    config_show_notification = gtk_check_button_new_with_label(_("Show notification popup"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_show_notification), show_notification);
    gtk_table_attach(GTK_TABLE(conf_table), config_show_notification, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;
#endif

#ifdef GTK2_12_ENABLED
    config_show_status_icon = gtk_check_button_new_with_label(_("Show status icon"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_show_status_icon), show_status_icon);
    gtk_table_attach(GTK_TABLE(conf_table), config_show_status_icon, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;
#endif

    config_use_xscrnsaver = gtk_check_button_new_with_label(_("Use X Screen Saver control over Gnome Power Manager"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_use_xscrnsaver), use_xscrnsaver);
    gtk_table_attach(GTK_TABLE(conf_table), config_use_xscrnsaver, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_vertical_layout =
        gtk_check_button_new_with_label(_("Place playlist below media (requires application restart)"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_vertical_layout), vertical_layout);
    gtk_table_attach(GTK_TABLE(conf_table), config_vertical_layout, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_single_instance = gtk_check_button_new_with_label(_("Only allow one instance of Gnome MPlayer"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_single_instance), single_instance);
    gtk_table_attach(GTK_TABLE(conf_table), config_single_instance, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    g_signal_connect(G_OBJECT(config_single_instance), "toggled", G_CALLBACK(config_single_instance_callback), NULL);
    i++;

    conf_label = gtk_label_new("");
    gtk_label_set_width_chars(GTK_LABEL(conf_label), 3);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    config_replace_and_play =
        gtk_check_button_new_with_label(_("When opening in single instance mode, replace existing file"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_replace_and_play), replace_and_play);
    gtk_table_attach(GTK_TABLE(conf_table), config_replace_and_play, 1, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_set_sensitive(config_replace_and_play,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_single_instance)));
    i++;
    conf_label = gtk_label_new("");
    gtk_label_set_width_chars(GTK_LABEL(conf_label), 3);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    config_bring_to_front = gtk_check_button_new_with_label(_("When opening file, bring main window to front"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_bring_to_front), bring_to_front);
    gtk_table_attach(GTK_TABLE(conf_table), config_bring_to_front, 1, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_set_sensitive(config_bring_to_front,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_single_instance)));

    i++;
    config_remember_loc = gtk_check_button_new_with_label(_("Remember Window Location and Size"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_remember_loc), remember_loc);
    gtk_table_attach(GTK_TABLE(conf_table), config_remember_loc, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_resize_on_new_media = gtk_check_button_new_with_label(_("Resize window when new video is loaded"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_resize_on_new_media), resize_on_new_media);
    gtk_table_attach(GTK_TABLE(conf_table), config_resize_on_new_media, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_keep_on_top = gtk_check_button_new_with_label(_("Keep window above other windows"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_keep_on_top), keep_on_top);
    gtk_table_attach(GTK_TABLE(conf_table), config_keep_on_top, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_pause_on_click = gtk_check_button_new_with_label(_("Pause playback on mouse click"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_pause_on_click), !disable_pause_on_click);
    gtk_table_attach(GTK_TABLE(conf_table), config_pause_on_click, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_disable_animation = gtk_check_button_new_with_label(_("Disable Fullscreen Control Bar Animation"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_disable_animation), disable_animation);
    gtk_table_attach(GTK_TABLE(conf_table), config_disable_animation, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_mouse_wheel = gtk_check_button_new_with_label(_("Use Mouse Wheel to change volume, instead of seeking"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_mouse_wheel), mouse_wheel_changes_volume);
    gtk_table_attach(GTK_TABLE(conf_table), config_mouse_wheel, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_verbose = gtk_check_button_new_with_label(_("Verbose Debug Enabled"));
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(config_verbose,
                                _
                                ("When this option is set, extra debug information is sent to the terminal or into ~/.xsession-errors"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_verbose,
                         _
                         ("When this option is set, extra debug information is sent to the terminal or into ~/.xsession-errors"),
                         NULL);
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_verbose), verbose);
    gtk_table_attach(GTK_TABLE(conf_table), config_verbose, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    // Page 6
    conf_table = gtk_table_new(20, 3, FALSE);
    gtk_container_add(GTK_CONTAINER(conf_page6), conf_table);
    i = 0;
    conf_label = gtk_label_new(_("<span weight=\"bold\">Advanced Settings for MPlayer</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 3, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(config_softvol,
                                _("Set this option if changing the volume in Gnome MPlayer changes the master volume"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_softvol,
                         _("Set this option if changing the volume in Gnome MPlayer changes the master volume"), NULL);
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_softvol), softvol);
    gtk_table_attach(GTK_TABLE(conf_table), config_softvol, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    g_signal_connect(G_OBJECT(config_softvol), "toggled", G_CALLBACK(config_softvol_callback), NULL);
    i++;

    conf_label = gtk_label_new("");
    gtk_label_set_width_chars(GTK_LABEL(conf_label), 3);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 1, 2, i, i + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
    config_remember_softvol = gtk_check_button_new_with_label(_("Remember last software volume level"));
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(config_remember_softvol,
                                _("Set this option if you want the software volume level to be remembered"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_remember_softvol,
                         _("Set this option if you want the software volume level to be remembered"), NULL);
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_remember_softvol), remember_softvol);
    gtk_table_attach(GTK_TABLE(conf_table), config_remember_softvol, 1, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_set_sensitive(config_remember_softvol, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_softvol)));
    i++;

    conf_label = gtk_label_new(_("Volume Gain (-200dB to +60dB)"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 1, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    config_volume_gain = gtk_spin_button_new_with_range(-200, 60, 1);
    gtk_widget_set_size_request(config_volume_gain, -1, -1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_volume_gain), volume_gain);
    gtk_table_attach(GTK_TABLE(conf_table), config_volume_gain, 2, 3, i, i + 1,
                     GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    gtk_widget_set_sensitive(config_volume_gain, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_softvol)));
    i++;

    config_deinterlace = gtk_check_button_new_with_mnemonic(_("De_interlace Video"));
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(config_deinterlace, _("Set this option if video looks striped"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_deinterlace, _("Set this option if video looks striped"), NULL);
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_deinterlace), !disable_deinterlace);
    gtk_table_attach(GTK_TABLE(conf_table), config_deinterlace, 0, 3, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_framedrop = gtk_check_button_new_with_mnemonic(_("_Drop frames"));
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(config_framedrop, _("Set this option if video is well behind the audio"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_framedrop, _("Set this option if video is well behind the audio"), NULL);
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_framedrop), !disable_framedrop);
    gtk_table_attach(GTK_TABLE(conf_table), config_framedrop, 0, 3, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    config_forcecache = gtk_check_button_new_with_label(_("Enable mplayer cache"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_forcecache), forcecache);
    gtk_table_attach(GTK_TABLE(conf_table), config_forcecache, 0, 3, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    g_signal_connect(G_OBJECT(config_forcecache), "toggled", G_CALLBACK(config_forcecache_callback), NULL);
    i++;

    conf_label = gtk_label_new(_("Cache Size (KB):"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    config_cachesize = gtk_spin_button_new_with_range(32, 256 * 1024, 512);
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(config_cachesize, _
                                ("Amount of data to cache when playing media, use higher values for slow devices and sites."));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_cachesize, _
                         ("Amount of data to cache when playing media, use higher values for slow devices and sites."),
                         NULL);
#endif
    gtk_widget_set_size_request(config_cachesize, 100, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_cachesize, 2, 3, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    //gtk_range_set_value(GTK_RANGE(config_cachesize), cache_size);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_cachesize), cache_size);
    gtk_entry_set_width_chars(GTK_ENTRY(config_cachesize), 6);
    gtk_entry_set_editable(GTK_ENTRY(config_cachesize), FALSE);
    gtk_entry_set_alignment(GTK_ENTRY(config_cachesize), 1);
    gtk_widget_show(config_cachesize);
    gtk_widget_set_sensitive(config_cachesize, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_forcecache)));
    i++;

    conf_label = gtk_label_new("");
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("MPlayer Executable:"));
    config_mplayer_bin = gtk_entry_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(config_mplayer_bin, _
                                ("Use this option to specify a mplayer application that is not in the path"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_mplayer_bin, _
                         ("Use this option to specify a mplayer application that is not in the path"), NULL);
#endif
    gtk_entry_set_text(GTK_ENTRY(config_mplayer_bin), ((mplayer_bin) ? mplayer_bin : ""));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_entry_set_width_chars(GTK_ENTRY(config_mplayer_bin), 40);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;
    gtk_table_attach(GTK_TABLE(conf_table), config_mplayer_bin, 0, 3, i, i + 1,
                     GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new("");
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("Extra Options to MPlayer:"));
    config_extraopts = gtk_entry_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(config_extraopts, _("Add any extra mplayer options here (filters etc)"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, config_extraopts, _("Add any extra mplayer options here (filters etc)"), NULL);
#endif
    gtk_entry_set_text(GTK_ENTRY(config_extraopts), ((extraopts) ? extraopts : ""));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_entry_set_width_chars(GTK_ENTRY(config_extraopts), 40);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;
    gtk_table_attach(GTK_TABLE(conf_table), config_extraopts, 0, 3, i, i + 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    conf_label = gtk_label_new(_("MPlayer Default Optical Device"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach(GTK_TABLE(conf_table), conf_label, 0, 2, i, i + 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_widget_show(conf_label);
    gtk_widget_set_size_request(GTK_WIDGET(config_mplayer_dvd_device), 200, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_mplayer_dvd_device, 2, 3, i, i + 1,
                     GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
    i++;

    gtk_container_add(GTK_CONTAINER(conf_hbutton_box), conf_cancel);
    gtk_container_add(GTK_CONTAINER(conf_vbox), conf_hbutton_box);

    gtk_widget_show_all(config_window);
    gtk_window_set_transient_for(GTK_WINDOW(config_window), GTK_WINDOW(window));
    gtk_window_set_keep_above(GTK_WINDOW(config_window), keep_on_top);
    gtk_window_present(GTK_WINDOW(config_window));

#ifdef GTK2_12_ENABLED
    gtk_window_reshow_with_initial_size(GTK_WINDOW(config_window));
#endif
}

gboolean tracker_callback(GtkWidget * widget, gint percent, void *data)
{
    gchar *cmd;

    if (!idledata->streaming) {
        if (!autopause) {
            if (state != STOPPED) {
                cmd = g_strdup_printf("seek %i 1\n", percent);
                send_command(cmd, TRUE);
                /*if (state == PAUSED) {
                   send_command("mute 1\nseek 0 0\npause\n", FALSE);
                   send_command("mute 0\n", TRUE);
                   idledata->position = idledata->length * percent;
                   gmtk_media_tracker_set_percentage(tracker, percent);
                   } */
                g_free(cmd);
            }
        }
    }

    return FALSE;
}

gboolean tracker_difference_callback(GtkWidget * widget, gdouble difference, void *data)
{
    gchar *cmd;

    if (!idledata->streaming || idledata->seekable == TRUE) {
        if (!autopause) {
            if (state != STOPPED) {
                if (difference > 0)
                    cmd = g_strdup_printf("seek +%f 0\n", difference);
                else
                    cmd = g_strdup_printf("seek %f 0\n", difference);
                send_command(cmd, TRUE);
                /*if (state == PAUSED) {
                   send_command("mute 1\nseek 0 0\npause\n", FALSE);
                   send_command("mute 0\n", TRUE);
                   idledata->position = idledata->length * percent;
                   gmtk_media_tracker_set_percentage(tracker, percent);
                   } */
                g_free(cmd);
            }
        }
    }

    return FALSE;
}

gboolean progress_callback(GtkWidget * widget, GdkEventButton * event, void *data)
{
    GdkEventButton *event_button;

    if (event->type == GDK_BUTTON_PRESS) {
        event_button = (GdkEventButton *) event;
        if (event_button->button == 3) {
            gtk_menu_popup(popup_menu, NULL, NULL, NULL, NULL, event_button->button, event_button->time);
            return TRUE;
        }
    }

    return TRUE;
}

gboolean load_href_callback(GtkWidget * widget, GdkEventExpose * event, gchar * hrefid)
{
    GdkEventButton *event_button;

    if (event->type == GDK_BUTTON_PRESS) {
        event_button = (GdkEventButton *) event;

        if (event_button->button == 3) {
            gtk_menu_popup(popup_menu, NULL, NULL, NULL, NULL, event_button->button, event_button->time);
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

gboolean idle_make_button(gpointer data)
{
    ButtonDef *b = (ButtonDef *) data;

    if (b != NULL) {
        make_button(b->uri, b->hrefid);
        g_free(b->uri);
        g_free(b->hrefid);
        g_free(b);
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

    idledata->showcontrols = FALSE;
    showcontrols = FALSE;
    set_show_controls(idledata);

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

        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status, &error);
        if (error != NULL) {
            printf("Error when running: %s\n", error->message);
            g_error_free(error);
            error = NULL;
        }

        if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
            pb_button = gdk_pixbuf_new_from_file(filename, &error);
            if (error != NULL) {
                printf("make_button error: %s\n", error->message);
                g_error_free(error);
                error = NULL;
            }
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
                         G_CALLBACK(load_href_callback), g_strdup(hrefid));
        gtk_widget_set_size_request(GTK_WIDGET(button_event_box), window_x, window_y);
        gtk_widget_show_all(button_event_box);
        gtk_widget_set_size_request(controls_box, 0, 0);
        gtk_widget_hide(controls_box);
        gtk_widget_show(vbox);
    } else {
        if (verbose)
            printf("unable to make button from media, using default\n");
        button_event_box = gtk_event_box_new();
        image_button = gtk_image_new_from_pixbuf(pb_icon);
        gtk_container_add(GTK_CONTAINER(button_event_box), image_button);
        gtk_box_pack_start(GTK_BOX(vbox), button_event_box, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(button_event_box), "button_press_event",
                         G_CALLBACK(load_href_callback), g_strdup(hrefid));
        gtk_widget_show_all(button_event_box);
        gtk_widget_set_size_request(controls_box, 0, 0);
        gtk_widget_hide(controls_box);
        gtk_widget_show(vbox);
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

}

void setup_accelerators(gboolean enable)
{
    if (gtk_accel_group_query(accel_group, GDK_c, 0, NULL) != NULL) {
        // printf("flushing accelerators\n");
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_edit_config), accel_group, GDK_p, GDK_CONTROL_MASK);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_edit_take_screenshot), accel_group, GDK_t, GDK_CONTROL_MASK);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_playlist), accel_group, GDK_F9, 0);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_file_open_location), accel_group, GDK_l, GDK_CONTROL_MASK);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_info), accel_group, GDK_i, 0);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_subtitles), accel_group, GDK_v, 0);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_details), accel_group, GDK_d, GDK_CONTROL_MASK);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_meter), accel_group, GDK_m, GDK_CONTROL_MASK);

        if (!disable_fullscreen) {
            gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_fullscreen), accel_group, GDK_f, 0);

            gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_fullscreen), accel_group, GDK_f, GDK_CONTROL_MASK);

        }
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_onetoone), accel_group, GDK_1, GDK_CONTROL_MASK);

        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_twotoone), accel_group, GDK_2, GDK_CONTROL_MASK);

        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_showcontrols), accel_group, GDK_c, 0);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_controls), accel_group, GDK_c, 0);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_angle), accel_group, GDK_a, GDK_CONTROL_MASK);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_aspect), accel_group, GDK_a, 0);
        //gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_decrease_subtitle_delay),
        //                              accel_group, GDK_z, 0);
        //gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_increase_subtitle_delay),
        //                              accel_group, GDK_x, 0);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_smaller_subtitle), accel_group, GDK_r, GDK_SHIFT_MASK);
        gtk_widget_remove_accelerator(GTK_WIDGET(menuitem_view_larger_subtitle), accel_group, GDK_t, GDK_SHIFT_MASK);

    }

    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_edit_config), "activate",
                               accel_group, GDK_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_edit_take_screenshot), "activate",
                               accel_group, GDK_t, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_playlist), "activate",
                               accel_group, GDK_F9, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_file_open_location), "activate",
                               accel_group, GDK_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_details), "activate",
                               accel_group, GDK_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_meter), "activate",
                               accel_group, GDK_m, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    if (!disable_fullscreen) {

        gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_fullscreen), "activate",
                                   accel_group, GDK_f, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    }
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_onetoone), "activate",
                               accel_group, GDK_1, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_twotoone), "activate",
                               accel_group, GDK_2, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_angle), "activate",
                               accel_group, GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // enable the accelerators without MASKS
    if (enable) {
        gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_info), "activate",
                                   accel_group, GDK_i, 0, GTK_ACCEL_VISIBLE);
        gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_subtitles), "activate",
                                   accel_group, GDK_v, 0, GTK_ACCEL_VISIBLE);
        if (!disable_fullscreen) {
            gtk_widget_add_accelerator(GTK_WIDGET(menuitem_fullscreen), "activate",
                                       accel_group, GDK_f, 0, GTK_ACCEL_VISIBLE);
        }

        gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_aspect), "activate",
                                   accel_group, GDK_a, 0, GTK_ACCEL_VISIBLE);
        // we want to use "window_key_callback" to handle this, due to GTK keyboard issues.
        //gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_decrease_subtitle_delay), "activate",
        //                           accel_group, GDK_z, 0, GTK_ACCEL_VISIBLE);
        //gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_increase_subtitle_delay), "activate",
        //                           accel_group, GDK_x, 0, GTK_ACCEL_VISIBLE);
        gtk_widget_add_accelerator(GTK_WIDGET(menuitem_showcontrols), "activate",
                                   accel_group, GDK_c, 0, GTK_ACCEL_VISIBLE);
        gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_controls), "activate",
                                   accel_group, GDK_c, 0, GTK_ACCEL_VISIBLE);
        gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_smaller_subtitle), "activate",
                                   accel_group, GDK_r, GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
        gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_larger_subtitle), "activate",
                                   accel_group, GDK_t, GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
    }

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
        g_signal_connect(window, "realize", G_CALLBACK(drawing_area_realized), NULL);
        gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
#ifdef GTK2_20_ENABLED
        gtk_widget_set_can_focus(window, TRUE);
#else
        GTK_WIDGET_SET_FLAGS(window, GTK_CAN_FOCUS);
#endif
    }
    //gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    g_object_set_property(G_OBJECT(window), "allow-shrink", &ALLOW_SHRINK_TRUE);

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

    delete_signal_id = g_signal_connect(GTK_OBJECT(window), "delete_event", G_CALLBACK(delete_callback), NULL);
    g_signal_connect(GTK_OBJECT(window), "motion_notify_event", G_CALLBACK(motion_notify_callback), NULL);
    g_signal_connect(GTK_OBJECT(window), "window_state_event", G_CALLBACK(window_state_callback), NULL);
    g_signal_connect(GTK_OBJECT(window), "configure_event", G_CALLBACK(configure_callback), NULL);

    accel_group = gtk_accel_group_new();

    // right click menu
    popup_menu = GTK_MENU(gtk_menu_new());
    menubar = gtk_menu_bar_new();
    menuitem_play = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_play));
    gtk_widget_show(GTK_WIDGET(menuitem_play));
    menuitem_stop = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_STOP, NULL));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_stop));
    gtk_widget_show(GTK_WIDGET(menuitem_stop));
    menuitem_prev = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PREVIOUS, NULL));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_prev));
    menuitem_next = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_NEXT, NULL));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_next));
    menuitem_sep1 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_sep1));
    gtk_widget_show(GTK_WIDGET(menuitem_sep1));
    menuitem_open = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, accel_group));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_open));
    gtk_widget_show(GTK_WIDGET(menuitem_open));
    menuitem_sep3 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_sep3));
    gtk_widget_show(GTK_WIDGET(menuitem_sep3));
    menuitem_showcontrols = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("S_how Controls")));
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
    menuitem_config = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL));
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
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_save), FALSE);
    }

    menuitem_sep3 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_sep3));
    gtk_widget_show(GTK_WIDGET(menuitem_sep3));
    menuitem_quit = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_quit));
    gtk_widget_show(GTK_WIDGET(menuitem_quit));


    g_signal_connect(GTK_OBJECT(menuitem_open), "activate", G_CALLBACK(menuitem_open_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_play), "activate", G_CALLBACK(menuitem_pause_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_stop), "activate", G_CALLBACK(menuitem_stop_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_prev), "activate", G_CALLBACK(menuitem_prev_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_next), "activate", G_CALLBACK(menuitem_next_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_showcontrols), "toggled", G_CALLBACK(menuitem_showcontrols_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_fullscreen), "toggled", G_CALLBACK(menuitem_fs_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_copyurl), "activate", G_CALLBACK(menuitem_copyurl_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_config), "activate", G_CALLBACK(menuitem_config_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_save), "activate", G_CALLBACK(menuitem_save_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_quit), "activate", G_CALLBACK(menuitem_quit_callback), NULL);


    g_signal_connect_swapped(G_OBJECT(window), "button_press_event", G_CALLBACK(popup_handler), GTK_OBJECT(popup_menu));
    g_signal_connect_swapped(G_OBJECT(window),
                             "button_release_event", G_CALLBACK(popup_handler), GTK_OBJECT(popup_menu));
    g_signal_connect_swapped(G_OBJECT(window),
                             "scroll_event", G_CALLBACK(drawing_area_scroll_event_callback), GTK_OBJECT(drawing_area));
    g_signal_connect_swapped(G_OBJECT(window), "enter_notify_event", G_CALLBACK(notification_handler), NULL);
    g_signal_connect_swapped(G_OBJECT(window), "leave_notify_event", G_CALLBACK(notification_handler), NULL);


    // File Menu
    menuitem_file = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_File")));
    menu_file = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_file));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), GTK_WIDGET(menuitem_file));
    gtk_menu_item_set_submenu(menuitem_file, GTK_WIDGET(menu_file));
    menuitem_file_open = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, accel_group));
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_open));
    menuitem_file_open_folder = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _Folder")));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem_file_open_folder),
                                  gtk_image_new_from_icon_name("folder", GTK_ICON_SIZE_MENU));

    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_open_folder));
    menuitem_file_open_location = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _Location")));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem_file_open_location),
                                  gtk_image_new_from_icon_name("network-server", GTK_ICON_SIZE_MENU));

    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_open_location));

    menuitem_file_disc = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Disc")));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem_file_disc),
                                  gtk_image_new_from_icon_name("media-optical", GTK_ICON_SIZE_MENU));
    menu_file_disc = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_file_disc));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_file), GTK_WIDGET(menuitem_file_disc));
    gtk_menu_item_set_submenu(menuitem_file_disc, GTK_WIDGET(menu_file_disc));

    menuitem_file_open_acd = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _Audio CD")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_acd));
    menuitem_file_open_sep1 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_sep1));

    menuitem_file_open_dvd = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _DVD")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_dvd));
    menuitem_file_open_dvdnav = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open DVD with _Menus")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_dvdnav));
    menuitem_file_open_dvd_folder = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open DVD from _Folder")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_dvd_folder));
    menuitem_file_open_dvdnav_folder =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open DVD from Folder with M_enus")));
    gtk_menu_append(menu_file_disc, GTK_WIDGET(menuitem_file_open_dvdnav_folder));
    menuitem_file_open_dvd_iso = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open DVD from _ISO")));
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

    menuitem_file_open_atv = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _Analog TV")));
    gtk_menu_append(menu_file_tv, GTK_WIDGET(menuitem_file_open_atv));
    menuitem_file_open_dtv = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _Digital TV")));
    gtk_menu_append(menu_file_tv, GTK_WIDGET(menuitem_file_open_dtv));
#ifdef HAVE_GPOD
    menuitem_file_open_ipod = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _iPod™")));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem_file_open_ipod),
                                  gtk_image_new_from_icon_name("multimedia-player", GTK_ICON_SIZE_MENU));

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
    gtk_recent_filter_add_application(recent_filter, "gnome-mplayer");
    gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(menuitem_file_recent_items), recent_filter);
    gtk_recent_chooser_set_show_tips(GTK_RECENT_CHOOSER(menuitem_file_recent_items), TRUE);
    gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(menuitem_file_recent_items), GTK_RECENT_SORT_MRU);
    gtk_menu_item_set_submenu(menuitem_file_recent, menuitem_file_recent_items);
    gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(menuitem_file_recent_items), FALSE);
#endif
#endif

    menuitem_file_sep2 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_sep2));

    menuitem_file_quit = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group));
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_quit));

    g_signal_connect(GTK_OBJECT(menuitem_file_open), "activate", G_CALLBACK(menuitem_open_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_folder), "activate",
                     G_CALLBACK(add_folder_to_playlist), menuitem_file_open_folder);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_location), "activate",
                     G_CALLBACK(menuitem_open_location_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_dvd), "activate", G_CALLBACK(menuitem_open_dvd_callback), NULL);
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
    g_signal_connect(GTK_OBJECT(menuitem_file_open_acd), "activate", G_CALLBACK(menuitem_open_acd_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_vcd), "activate", G_CALLBACK(menuitem_open_vcd_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_atv), "activate", G_CALLBACK(menuitem_open_atv_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_dtv), "activate", G_CALLBACK(menuitem_open_dtv_callback), NULL);
#ifdef HAVE_GPOD
    g_signal_connect(GTK_OBJECT(menuitem_file_open_ipod), "activate", G_CALLBACK(menuitem_open_ipod_callback), NULL);
#endif
#ifdef GTK2_12_ENABLED
#ifdef GIO_ENABLED
    g_signal_connect(GTK_OBJECT(menuitem_file_recent_items), "item-activated",
                     G_CALLBACK(menuitem_open_recent_callback), NULL);
#endif
#endif
    g_signal_connect(GTK_OBJECT(menuitem_file_quit), "activate", G_CALLBACK(menuitem_quit_callback), NULL);
    // Edit Menu
    menuitem_edit = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Edit")));
    menu_edit = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_edit));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), GTK_WIDGET(menuitem_edit));
    gtk_menu_item_set_submenu(menuitem_edit, GTK_WIDGET(menu_edit));

    menuitem_edit_random = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Shuffle Playlist")));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_edit_random), random_order);
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_random));

    menuitem_edit_loop = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Loop Playlist")));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_edit_loop), loop);
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_loop));

    menuitem_edit_switch_audio = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("S_witch Audio Track")));
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_switch_audio));

    menuitem_edit_set_audiofile = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("Set Audi_o")));
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_set_audiofile));

    menuitem_edit_select_audio_lang = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("Select _Audio Language")));
    menu_edit_audio_langs = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_edit_select_audio_lang));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_edit), GTK_WIDGET(menuitem_edit_select_audio_lang));
    gtk_menu_item_set_submenu(menuitem_edit_select_audio_lang, GTK_WIDGET(menu_edit_audio_langs));

    menuitem_edit_set_subtitle = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("Set Sub_title")));
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_set_subtitle));

    menuitem_edit_select_sub_lang = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("S_elect Subtitle Language")));
    menu_edit_sub_langs = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_edit_select_sub_lang));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_edit), GTK_WIDGET(menuitem_edit_select_sub_lang));
    gtk_menu_item_set_submenu(menuitem_edit_select_sub_lang, GTK_WIDGET(menu_edit_sub_langs));

    menuitem_edit_take_screenshot = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Take Screenshot")));
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_take_screenshot));
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(GTK_WIDGET(menuitem_edit_take_screenshot),
                                _("Files named ’shotNNNN.png’ will be saved in the working directory"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, GTK_WIDGET(menuitem_edit_take_screenshot),
                         _("Files named ’shotNNNN.png’ will be saved in the working directory"), NULL);
#endif
    menuitem_edit_sep1 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_sep1));

    menuitem_edit_config = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL));
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_config));
    g_signal_connect(GTK_OBJECT(menuitem_edit_random), "activate", G_CALLBACK(menuitem_edit_random_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_edit_loop), "activate", G_CALLBACK(menuitem_edit_loop_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_edit_set_audiofile), "activate",
                     G_CALLBACK(menuitem_edit_set_audiofile_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_edit_switch_audio), "activate",
                     G_CALLBACK(menuitem_edit_switch_audio_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_edit_set_subtitle), "activate",
                     G_CALLBACK(menuitem_edit_set_subtitle_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_edit_take_screenshot), "activate",
                     G_CALLBACK(menuitem_edit_take_screenshot_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_edit_config), "activate", G_CALLBACK(menuitem_config_callback), NULL);



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

    menuitem_view_fullscreen = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Full Screen")));
    menuitem_view_sep1 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    if (!disable_fullscreen) {
        gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_fullscreen));
        gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_sep1));
    }
    menuitem_view_onetoone = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Normal (1:1)")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_onetoone));
    menuitem_view_twotoone = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Double Size (2:1)")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_twotoone));
    menuitem_view_onetotwo = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Half Size (1:2)")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_onetotwo));

    menuitem_view_aspect = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Aspect")));
    menu_view_aspect = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_view_aspect));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_view), GTK_WIDGET(menuitem_view_aspect));
    gtk_menu_item_set_submenu(menuitem_view_aspect, GTK_WIDGET(menu_view_aspect));

    menuitem_view_aspect_default = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("D_efault Aspect")));
    gtk_menu_append(menu_view_aspect, GTK_WIDGET(menuitem_view_aspect_default));
    menuitem_view_aspect_four_three = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_4:3 Aspect")));
    gtk_menu_append(menu_view_aspect, GTK_WIDGET(menuitem_view_aspect_four_three));
    menuitem_view_aspect_sixteen_nine = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_16:9 Aspect")));
    gtk_menu_append(menu_view_aspect, GTK_WIDGET(menuitem_view_aspect_sixteen_nine));
    menuitem_view_aspect_sixteen_ten = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("1_6:10 Aspect")));
    gtk_menu_append(menu_view_aspect, GTK_WIDGET(menuitem_view_aspect_sixteen_ten));
    menuitem_view_aspect_follow_window = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Follow Window")));
    gtk_menu_append(menu_view_aspect, GTK_WIDGET(menuitem_view_aspect_follow_window));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), TRUE);

    menuitem_view_sep2 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    menuitem_view_sep5 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    menuitem_view_subtitles = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("Show _Subtitles")));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_subtitles), showsubtitles);
    menuitem_view_smaller_subtitle = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Decrease Subtitle Size")));
    menuitem_view_larger_subtitle = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Increase Subtitle Size")));
    menuitem_view_decrease_subtitle_delay =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Decrease Subtitle Delay")));
    menuitem_view_increase_subtitle_delay =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Increase Subtitle Delay")));
    menuitem_view_angle = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Switch An_gle")));
    menuitem_view_controls = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Controls")));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_controls), TRUE);
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_sep2));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_subtitles));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_smaller_subtitle));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_larger_subtitle));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_decrease_subtitle_delay));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_increase_subtitle_delay));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_sep5));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_angle));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_controls));
    menuitem_view_sep3 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_sep3));
    menuitem_view_advanced = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Video Picture Adjustments")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_advanced));

    g_signal_connect(GTK_OBJECT(menuitem_view_playlist), "toggled", G_CALLBACK(menuitem_view_playlist_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_info), "activate", G_CALLBACK(menuitem_view_info_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_details), "activate", G_CALLBACK(menuitem_details_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_meter), "activate", G_CALLBACK(menuitem_meter_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_fullscreen), "toggled", G_CALLBACK(menuitem_fs_callback), NULL);
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
    g_signal_connect(GTK_OBJECT(menuitem_view_decrease_subtitle_delay), "activate",
                     G_CALLBACK(menuitem_view_decrease_subtitle_delay_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_increase_subtitle_delay), "activate",
                     G_CALLBACK(menuitem_view_increase_subtitle_delay_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_angle), "activate", G_CALLBACK(menuitem_view_angle_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_controls), "toggled", G_CALLBACK(menuitem_showcontrols_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_advanced), "activate", G_CALLBACK(menuitem_advanced_callback), idledata);

    // Help Menu
    menuitem_help = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Help")));
    menu_help = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_help));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), GTK_WIDGET(menuitem_help));
    gtk_menu_item_set_submenu(menuitem_help, GTK_WIDGET(menu_help));
    menuitem_help_about = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL));
    gtk_menu_append(menu_help, GTK_WIDGET(menuitem_help_about));
    g_signal_connect(GTK_OBJECT(menuitem_help_about), "activate", G_CALLBACK(menuitem_about_callback), NULL);

    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
    setup_accelerators(TRUE);
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
    g_signal_connect(GTK_OBJECT(window), "drag_data_received", G_CALLBACK(drop_callback), NULL);


    vbox = gtk_vbox_new(FALSE, 0);
    hbox = gtk_hbox_new(FALSE, 0);
    controls_box = gtk_vbox_new(FALSE, 0);
    fixed = gtk_fixed_new();
    drawing_area = gtk_socket_new();
#ifdef GTK2_18_ENABLED
    gtk_widget_set_has_window(GTK_WIDGET(drawing_area), TRUE);
#endif
    g_signal_connect(drawing_area, "realize", G_CALLBACK(drawing_area_realized), NULL);

    cover_art = gtk_image_new();
    media_label = gtk_label_new("");
    gtk_widget_set_size_request(media_label, 300, -1);
    gtk_label_set_ellipsize(GTK_LABEL(media_label), PANGO_ELLIPSIZE_END);
    media_hbox = gtk_hbox_new(FALSE, 10);
    g_signal_connect(media_hbox, "show", G_CALLBACK(view_option_show_callback), NULL);
    g_signal_connect(media_hbox, "size_allocate", G_CALLBACK(view_option_size_allocate_callback), NULL);

    details_vbox = gtk_vbox_new(FALSE, 10);
    details_table = gtk_table_new(20, 2, FALSE);
    g_signal_connect(details_vbox, "show", G_CALLBACK(view_option_show_callback), NULL);
    g_signal_connect(details_vbox, "size_allocate", G_CALLBACK(view_option_size_allocate_callback), NULL);
    create_details_table();
    gtk_container_add(GTK_CONTAINER(details_vbox), details_table);

    gtk_misc_set_alignment(GTK_MISC(media_label), 0, 0);
    audio_meter = gmtk_audio_meter_new(METER_BARS);
    g_signal_connect(audio_meter, "show", G_CALLBACK(view_option_show_callback), NULL);
    g_signal_connect(audio_meter, "size_allocate", G_CALLBACK(view_option_size_allocate_callback), NULL);
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

    g_signal_connect(GTK_OBJECT(drawing_area), "motion_notify_event", G_CALLBACK(motion_notify_callback), NULL);
    gtk_widget_show(drawing_area);

    if (vertical_layout) {
        pane = gtk_vpaned_new();
    } else {
        pane = gtk_hpaned_new();
    }
    gtk_paned_pack1(GTK_PANED(pane), vbox, FALSE, TRUE);
    create_playlist_widget();
    g_signal_connect(plvbox, "show", G_CALLBACK(view_option_show_callback), NULL);
    g_signal_connect(plvbox, "hide", G_CALLBACK(view_option_hide_callback), NULL);
    g_signal_connect(plvbox, "size_allocate", G_CALLBACK(view_option_size_allocate_callback), NULL);

    //if (remember_loc)
    //      gtk_paned_set_position(GTK_PANED(pane),loc_panel_position);

    vbox_master = gtk_vbox_new(FALSE, 0);
    if (windowid == 0)
        gtk_box_pack_start(GTK_BOX(vbox_master), menubar, FALSE, FALSE, 0);
    gtk_widget_show(menubar);

    gtk_box_pack_start(GTK_BOX(vbox_master), pane, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox_master), controls_box, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), vbox_master);

    error = NULL;
    icon_theme = gtk_icon_theme_get_default();



    image_play = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, button_size);
    image_stop = gtk_image_new_from_stock(GTK_STOCK_MEDIA_STOP, button_size);
    image_pause = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE, button_size);

    image_ff = gtk_image_new_from_stock(GTK_STOCK_MEDIA_FORWARD, button_size);
    image_rew = gtk_image_new_from_stock(GTK_STOCK_MEDIA_REWIND, button_size);

    image_prev = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PREVIOUS, button_size);
    image_next = gtk_image_new_from_stock(GTK_STOCK_MEDIA_NEXT, button_size);
    image_menu = gtk_image_new_from_stock(GTK_STOCK_INDEX, button_size);

    image_fs = gtk_image_new_from_stock(GTK_STOCK_FULLSCREEN, button_size);

    icon_list = NULL;
    if (gtk_icon_theme_has_icon(icon_theme, "gnome-mplayer")) {
        pb_icon = gtk_icon_theme_load_icon(icon_theme, "gnome-mplayer", 128, 0, NULL);
        if (pb_icon)
            icon_list = g_list_prepend(icon_list, pb_icon);
        pb_icon = gtk_icon_theme_load_icon(icon_theme, "gnome-mplayer", 64, 0, NULL);
        if (pb_icon)
            icon_list = g_list_prepend(icon_list, pb_icon);
        pb_icon = gtk_icon_theme_load_icon(icon_theme, "gnome-mplayer", 48, 0, NULL);
        if (pb_icon)
            icon_list = g_list_prepend(icon_list, pb_icon);
        pb_icon = gtk_icon_theme_load_icon(icon_theme, "gnome-mplayer", 24, 0, NULL);
        if (pb_icon)
            icon_list = g_list_prepend(icon_list, pb_icon);
        pb_icon = gtk_icon_theme_load_icon(icon_theme, "gnome-mplayer", 22, 0, NULL);
        if (pb_icon)
            icon_list = g_list_prepend(icon_list, pb_icon);
        pb_icon = gtk_icon_theme_load_icon(icon_theme, "gnome-mplayer", 16, 0, NULL);
        if (pb_icon)
            icon_list = g_list_prepend(icon_list, pb_icon);
    } else {
        pb_icon = gdk_pixbuf_new_from_xpm_data((const char **) gnome_mplayer_xpm);
        if (pb_icon)
            icon_list = g_list_prepend(icon_list, pb_icon);
    }
    gtk_window_set_default_icon_list(icon_list);
    gtk_window_set_icon_list(GTK_WINDOW(window), icon_list);

#ifdef GTK2_12_ENABLED
    if (control_id == 0 && show_status_icon) {
        if (gtk_icon_theme_has_icon(icon_theme, "gnome-mplayer")) {
            status_icon = gtk_status_icon_new_from_icon_name("gnome-mplayer");
        } else {
            status_icon = gtk_status_icon_new_from_pixbuf(pb_icon);
        }
        if (control_id != 0) {
            gtk_status_icon_set_visible(status_icon, FALSE);
        } else {
            gtk_status_icon_set_visible(status_icon, show_status_icon);
        }
        g_signal_connect(status_icon, "activate", G_CALLBACK(status_icon_callback), NULL);
        g_signal_connect(status_icon, "popup_menu", G_CALLBACK(status_icon_context_callback), NULL);
    }
#endif

    menu_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(menu_event_box), image_menu);
    gtk_button_set_relief(GTK_BUTTON(menu_event_box), GTK_RELIEF_NONE);
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(menu_event_box, _("Menu"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, menu_event_box, _("Menu"), NULL);
#endif
    gtk_widget_set_events(menu_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(menu_event_box), "button_press_event", G_CALLBACK(menu_callback), NULL);

    gtk_box_pack_start(GTK_BOX(hbox), menu_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_menu);
    gtk_widget_show(menu_event_box);

    prev_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(prev_event_box), image_prev);
    gtk_button_set_relief(GTK_BUTTON(prev_event_box), GTK_RELIEF_NONE);
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(prev_event_box, _("Previous"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, prev_event_box, _("Previous"), NULL);
#endif
    gtk_widget_set_events(prev_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(prev_event_box), "button_press_event", G_CALLBACK(prev_callback), NULL);

    gtk_box_pack_start(GTK_BOX(hbox), prev_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_prev);
    gtk_widget_show(prev_event_box);

    rew_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(rew_event_box), image_rew);
    gtk_button_set_relief(GTK_BUTTON(rew_event_box), GTK_RELIEF_NONE);
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(rew_event_box, _("Rewind"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, rew_event_box, _("Rewind"), NULL);
#endif
    gtk_widget_set_events(rew_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(rew_event_box), "button_press_event", G_CALLBACK(rew_callback), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), rew_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_rew);
    gtk_widget_show(rew_event_box);

    play_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(play_event_box), image_play);
    gtk_button_set_relief(GTK_BUTTON(play_event_box), GTK_RELIEF_NONE);
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(play_event_box, _("Play"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
#endif
    gtk_widget_set_events(play_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(play_event_box), "button_press_event", G_CALLBACK(play_callback), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), play_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_play);
    gtk_widget_show(play_event_box);

    stop_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(stop_event_box), image_stop);
    gtk_button_set_relief(GTK_BUTTON(stop_event_box), GTK_RELIEF_NONE);
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(stop_event_box, _("Stop"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, stop_event_box, _("Stop"), NULL);
#endif
    gtk_widget_set_events(stop_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(stop_event_box), "button_press_event", G_CALLBACK(stop_callback), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), stop_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_stop);
    gtk_widget_show(stop_event_box);

    ff_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(ff_event_box), image_ff);
    gtk_button_set_relief(GTK_BUTTON(ff_event_box), GTK_RELIEF_NONE);
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(ff_event_box, _("Fast Forward"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, ff_event_box, _("Fast Forward"), NULL);
#endif
    gtk_widget_set_events(ff_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(ff_event_box), "button_press_event", G_CALLBACK(ff_callback), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), ff_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_ff);
    gtk_widget_show(ff_event_box);

    next_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(next_event_box), image_next);
    gtk_button_set_relief(GTK_BUTTON(next_event_box), GTK_RELIEF_NONE);
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(next_event_box, _("Next"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, next_event_box, _("Next"), NULL);
#endif
    gtk_widget_set_events(next_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(next_event_box), "button_press_event", G_CALLBACK(next_callback), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), next_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_next);
    gtk_widget_show(next_event_box);

    // progress bar
    tracker = GMTK_MEDIA_TRACKER(gmtk_media_tracker_new());
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(tracker), TRUE, TRUE, 2);
    // g_signal_connect(G_OBJECT(tracker), "value-changed", G_CALLBACK(tracker_callback), NULL);
    g_signal_connect(G_OBJECT(tracker), "difference-changed", G_CALLBACK(tracker_difference_callback), NULL);
    g_signal_connect(G_OBJECT(tracker), "button_press_event", G_CALLBACK(progress_callback), NULL);
    gtk_widget_show(GTK_WIDGET(tracker));

    // fullscreen button, pack from end for this button and the vol slider
    fs_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(fs_event_box), image_fs);
    gtk_button_set_relief(GTK_BUTTON(fs_event_box), GTK_RELIEF_NONE);
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(fs_event_box, _("Full Screen"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, fs_event_box, _("Full Screen"), NULL);
#endif
    gtk_widget_set_events(fs_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(fs_event_box), "button_press_event", G_CALLBACK(fs_callback), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), fs_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_fs);
    if (!disable_fullscreen)
        gtk_widget_show(fs_event_box);


    // volume control
    if ((window_y > window_x)
        && (rpcontrols != NULL && g_strcasecmp(rpcontrols, "volumeslider") == 0)) {
        vol_slider = gtk_vscale_new_with_range(0.0, 1.0, 0.20);
        gtk_widget_set_size_request(vol_slider, -1, window_y);
        gtk_range_set_inverted(GTK_RANGE(vol_slider), TRUE);
    } else {
#ifdef GTK2_12_ENABLED
        vol_slider = gtk_volume_button_new();
        adj = gtk_scale_button_get_adjustment(GTK_SCALE_BUTTON(vol_slider));
        gtk_scale_button_set_adjustment(GTK_SCALE_BUTTON(vol_slider), adj);
        gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider), audio_device.volume);
        if (large_buttons)
            gtk_object_set(GTK_OBJECT(vol_slider), "size", GTK_ICON_SIZE_BUTTON, NULL);
        else
            gtk_object_set(GTK_OBJECT(vol_slider), "size", GTK_ICON_SIZE_MENU, NULL);

        g_signal_connect(G_OBJECT(vol_slider), "value_changed",
                         G_CALLBACK(vol_button_value_changed_callback), idledata);
        gtk_button_set_relief(GTK_BUTTON(vol_slider), GTK_RELIEF_NONE);
#else
        vol_slider = gtk_hscale_new_with_range(0.0, 1.0, 0.2);
        gtk_widget_set_size_request(vol_slider, 44, button_size);
        gtk_scale_set_draw_value(GTK_SCALE(vol_slider), FALSE);
        gtk_range_set_value(GTK_RANGE(vol_slider), audio_device.volume);
        g_signal_connect(G_OBJECT(vol_slider), "value_changed", G_CALLBACK(vol_slider_callback), idledata);
#endif
    }
#ifdef GTK2_12_ENABLED
    // no tooltip on the volume_button is needed
    // gtk_widget_set_tooltip_text(vol_slider, idledata->vol_tooltip);
#else
    volume_tip = gtk_tooltips_new();
    gtk_tooltips_set_tip(volume_tip, vol_slider, _("Volume 100%"), NULL);
#endif
    gtk_box_pack_end(GTK_BOX(hbox), vol_slider, FALSE, FALSE, 0);
#ifdef GTK2_20_ENABLED
    gtk_widget_set_can_focus(vol_slider, FALSE);
#else
    GTK_WIDGET_UNSET_FLAGS(vol_slider, GTK_CAN_FOCUS);
#endif
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
#ifdef GTK2_20_ENABLED
        if (gtk_widget_get_mapped(window))
            gtk_widget_unmap(window);
#else
        if (GTK_WIDGET_MAPPED(window))
            gtk_widget_unmap(window);
#endif
        gdk_window_reparent(get_window(window), window_container, 0, 0);
    }

    if (rpcontrols == NULL || (rpcontrols != NULL && g_strcasecmp(rpcontrols, "all") == 0)) {
        if (windowid != -1)
            gtk_widget_show_all(window);
        gtk_widget_hide(media_hbox);
        if (lastfile != NULL && g_ascii_strcasecmp(lastfile, "dvdnav://") == 0) {
            gtk_widget_show(menu_event_box);
        } else {
            gtk_widget_hide(menu_event_box);
        }
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
                gtk_widget_set_size_request(window, window_x, window_y);
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
                || g_strcasecmp(visuals[i], "positionfield") == 0 || g_strcasecmp(visuals[i], "positionslider") == 0) {
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
    g_signal_connect(G_OBJECT(fixed), "size_allocate", G_CALLBACK(allocate_fixed_callback), idledata);
    g_signal_connect(G_OBJECT(fixed), "expose_event", G_CALLBACK(expose_fixed_callback), NULL);

    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_fullscreen), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_details), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(fs_event_box), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_loop), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_subtitle), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_audiofile), FALSE);
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
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_decrease_subtitle_delay), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_increase_subtitle_delay), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_angle), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_advanced), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), FALSE);
    //gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    g_object_set_property(G_OBJECT(window), "allow-shrink", &ALLOW_SHRINK_FALSE);
    gtk_widget_hide(prev_event_box);
    gtk_widget_hide(next_event_box);
    gtk_widget_hide(GTK_WIDGET(menuitem_prev));
    gtk_widget_hide(GTK_WIDGET(menuitem_next));
    gtk_widget_hide(media_hbox);
    gtk_widget_hide(audio_meter);
    gtk_widget_hide(plvbox);
    gtk_widget_hide(details_vbox);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_details), details_visible);

    gtk_widget_hide(GTK_WIDGET(menuitem_edit_switch_audio));
    if (keep_on_top)
        gtk_window_set_keep_above(GTK_WINDOW(window), keep_on_top);

    update_status_icon();

}

void present_main_window()
{
    gtk_window_present(GTK_WINDOW(window));
}

static inline double logdb(double v)
{
    if (v > 1)
        return 0;
    if (v <= 1E-8)
        return METER_BARS - 1;
    return log(v) / -0.23025850929940456840179914546843642076;
}

gboolean update_audio_meter(gpointer data)
{
    gint i, j;
    gfloat f;
    gint max;
    //gfloat freq;
    gfloat v;
    //gfloat lsb16, rsb16;
    Export *export;
    //gint bucketid;
    static gint update_counter = 0;
    static gint export_counter = 0;
    gboolean refresh;
    long long histogram[65536];

    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_meter)))
        return TRUE;

    if (idledata->mapped_af_export == NULL)
        return TRUE;

    if (state != PLAYING)
        return TRUE;


    if (audio_meter != NULL && get_visible(audio_meter)) {

        if (data)
            g_array_free(data, TRUE);
        if (max_data)
            g_array_free(max_data, TRUE);
        data = g_array_new(FALSE, TRUE, sizeof(gfloat));
        max_data = g_array_new(FALSE, TRUE, sizeof(gfloat));

        for (i = 0; i < METER_BARS; i++) {
            buckets[i] = 0;
        }

        refresh = FALSE;
        reading_af_export = TRUE;
        export = NULL;
        if (idledata->mapped_af_export != NULL)
            export = (Export *) g_mapped_file_get_contents(idledata->mapped_af_export);
        if (export != NULL) {
            if (export->counter != export_counter) {
                /*
                   for (i = 0; export != NULL && i < (export->size / (export->nch * sizeof(gint16))); i++) {
                   freq = 0;

                   // this is an testing meter for two channel
                   for (j = 0; j < export->nch; j++) {
                   // scale SB16 data to 0 - 22000 range, believe this is Hz now
                   freq += (export->payload[j][i]) * 22000 / (32768 * export->nch);
                   }
                   // ignore values below 20, as this is unaudible and may skew data
                   if (freq > (22000.0 / METER_BARS)) {
                   bucketid = (gint) (freq / (gfloat) (22000.0 / (gfloat) METER_BARS)) - 1;
                   if (bucketid >= METER_BARS) {
                   printf("bucketid = %i freq = %f\n", bucketid, freq);
                   bucketid = METER_BARS - 1;
                   }
                   buckets[bucketid]++;
                   }

                   }
                   export_counter = export->counter;
                   refresh = TRUE;
                 */
                for (j = 0; j < 65336; j++) {
                    histogram[j] = 0;
                }
                for (i = 0; export != NULL && i < (export->size / (export->nch * sizeof(gint16))); i++) {
                    for (j = 0; j < export->nch; j++) {
                        // scale SB16 data to 0 - 22000 range, believe this is Hz now
                        histogram[(export->payload[j][i]) + 32768]++;
                    }
                }
                for (i = 0; i < 65536; i++) {
                    v = (i - 32768) / 32768.0;
                    buckets[(gint) (logdb(v * v) / 2.0)] += histogram[i];
                }
                buckets[0] = buckets[1] - (buckets[2] - buckets[1]);
                if (buckets[0] < 0)
                    buckets[0] = 0;
                export_counter = export->counter;
                refresh = TRUE;
            }
            // g_free(export);
            /*
               if (export->counter > export_counter) {
               for (j =0; j < export->nch ; j++) {
               lsb16 = 0;
               for(i=0; i < (export->size / ( export->nch * sizeof(gint16))); i++) {
               lsb16 += export->payload[j][i];
               bucketid = abs(lsb16) * METER_BARS / 1000000;
               buckets[bucketid - 1]++;
               }
               }
               export_counter = export->counter;
               refresh = TRUE;
               }
             */
        }
        reading_af_export = FALSE;

        if (refresh) {
            max = 0;
            update_counter++;
            for (i = 0; i < METER_BARS; i++) {
                if (buckets[i] > max_buckets[i]) {
                    max_buckets[i] = buckets[i];
                } else {
                    // raise this value for slower melting of max
                    if (update_counter % 1 == 0) {
                        update_counter = 0;
                        max_buckets[i]--;
                        if (max_buckets[i] < 0)
                            max_buckets[i] = 0;
                    }
                }

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
    }
    return TRUE;

}


void show_fs_controls()
{
    gint x, y;
    GdkScreen *screen;
    GdkRectangle rect;
    GtkAllocation alloc;

    if (fs_controls == NULL && fullscreen) {
        fs_controls = gtk_window_new(GTK_WINDOW_POPUP);
        gtk_widget_add_events(fs_controls, GDK_ENTER_NOTIFY_MASK);
        gtk_widget_add_events(fs_controls, GDK_LEAVE_NOTIFY_MASK);
        g_signal_connect(G_OBJECT(fs_controls), "enter_notify_event", G_CALLBACK(fs_controls_entered), NULL);
        g_signal_connect(G_OBJECT(fs_controls), "leave_notify_event", G_CALLBACK(fs_controls_left), NULL);
        g_object_ref(hbox);
        gtk_image_set_from_stock(GTK_IMAGE(image_fs), GTK_STOCK_LEAVE_FULLSCREEN, button_size);
        gtk_container_remove(GTK_CONTAINER(controls_box), hbox);
        gtk_container_add(GTK_CONTAINER(fs_controls), hbox);
        gtk_window_set_transient_for(GTK_WINDOW(fs_controls), GTK_WINDOW(fs_window));
        g_object_unref(hbox);
        gtk_widget_show(fs_controls);
#ifdef GTK2_12_ENABLED
        gtk_window_set_opacity(GTK_WINDOW(fs_controls), 0.75);
#endif
        //while (gtk_events_pending())
        //    gtk_main_iteration();
        // center fs_controls
        screen = gtk_window_get_screen(GTK_WINDOW(window));
        gtk_window_set_screen(GTK_WINDOW(fs_controls), screen);
        gdk_screen_get_monitor_geometry(screen, gdk_screen_get_monitor_at_window(screen, get_window(window)), &rect);

        get_allocation(fs_controls, &alloc);
        gtk_widget_set_size_request(fs_controls, rect.width / 2, alloc.height);

        x = rect.x + (rect.width / 4);
        y = rect.y + rect.height - alloc.height;
        gtk_window_move(GTK_WINDOW(fs_controls), x, y);
    }
}

void hide_fs_controls()
{

    if (fs_controls != NULL) {
        g_object_ref(hbox);
        gtk_image_set_from_stock(GTK_IMAGE(image_fs), GTK_STOCK_FULLSCREEN, button_size);
        gtk_container_remove(GTK_CONTAINER(fs_controls), hbox);
        gtk_container_add(GTK_CONTAINER(controls_box), hbox);
        g_object_unref(hbox);
        //while (gtk_events_pending())
        //    gtk_main_iteration();
        gtk_widget_destroy(fs_controls);
        fs_controls = NULL;
    }
}
