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

gint get_player_window()
{
    if (GTK_IS_WIDGET(drawing_area)) {
        return gtk_socket_get_id(GTK_SOCKET(drawing_area));
    } else {
        return 0;
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
    if (GTK_IS_WIDGET(menuitem_file_details))
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_file_details), TRUE);

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) < 2 && lastfile != NULL
        && g_strncasecmp(lastfile, "dvdnav", 6) != 0) {
        gtk_widget_hide(prev_event_box);
        gtk_widget_hide(next_event_box);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), FALSE);
    } else {
        gtk_widget_show_all(prev_event_box);
        gtk_widget_show_all(next_event_box);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), TRUE);
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
        if (idle->streaming == FALSE) {
            if (g_strrstr(idle->info, "/") != NULL) {
                name = g_strdup_printf("%s", g_strrstr(idle->info, "/") + 1);
            } else {
                name = g_strdup(idle->info);
            }

            total = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
            path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &iter);
            if (path != NULL) {
                current = (gint) g_strtod(gtk_tree_path_to_string(path), NULL);
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
    }
    return FALSE;
}


gboolean set_media_info_name(gchar * filename)
{

    gchar *buf;
    gchar *name;

    if (filename != NULL) {
        if (g_strrstr(filename, "/") != NULL) {
            name = g_strdup_printf("%s", g_strrstr(filename, "/") + 1);
        } else {
            name = g_strdup(filename);
        }
        buf = g_strdup_printf(_("%s - GNOME MPlayer"), name);
        gtk_window_set_title(GTK_WINDOW(window), buf);
        g_free(buf);
        g_free(name);
    }
    return FALSE;
}


gboolean set_media_label(void *data)
{

    IdleData *idle = (IdleData *) data;

    if (data != NULL && idle != NULL && GTK_IS_WIDGET(media_label)) {
        gtk_label_set_markup(GTK_LABEL(media_label), idle->media_info);
        gtk_label_set_max_width_chars(GTK_LABEL(media_label), 10);
    }

    if (idle->videopresent == FALSE && show_media_label) {
        gtk_widget_show(media_label);
    }

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

        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_file_details), TRUE);
    }

    if (idle->fromdbus == FALSE)
        dbus_send_rpsignal_with_string("RP_SetMediaLabel", idle->media_info);

    return FALSE;
}

gboolean set_progress_value(void *data)
{

    IdleData *idle = (IdleData *) data;
    gchar *text;
    struct stat buf;
    gchar *iterfilename;

    if (GTK_IS_WIDGET(progress)) {
        if (state == QUIT && rpcontrols == NULL) {
            js_state = STATE_BUFFERING;
            gtk_progress_bar_update(progress, idle->cachepercent);
            gtk_widget_set_sensitive(play_event_box, FALSE);
        } else {
            gtk_progress_bar_update(progress, idle->percent);
            if (autopause == FALSE)
                gtk_widget_set_sensitive(play_event_box, TRUE);
        }
        if (idle->cachepercent < 1.0 && state == PAUSED) {
            text =
                g_strdup_printf(_("Paused | %2i%% \342\226\274"),
                                (gint) (idle->cachepercent * 100));
            gtk_progress_bar_set_text(progress, text);
            g_free(text);
        } else {
            gtk_progress_bar_set_text(progress, idle->progress_text);
        }
    }

    if (idle->cachepercent > 0.0 && idle->cachepercent < 0.9) {
        if (autopause == FALSE && state == PLAYING) {
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iterfilename,
                               -1);
            if (iterfilename != NULL) {
                g_stat(iterfilename, &buf);
                //printf("filename = %s, disk size = %i, byte pos = %i\n",iterfilename,buf.st_size,idle->byte_pos);
                if (((idle->percent + 0.05) > idle->cachepercent)
                    || ((idle->byte_pos + (512 * 1024)) > buf.st_size)) {
                    pause_callback(NULL, NULL, NULL);
                    gtk_widget_set_sensitive(play_event_box, FALSE);
                    autopause = TRUE;
                }
                g_free(iterfilename);
            }
        } else if (autopause == TRUE && state == PAUSED) {
            if (idle->cachepercent > (idle->percent + 0.10)) {
                play_callback(NULL, NULL, NULL);
                gtk_widget_set_sensitive(play_event_box, TRUE);
                autopause = FALSE;
            }
        }
    }

    if (idle->cachepercent > 0.9) {
        if (autopause == TRUE && state == PAUSED) {
            play_callback(NULL, NULL, NULL);
            gtk_widget_set_sensitive(play_event_box, TRUE);
            autopause = FALSE;
        }
    }

    if (state == QUIT) {
        gtk_widget_set_sensitive(play_event_box, TRUE);
    }
    if (idle->fromdbus == FALSE) {
        dbus_send_rpsignal_with_double("RP_SetPercent", idle->percent);
        dbus_send_rpsignal_with_int("RP_SetGUIState", state);
    }

    return FALSE;
}

gboolean set_progress_text(void *data)
{

    IdleData *idle = (IdleData *) data;


    if (GTK_IS_WIDGET(progress)) {
        gtk_progress_bar_set_text(progress, idle->progress_text);
    }
    if (idle->fromdbus == FALSE)
        dbus_send_rpsignal_with_string("RP_SetProgressText", idle->progress_text);

    return FALSE;
}

gboolean set_progress_time(void *data)
{
    int hour = 0, min = 0, length_hour = 0, length_min = 0;
    long int seconds, length_seconds;

    IdleData *idle = (IdleData *) data;

    seconds = (int) idle->position;
    if (seconds >= 3600) {
        hour = seconds / 3600;
        seconds = seconds - (hour * 3600);
    }
    if (seconds >= 60) {
        min = seconds / 60;
        seconds = seconds - (min * 60);
    }
    length_seconds = (int) idle->length;
    if (length_seconds >= 3600) {
        length_hour = length_seconds / 3600;
        length_seconds = length_seconds - (length_hour * 3600);
    }
    if (length_seconds >= 60) {
        length_min = length_seconds / 60;
        length_seconds = length_seconds - (length_min * 60);
    }

    if (hour == 0 && length_hour == 0) {

        if ((int) idle->length == 0 || idle->position > idle->length) {
            if (idle->cachepercent > 0 && idle->cachepercent < 1.0) {
                g_snprintf(idle->progress_text, 128,
                           _("%2i:%02i | %2i%% \342\226\274"),
                           min, (int) seconds, (int) (idle->cachepercent * 100));
            } else {
                g_snprintf(idle->progress_text, 128, _("%2i:%02i"), min, (int) seconds);
            }
        } else {
            if (idle->cachepercent > 0 && idle->cachepercent < 1.0) {
                g_snprintf(idle->progress_text, 128,
                           _("%2i:%02i / %2i:%02i | %2i%% \342\226\274"),
                           min, (int) seconds, length_min,
                           (int) length_seconds, (int) (idle->cachepercent * 100));
            } else {
                g_snprintf(idle->progress_text, 128,
                           _("%2i:%02i / %2i:%02i"),
                           min, (int) seconds, length_min, (int) length_seconds);
            }
        }
    } else {
        if ((int) idle->length == 0 || idle->position > idle->length || length_hour > 24) {

            if (idle->cachepercent > 0 && idle->cachepercent < 1.0) {
                g_snprintf(idle->progress_text, 128,
                           _("%i:%02i:%02i | %2i%% \342\226\274"),
                           hour, min, (int) seconds, (int) (idle->cachepercent * 100));
            } else {
                g_snprintf(idle->progress_text, 128, _("%i:%02i:%02i"), hour, min, (int) seconds);
            }

        } else {

            if (idle->cachepercent > 0 && idle->cachepercent < 1.0) {
                g_snprintf(idle->progress_text, 128,
                           _("%i:%02i:%02i / %i:%02i:%02i | %2i%% \342\226\274"),
                           hour, min, (int) seconds,
                           length_hour, length_min,
                           (int) length_seconds, (int) (idle->cachepercent * 100));
            } else {
                g_snprintf(idle->progress_text, 128,
                           _("%i:%02i:%02i / %i:%02i:%02i"),
                           hour, min, (int) seconds, length_hour, length_min, (int) length_seconds);
            }

        }
    }

    if (GTK_IS_WIDGET(progress) && idle->position > 0 && state != PAUSED) {
        gtk_progress_bar_set_text(progress, idle->progress_text);
    }

    if (idle->fromdbus == FALSE && state != PAUSED)
        dbus_send_rpsignal_with_string("RP_SetProgressText", idle->progress_text);

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
    send_command(cmd);
    g_free(cmd);
    send_command("get_property volume\n");
    if (state == PAUSED || state == STOPPED) {
        send_command("pause\n");
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

    if (GTK_IS_WIDGET(fixed)) {
        gtk_widget_show_all(fixed);
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
        }

        if (guistate == PAUSED) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_play);
            gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
            gtk_widget_set_sensitive(ff_event_box, FALSE);
            gtk_widget_set_sensitive(rew_event_box, FALSE);
        }

        if (guistate == STOPPED) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_play);
            gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
            gtk_widget_set_sensitive(ff_event_box, FALSE);
            gtk_widget_set_sensitive(rew_event_box, FALSE);
        }
        lastguistate = guistate;
    }
    return FALSE;
}


