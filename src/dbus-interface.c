/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * dbus-interface.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * dbus-interface.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * dbus-interface.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with dbus-interface.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "dbus-interface.h"
#include "support.h"
#include <unistd.h>
/*

To send command to ALL running gnome-mplayers (multihead applications)
dbus-send  --type=signal / com.gnome.mplayer.Play string:'http://www.hotmail.com/playfile.asx'


When windowid is not specified
dbus-send  --type=signal /pid/[pid] com.gnome.mplayer.Play string:'http://www.hotmail.com/playfile.asx'


When windowid is specified
dbus-send  --type=signal /window/[windowid] com.gnome.mplayer.Play string:'http://www.hotmail.com/playfile.asx'

When controlid is specified
dbus-send  --type=signal /control/[controlid] com.gnome.mplayer.Play string:'http://www.hotmail.com/playfile.asx'

cleanup
indent -kr -l100 -i4 -nut

*/


static DBusHandlerResult filter_func(DBusConnection * connection,
                                     DBusMessage * message, void *user_data)
{

    const gchar *sender;
    const gchar *destination;
    gint message_type;
    gchar *s = NULL;
    gchar *hrefid = NULL;
    gchar *path = NULL;
    gchar *keyname = NULL;
    DBusError error;
    DBusMessage *reply_message;
    gchar *path1;
    gchar *path2;
    gchar *path3;
    gchar *path4;
    GString *xml;
    gchar *xml_string;
    gint source_id;
    gint bitrate;

    message_type = dbus_message_get_type(message);
    sender = dbus_message_get_sender(message);
    destination = dbus_message_get_destination(message);

    /*
       printf("path=%s; interface=%s; member=%s; data=%s\n",
       dbus_message_get_path(message),
       dbus_message_get_interface(message), dbus_message_get_member(message), s);
     */

    path1 = g_strdup_printf("/control/%i", control_id);
    path2 = g_strdup_printf("/window/%i", embed_window);
    path3 = g_strdup_printf("/pid/%i", getpid());
    path4 = g_strdup_printf("/console/%s", rpconsole);

    if (dbus_message_get_path(message)) {

        if (g_ascii_strcasecmp(dbus_message_get_path(message), "/") == 0 ||
            g_ascii_strcasecmp(dbus_message_get_path(message), path1) == 0 ||
            g_ascii_strcasecmp(dbus_message_get_path(message), path2) == 0 ||
            g_ascii_strcasecmp(dbus_message_get_path(message), path3) == 0 ||
            g_ascii_strcasecmp(dbus_message_get_path(message), path4) == 0 ||
            g_ascii_strcasecmp(dbus_message_get_path(message), "/org/gnome/SettingsDaemon") == 0 ||
            g_ascii_strcasecmp(dbus_message_get_path(message),
                               "/org/gnome/SettingsDaemon/MediaKeys") == 0) {

            // printf("Path matched %s\n", dbus_message_get_path(message));
            if (message_type == DBUS_MESSAGE_TYPE_SIGNAL) {
                if (g_ascii_strcasecmp(dbus_message_get_member(message), "Open") == 0) {
                    g_idle_add(set_kill_mplayer,NULL);
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
                        if (strlen(s) > 0) {
                            if (strcmp(rpconsole, "NONE") == 0 || control_instance == FALSE) {
                                g_idle_add(clear_playlist_and_play, g_strdup(s));
                            }
                        }

                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "OpenPlaylist") == 0) {
                    g_idle_add(set_kill_mplayer,NULL);
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
                        selection = NULL;
                        g_idle_add(clear_playlist_and_play, g_strdup(s));

                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "OpenButton") == 0) {
                    g_idle_add(set_kill_mplayer,NULL);
                    dbus_error_init(&error);
                    if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &s,
                                              DBUS_TYPE_STRING, &hrefid, DBUS_TYPE_INVALID)) {
                        make_button(s, g_strdup(hrefid));
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "Add") == 0) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
                        if (strlen(s) > 0) {
                            g_idle_add(add_to_playlist_and_play, g_strdup(s));
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "SetPlaylistName") == 0) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
                        if (playlistname != NULL)
                            g_free(playlistname);
                        playlistname = g_strdup(s);
                        g_idle_add(set_update_gui, NULL);
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "Close") == 0) {
                    g_idle_add(set_kill_mplayer,NULL);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "Quit") == 0) {
                    g_idle_add(set_kill_mplayer,NULL);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "Play") == 0
                    && idledata != NULL) {
                    g_idle_add(set_play, idledata);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "RP_Play") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_INT32, &source_id, DBUS_TYPE_INVALID)) {
                        if (source_id != control_id) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_play, idledata);
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "Pause") == 0
                    && idledata != NULL) {
                    g_idle_add(set_pause, idledata);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "RP_Pause") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_INT32, &source_id, DBUS_TYPE_INVALID)) {
                        if (source_id != control_id) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_pause, idledata);
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "Stop") == 0
                    && idledata != NULL) {
                    g_idle_add(set_stop, idledata);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "RP_Stop") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_INT32, &source_id, DBUS_TYPE_INVALID)) {
                        if (source_id != control_id) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_stop, idledata);
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "FastForward") == 0
                    && idledata != NULL) {
                    g_idle_add(set_ff, idledata);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "RP_FastForward") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_INT32, &source_id, DBUS_TYPE_INVALID)) {
                        if (source_id != control_id) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_ff, idledata);
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }


                if (g_ascii_strcasecmp(dbus_message_get_member(message), "FastReverse") == 0
                    && idledata != NULL) {
                    g_idle_add(set_rew, idledata);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "RP_FastReverse") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_INT32, &source_id, DBUS_TYPE_INVALID)) {
                        if (source_id != control_id) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_rew, idledata);
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "Seek") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_DOUBLE, &(idledata->position),
                         DBUS_TYPE_INVALID)) {
                        g_idle_add(set_position, idledata);
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "Terminate") == 0) {
/*
					shutdown();
					while(gtk_events_pending() || thread != NULL) {
						gtk_main_iteration();
					}					
					dbus_unhook();
                    gtk_main_quit();
*/
                    dontplaynext = TRUE;
                    g_idle_add(set_quit, idledata);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "Volume") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_DOUBLE, &(idledata->volume),
                         DBUS_TYPE_INVALID)) {
                        g_idle_add(set_volume, idledata);
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "VolumeUp") == 0
                    && idledata != NULL) {
                    if (idledata->volume < 100) {
                        idledata->volume++;
                        g_idle_add(set_volume, idledata);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "VolumeDown") == 0
                    && idledata != NULL) {
                    if (idledata->volume > 0) {
                        idledata->volume--;
                        g_idle_add(set_volume, idledata);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "RP_Volume") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_DOUBLE, &(idledata->volume), DBUS_TYPE_INT32,
                         &source_id, DBUS_TYPE_INVALID)) {
                        if (source_id != control_id) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_volume, idledata);
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "SetFullScreen") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_BOOLEAN, &(idledata->fullscreen),
                         DBUS_TYPE_INVALID)) {
                        g_idle_add(set_fullscreen, idledata);
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "SetPercent") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_DOUBLE, &(idledata->percent),
                         DBUS_TYPE_INVALID)) {
                        g_idle_add(set_progress_value, idledata);
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "RP_SetPercent") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_DOUBLE, &(idledata->percent), DBUS_TYPE_INT32,
                         &source_id, DBUS_TYPE_INVALID)) {
                        if (source_id != control_id) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_progress_value, idledata);
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "SetCachePercent") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_DOUBLE, &(idledata->cachepercent),
                         DBUS_TYPE_INVALID)) {
                        g_idle_add(set_progress_value, idledata);
                        if (idledata->cachepercent > 0.99 && idledata->retry_on_full_cache) {
                            if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
								//printf("retrying on full cache\n");
                                play_iter(&iter, 0);
                            }
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "SetProgressText") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
                        g_strlcpy(idledata->progress_text, s, 1024);
                        if (state != PLAYING)
                            g_idle_add(set_progress_text, idledata);
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "RP_SetProgressText") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INT32, &source_id,
                         DBUS_TYPE_INVALID)) {
                        g_strlcpy(idledata->progress_text, s, 1024);
                        if (source_id != control_id) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_progress_text, idledata);
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }


                if (g_ascii_strcasecmp(dbus_message_get_member(message), "SetMediaLabel") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
                        g_strlcpy(idledata->media_info, s, 1024);
                        g_idle_add(set_media_label, idledata);
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "RP_SetMediaLabel") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INT32, &source_id,
                         DBUS_TYPE_INVALID)) {
                        g_strlcpy(idledata->media_info, s, 1024);
                        if (source_id != control_id) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_media_label, idledata);
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "SetInfo") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
                        g_strlcpy(idledata->info, s, 1024);
                        g_idle_add(set_media_info, idledata);
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "SetURL") == 0) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
                        g_strlcpy(idledata->url, s, 1024);
                        show_copyurl(idledata);
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }


                if (g_ascii_strcasecmp(dbus_message_get_member(message), "SetShowControls") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_BOOLEAN, &(idledata->showcontrols),
                         DBUS_TYPE_INVALID)) {
                        g_idle_add(set_show_controls, idledata);
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "ResizeWindow") == 0
                    && idledata != NULL) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_INT32, &window_x, DBUS_TYPE_INT32, &window_y,
                         DBUS_TYPE_INVALID)) {
                        g_idle_add(resize_window, idledata);
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "SetGUIState") == 0) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_INT32, &guistate, DBUS_TYPE_INVALID)) {
                        g_idle_add(set_gui_state, NULL);
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "RP_SetGUIState") == 0) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_INT32, &guistate, DBUS_TYPE_INT32, &source_id,
                         DBUS_TYPE_INVALID)) {
                        if (source_id != control_id) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_gui_state, NULL);
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "MediaPlayerKeyPressed") ==
                    0) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_STRING, &keyname,
                         DBUS_TYPE_INVALID)) {
                        if (g_ascii_strncasecmp(keyname, "Play", strlen("Play")) == 0) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_play, idledata);
                        }
                        if (g_ascii_strncasecmp(keyname, "Stop", strlen("Stop")) == 0) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_stop, idledata);
                        }
                        if (g_ascii_strncasecmp(keyname, "Previous", strlen("Previous")) == 0) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_prev, idledata);
                        }
                        if (g_ascii_strncasecmp(keyname, "Next", strlen("Next")) == 0) {
                            idledata->fromdbus = TRUE;
                            g_idle_add(set_next, idledata);
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }


                if (g_ascii_strcasecmp(dbus_message_get_member(message), "Ping") == 0) {
                    if (control_id != 0) {
                        path = g_strdup_printf("/control/%i", control_id);
                    }
                    if (embed_window != 0) {
                        path = g_strdup_printf("/window/%i", embed_window);
                    }
                    if (path == NULL) {
                        path = g_strdup_printf("/");
                    }

                    reply_message = dbus_message_new_signal(path, "com.gecko.mediaplayer", "Pong");
                    dbus_connection_send(connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    g_free(path);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
            } else if (message_type == DBUS_MESSAGE_TYPE_METHOD_CALL) {
                // printf("Got member %s\n",dbus_message_get_member(message));
                if (dbus_message_is_method_call
                    (message, "org.freedesktop.DBus.Introspectable", "Introspect")) {

                    xml =
                        g_string_new
                        ("<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
                         "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
                         "<node>\n" "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
                         "    <method name=\"Introspect\">\n"
                         "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
                         "    </method>\n" "  </interface>\n");

                    xml = g_string_append(xml,
                                          "<interface name=\"com.gnome.mplayer\">\n"
                                          "    <method name=\"Test\">\n"
                                          "    </method>\n"
                                          "    <method name=\"GetVolume\">\n"
                                          "    </method>\n"
                                          "    <method name=\"GetFullScreen\">\n"
                                          "    </method>\n"
                                          "    <method name=\"GetShowControls\">\n"
                                          "    </method>\n"
                                          "    <method name=\"GetTime\">\n"
                                          "    </method>\n"
                                          "    <method name=\"GetDuration\">\n"
                                          "    </method>\n"
                                          "    <method name=\"GetPercent\">\n"
                                          "    </method>\n"
                                          "    <method name=\"GetCacheSize\">\n"
                                          "    </method>\n"
                                          "    <method name=\"GetPlayState\">\n"
                                          "    </method>\n"
                                          "    <method name=\"GetBitrate\">\n"
                                          "        <arg name=\"url\" type=\"s\" />\n"
                                          "    </method>\n"
                                          "    <signal name=\"Open\">\n"
                                          "        <arg name=\"url\" type=\"s\" />\n"
                                          "    </signal>\n"
                                          "    <signal name=\"OpenPlaylist\">\n"
                                          "        <arg name=\"url\" type=\"s\" />\n"
                                          "    </signal>\n"
                                          "    <signal name=\"OpenButton\">\n"
                                          "        <arg name=\"url\" type=\"s\" />\n"
                                          "        <arg name=\"hrefid\" type=\"s\" />\n"
                                          "    </signal>\n"
                                          "    <signal name=\"Close\">\n"
                                          "    </signal>\n"
                                          "    <signal name=\"Quit\">\n"
                                          "    </signal>\n"
                                          "    <signal name=\"ResizeWindow\">\n"
                                          "        <arg name=\"width\" type=\"i\" />\n"
                                          "        <arg name=\"height\" type=\"i\" />\n"
                                          "    </signal>\n" "  </interface>\n");
                    xml = g_string_append(xml, "</node>\n");

                    xml_string = g_string_free(xml, FALSE);
                    reply_message = dbus_message_new_method_return(message);
                    dbus_message_append_args(reply_message,
                                             DBUS_TYPE_STRING, &xml_string, DBUS_TYPE_INVALID);
                    g_free(xml_string);
                    dbus_connection_send(connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (dbus_message_is_method_call(message, "com.gnome.mplayer", "Test")) {
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "com.gnome.mplayer", "GetVolume")) {
                    reply_message = dbus_message_new_method_return(message);
                    dbus_message_append_args(reply_message, DBUS_TYPE_DOUBLE, &idledata->volume,
                                             DBUS_TYPE_INVALID);
                    dbus_connection_send(connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "com.gnome.mplayer", "GetFullScreen")) {
                    reply_message = dbus_message_new_method_return(message);
                    dbus_message_append_args(reply_message, DBUS_TYPE_BOOLEAN, &fullscreen,
                                             DBUS_TYPE_INVALID);
                    dbus_connection_send(connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "com.gnome.mplayer", "GetShowControls")) {
                    showcontrols = get_show_controls();
                    reply_message = dbus_message_new_method_return(message);
                    dbus_message_append_args(reply_message, DBUS_TYPE_BOOLEAN, &showcontrols,
                                             DBUS_TYPE_INVALID);
                    dbus_connection_send(connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "com.gnome.mplayer", "GetTime")) {
                    reply_message = dbus_message_new_method_return(message);
                    dbus_message_append_args(reply_message, DBUS_TYPE_DOUBLE, &idledata->position,
                                             DBUS_TYPE_INVALID);
                    dbus_connection_send(connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "com.gnome.mplayer", "GetDuration")) {
                    reply_message = dbus_message_new_method_return(message);
                    dbus_message_append_args(reply_message, DBUS_TYPE_DOUBLE, &idledata->length,
                                             DBUS_TYPE_INVALID);
                    dbus_connection_send(connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "com.gnome.mplayer", "GetPercent")) {
                    reply_message = dbus_message_new_method_return(message);
                    dbus_message_append_args(reply_message, DBUS_TYPE_DOUBLE, &idledata->percent,
                                             DBUS_TYPE_INVALID);
                    dbus_connection_send(connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "com.gnome.mplayer", "GetCacheSize")) {
                    reply_message = dbus_message_new_method_return(message);
                    dbus_message_append_args(reply_message, DBUS_TYPE_INT32, &plugin_cache_size,
                                             DBUS_TYPE_INVALID);
                    dbus_connection_send(connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "com.gnome.mplayer", "GetPlayState")) {
                    reply_message = dbus_message_new_method_return(message);
                    dbus_message_append_args(reply_message, DBUS_TYPE_INT32, &js_state,
                                             DBUS_TYPE_INVALID);
                    dbus_connection_send(connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "com.gnome.mplayer", "GetBitrate")) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
                        bitrate = get_bitrate(s);
                    } else {
                        dbus_error_free(&error);
                        bitrate = 0;
                    }
                    reply_message = dbus_message_new_method_return(message);
                    dbus_message_append_args(reply_message, DBUS_TYPE_INT32, &bitrate,
                                             DBUS_TYPE_INVALID);
                    dbus_connection_send(connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
            } else {
                if (verbose)
                    printf("path didn't match - %s\n", dbus_message_get_path(message));
            }

        }
    }
    g_free(path1);
    g_free(path2);
    g_free(path3);
    g_free(path4);
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void dbus_open_by_hrefid(gchar * hrefid)
{
    gchar *path;
    DBusMessage *message;
    gchar *id;

    id = g_strdup(hrefid);
    if (verbose)
        printf("requesting id = %s\n", id);
    if (connection != NULL && control_id != 0) {
        path = g_strdup_printf("/control/%i", control_id);
        message = dbus_message_new_signal(path, "com.gecko.mediaplayer", "RequestById");
        dbus_message_append_args(message, DBUS_TYPE_STRING, &id, DBUS_TYPE_INVALID);
        dbus_connection_send(connection, message, NULL);
        dbus_message_unref(message);
        g_free(path);
    }
    g_free(id);
}