gboolean resize_window(void *data)
{

    IdleData *idle = (IdleData *) data;
    gint total_height = 0;
    gint total_width = 0;
    GtkRequisition req;
    GTimeVal currenttime;

    if (GTK_IS_WIDGET(window)) {
        if (idle->videopresent) {
            gtk_widget_show(vbox);
            show_media_label = FALSE;
            gtk_widget_hide(media_label);
            g_get_current_time(&currenttime);
            last_movement_time = currenttime.tv_sec;
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_info), TRUE);
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_subtitle), TRUE);
            dbus_disable_screensaver();
            if (embed_window == -1) {
                gtk_widget_show_all(window);
                gtk_widget_hide(media_label);
                hide_buttons(idle);
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols),
                                               showcontrols);
            }
            gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, TRUE);

            if (window_x == 0 && window_y == 0) {
                gtk_widget_show_all(GTK_WIDGET(fixed));
                gtk_widget_set_size_request(fixed, -1, -1);
                gtk_widget_set_size_request(drawing_area, -1, -1);
                //printf("%i x %i \n",idle->x,idle->y);
                if (last_window_width == 0 && last_window_height == 0) {
                    if (idle->width > 0 && idle->height > 0) {
                        gtk_widget_set_size_request(fixed, idle->width, idle->height);
                        gtk_widget_set_size_request(drawing_area, idle->width, idle->height);
                        total_height = idle->height;
                        gtk_widget_size_request(GTK_WIDGET(menubar), &req);
                        total_height += req.height;
                        if (showcontrols) {
                            gtk_widget_size_request(GTK_WIDGET(controls_box), &req);
                            total_height += req.height;
                        }
                        if (GTK_WIDGET_VISIBLE(media_label)) {
                            gtk_widget_size_request(GTK_WIDGET(media_label), &req);
                            total_height += req.height;
                        }

                        total_width = idle->width;
                        if (GTK_IS_WIDGET(plvbox) && GTK_WIDGET_VISIBLE(plvbox)) {
                            gtk_widget_size_request(GTK_WIDGET(plvbox), &req);
                            total_height += req.height;
                            total_width = idle->width + req.width;
                        }

                        gtk_window_resize(GTK_WINDOW(window), total_width, total_height);
                        last_window_width = idle->width;
                        last_window_height = idle->height;
                    }
                }
            } else {
                if (window_x > 0 && window_y > 0) {
                    total_height = window_y;
                    gtk_widget_set_size_request(fixed, -1, -1);
                    gtk_widget_set_size_request(drawing_area, -1, -1);
                    gtk_widget_show_all(GTK_WIDGET(fixed));
                    if (showcontrols) {
                        gtk_widget_size_request(GTK_WIDGET(controls_box), &req);
                        total_height -= req.height;
                    }

                    if (window_x > 0 && total_height > 0)
                        gtk_widget_set_size_request(fixed, window_x, total_height);
                    gtk_window_resize(GTK_WINDOW(window), window_x, window_y);
                }
            }
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_info), FALSE);
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_subtitle), FALSE);
            if (window_x > 0 && window_y > 0) {
                total_height = window_y;
                gtk_widget_set_size_request(fixed, -1, -1);
                gtk_widget_set_size_request(drawing_area, -1, -1);
                gtk_widget_hide_all(GTK_WIDGET(drawing_area));
                if (showcontrols && rpcontrols == NULL) {
                    gtk_widget_size_request(GTK_WIDGET(controls_box), &req);
                    total_height -= req.height;
                }
                if (window_x > 0 && total_height > 0) {
                    gtk_widget_set_size_request(media_label, window_x, total_height);
                }
                gtk_window_resize(GTK_WINDOW(window), window_x, window_y);
            } else {
                if (embed_window != -1) {
                    show_media_label = TRUE;
                    gtk_widget_show(media_label);
                    if (GTK_IS_WIDGET(plvbox) && GTK_WIDGET_VISIBLE(plvbox)) {
                        // gtk_widget_hide(drawing_area);
                        gtk_widget_hide(vbox);
                    } else {
                        gtk_widget_hide_all(GTK_WIDGET(fixed));
                        if (GTK_IS_WIDGET(plvbox) && GTK_WIDGET_VISIBLE(plvbox)) {
                            gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
                            gtk_widget_show(GTK_WIDGET(fixed));
                            gtk_widget_show(GTK_WIDGET(media_label));
                        } else {
                            gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);
                        }
                        gtk_widget_set_size_request(fixed, -1, -1);
                        gtk_widget_set_size_request(drawing_area, -1, -1);
                        gtk_widget_size_request(GTK_WIDGET(menubar), &req);
                        total_height = req.height;
                        if (showcontrols && rpcontrols == NULL) {
                            gtk_widget_size_request(GTK_WIDGET(controls_box), &req);
                            total_height += req.height;
                        }
                        if (GTK_WIDGET_VISIBLE(media_label)) {
                            gtk_widget_size_request(GTK_WIDGET(media_label), &req);
                            total_height += req.height;
                        }
                        if (req.width > 0 && total_height > 0)
                            gtk_window_resize(GTK_WINDOW(window), req.width, total_height);
                    }
                }
            }
        }
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_fullscreen), idle->videopresent);
        gtk_widget_set_sensitive(GTK_WIDGET(fs_event_box), idle->videopresent);
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
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_subtitles), idle->videopresent);

    }
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

    cmd = g_strdup_printf("seek %5.0f 2\n", idle->position);
    send_command(cmd);
    g_free(cmd);
    return FALSE;
}

gboolean set_volume(void *data)
{
    IdleData *idle = (IdleData *) data;
    gchar *buf = NULL;

    if (GTK_IS_WIDGET(vol_slider)) {
        // printf("setting slider to %f\n", idle->volume);
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
    if (idle->videopresent)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen), idle->fullscreen);
    return FALSE;
}

gboolean set_show_controls(void *data)
{

    IdleData *idle = (IdleData *) data;

    showcontrols = (gint) idle->showcontrols;

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), showcontrols);
    return FALSE;
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

        if (event_button->button == 1 && idledata->videopresent == TRUE) {
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
    shutdown();
    while (gtk_events_pending() || thread != NULL) {
        gtk_main_iteration();
    }

    if (control_id != 0)
        dbus_cancel();

    dbus_unhook();

    gtk_main_quit();
    return FALSE;
}

gboolean motion_notify_callback(GtkWidget * widget, GdkEventMotion * event, gpointer data)
{
    GTimeVal currenttime;

    g_get_current_time(&currenttime);
    last_movement_time = currenttime.tv_sec;

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
    GdkGC *gc;

    if (GDK_IS_DRAWABLE(fixed->window)) {
        if (videopresent || embed_window != 0) {
            gc = gdk_gc_new(fixed->window);
            // printf("drawing box %i x %i at %i x %i \n",event->area.width,event->area.height, event->area.x, event->area.y );
            gdk_draw_rectangle(fixed->window, gc, TRUE, event->area.x, event->area.y,
                               event->area.width, event->area.height);
            gdk_gc_unref(gc);
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
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three)))
            movie_ratio = 4.0 / 3.0;
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine)))
            movie_ratio = 16.0 / 9.0;
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten)))
            movie_ratio = 16.0 / 10.0;

        window_ratio = (gdouble) allocation->width / (gdouble) allocation->height;

        if (allocation->width == idledata->width && allocation->height == idledata->width) {
            new_width = allocation->width;
            new_height = allocation->height;
        } else {
            if (movie_ratio > window_ratio) {
                //printf("movie %lf > window %lf\n",movie_ratio,window_ratio);
                new_width = allocation->width;
                new_height = allocation->width / movie_ratio;
            } else {
                //printf("movie %lf < window %lf\n",movie_ratio,window_ratio);
                new_height = allocation->height;
                new_width = allocation->height * movie_ratio;
            }
        }
        //printf("new movie size = %i x %i (%i x %i)\n",new_width,new_height,allocation->width, allocation->height);
        gtk_widget_set_usize(drawing_area, new_width, new_height);

        idledata->x = (allocation->width - new_width) / 2;
        idledata->y = (allocation->height - new_height) / 2;
        move_window(idledata);
        return FALSE;
    } else {
        return TRUE;
    }

}

gboolean window_key_callback(GtkWidget * widget, GdkEventKey * event, gpointer user_data)
{
    GTimeVal currenttime;
    gchar *cmd;

    // printf("key = %i\n",event->keyval);
    // printf("state = %i\n",event->state);
    // printf("other = %i\n", event->state & ~GDK_CONTROL_MASK);

    // We don't want to handle CTRL accelerators here
    // if we pass in items with CTRL then 2 and Ctrl-2 do the same thing
    if (event->state == (event->state & (~GDK_CONTROL_MASK))) {

        g_get_current_time(&currenttime);
        last_movement_time = currenttime.tv_sec;

        g_idle_add(make_panel_and_mouse_visible, NULL);
        switch (event->keyval) {
        case GDK_Right:
            if (g_strncasecmp(lastfile, "dvdnav", strlen("dvdnav")) == 0) {
                send_command("dvdnav 4\n");
                return FALSE;
            } else {
                return ff_callback(NULL, NULL, NULL);
            }
            break;
        case GDK_Left:
            if (g_strncasecmp(lastfile, "dvdnav", strlen("dvdnav")) == 0) {
                send_command("dvdnav 3\n");
                return FALSE;
            } else {
                return rew_callback(NULL, NULL, NULL);
            }
            break;
        case GDK_Page_Up:
            if (state == PLAYING)
                send_command("pausing_keep seek +600 0\n");
            return FALSE;
        case GDK_Page_Down:
            if (state == PLAYING)
                send_command("pausing_keep seek -600 0\n");
            return FALSE;
        case GDK_Up:
            if (g_strncasecmp(lastfile, "dvdnav", strlen("dvdnav")) == 0) {
                send_command("dvdnav 1\n");
            } else {
                if (state == PLAYING)
                    send_command("pausing_keep seek +60 0\n");
            }

            return FALSE;
        case GDK_Down:
            if (g_strncasecmp(lastfile, "dvdnav", strlen("dvdnav")) == 0) {
                send_command("dvdnav 2\n");
            } else {
                if (state == PLAYING)
                    send_command("pausing_keep seek -60 0\n");
            }
            return FALSE;
        case GDK_Return:
            if (g_strncasecmp(lastfile, "dvdnav", strlen("dvdnav")) == 0) {
                send_command("dvdnav 6\n");
            }
            return FALSE;
        case GDK_space:
        case GDK_p:
            return play_callback(NULL, NULL, NULL);
            break;
        case GDK_m:
            if (idledata->mute) {
                send_command("pausing_keep mute 0\n");
                idledata->mute = 0;
            } else {
                send_command("pausing_keep mute 1\n");
                idledata->mute = 1;
            }
            return FALSE;

        case GDK_1:
            send_command("pausing_keep contrast -5\n");
            send_command("get_property contrast\n");
            return FALSE;
        case GDK_2:
            send_command("pausing_keep contrast 5\n");
            send_command("get_property contrast\n");
            return FALSE;
        case GDK_3:
            send_command("pausing_keep brightness -5\n");
            send_command("get_property brightness\n");
            return FALSE;
        case GDK_4:
            send_command("pausing_keep brightness 5\n");
            send_command("get_property brightness\n");
            return FALSE;
        case GDK_5:
            send_command("pausing_keep hue -5\n");
            send_command("get_property hue\n");
            return FALSE;
        case GDK_6:
            send_command("pausing_keep hue 5\n");
            send_command("get_property hue\n");
            return FALSE;
        case GDK_7:
            send_command("pausing_keep saturation -5\n");
            send_command("get_property saturation\n");
            return FALSE;
        case GDK_8:
            send_command("pausing_keep saturation 5\n");
            send_command("get_property saturation\n");
            return FALSE;
        case GDK_bracketleft:
            send_command("pausing_keep speed_mult 0.90\n");
            return FALSE;
        case GDK_bracketright:
            send_command("pausing_keep speed_mult 1.10\n");
            return FALSE;
        case GDK_braceleft:
            send_command("pausing_keep speed_mult 0.50\n");
            return FALSE;
        case GDK_braceright:
            send_command("pausing_keep speed_mult 2.0\n");
            return FALSE;
        case GDK_BackSpace:
            send_command("pausing_keep speed_set 1.0\n");
            return FALSE;
        case GDK_9:
            gtk_range_set_value(GTK_RANGE(vol_slider),
                                gtk_range_get_value(GTK_RANGE(vol_slider)) - 10);
            return FALSE;
        case GDK_0:
            gtk_range_set_value(GTK_RANGE(vol_slider),
                                gtk_range_get_value(GTK_RANGE(vol_slider)) + 10);
            return FALSE;
        case GDK_numbersign:
            send_command("pausing_keep switch_audio\n");
            return FALSE;
        case GDK_j:
            send_command("pausing_keep sub_select\n");
            return FALSE;
        case GDK_q:
            delete_callback(NULL, NULL, NULL);
            return FALSE;
        case GDK_v:
            //send_command("pausing_keep sub_visibility\n");
            return FALSE;
        case GDK_plus:
        case GDK_KP_Add:
            send_command("pausing_keep audio_delay 0.1 0\n");
            return FALSE;
        case GDK_minus:
        case GDK_KP_Subtract:
            send_command("pausing_keep audio_delay -0.1 0\n");
            return FALSE;
        case GDK_z:
            send_command("pausing_keep sub_delay -0.1 0\n");
            return FALSE;
        case GDK_x:
            send_command("pausing_keep sub_delay 0.1 0\n");
            return FALSE;
        case GDK_F11:
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen), !fullscreen);
            return FALSE;
		case GDK_Escape:
			return delete_callback(NULL, NULL, NULL);
        default:
            if (state == PLAYING) {
                cmd = g_strdup_printf("key_down_event %d\n", event->keyval);
                send_command(cmd);
                g_free(cmd);
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
    gchar *filename;
    GtkTreeIter localiter;
    gchar **list;
    gint i = 0;
    gint playlist;
    gint itemcount;

    /* Important, check if we actually got data.  Sometimes errors
     * occure and selection_data will be NULL.
     */
    if (selection_data == NULL)
        return FALSE;
    if (selection_data->length < 0)
        return FALSE;

    if ((info == DRAG_INFO_0) || (info == DRAG_INFO_1) || (info == DRAG_INFO_2)) {
        filename = g_filename_from_uri((const gchar *) selection_data->data, NULL, NULL);
        if (filename == NULL)
            return FALSE;
        list = g_strsplit(filename, "\n", 0);
        //gtk_list_store_clear(playliststore);
        //gtk_list_store_clear(nonrandomplayliststore);
        itemcount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);

        while (list[i] != NULL) {
            g_strchomp(list[i]);
            if (strlen(list[i]) > 0) {
                playlist = detect_playlist(list[i]);

                if (!playlist) {
                    add_item_to_playlist(list[i], playlist);
                } else {
                    if (!parse_playlist(list[i])) {
                        localiter = add_item_to_playlist(list[i], playlist);
                    }
                }
            }
            i++;
        }

        if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) > itemcount) {
            gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(playliststore), &iter, NULL, itemcount);
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &filename,
                               PLAYLIST_COLUMN, &playlist, -1);

            shutdown();
            play_file(filename, playlist);
            g_free(filename);
        }
        g_strfreev(list);
        update_gui();
    }
    return FALSE;

}

gboolean pause_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    return play_callback(widget, event, data);
}

gboolean play_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    IdleData *idle = (IdleData *) data;
    gchar *filename;
    gint count, playlist;
    GtkTreePath *path;

    if (state == PAUSED || state == STOPPED) {
        send_command("seek 0 0\n");
        state = PLAYING;
        js_state = STATE_PLAYING;
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_pause);
        gtk_tooltips_set_tip(tooltip, play_event_box, _("Pause"), NULL);
        g_strlcpy(idledata->progress_text, _("Playing"), 1024);
        g_idle_add(set_progress_text, idledata);
        gtk_widget_set_sensitive(ff_event_box, TRUE);
        gtk_widget_set_sensitive(rew_event_box, TRUE);
    } else if (state == PLAYING) {
        send_command("pause\n");
        state = PAUSED;
        js_state = STATE_PAUSED;
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_play);
        gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
        g_strlcpy(idledata->progress_text, _("Paused"), 1024);
        g_idle_add(set_progress_text, idledata);
        gtk_widget_set_sensitive(ff_event_box, FALSE);
        gtk_widget_set_sensitive(rew_event_box, FALSE);
    }

    if (state == QUIT) {
        if (next_item_in_playlist(&iter)) {
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &filename,
                               COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
            set_media_info_name(filename);
            play_file(filename, playlist);
            gtk_list_store_set(playliststore, &iter, COUNT_COLUMN, count + 1, -1);
            g_free(filename);
            if (GTK_IS_TREE_SELECTION(selection)) {
                path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &iter);
                gtk_tree_selection_select_path(selection, path);
                if (GTK_IS_WIDGET(list))
                    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list), path, NULL, FALSE, 0, 0);
                gtk_tree_path_free(path);
            }
        } else {
            if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &filename,
                                   COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
                set_media_info_name(filename);
                play_file(filename, playlist);
                gtk_list_store_set(playliststore, &iter, COUNT_COLUMN, count + 1, -1);
                g_free(filename);
                if (GTK_IS_TREE_SELECTION(selection)) {
                    path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &iter);
                    gtk_tree_selection_select_path(selection, path);
                    if (GTK_IS_WIDGET(list))
                        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list), path, NULL, FALSE, 0, 0);
                    gtk_tree_path_free(path);
                }
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
        send_command("pause\n");
        if (idle == NULL)
            dbus_send_rpsignal("RP_Play");
        state = PLAYING;
    }
    if (state == PLAYING) {
        if (idledata != NULL && idledata->streaming) {
            send_command("quit\n");
            state = QUIT;
            autopause = FALSE;
        } else {
            send_command("seek 0 2\npause\n");
            state = STOPPED;
            autopause = FALSE;
        }
        gtk_widget_set_sensitive(play_event_box, TRUE);
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_play);
        gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
    }
    if (state == QUIT) {
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
    if (state == PLAYING) {
        send_command("seek +10 0\n");
    }

    if (rpconsole != NULL && widget != NULL) {
        dbus_send_rpsignal("RP_FastForward");
    }

    return FALSE;
}

gboolean rew_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    if (state == PLAYING) {
        send_command("seek -10 0\n");
    }

    if (rpconsole != NULL && widget != NULL) {
        dbus_send_rpsignal("RP_FastReverse");
    }

    return FALSE;
}

gboolean prev_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    gchar *filename;
    gchar *iterfilename;
    gchar *localfilename;
    gint count;
    gint playlist;
    GtkTreeIter localiter;
    GtkTreeIter previter;
    gboolean valid = FALSE;
    GtkTreePath *path;

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iterfilename, -1);
        if (g_strncasecmp(lastfile, "dvdnav", strlen("dvdnav")) == 0) {
            valid = FALSE;
            send_command("seek_chapter -2 0\n");
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

        if (lastfile != NULL && g_strncasecmp(lastfile, "dvdnav", strlen("dvdnav")) == 0) {
            valid = FALSE;
            send_command("seek_chapter -1 0\n");
        }
    }

    if (valid) {
        dontplaynext = TRUE;
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &previter, ITEM_COLUMN, &filename,
                           COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
        shutdown();
        set_media_info_name(filename);
        play_file(filename, playlist);
        gtk_list_store_set(playliststore, &previter, COUNT_COLUMN, count + 1, -1);
        g_free(filename);
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
        if (g_strncasecmp(lastfile, "dvdnav", strlen("dvdnav")) == 0) {
            send_command("seek_chapter 1 0\n");
        } else {
            shutdown();
            if (autopause) {
                autopause = FALSE;
                gtk_widget_set_sensitive(play_event_box, TRUE);
                gtk_image_set_from_pixbuf(GTK_IMAGE(image_play), pb_play);
            }
            gtk_widget_set_sensitive(ff_event_box, TRUE);
            gtk_widget_set_sensitive(rew_event_box, TRUE);
        }
    } else {
        if (lastfile != NULL && g_strncasecmp(lastfile, "dvdnav", strlen("dvdnav")) == 0) {
            send_command("seek_chapter 1 0\n");
        }
    }

    return FALSE;
}

gboolean menu_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    send_command("dvdnav 5\n");
    return FALSE;
}

void vol_slider_callback(GtkRange * range, gpointer user_data)
{
    gint vol;
    gchar *cmd;
    gchar *buf;

    vol = (gint) gtk_range_get_value(range);
    cmd = g_strdup_printf("pausing_keep volume %i 1\n", vol);
    send_command(cmd);
    g_free(cmd);
    if (idledata->volume != vol) {

        buf = g_strdup_printf(_("Volume %i%%"), vol);
        g_strlcpy(idledata->vol_tooltip, buf, 128);
        g_idle_add(set_volume_tip, idledata);
        g_free(buf);
    }
    send_command("get_property volume\n");

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
    cmd = g_strdup_printf("pausing_keep volume %i 1\n", vol);
    send_command(cmd);
    g_free(cmd);
    if (idledata->volume != vol) {

        buf = g_strdup_printf(_("Volume %i%%"), vol);
        g_strlcpy(idledata->vol_tooltip, buf, 128);
        g_idle_add(set_volume_tip, idledata);
        g_free(buf);
    }
    send_command("get_property volume\n");

    dbus_send_rpsignal_with_double("RP_Volume",
                                   gtk_scale_button_get_value(GTK_SCALE_BUTTON(vol_slider)));

}
#endif