void dbus_open_next()
{
    gchar *path;
    DBusMessage *message;

    if (connection != NULL && control_id != 0) {
        path = g_strdup_printf("/control/%i", control_id);
        message = dbus_message_new_signal(path, "com.gecko.mediaplayer", "Next");
        dbus_connection_send(connection, message, NULL);
        dbus_message_unref(message);
        g_free(path);
    }
}

void dbus_open(gchar * arg)
{
    gchar *path;
    gchar *localarg;
    DBusMessage *message;

    if (connection != NULL) {
        path = g_strdup_printf("/");
        if (replace_and_play) {
            message = dbus_message_new_signal(path, "com.gnome.mplayer", "Open");
        } else {
            message = dbus_message_new_signal(path, "com.gnome.mplayer", "Add");
        }
        localarg = g_strdup(arg);
        dbus_message_append_args(message, DBUS_TYPE_STRING, &localarg, DBUS_TYPE_INVALID);
        dbus_connection_send(connection, message, NULL);
        dbus_message_unref(message);
        g_free(localarg);
        g_free(path);
    }
}

void dbus_set_playlist_name(gchar * arg)
{
    gchar *path;
    gchar *localarg;
    DBusMessage *message;

    if (connection != NULL) {
        path = g_strdup_printf("/");
        message = dbus_message_new_signal(path, "com.gnome.mplayer", "SetPlaylistName");
        localarg = g_strdup(arg);
        dbus_message_append_args(message, DBUS_TYPE_STRING, &localarg, DBUS_TYPE_INVALID);
        dbus_connection_send(connection, message, NULL);
        dbus_message_unref(message);
        g_free(localarg);
        g_free(path);
    }
}

void dbus_cancel()
{
    gchar *path;
    DBusMessage *message;
    gint id;

    id = control_id;
    if (connection != NULL && control_id != 0) {
        path = g_strdup_printf("/control/%i", control_id);
        message = dbus_message_new_signal(path, "com.gecko.mediaplayer", "Cancel");
        dbus_message_append_args(message, DBUS_TYPE_INT32, &id, DBUS_TYPE_INVALID);
        dbus_connection_send(connection, message, NULL);
        dbus_message_unref(message);
        g_free(path);
    }

}

void dbus_send_rpsignal(gchar * signal)
{
    gchar *path;
    gchar *localsignal;
    DBusMessage *message;
    gint id;

    if (g_strcasecmp(rpconsole, "NONE") == 0)
        return;

    id = control_id;
    if (connection != NULL) {
        path = g_strdup_printf("/console/%s", rpconsole);
        localsignal = g_strdup(signal);
        message = dbus_message_new_signal(path, "com.gnome.mplayer", localsignal);
        dbus_message_append_args(message, DBUS_TYPE_INT32, &id, DBUS_TYPE_INVALID);
        dbus_connection_send(connection, message, NULL);
        dbus_message_unref(message);
        g_free(localsignal);
        g_free(path);
    }
}

void dbus_send_rpsignal_with_int(gchar * signal, int value)
{
    gchar *path;
    gchar *localsignal;
    DBusMessage *message;
    gint id;

    if (g_strcasecmp(rpconsole, "NONE") == 0)
        return;

    id = control_id;
    if (connection != NULL) {
        path = g_strdup_printf("/console/%s", rpconsole);
        localsignal = g_strdup(signal);
        message = dbus_message_new_signal(path, "com.gnome.mplayer", localsignal);
        dbus_message_append_args(message, DBUS_TYPE_INT32, &value, DBUS_TYPE_INT32, &id,
                                 DBUS_TYPE_INVALID);
        dbus_connection_send(connection, message, NULL);
        dbus_message_unref(message);
        g_free(localsignal);
        g_free(path);
    }
}