gboolean slide_panel_away(gpointer data)
{
    GtkRequisition req;

    if (GTK_IS_WIDGET(controls_box) && GTK_WIDGET_VISIBLE(controls_box)) {
        gtk_widget_size_request(controls_box, &req);
        if (req.height <= 1) {
            gtk_widget_hide(controls_box);
            g_mutex_unlock(slide_away);
            return FALSE;
        } else {
            gtk_widget_set_size_request(controls_box, req.width, req.height - 1);
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

    if (fullscreen) {
        g_get_current_time(&currenttime);
        g_time_val_add(&currenttime, -5 * G_USEC_PER_SEC);
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
        }

    }
    return FALSE;
}

gboolean make_panel_and_mouse_visible(gpointer data)
{
    if (fullscreen) {

        if (showcontrols && GTK_IS_WIDGET(controls_box) && !GTK_WIDGET_VISIBLE(controls_box)) {
            gtk_widget_set_size_request(controls_box, -1, -1);
            gtk_widget_show(controls_box);
        }
        gdk_window_set_cursor(window->window, NULL);
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
    gchar *name;
    GConfClient *gconf;
    gchar *last_dir;
    gint playlist, count;
    GtkTreeViewColumn *column;
    gchar *coltitle;

    dialog = gtk_file_chooser_dialog_new(_("Open File"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    /*allow multiple files to be selected */
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

    gconf = gconf_client_get_default();
    last_dir = gconf_client_get_string(gconf, LAST_DIR, NULL);
    if (last_dir != NULL) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), last_dir);
        g_free(last_dir);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        filename = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        last_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
        if (last_dir != NULL) {
            gconf_client_set_string(gconf, LAST_DIR, last_dir, NULL);
            g_free(last_dir);
        }

        dontplaynext = TRUE;
        shutdown();
        gtk_list_store_clear(playliststore);
        gtk_list_store_clear(nonrandomplayliststore);

        if (filename != NULL) {
            g_slist_foreach(filename, &add_item_to_playlist_callback, NULL);
            g_slist_free(filename);

            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &name,
                               COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
            set_media_info_name(name);
            play_file(name, playlist);
            gtk_list_store_set(playliststore, &iter, COUNT_COLUMN, count + 1, -1);
            g_free(name);
            dontplaynext = FALSE;
        }
    }

    g_object_unref(G_OBJECT(gconf));
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
    gint count;
    GtkTreeIter localiter;

    filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(open_location)));

    if (filename != NULL && strlen(filename) > 0) {
        dontplaynext = TRUE;
        shutdown();
        gtk_list_store_clear(playliststore);
        gtk_list_store_clear(nonrandomplayliststore);

        if (filename != NULL) {

            playlist = detect_playlist(filename);

            if (!playlist) {
                localiter = add_item_to_playlist(filename, playlist);
            } else {
                if (!parse_playlist(filename)) {
                    localiter = add_item_to_playlist(filename, playlist);
                }

            }

            g_free(filename);
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &filename,
                               COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
            set_media_info_name(filename);
            play_file(filename, playlist);
            gtk_list_store_set(playliststore, &iter, COUNT_COLUMN, count + 1, -1);
            g_free(filename);
            dontplaynext = FALSE;
        }
    }
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
    gtk_widget_grab_default(open_button);
}


void menuitem_open_dvd_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *filename = NULL;
    gint count;
    gint playlist = 0;

    gtk_list_store_clear(playliststore);
    gtk_list_store_clear(nonrandomplayliststore);
    parse_dvd("dvd://");

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &filename,
                           COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
        set_media_info_name(filename);
        printf("playing - %s is playlist = %i\n", filename, playlist);
        play_file(filename, playlist);
        gtk_list_store_set(playliststore, &iter, COUNT_COLUMN, count + 1, -1);
        g_free(filename);
    }
}

void menuitem_open_dvdnav_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_list_store_clear(playliststore);
    gtk_list_store_clear(nonrandomplayliststore);
    add_item_to_playlist("dvdnav://", 0);
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
    play_file("dvdnav://", 0);
    gtk_widget_show(menu_event_box);
}

void menuitem_open_acd_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *filename = NULL;
    gint count;
    gint playlist = 0;

    gtk_list_store_clear(playliststore);
    gtk_list_store_clear(nonrandomplayliststore);
    parse_playlist("cdda://");

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &filename,
                           COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
        set_media_info_name(filename);
        printf("playing - %s is playlist = %i\n", filename, playlist);
        play_file(filename, playlist);
        gtk_list_store_set(playliststore, &iter, COUNT_COLUMN, count + 1, -1);
        g_free(filename);
    }

}

void menuitem_open_atv_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_list_store_clear(playliststore);
    gtk_list_store_clear(nonrandomplayliststore);
    add_item_to_playlist("tv://", 0);
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
    play_file("tv://", 0);
}

void parseChannels(FILE * f)
{
    int parsing = 0, i = 0, firstW = 0, firstP = 0;
    char ch, s[20], strout[50];

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
                    strout[49] = '\0';
                    strcpy(strout, "dvb://");
                    strcat(strout, s);
                    add_item_to_playlist(strout, 0);    //add to playlist
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
    mpconf = g_strdup_printf("%s/.mplayer/channels.conf", getenv("HOME"));
    fi = fopen(mpconf, "r");    // Make sure this is pointing to
    // the appropriate file
    if (fi != NULL) {
        parseChannels(fi);
        //fclose( fi );
    } else
        printf("Unable to open the config file\n");     //can change this to whatever error message system is used

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
    play_file("dvb://", 0);
}

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

void menuitem_about_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *authors[] = { "Kevin DeKorte", "James Carthew", "Diogo Franco", NULL };
    gtk_show_about_dialog(GTK_WINDOW(window), "name", _("GNOME MPlayer"),
                          "logo", pb_logo,
                          "authors", authors,
                          "copyright", "Copyright © 2007 Kevin DeKorte",
                          "comments", _("A media player for GNOME that uses MPlayer"),
                          "version", VERSION,
                          "license",
                          _
                          ("Gnome MPlayer is free software; you can redistribute it and/or modify it under\nthe terms of the GNU General Public License as published by the Free\nSoftware Foundation; either version 2 of the License, or (at your option)\nany later version."
                           "\n\nGnome MPlayer is distributed in the hope that it will be useful, but WITHOUT\nANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\nFITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for\nmore details."
                           "\n\nYou should have received a copy of the GNU General Public License along with\nGnome MPlayer if not, write to the\n\nFree Software Foundation, Inc.,\n51 Franklin St, Fifth Floor\nBoston, MA 02110-1301 USA")


                          ,
                          "website", "http://code.google.com/p/gnome-mplayer/",
                          "translator-credits", "Chinese (simplified) - Wenzheng Hu\n"
                          "Chinese (Hong Kong) - Hialan Liu\n"
                          "Chinese (Taiwan) - Hailan Liu\n"
                          "French - Alexandre Bedot\n"
                          "Italian - Cesare Tirabassi\n"
                          "Korean - ByeongSik Jeon\n"
                          "Polish - Julian Sikorski\n"
                          "Russian - Dmitry Stropaloff\n"
                          "Serbian - Милош Поповић\n"
                          "Spanish - Festor Wailon Dacoba\n" 
						  "Swedish - Daniel Nylander\n"
						  "Turkish - Onur Küçük"
						  , NULL);
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

    random_order = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_edit_random));
    if (random_order) {
        randomize_playlist(playliststore);
    } else {
        copy_playlist(nonrandomplayliststore, playliststore);
    }

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        if (GTK_IS_TREE_SELECTION(selection)) {
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
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
    if (GTK_IS_WIDGET(media_label)) {
        if (GTK_WIDGET_VISIBLE(media_label)) {
            gtk_widget_hide(media_label);
            show_media_label = FALSE;
        } else {
            gtk_widget_show(media_label);
            show_media_label = TRUE;
        }
    }
}

void menuitem_view_fullscreen_callback(GtkMenuItem * menuitem, void *data)
{
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen), !fullscreen);
}

void menuitem_view_onetoone_callback(GtkMenuItem * menuitem, void *data)
{
    gint total_height;
    GtkRequisition req;
    IdleData *idle = (IdleData *) data;

    gtk_widget_set_size_request(fixed, -1, -1);
    gtk_widget_set_size_request(drawing_area, -1, -1);
    if (idle->width > 0 && idle->height > 0) {
        gtk_widget_set_size_request(fixed, idle->width, idle->height);
        gtk_widget_set_size_request(drawing_area, idle->width, idle->height);
        total_height = idle->height;
        gtk_widget_size_request(GTK_WIDGET(menubar), &req);
        total_height += req.height;
        if (GTK_WIDGET_VISIBLE(media_label)) {
            gtk_widget_size_request(GTK_WIDGET(media_label), &req);
            total_height += req.height;
        }
        gtk_widget_size_request(GTK_WIDGET(controls_box), &req);
        total_height += req.height;
        gtk_window_resize(GTK_WINDOW(window), idle->width, total_height);
    }
}

void menuitem_view_twotoone_callback(GtkMenuItem * menuitem, void *data)
{
    gint total_height;
    GtkRequisition req;
    IdleData *idle = (IdleData *) data;

    gtk_widget_set_size_request(fixed, -1, -1);
    gtk_widget_set_size_request(drawing_area, -1, -1);
    if (idle->width > 0 && idle->height > 0) {
        gtk_widget_set_size_request(fixed, idle->width, idle->height);
        gtk_widget_set_size_request(drawing_area, idle->width * 2, idle->height * 2);
        total_height = idle->height * 2;
        gtk_widget_size_request(GTK_WIDGET(menubar), &req);
        total_height += req.height;
        if (GTK_WIDGET_VISIBLE(media_label)) {
            gtk_widget_size_request(GTK_WIDGET(media_label), &req);
            total_height += req.height;
        }
        gtk_widget_size_request(GTK_WIDGET(controls_box), &req);
        total_height += req.height;
        gtk_window_resize(GTK_WINDOW(window), idle->width * 2, total_height);
    }
}

void menuitem_view_onetotwo_callback(GtkMenuItem * menuitem, void *data)
{
    gint total_height;
    GtkRequisition req;
    IdleData *idle = (IdleData *) data;

    gtk_widget_set_size_request(fixed, -1, -1);
    gtk_widget_set_size_request(drawing_area, -1, -1);
    if (idle->width > 0 && idle->height > 0) {
        gtk_widget_set_size_request(fixed, idle->width, idle->height);
        gtk_widget_set_size_request(drawing_area, idle->width / 2, idle->height / 2);
        total_height = idle->height / 2;
        gtk_widget_size_request(GTK_WIDGET(menubar), &req);
        total_height += req.height;
        if (GTK_WIDGET_VISIBLE(media_label)) {
            gtk_widget_size_request(GTK_WIDGET(media_label), &req);
            total_height += req.height;
        }
        gtk_widget_size_request(GTK_WIDGET(controls_box), &req);
        total_height += req.height;
        gtk_window_resize(GTK_WINDOW(window), idle->width / 2, total_height);
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
    cmd = g_strdup_printf("pausing_keep sub_visibility\n");
    send_command(cmd);
    g_free(cmd);
}

//      Switch Audio Streams 
void menuitem_edit_switch_audio_callback(GtkMenuItem * menuitem, void *data)
{
    gchar *cmd;
    cmd = g_strdup_printf("pausing_keep switch_audio\n");
    send_command(cmd);
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
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            subtitle = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            gtk_list_store_set(playliststore, &iter, SUBTITLE_COLUMN, subtitle, -1);
        }
        gtk_widget_destroy(dialog);

        if (subtitle != NULL) {
            cmd = g_strdup_printf("pausing_keep sub_remove\n");
            send_command(cmd);
            g_free(cmd);
            cmd = g_strdup_printf("pausing_keep sub_load \"%s\"\n", subtitle);
            send_command(cmd);
            g_free(cmd);
            cmd = g_strdup_printf("pausing_keep sub_file 0");
            send_command(cmd);
            g_free(cmd);
            g_free(subtitle);
        }
    }
}