void dbus_send_rpsignal_with_double(gchar * signal, gdouble value)
{
    gchar *path;
    gchar *localsignal;
    DBusMessage *message;
    gint id;

    if (g_strcasecmp(rpconsole, "NONE") == 0)
        return;

    id = control_id;
    if (connection != NULL) {
        path = g_strdup_printf("/console/%s", rpconsole);
        localsignal = g_strdup(signal);
        message = dbus_message_new_signal(path, "com.gnome.mplayer", localsignal);
        dbus_message_append_args(message, DBUS_TYPE_DOUBLE, &value, DBUS_TYPE_INT32, &id,
                                 DBUS_TYPE_INVALID);
        dbus_connection_send(connection, message, NULL);
        dbus_message_unref(message);
        g_free(localsignal);
        g_free(path);
    }
}

void dbus_send_rpsignal_with_string(gchar * signal, gchar * value)
{
    gchar *path;
    gchar *localsignal;
    DBusMessage *message;
    gint id;
    gchar *localstr;

    if (g_strcasecmp(rpconsole, "NONE") == 0)
        return;

    id = control_id;
    if (connection != NULL) {
        path = g_strdup_printf("/console/%s", rpconsole);
        localsignal = g_strdup(signal);
        localstr = g_strdup(value);
        message = dbus_message_new_signal(path, "com.gnome.mplayer", localsignal);
        dbus_message_append_args(message, DBUS_TYPE_STRING, &localstr, DBUS_TYPE_INT32, &id,
                                 DBUS_TYPE_INVALID);
        dbus_connection_send(connection, message, NULL);
        dbus_message_unref(message);
        g_free(localstr);
        g_free(localsignal);
        g_free(path);
    }
}

void dbus_reload_plugins()
{
    gchar *path;
    DBusMessage *message;

    if (connection != NULL && control_id != 0) {
        path = g_strdup_printf("/control/%i", control_id);
        message = dbus_message_new_signal(path, "com.gecko.mediaplayer", "ReloadPlugins");
        dbus_connection_send(connection, message, NULL);
        dbus_message_unref(message);
        g_free(path);
    }

}

void dbus_send_event(gchar * event, gint button)
{
    gchar *path;
    gchar *localevent;
    gint localbutton = 0;
    DBusMessage *message;

    localbutton = button;


    if (connection != NULL && control_id != 0) {
        path = g_strdup_printf("/control/%i", control_id);
        localevent = g_strdup_printf("%s", event);
        if (verbose) {
            //printf("Posting Event %s\n",localevent);
        }
        message = dbus_message_new_signal(path, "com.gecko.mediaplayer", "Event");
        dbus_message_append_args(message, DBUS_TYPE_STRING, &localevent, DBUS_TYPE_INT32,
                                 &localbutton, DBUS_TYPE_INVALID);
        dbus_connection_send(connection, message, NULL);
        dbus_message_unref(message);
        g_free(path);
        g_free(localevent);
    }
}

gboolean GetProperty(gchar * property)
{

    return TRUE;
}