void menuitem_fs_callback(GtkMenuItem * menuitem, void *data)
{
    gint width = 0, height = 0;
    static gint controls_height;

    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen))) {
        gtk_window_unfullscreen(GTK_WINDOW(window));

        if (embed_window != 0) {
            while (gtk_events_pending())
                gtk_main_iteration();

            if (GTK_WIDGET_MAPPED(window))
                gtk_widget_unmap(window);

            gdk_window_reparent(window->window, window_container, 0, 0);
            gtk_widget_map(window);

            if (window_x < 250) {
                gtk_widget_hide(fs_event_box);
            }
            if (window_x < 170) {
                gtk_widget_hide(GTK_WIDGET(progress));
            }
        } else {
            gtk_widget_show(menubar);
        }

        if (restore_playlist) {
            menuitem_view_playlist_callback(NULL, NULL);
            restore_playlist = FALSE;
        }
        make_panel_and_mouse_visible(NULL);
        if (GDK_IS_DRAWABLE(window_container))
            gdk_drawable_get_size(GDK_DRAWABLE(window_container), &width, &height);
        if (width > 0 && height > 0)
            gtk_window_resize(GTK_WINDOW(window), width, height);
        if (stored_window_width != -1 && stored_window_width > 0) {
            if (GTK_WIDGET_FLAGS(controls_box) & GTK_VISIBLE)
                stored_window_height += controls_height;
            gtk_window_resize(GTK_WINDOW(window), stored_window_width, stored_window_height);
        }
        fullscreen = 0;
    } else {
        if (embed_window != 0) {

            if (GTK_WIDGET_MAPPED(window))
                gtk_widget_unmap(window);

            gdk_window_reparent(window->window, NULL, 0, 0);
            gtk_widget_map(window);

            if (window_x < 250) {
                gtk_widget_show(fs_event_box);
            }
            if (window_x < 170) {
                gtk_widget_show(GTK_WIDGET(progress));
            }

        } else {
            gtk_widget_hide(menubar);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_file_details), FALSE);
            while (gtk_events_pending())
                gtk_main_iteration();
        }

        gtk_window_get_size(GTK_WINDOW(window), &stored_window_width, &stored_window_height);

        controls_height = controls_box->allocation.height;

        if (GTK_WIDGET_FLAGS(controls_box) & GTK_VISIBLE)
            stored_window_height -= controls_height;

        gtk_window_fullscreen(GTK_WINDOW(window));
        fullscreen = 1;
        if (GTK_IS_WIDGET(plvbox) && GTK_WIDGET_VISIBLE(plvbox)) {
            restore_playlist = TRUE;
            menuitem_view_playlist_callback(NULL, NULL);
        }
        motion_notify_callback(NULL, NULL, NULL);
    }

}

void menuitem_copyurl_callback(GtkMenuItem * menuitem, void *data)
{
    GtkClipboard *clipboard;
    gchar *url;

    url = g_strdup(idledata->url);
    clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text(clipboard, url, -1);
    clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, url, -1);

    g_free(url);
}

void menuitem_showcontrols_callback(GtkCheckMenuItem * menuitem, void *data)
{
    int width, height;

    if (gtk_check_menu_item_get_active(menuitem)) {
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
    GConfClient *gconf;
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

    update_mplayer_config();

    cache_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(config_cachesize));
    old_disable_framedrop = disable_framedrop;
    disable_deinterlace =
        !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_deinterlace));
    disable_framedrop =
        !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_framedrop));
    disable_ass = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_ass));
    disable_embeddedfonts =
        !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_embeddedfonts));
    oldosd = osdlevel;
    osdlevel = (gint) gtk_range_get_value(GTK_RANGE(config_osdlevel));
    pplevel = (gint) gtk_range_get_value(GTK_RANGE(config_pplevel));
    softvol = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_softvol));
    verbose = (gint) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_verbose));
    playlist_visible = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_playlist_visible));
    vertical_layout = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_vertical_layout));
    forcecache = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_forcecache));
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

    if (old_disable_framedrop != disable_framedrop) {
        cmd = g_strdup_printf("pausing_keep frame_drop %d\n", !disable_framedrop);
        send_command(cmd);
        g_free(cmd);
    }

    if (oldosd != osdlevel) {
        cmd = g_strdup_printf("pausing_keep osd %i\n", osdlevel);
        send_command(cmd);
        g_free(cmd);
    }

    qt_disabled = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_qt));
    real_disabled = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_real));
    wmp_disabled = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_wmp));
    dvx_disabled = !(gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_dvx));
    embedding_disabled = (gboolean) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_noembed));

    extraopts = g_strdup(gtk_entry_get_text(GTK_ENTRY(config_extraopts)));

    gconf = gconf_client_get_default();
    gconf_client_set_int(gconf, CACHE_SIZE, cache_size, NULL);
    gconf_client_set_int(gconf, OSDLEVEL, osdlevel, NULL);
    gconf_client_set_int(gconf, PPLEVEL, pplevel, NULL);
    gconf_client_set_bool(gconf, SOFTVOL, softvol, NULL);
    gconf_client_set_bool(gconf, FORCECACHE, forcecache, NULL);
    gconf_client_set_bool(gconf, DISABLEASS, disable_ass, NULL);
    gconf_client_set_bool(gconf, DISABLEEMBEDDEDFONTS, disable_embeddedfonts, NULL);
    gconf_client_set_bool(gconf, DISABLEDEINTERLACE, disable_deinterlace, NULL);
    gconf_client_set_bool(gconf, DISABLEFRAMEDROP, disable_framedrop, NULL);
    gconf_client_set_bool(gconf, SHOWPLAYLIST, playlist_visible, NULL);
    gconf_client_set_bool(gconf, VERTICAL, vertical_layout, NULL);
    gconf_client_set_int(gconf, VERBOSE, verbose, NULL);
    gconf_client_set_string(gconf, METADATACODEPAGE, metadata_codepage, NULL);
    gconf_client_set_string(gconf, SUBTITLEFONT, subtitlefont, NULL);
    gconf_client_set_float(gconf, SUBTITLESCALE, subtitle_scale, NULL);
    gconf_client_set_string(gconf, SUBTITLECODEPAGE, subtitle_codepage, NULL);
    gconf_client_set_string(gconf, SUBTITLECOLOR, subtitle_color, NULL);
    gconf_client_set_string(gconf, EXTRAOPTS, extraopts, NULL);

    gconf_client_set_bool(gconf, DISABLE_QT, qt_disabled, NULL);
    gconf_client_set_bool(gconf, DISABLE_REAL, real_disabled, NULL);
    gconf_client_set_bool(gconf, DISABLE_WMP, wmp_disabled, NULL);
    gconf_client_set_bool(gconf, DISABLE_DVX, dvx_disabled, NULL);
    gconf_client_set_bool(gconf, DISABLE_EMBEDDING, embedding_disabled, NULL);
    g_object_unref(G_OBJECT(gconf));

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

void config_close(GtkWidget * widget, void *data)
{
    selection = NULL;
    gtk_widget_destroy(widget);
}

void brightness_callback(GtkRange * range, gpointer data)
{
    gint brightness;
    gchar *cmd;
    IdleData *idle = (IdleData *) data;

    brightness = (gint) gtk_range_get_value(range);
    cmd = g_strdup_printf("pausing_keep brightness %i 1\n", brightness);
    send_command(cmd);
    g_free(cmd);
    send_command("get_property brightness\n");
    idle->brightness = brightness;
}

void contrast_callback(GtkRange * range, gpointer data)
{
    gint contrast;
    gchar *cmd;
    IdleData *idle = (IdleData *) data;

    contrast = (gint) gtk_range_get_value(range);
    cmd = g_strdup_printf("pausing_keep contrast %i 1\n", contrast);
    send_command(cmd);
    g_free(cmd);
    send_command("get_property contrast\n");
    idle->contrast = contrast;
}

void gamma_callback(GtkRange * range, gpointer data)
{
    gint gamma;
    gchar *cmd;
    IdleData *idle = (IdleData *) data;

    gamma = (gint) gtk_range_get_value(range);
    cmd = g_strdup_printf("pausing_keep gamma %i 1\n", gamma);
    send_command(cmd);
    g_free(cmd);
    send_command("get_property gamma\n");
    idle->hue = gamma;
}

void hue_callback(GtkRange * range, gpointer data)
{
    gint hue;
    gchar *cmd;
    IdleData *idle = (IdleData *) data;

    hue = (gint) gtk_range_get_value(range);
    cmd = g_strdup_printf("pausing_keep hue %i 1\n", hue);
    send_command(cmd);
    g_free(cmd);
    send_command("get_property hue\n");
    idle->hue = hue;
}

void saturation_callback(GtkRange * range, gpointer data)
{
    gint saturation;
    gchar *cmd;
    IdleData *idle = (IdleData *) data;

    saturation = (gint) gtk_range_get_value(range);
    cmd = g_strdup_printf("pausing_keep saturation %i 1\n", saturation);
    send_command(cmd);
    g_free(cmd);
    send_command("get_property saturation\n");
    idle->saturation = saturation;
}

void menuitem_details_callback(GtkMenuItem * menuitem, void *data)
{
    GtkWidget *label;
    gchar *buf;
    gint i = 0;
    IdleData *idle = idledata;
    gint width, height;
    static gint normal_width, normal_height;
    GtkRequisition req;

    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_file_details))) {
        if (GTK_IS_WIDGET(details_table)) {
            gtk_widget_destroy(details_table);
            details_table = NULL;
            gtk_window_resize(GTK_WINDOW(window), normal_width, normal_height);
        }
    } else {

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
            buf = g_strdup_printf("%i Kb/s", (gint) (g_strtod(idle->video_bitrate, NULL) / 1024));
            label = gtk_label_new(buf);
            g_free(buf);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 1, 2, i, i + 1);
            i++;

            label = gtk_label_new("");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 0, 1, i, i + 1);
            i++;
        }

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
            buf = g_strdup_printf("%i Kb/s", (gint) (g_strtod(idle->audio_bitrate, NULL) / 1024));
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
                g_strdup_printf("%i Kb/s", (gint) (g_strtod(idle->audio_samplerate, NULL) / 1024));
            label = gtk_label_new(buf);
            g_free(buf);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(details_table), label, 1, 2, i, i + 1);
        }
        i++;

        gdk_window_get_geometry(window->window, NULL, NULL, &width, &height, NULL);
        normal_width = width;
        normal_height = height;
        gtk_widget_show_all(details_table);
        gtk_widget_size_request(GTK_WIDGET(details_table), &req);
        height += req.height;
        gtk_window_resize(GTK_WINDOW(window), width, height);
    }
}