gboolean dbus_hookup(gint windowid, gint controlid)
{

    DBusError error;
    DBusBusType type = DBUS_BUS_SESSION;
    gchar *match;
    gchar *path;
    DBusMessage *reply_message;
    gint id;
    gint ret;
    gchar *filename;
    gboolean local_rnp;

    dbus_error_init(&error);
    connection = dbus_bus_get(type, &error);
    if (connection == NULL) {
        printf("Failed to open connection to %s message bus: %s\n",
               (type == DBUS_BUS_SYSTEM) ? "system" : "session", error.message);
        dbus_error_free(&error);
        return FALSE;
    }
    dbus_connection_setup_with_g_main(connection, NULL);

    // vtable.message_function = &message_handler;


    // dbus_bus_request_name(connection,"com.gnome.mplayer",0,NULL);
    // dbus_connection_register_object_path (connection,"/com/gnome/mplayer", &vtable,NULL);

    match = g_strdup_printf("type='signal',interface='com.gnome.mplayer'");
    dbus_bus_add_match(connection, match, &error);
    if (verbose)
        printf("Using match: %s\n", match);
    g_free(match);
    dbus_error_free(&error);

    match = g_strdup_printf("type='signal',interface='org.gnome.SettingsDaemon'");
    dbus_bus_add_match(connection, match, &error);
    if (verbose)
        printf("Using match: %s\n", match);
    g_free(match);
    dbus_error_free(&error);
    if (use_mediakeys) {
        match = g_strdup_printf("type='signal',interface='org.gnome.SettingsDaemon.MediaKeys'");
        dbus_bus_add_match(connection, match, &error);
        if (verbose)
            printf("Using match: %s\n", match);
        g_free(match);
        dbus_error_free(&error);
    }

    dbus_connection_add_filter(connection, filter_func, NULL, NULL);

    if (verbose)
        printf("Proxy connections and Command connected\n");

    if (control_id != 0) {
        path = g_strdup_printf("com.gnome.mplayer.cid%i", control_id);
        ret = dbus_bus_request_name(connection, path, 0, NULL);
        g_free(path);
        path = g_strdup_printf("/control/%i", control_id);
        id = control_id;
        reply_message = dbus_message_new_signal(path, "com.gecko.mediaplayer", "Ready");
        dbus_message_append_args(reply_message, DBUS_TYPE_INT32, &id, DBUS_TYPE_INVALID);
        dbus_connection_send(connection, reply_message, NULL);
        dbus_message_unref(reply_message);
        g_free(path);
    } else {
        ret = dbus_bus_request_name(connection, "com.gnome.mplayer", 0, NULL);
        if (single_instance && ret > 1) {
            if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &filename,
                                   -1);
                dbus_open(filename);
                local_rnp = replace_and_play;
                while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &iter)) {
                    replace_and_play = FALSE;
                    gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &filename,
                                       -1);
                    dbus_open(filename);
                }
                if (playlistname != NULL && local_rnp)
                    dbus_set_playlist_name(playlistname);
            }
            exit(EXIT_SUCCESS);
        }
    }
    screensaver_disabled = FALSE;

    return TRUE;
}