void menuitem_advanced_callback(GtkMenuItem * menuitem, void *data)
{
    GtkWidget *adv_window;
    GtkWidget *adv_vbox;
    GtkWidget *adv_hbutton_box;
    GtkWidget *adv_table;
    GtkWidget *adv_close;
    GtkWidget *label;
    GtkWidget *brightness;
    GtkWidget *contrast;
    GtkWidget *gamma;
    GtkWidget *hue;
    GtkWidget *saturation;
    gint i = 0;

    adv_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(adv_window), GDK_WINDOW_TYPE_HINT_UTILITY);
    gtk_window_set_resizable(GTK_WINDOW(adv_window), FALSE);
    gtk_window_set_title(GTK_WINDOW(adv_window), _("Advanced Video Controls"));

    adv_vbox = gtk_vbox_new(FALSE, 10);
    adv_hbutton_box = gtk_hbutton_box_new();
    gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
    adv_table = gtk_table_new(20, 2, FALSE);

    gtk_container_add(GTK_CONTAINER(adv_vbox), adv_table);
    gtk_container_add(GTK_CONTAINER(adv_vbox), adv_hbutton_box);
    gtk_container_add(GTK_CONTAINER(adv_window), adv_vbox);

    gtk_container_set_border_width(GTK_CONTAINER(adv_window), 5);

    label = gtk_label_new(_("<span weight=\"bold\">Adjust Video Settings</span>"));
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(label), 0, 6);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), label, 0, 1, i, i + 1);
    i++;

    label = gtk_label_new(_("Brightness"));
    brightness = gtk_hscale_new_with_range(-100.0, 100.0, 1.0);
    gtk_widget_set_size_request(brightness, 200, -1);
    gtk_range_set_value(GTK_RANGE(brightness), idledata->brightness);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), label, 0, 1, i, i + 1);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), brightness, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Contrast"));
    contrast = gtk_hscale_new_with_range(-100.0, 100.0, 1.0);
    gtk_widget_set_size_request(contrast, 200, -1);
    gtk_range_set_value(GTK_RANGE(contrast), idledata->contrast);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), label, 0, 1, i, i + 1);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), contrast, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Gamma"));
    gamma = gtk_hscale_new_with_range(-100.0, 100.0, 1.0);
    gtk_widget_set_size_request(gamma, 200, -1);
    gtk_range_set_value(GTK_RANGE(gamma), idledata->gamma);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), label, 0, 1, i, i + 1);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), gamma, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Hue"));
    hue = gtk_hscale_new_with_range(-100.0, 100.0, 1.0);
    gtk_widget_set_size_request(hue, 200, -1);
    gtk_range_set_value(GTK_RANGE(hue), idledata->hue);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), label, 0, 1, i, i + 1);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), hue, 1, 2, i, i + 1);
    i++;

    label = gtk_label_new(_("Saturation"));
    saturation = gtk_hscale_new_with_range(-100.0, 100.0, 1.0);
    gtk_widget_set_size_request(saturation, 200, -1);
    gtk_range_set_value(GTK_RANGE(saturation), idledata->saturation);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), label, 0, 1, i, i + 1);
    gtk_table_attach_defaults(GTK_TABLE(adv_table), saturation, 1, 2, i, i + 1);
    i++;

    g_signal_connect(G_OBJECT(brightness), "value_changed", G_CALLBACK(brightness_callback),
                     idledata);
    g_signal_connect(G_OBJECT(contrast), "value_changed", G_CALLBACK(contrast_callback), idledata);
    g_signal_connect(G_OBJECT(gamma), "value_changed", G_CALLBACK(gamma_callback), idledata);
    g_signal_connect(G_OBJECT(hue), "value_changed", G_CALLBACK(hue_callback), idledata);
    g_signal_connect(G_OBJECT(saturation), "value_changed", G_CALLBACK(saturation_callback),
                     idledata);

    adv_close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(GTK_OBJECT(adv_close), "clicked",
                             GTK_SIGNAL_FUNC(config_close), adv_window);

    gtk_container_add(GTK_CONTAINER(adv_hbutton_box), adv_close);
    gtk_widget_show_all(adv_window);

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
        i--;
    }

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_four_three) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine),
                                       FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten),
                                       FALSE);
        i--;
    }

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_sixteen_nine) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_ten),
                                       FALSE);
        i--;
    }

    if ((gpointer) menuitem == (gpointer) menuitem_view_aspect_sixteen_ten) {
        i++;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_four_three), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_sixteen_nine),
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
    cmd = g_strdup_printf("pausing_keep osd %i\n", (gint) value);
    send_command(cmd);
    g_free(cmd);

    return;
}

void ass_toggle_callback(GtkToggleButton * source, gpointer user_data)
{
    gtk_widget_set_sensitive(config_subtitle_color, gtk_toggle_button_get_active(source));
    gtk_widget_set_sensitive(config_embeddedfonts, gtk_toggle_button_get_active(source));
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
    GtkWidget *notebook;
    GdkColor sub_color;
    gint i = 0;
    gint j = -1;

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
    conf_hbutton_box = gtk_hbutton_box_new();
    gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
    conf_table = gtk_table_new(20, 2, FALSE);

    notebook = gtk_notebook_new();
    conf_label = gtk_label_new(_("Player"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), conf_page1, conf_label);
    conf_label = gtk_label_new(_("Plugin"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), conf_page2, conf_label);
    conf_label = gtk_label_new(_("Language Settings"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), conf_page3, conf_label);
    conf_label = gtk_label_new(_("Subtitles"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), conf_page4, conf_label);
    conf_label = gtk_label_new(_("Advanced"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), conf_page5, conf_label);

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
    if (config_vo != NULL) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "gl");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "x11");
        gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "xv");
        if (vo != NULL) {
            if (strcmp(vo, "gl") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 0);
            if (strcmp(vo, "x11") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 1);
            if (strcmp(vo, "xv") == 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 2);
            if (gtk_combo_box_get_active(GTK_COMBO_BOX(config_vo))
                == -1) {
                gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), vo);
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_vo), 3);
            }
        }
    }

    config_ao = gtk_combo_box_entry_new_text();
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
            if (j != -1)
                gtk_combo_box_set_active(GTK_COMBO_BOX(config_alang), j);
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
    gtk_widget_set_size_request(GTK_WIDGET(config_vo), 100, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_vo, 1, 2, i, i + 1, GTK_SHRINK, GTK_SHRINK, 0,
                     0);
    i++;

    conf_label = gtk_label_new(_("Audio Output:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_widget_set_size_request(GTK_WIDGET(config_ao), 100, -1);
    gtk_table_attach(GTK_TABLE(conf_table), config_ao, 1, 2, i, i + 1, GTK_SHRINK, GTK_SHRINK, 0,
                     0);
    i++;

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

    conf_label = gtk_label_new(_("Minimum Cache Size (KB):"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    config_cachesize = gtk_spin_button_new_with_range(0, 32767, 512);
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
    conf_label = gtk_label_new(_("<span weight=\"bold\">Adjust Plugin Emulation Settings</span>"));
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

    config_ass = gtk_check_button_new_with_mnemonic(_("Enable _ASS"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_ass), !disable_ass);
    g_signal_connect(GTK_OBJECT(config_ass), "toggled", GTK_SIGNAL_FUNC(ass_toggle_callback), NULL);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_ass, 0, 2, i, i + 1);
    gtk_widget_show(config_ass);
    i++;

    config_embeddedfonts = gtk_check_button_new_with_mnemonic(_("Use _Embedded Fonts"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_embeddedfonts), !disable_embeddedfonts);
    gtk_widget_set_sensitive(config_embeddedfonts, !disable_ass);
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
    gtk_font_button_set_show_size(GTK_FONT_BUTTON(config_subtitle_font), FALSE);
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


    conf_label = gtk_label_new(_("Subtitle Font Scaling:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(conf_label), 12, 0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1, i, i + 1);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    config_subtitle_scale = gtk_spin_button_new_with_range(0.25, 10, 0.25);
    gtk_widget_set_size_request(config_subtitle_scale, 50, -1);
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


    // Page 5
    conf_table = gtk_table_new(20, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(conf_page5), conf_table);
    i = 0;
    conf_label = gtk_label_new(_("<span weight=\"bold\">Advanced Settings</span>"));
    gtk_label_set_use_markup(GTK_LABEL(conf_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(conf_label), 0, 6);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 2, i, i + 1);
    i++;

    config_playlist_visible = gtk_check_button_new_with_label(_("Start with playlist visible"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_playlist_visible), playlist_visible);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_playlist_visible, 0, 2, i, i + 1);
    i++;

    config_vertical_layout =
        gtk_check_button_new_with_label(_("Start with playlist below media window"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_vertical_layout), vertical_layout);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_vertical_layout, 0, 2, i, i + 1);
    i++;

    config_softvol = gtk_check_button_new_with_label(_("Mplayer Software Volume Control Enabled"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_softvol), softvol);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_softvol, 0, 2, i, i + 1);
    i++;

    config_deinterlace = gtk_check_button_new_with_mnemonic(_("De_interlace Video"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_deinterlace), !disable_deinterlace);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_deinterlace, 0, 2, i, i + 1);
    i++;
	
    config_framedrop = gtk_check_button_new_with_mnemonic(_("_Drop frames"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_framedrop), !disable_framedrop);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_framedrop, 0, 2, i, i + 1);
    i++;

    config_forcecache =
        gtk_check_button_new_with_label(_("Force the use of cache setting on streaming media"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_forcecache), forcecache);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_forcecache, 0, 2, i, i + 1);
    i++;

    config_verbose = gtk_check_button_new_with_label(_("Verbose Debug Enabled"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_verbose), verbose);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_verbose, 0, 2, i, i + 1);
    i++;

    conf_label = gtk_label_new(_("Extra Options to MPlayer:"));
    config_extraopts = gtk_entry_new();
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
                    if (state == PLAYING) {
                        cmd = g_strdup_printf("seek %i 1\n", (gint) (percent * 100));
                        send_command(cmd);
                        g_free(cmd);
                        state = PLAYING;
                    }
                }
            }

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
    gint exit_status;
    gchar *stdout = NULL;
    gchar *stderr = NULL;
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

        dirname = g_strdup_printf("%s", tempnam("/tmp", "gnome-mplayerXXXXXX"));
        filename = g_strdup_printf("%s/00000001.jpg", dirname);

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

        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &stdout, &stderr,
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

        if (stderr != NULL)
            g_free(stderr);

        if (stdout != NULL)
            g_free(stdout);
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