void dbus_unhook()
{
    if (connection != NULL) {
        dbus_enable_screensaver();
        // dbus_connection_close(connection);
        dbus_connection_unref(connection);
        connection = NULL;
    }
}

void dbus_enable_screensaver()
{
#if SM_INHIBIT || SS_INHIBIT
    DBusMessage *message;
#endif

    if (connection != NULL && screensaver_disabled) {

		if (use_xscrnsaver) {
#ifdef XSCRNSAVER_ENABLED
			XScreenSaverSuspend(GDK_WINDOW_XDISPLAY(window->window), FALSE);
#endif

		} else {
#if SM_INHIBIT
		    message =
		        dbus_message_new_method_call("org.gnome.SessionManager", "/org/gnome/SessionManager",
		                                     "org.gnome.SessionManager", "UnInhibit");
		    dbus_message_append_args(message, DBUS_TYPE_INT32, &sm_cookie, DBUS_TYPE_INVALID);
		    dbus_connection_send(connection, message, NULL);
		    dbus_message_unref(message);
#endif
#if SS_INHIBIT
		    message =
		        dbus_message_new_method_call("org.gnome.ScreenSaver", "/org/gnome/ScreenSaver",
		                                     "org.gnome.ScreenSaver", "UnInhibit");
		    dbus_message_append_args(message, DBUS_TYPE_INT32, &ss_cookie, DBUS_TYPE_INVALID);
		    dbus_connection_send(connection, message, NULL);
	        dbus_message_unref(message);
#endif
		}
	
        screensaver_disabled = FALSE;
    }
}

void dbus_disable_screensaver()
{
#if SM_INHIBIT || SS_INHIBIT
    DBusError error;
    DBusMessage *reply_message;
    DBusMessage *message;
    const gchar *app;
    const gchar *reason;
    gint flags;
#endif

    if (connection != NULL) {
		if (use_xscrnsaver) {

#ifdef XSCRNSAVER_ENABLED
			XScreenSaverSuspend(GDK_WINDOW_XDISPLAY(window->window), TRUE);
#endif

		} else {

#if SS_INHIBIT
		    dbus_error_init(&error);
		    message =
		        dbus_message_new_method_call("org.gnome.ScreenSaver", "/org/gnome/ScreenSaver",
		                                     "org.gnome.ScreenSaver", "Inhibit");
		    app = g_strdup_printf("gnome-mplayer");
		    reason = g_strdup_printf("playback");
		    dbus_message_append_args(message, DBUS_TYPE_STRING, &app, DBUS_TYPE_STRING, &reason,
		                             DBUS_TYPE_INVALID);
		    reply_message = dbus_connection_send_with_reply_and_block(connection, message, 200, &error);

		    if (reply_message) {
		        dbus_message_get_args(reply_message, &error, DBUS_TYPE_INT32, &ss_cookie, NULL);
		        dbus_message_unref(reply_message);
		    }

		    dbus_message_unref(message);
		    dbus_error_free(&error);
#endif

#if SM_INHIBIT
		    dbus_error_init(&error);
		    message =
		        dbus_message_new_method_call("org.gnome.SessionManager", "/org/gnome/SessionManager",
		                                     "org.gnome.SessionManager", "Inhibit");
		    app = g_strdup_printf("gnome-mplayer");
		    reason = g_strdup_printf("playback");
		    flags = 8;
		    dbus_message_append_args(message, DBUS_TYPE_STRING, &app, DBUS_TYPE_UINT32,
		                             &(idledata->windowid), DBUS_TYPE_STRING, &reason, DBUS_TYPE_UINT32,
		                             &flags, DBUS_TYPE_INVALID);
		    reply_message = dbus_connection_send_with_reply_and_block(connection, message, 200, &error);

		    if (reply_message) {
		        dbus_message_get_args(reply_message, &error, DBUS_TYPE_INT32, &sm_cookie, NULL);
		        dbus_message_unref(reply_message);
		    }

		    dbus_message_unref(message);
		    dbus_error_free(&error);
#endif
		}
		
        screensaver_disabled = TRUE;
    }
}