GtkWidget *create_window(gint windowid)
{
    GError *error = NULL;
    GtkIconTheme *icon_theme;
    GtkTargetEntry target_entry[3];
    gint i = 0;
    gchar **visuals;

#ifdef GTK2_12_ENABLED
    GtkAdjustment *adj;
#endif

    in_button = FALSE;
    last_movement_time = -1;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), _("GNOME MPlayer"));

    if (windowid > 0) {
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
    menuitem_pause = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PAUSE, NULL));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_pause));
    gtk_widget_show(GTK_WIDGET(menuitem_pause));
    menuitem_stop = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_STOP, NULL));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_stop));
    gtk_widget_show(GTK_WIDGET(menuitem_stop));
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
    menuitem_copyurl = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Copy URL")));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_copyurl));
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
                     G_CALLBACK(menuitem_play_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_pause), "activate",
                     G_CALLBACK(menuitem_pause_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_stop), "activate",
                     G_CALLBACK(menuitem_stop_callback), NULL);
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
                             "enter_notify_event", G_CALLBACK(notification_handler), NULL);
    g_signal_connect_swapped(G_OBJECT(window),
                             "leave_notify_event", G_CALLBACK(notification_handler), NULL);


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
    menuitem_file_open_acd =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _Audio CD")));
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_open_acd));

    menuitem_file_dvd = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_DVD")));
    menu_file_dvd = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_file_dvd));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_file), GTK_WIDGET(menuitem_file_dvd));
    gtk_menu_item_set_submenu(menuitem_file_dvd, GTK_WIDGET(menu_file_dvd));

    menuitem_file_open_dvd =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open _DVD Direct")));
    gtk_menu_append(menu_file_dvd, GTK_WIDGET(menuitem_file_open_dvd));
    menuitem_file_open_dvdnav =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open DVD with _Menus")));
    gtk_menu_append(menu_file_dvd, GTK_WIDGET(menuitem_file_open_dvdnav));

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
//    menuitem_file_open_playlist =
//        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Open Playlist")));
//    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_open_playlist));
    menuitem_file_sep1 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_sep1));
    menuitem_file_details = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("D_etails")));
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_details));
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
    g_signal_connect(GTK_OBJECT(menuitem_file_open_acd), "activate",
                     G_CALLBACK(menuitem_open_acd_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_atv), "activate",
                     G_CALLBACK(menuitem_open_atv_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_open_dtv), "activate",
                     G_CALLBACK(menuitem_open_dtv_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_details), "activate",
                     G_CALLBACK(menuitem_details_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_quit), "activate",
                     G_CALLBACK(menuitem_quit_callback), NULL);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_file_details), "activate",
                               accel_group, 'd', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
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
    menuitem_edit_set_subtitle = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("Set Sub_title")));
    gtk_menu_append(menu_edit, GTK_WIDGET(menuitem_edit_set_subtitle));

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
    g_signal_connect(GTK_OBJECT(menuitem_edit_config), "activate",
                     G_CALLBACK(menuitem_config_callback), NULL);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_edit_config), "activate",
                               accel_group, 'p', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);



    // View Menu
    menuitem_view = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_View")));
    menu_view = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_view));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), GTK_WIDGET(menuitem_view));
    gtk_menu_item_set_submenu(menuitem_view, GTK_WIDGET(menu_view));
    menuitem_view_playlist = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Playlist")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_playlist));
    menuitem_view_info = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Media _Info")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_info));
    menuitem_view_sep0 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_sep0));

//      menuitem_view_fullscreen =
//        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Fullscreen")));
    menuitem_view_fullscreen =
        GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_FULLSCREEN, NULL));
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
    //menuitem_view_sep4 = GTK_MENU_ITEM(gtk_separator_menu_item_new());

    menuitem_view_aspect = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Aspect")));
    menu_view_aspect = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(menuitem_view_aspect));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_view), GTK_WIDGET(menuitem_view_aspect));
    gtk_menu_item_set_submenu(menuitem_view_aspect, GTK_WIDGET(menu_view_aspect));

    //gtk_menu_append(menu_view_aspect, GTK_WIDGET(menuitem_view_sep4));
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
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_aspect_default), TRUE);

    menuitem_view_sep2 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    menuitem_view_subtitles =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Show _Subtitles")));
    menuitem_view_controls = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Controls")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_sep2));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_subtitles));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_controls));
    menuitem_view_sep3 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_sep3));
    menuitem_view_advanced =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Advanced _Options...")));
    gtk_menu_append(menu_view, GTK_WIDGET(menuitem_view_advanced));

    g_signal_connect(GTK_OBJECT(menuitem_view_playlist), "activate",
                     G_CALLBACK(menuitem_view_playlist_callback), NULL);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_playlist), "activate",
                               accel_group, 'l', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_file_open_location), "activate",
                               accel_group, 'u', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_info), "activate",
                               accel_group, 'i', 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_subtitles), "activate",
                               accel_group, 'v', 0, GTK_ACCEL_VISIBLE);
    g_signal_connect(GTK_OBJECT(menuitem_view_info), "activate",
                     G_CALLBACK(menuitem_view_info_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_fullscreen), "activate",
                     G_CALLBACK(menuitem_view_fullscreen_callback), NULL);
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
    g_signal_connect(GTK_OBJECT(menuitem_view_subtitles), "activate",
                     G_CALLBACK(menuitem_view_subtitles_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_view_controls), "activate",
                     G_CALLBACK(menuitem_view_controls_callback), NULL);
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
    if (!disable_fullscreen) {
        gtk_widget_add_accelerator(GTK_WIDGET(menuitem_fullscreen), "activate",
                                   accel_group, 'f', 0, GTK_ACCEL_VISIBLE);

        gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_fullscreen), "activate",
                                   accel_group, 'f', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    }
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_onetoone), "activate",
                               accel_group, '1', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_twotoone), "activate",
                               accel_group, '2', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_showcontrols), "activate",
                               accel_group, 'c', 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_controls), "activate",
                               accel_group, 'c', 0, GTK_ACCEL_VISIBLE);
//    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_view_subtitles), "activate",
//                               accel_group, 'j', 0, GTK_ACCEL_VISIBLE);
//    gtk_widget_add_accelerator(GTK_WIDGET(menuitem_sound_switch), "activate",
//                               accel_group, '#', 0, GTK_ACCEL_VISIBLE);

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

    //Connect the signal for DnD
    g_signal_connect(GTK_OBJECT(window), "drag_data_received", GTK_SIGNAL_FUNC(drop_callback),
                     NULL);


    vbox = gtk_vbox_new(FALSE, 0);
    hbox = gtk_hbox_new(FALSE, 0);
    controls_box = gtk_vbox_new(FALSE, 0);
    fixed = gtk_fixed_new();
    drawing_area = gtk_socket_new();
    //gtk_widget_set_size_request(drawing_area, 1, 1);
    media_label = gtk_label_new("");
    details_vbox = gtk_vbox_new(FALSE, 10);
    gtk_misc_set_alignment(GTK_MISC(media_label), 0, 0);

    gtk_fixed_put(GTK_FIXED(fixed), drawing_area, 0, 0);
    gtk_box_pack_start(GTK_BOX(vbox), fixed, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), media_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), details_vbox, FALSE, FALSE, 0);
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

        pb_play = gtk_icon_theme_load_icon(icon_theme, "media-playback-start", 16, 0, &error);
        pb_pause = gtk_icon_theme_load_icon(icon_theme, "media-playback-pause", 16, 0, &error);
        pb_stop = gtk_icon_theme_load_icon(icon_theme, "media-playback-stop", 16, 0, &error);
        pb_ff = gtk_icon_theme_load_icon(icon_theme, "media-seek-forward", 16, 0, &error);
        pb_rew = gtk_icon_theme_load_icon(icon_theme, "media-seek-backward", 16, 0, &error);
        pb_next = gtk_icon_theme_load_icon(icon_theme, "media-skip-forward", 16, 0, &error);
        pb_prev = gtk_icon_theme_load_icon(icon_theme, "media-skip-backward", 16, 0, &error);
        pb_menu = gtk_icon_theme_load_icon(icon_theme, GTK_STOCK_INDEX, 16, 0, &error);
        pb_fs = gtk_icon_theme_load_icon(icon_theme, "view-fullscreen", 16, 0, &error);

    } else if (gtk_icon_theme_has_icon(icon_theme, "stock_media-play")
               && gtk_icon_theme_has_icon(icon_theme, "stock_media-pause")
               && gtk_icon_theme_has_icon(icon_theme, "stock_media-stop")
               && gtk_icon_theme_has_icon(icon_theme, "stock_media-fwd")
               && gtk_icon_theme_has_icon(icon_theme, "stock_media-rew")
               && gtk_icon_theme_has_icon(icon_theme, "stock_media-prev")
               && gtk_icon_theme_has_icon(icon_theme, "stock_media-next")
               && gtk_icon_theme_has_icon(icon_theme, GTK_STOCK_INDEX)
               && gtk_icon_theme_has_icon(icon_theme, "view-fullscreen")) {

        pb_play = gtk_icon_theme_load_icon(icon_theme, "stock_media-play", 16, 0, &error);
        pb_pause = gtk_icon_theme_load_icon(icon_theme, "stock_media-pause", 16, 0, &error);
        pb_stop = gtk_icon_theme_load_icon(icon_theme, "stock_media-stop", 16, 0, &error);
        pb_ff = gtk_icon_theme_load_icon(icon_theme, "stock_media-fwd", 16, 0, &error);
        pb_rew = gtk_icon_theme_load_icon(icon_theme, "stock_media-rew", 16, 0, &error);
        pb_next = gtk_icon_theme_load_icon(icon_theme, "stock_media-next", 16, 0, &error);
        pb_prev = gtk_icon_theme_load_icon(icon_theme, "stock_media-prev", 16, 0, &error);
        pb_menu = gtk_icon_theme_load_icon(icon_theme, GTK_STOCK_INDEX, 16, 0, &error);
        pb_fs = gtk_icon_theme_load_icon(icon_theme, "view-fullscreen", 16, 0, &error);

    } else {

        pb_play = gdk_pixbuf_new_from_xpm_data((const char **) media_playback_start_xpm);
        pb_pause = gdk_pixbuf_new_from_xpm_data((const char **) media_playback_pause_xpm);
        pb_stop = gdk_pixbuf_new_from_xpm_data((const char **) media_playback_stop_xpm);
        pb_ff = gdk_pixbuf_new_from_xpm_data((const char **) media_seek_forward_xpm);
        pb_rew = gdk_pixbuf_new_from_xpm_data((const char **) media_seek_backward_xpm);
        pb_next = gdk_pixbuf_new_from_xpm_data((const char **) media_skip_forward_xpm);
        pb_prev = gdk_pixbuf_new_from_xpm_data((const char **) media_skip_backward_xpm);
        pb_menu = gtk_icon_theme_load_icon(icon_theme, GTK_STOCK_INDEX, 16, 0, &error);
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

//    menu_event_box = gtk_event_box_new();
    menu_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(menu_event_box), image_menu);
    gtk_button_set_relief(GTK_BUTTON(menu_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, menu_event_box, _("Menu"), NULL);
    gtk_widget_set_events(menu_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(menu_event_box), "button_press_event", G_CALLBACK(menu_callback),
                     NULL);
//    g_signal_connect(G_OBJECT(menu_event_box), "enter_notify_event",
//                     G_CALLBACK(enter_button_callback), NULL);
//    g_signal_connect(G_OBJECT(menu_event_box), "leave_notify_event",
//                     G_CALLBACK(leave_button_callback), NULL);
//    gtk_widget_set_size_request(GTK_WIDGET(menu_event_box), 22, 16);

//    gtk_container_add(GTK_CONTAINER(menu_event_box), image_menu);
    gtk_box_pack_start(GTK_BOX(hbox), menu_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_menu);
    gtk_widget_show(menu_event_box);

    //prev_event_box = gtk_event_box_new();
    prev_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(prev_event_box), image_prev);
    gtk_button_set_relief(GTK_BUTTON(prev_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, prev_event_box, _("Previous"), NULL);
    gtk_widget_set_events(prev_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(prev_event_box), "button_press_event", G_CALLBACK(prev_callback),
                     NULL);
//    g_signal_connect(G_OBJECT(prev_event_box), "enter_notify_event",
//                     G_CALLBACK(enter_button_callback), NULL);
//    g_signal_connect(G_OBJECT(prev_event_box), "leave_notify_event",
//                     G_CALLBACK(leave_button_callback), NULL);
//    gtk_widget_set_size_request(GTK_WIDGET(prev_event_box), 22, 16);

//    gtk_container_add(GTK_CONTAINER(prev_event_box), image_prev);
    gtk_box_pack_start(GTK_BOX(hbox), prev_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_prev);
    gtk_widget_show(prev_event_box);

    //rew_event_box = gtk_event_box_new();
    rew_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(rew_event_box), image_rew);
    gtk_button_set_relief(GTK_BUTTON(rew_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, rew_event_box, _("Rewind"), NULL);
    gtk_widget_set_events(rew_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(rew_event_box), "button_press_event", G_CALLBACK(rew_callback), NULL);
//    g_signal_connect(G_OBJECT(rew_event_box), "enter_notify_event",
//                     G_CALLBACK(enter_button_callback), NULL);
//    g_signal_connect(G_OBJECT(rew_event_box), "leave_notify_event",
//                     G_CALLBACK(leave_button_callback), NULL);
//    gtk_widget_set_size_request(GTK_WIDGET(rew_event_box), 22, 16);

//    gtk_container_add(GTK_CONTAINER(rew_event_box), image_rew);
    gtk_box_pack_start(GTK_BOX(hbox), rew_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_rew);
    gtk_widget_show(rew_event_box);

//    play_event_box = gtk_event_box_new();
    play_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(play_event_box), image_play);
    gtk_button_set_relief(GTK_BUTTON(play_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
    gtk_widget_set_events(play_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(play_event_box),
                     "button_press_event", G_CALLBACK(play_callback), NULL);
//    g_signal_connect(G_OBJECT(play_event_box), "enter_notify_event",
//                     G_CALLBACK(enter_button_callback), NULL);
//    g_signal_connect(G_OBJECT(play_event_box), "leave_notify_event",
//                     G_CALLBACK(leave_button_callback), NULL);
//    gtk_widget_set_size_request(GTK_WIDGET(play_event_box), 22, 16);

//    gtk_container_add(GTK_CONTAINER(play_event_box), image_play);
    gtk_box_pack_start(GTK_BOX(hbox), play_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_play);
    gtk_widget_show(play_event_box);

//    stop_event_box = gtk_event_box_new();
    stop_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(stop_event_box), image_stop);
    gtk_button_set_relief(GTK_BUTTON(stop_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, stop_event_box, _("Stop"), NULL);
    gtk_widget_set_events(stop_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(stop_event_box),
                     "button_press_event", G_CALLBACK(stop_callback), NULL);
//    g_signal_connect(G_OBJECT(stop_event_box), "enter_notify_event",
//                     G_CALLBACK(enter_button_callback), NULL);
//    g_signal_connect(G_OBJECT(stop_event_box), "leave_notify_event",
//                     G_CALLBACK(leave_button_callback), NULL);
//    gtk_widget_set_size_request(GTK_WIDGET(stop_event_box), 22, 16);

//    gtk_container_add(GTK_CONTAINER(stop_event_box), image_stop);
    gtk_box_pack_start(GTK_BOX(hbox), stop_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_stop);
    gtk_widget_show(stop_event_box);

//    ff_event_box = gtk_event_box_new();
    ff_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(ff_event_box), image_ff);
    gtk_button_set_relief(GTK_BUTTON(ff_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, ff_event_box, _("Fast Forward"), NULL);
    gtk_widget_set_events(ff_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(ff_event_box), "button_press_event", G_CALLBACK(ff_callback), NULL);
//    g_signal_connect(G_OBJECT(ff_event_box), "enter_notify_event",
//                     G_CALLBACK(enter_button_callback), NULL);
//    g_signal_connect(G_OBJECT(ff_event_box), "leave_notify_event",
//                     G_CALLBACK(leave_button_callback), NULL);
//    gtk_widget_set_size_request(GTK_WIDGET(ff_event_box), 22, 16);
//    gtk_container_add(GTK_CONTAINER(ff_event_box), image_ff);
    gtk_box_pack_start(GTK_BOX(hbox), ff_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_ff);
    gtk_widget_show(ff_event_box);

//    next_event_box = gtk_event_box_new();
    next_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(next_event_box), image_next);
    gtk_button_set_relief(GTK_BUTTON(next_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, next_event_box, _("Next"), NULL);
    gtk_widget_set_events(next_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(next_event_box), "button_press_event", G_CALLBACK(next_callback),
                     NULL);
//    g_signal_connect(G_OBJECT(next_event_box), "enter_notify_event",
//                     G_CALLBACK(enter_button_callback), NULL);
//    g_signal_connect(G_OBJECT(next_event_box), "leave_notify_event",
//                     G_CALLBACK(leave_button_callback), NULL);
//    gtk_widget_set_size_request(GTK_WIDGET(next_event_box), 22, 16);

//    gtk_container_add(GTK_CONTAINER(next_event_box), image_next);
    gtk_box_pack_start(GTK_BOX(hbox), next_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_next);
    gtk_widget_show(next_event_box);

    // progress bar
    progress = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(progress), TRUE, TRUE, 2);
    gtk_widget_set_events(GTK_WIDGET(progress), GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(progress), "button_press_event", G_CALLBACK(progress_callback), NULL);
    gtk_widget_show(GTK_WIDGET(progress));

    // fullscreen button, pack from end for this button and the vol slider
    //fs_event_box = gtk_event_box_new();
    fs_event_box = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(fs_event_box), image_fs);
    gtk_button_set_relief(GTK_BUTTON(fs_event_box), GTK_RELIEF_NONE);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, fs_event_box, _("Full Screen"), NULL);
    gtk_widget_set_events(fs_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(fs_event_box), "button_press_event", G_CALLBACK(fs_callback), NULL);
    //g_signal_connect(G_OBJECT(fs_event_box), "enter_notify_event",
    //                 G_CALLBACK(enter_button_callback), NULL);
    //g_signal_connect(G_OBJECT(fs_event_box), "leave_notify_event",
    //                 G_CALLBACK(leave_button_callback), NULL);
    //gtk_widget_set_size_request(GTK_WIDGET(fs_event_box), 22, 16);
    //gtk_container_add(GTK_CONTAINER(fs_event_box), image_fs);
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
        gtk_widget_set_size_request(vol_slider, -1, 22);
        g_signal_connect(G_OBJECT(vol_slider), "value_changed", G_CALLBACK(vol_button_callback),
                         NULL);
        gtk_button_set_relief(GTK_BUTTON(vol_slider), GTK_RELIEF_NONE);
#else
        vol_slider = gtk_hscale_new_with_range(0.0, 100.0, 1.0);
        gtk_widget_set_size_request(vol_slider, 44, 16);
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

    if (windowid != 0 && embedding_disabled == FALSE) {
        while (gtk_events_pending())
            gtk_main_iteration();

        window_container = gdk_window_foreign_new(windowid);
        if (GTK_WIDGET_MAPPED(window))
            gtk_widget_unmap(window);

        gdk_window_reparent(window->window, window_container, 0, 0);

    }

    if (rpcontrols == NULL || (rpcontrols != NULL && g_strcasecmp(rpcontrols, "all") == 0)) {
        if (windowid != -1)
            gtk_widget_show_all(window);
        gtk_widget_hide(media_label);
        gtk_widget_hide(menu_event_box);
        show_media_label = TRUE;
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
                gtk_widget_hide(GTK_WIDGET(progress));
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
        gtk_widget_hide(media_label);
        show_media_label = FALSE;

        gtk_widget_hide_all(controls_box);

        printf("showing the following controls = %s\n", rpcontrols);
        visuals = g_strsplit(rpcontrols, ",", 0);
        i = 0;
        while (visuals[i] != NULL) {
            if (g_strcasecmp(visuals[i], "statusbar") == 0
                || g_strcasecmp(visuals[i], "statusfield") == 0
                || g_strcasecmp(visuals[i], "positionfield") == 0
                || g_strcasecmp(visuals[i], "positionslider") == 0) {
                gtk_widget_show(GTK_WIDGET(progress));
                gtk_widget_show(controls_box);
                gtk_widget_show(hbox);
                control_instance = FALSE;
            }
            if (g_strcasecmp(visuals[i], "infovolumepanel") == 0) {
                gtk_widget_show(GTK_WIDGET(vol_slider));
                gtk_widget_show(controls_box);
                gtk_widget_show(hbox);
                gtk_widget_show(GTK_WIDGET(media_label));
                show_media_label = TRUE;

                control_instance = FALSE;
            }
            if (g_strcasecmp(visuals[i], "infopanel") == 0) {
                gtk_widget_show(GTK_WIDGET(media_label));
                show_media_label = TRUE;

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
                gtk_widget_hide(GTK_WIDGET(progress));
            }

            i++;
        }


    }
    g_signal_connect(G_OBJECT(fixed), "size_allocate", G_CALLBACK(allocate_fixed_callback), NULL);
    g_signal_connect(G_OBJECT(fixed), "expose_event", G_CALLBACK(expose_fixed_callback), NULL);

    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_fullscreen), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_file_details), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(fs_event_box), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_set_subtitle), FALSE);
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
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_view_subtitles), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_file_details), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), FALSE);
    gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);
    gtk_widget_hide(prev_event_box);
    gtk_widget_hide(next_event_box);
    gtk_widget_hide(media_label);

    //while (gtk_events_pending())
    //    gtk_main_iteration();

    return window;
}
