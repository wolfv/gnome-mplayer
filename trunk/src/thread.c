/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * thread.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * thread.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * thread.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with thread.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "thread.h"
#include "common.h"


void shutdown()
{
    // printf("state = %i quit = %i\n",state,QUIT);
    if (state != QUIT) {
        idledata->percent = 1.0;
        g_idle_add(set_progress_value, idledata);
        g_idle_add(set_stop, idledata);
        send_command("quit\n");
    }

}

gboolean send_command(gchar * command)
{
    gint ret;

    // printf("send command = %s\n",command);
    ret = write(std_in, command, strlen(command));
    fsync(std_in);
    if (ret < 0) {
        return FALSE;
    } else {
        return TRUE;
    }

}

gboolean thread_complete(GIOChannel * source, GIOCondition condition, gpointer data)
{
    g_idle_add(set_stop, idledata);
    state = QUIT;
	g_source_remove(watch_in_id);
	g_source_remove(watch_err_id);
    g_mutex_unlock(thread_running);
    return FALSE;
}

gboolean thread_reader_error(GIOChannel * source, GIOCondition condition, gpointer data)
{
    GString *mplayer_output;
    GIOStatus status;
    gchar *error_msg = NULL;
    GtkWidget *dialog;

    if (source == NULL) {
		g_source_remove(watch_in_id);
		g_source_remove(watch_in_hup_id);
        return FALSE;
    }


    if (state == QUIT) {
        g_idle_add(set_stop, idledata);
        state = QUIT;
		g_source_remove(watch_in_id);
		g_source_remove(watch_in_hup_id);
        g_mutex_unlock(thread_running);
        return FALSE;
    }

    mplayer_output = g_string_new("");

    status = g_io_channel_read_line_string(source, mplayer_output, NULL, NULL);
    if (verbose && strstr(mplayer_output->str, "ANS_") == NULL)
        printf("ERROR: %s", mplayer_output->str);

    if (strstr(mplayer_output->str, "Couldn't open DVD device") != 0) {
        error_msg = g_strdup(mplayer_output->str);
    }

    if (strstr(mplayer_output->str, "Failed to open") != NULL) {
        if (strstr(mplayer_output->str, "LIRC") == NULL &&
			strstr(mplayer_output->str, "/dev/rtc") == NULL &&
			strstr(mplayer_output->str, "registry file") == NULL) {
            	error_msg = g_strdup(mplayer_output->str);
		}
    }

    if (error_msg != NULL) {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE, error_msg);
        gtk_window_set_title(GTK_WINDOW(dialog), "GNOME MPlayer Error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(error_msg);
    }

    if (status != G_IO_STATUS_NORMAL) {
        g_string_free(mplayer_output, TRUE);
        return FALSE;
    }

    g_string_free(mplayer_output, TRUE);
    return TRUE;

}

gboolean thread_reader(GIOChannel * source, GIOCondition condition, gpointer data)
{

    GString *mplayer_output;
    GIOStatus status;
    gchar *buf, *message = NULL;
    gint pos, volume, i;
    gfloat percent;
    GError *error = NULL;
    gchar *error_msg = NULL;
    GtkWidget *dialog;

    if (source == NULL) {
		g_source_remove(watch_err_id);
		g_source_remove(watch_in_hup_id);
        return FALSE;
    }


    if (state == QUIT) {
        g_idle_add(set_stop, idledata);
        state = QUIT;
		g_source_remove(watch_err_id);
		g_source_remove(watch_in_hup_id);
        g_mutex_unlock(thread_running);
        return FALSE;
    }

    mplayer_output = g_string_new("");

    status = g_io_channel_read_line_string(source, mplayer_output, NULL, &error);
    if (status != G_IO_STATUS_NORMAL) {
        if (error != NULL) {
            //printf("%i: %s\n",error->code,error->message);
            if (error->code == G_CONVERT_ERROR_ILLEGAL_SEQUENCE) {
                g_error_free(error);
                error = NULL;
                g_io_channel_set_encoding(source, NULL, NULL);
            } else {
                g_string_free(mplayer_output, TRUE);
                g_idle_add(set_stop, idledata);
                state = QUIT;
                g_mutex_unlock(thread_running);
                g_error_free(error);
                error = NULL;
                return FALSE;
            }
        } else {
            g_string_free(mplayer_output, TRUE);
            g_idle_add(set_stop, idledata);
            state = QUIT;
            g_mutex_unlock(thread_running);
            error = NULL;
            return FALSE;
        }
    }
    if ((strstr(mplayer_output->str, "A:") != NULL) || (strstr(mplayer_output->str, "V:") != NULL)) {
        g_string_free(mplayer_output, TRUE);
        return TRUE;
    }
    //printf("thread_reader state = %i : status = %i\n",state,status);
    if (verbose && strstr(mplayer_output->str, "ANS_") == NULL)
        printf("%s", mplayer_output->str);


    if ((strstr(mplayer_output->str, "(Quit)") != NULL)
        || (strstr(mplayer_output->str, "(End of file)") != NULL)) {
        state = QUIT;
        g_idle_add(set_stop, idledata);
        g_string_free(mplayer_output, TRUE);
        g_mutex_unlock(thread_running);
        return FALSE;
    }

    if (strstr(mplayer_output->str, "VO:") != NULL) {
        buf = strstr(mplayer_output->str, "VO:");
        sscanf(buf, "VO: [%9[^]]] %ix%i => %ix%i", vm, &actual_x, &actual_y, &play_x, &play_y);

        printf("Resizing to %i x %i \n", actual_x, actual_y);
        idledata->width = actual_x;
        idledata->height = actual_y;
        idledata->videopresent = 1;
        g_idle_add(resize_window, idledata);
        videopresent = 1;
        g_idle_add(set_volume_from_slider, NULL);
    }

    if (strstr(mplayer_output->str, "Video: no video") != NULL) {
        actual_x = 150;
        actual_y = 1;

        // printf("Resizing to %i x %i \n", actual_x, actual_y);
        idledata->width = actual_x;
        idledata->height = actual_y;
        idledata->videopresent = 0;
        g_idle_add(resize_window, idledata);
        g_idle_add(set_volume_from_slider, NULL);

    }

    if (strstr(mplayer_output->str, "ANS_PERCENT_POSITION") != 0) {
        buf = strstr(mplayer_output->str, "ANS_PERCENT_POSITION");
        sscanf(buf, "ANS_PERCENT_POSITION=%i", &pos);
        //printf("Position = %i\n",pos);
        idledata->percent = pos / 100.0;
        g_idle_add(set_progress_value, idledata);
    }

    if (strstr(mplayer_output->str, "ANS_LENGTH") != 0) {
        buf = strstr(mplayer_output->str, "ANS_LENGTH");
        sscanf(buf, "ANS_LENGTH=%lf", &idledata->length);
        g_idle_add(set_progress_time, idledata);
    }

    if (strstr(mplayer_output->str, "ANS_TIME_POSITION") != 0) {
        buf = strstr(mplayer_output->str, "ANS_TIME_POSITION");
        sscanf(buf, "ANS_TIME_POSITION=%lf", &idledata->position);
        g_idle_add(set_progress_time, idledata);
    }

    if (strstr(mplayer_output->str, "ANS_volume") != 0) {
        buf = strstr(mplayer_output->str, "ANS_volume");
        sscanf(buf, "ANS_volume=%i", &volume);
        idledata->volume = volume;
        idledata->mute = 0;
        buf = g_strdup_printf(_("Volume %i%%"), volume);
        g_strlcpy(idledata->vol_tooltip, buf, 128);
        g_idle_add(set_volume_tip, idledata);
        g_free(buf);
    }

    if (strstr(mplayer_output->str, "ANS_brightness") != 0) {
        buf = strstr(mplayer_output->str, "ANS_brightness");
        sscanf(buf, "ANS_brightness=%i", &idledata->brightness);
    }

    if (strstr(mplayer_output->str, "ANS_contrast") != 0) {
        buf = strstr(mplayer_output->str, "ANS_contrast");
        sscanf(buf, "ANS_contrast=%i", &idledata->contrast);
    }

    if (strstr(mplayer_output->str, "ANS_gamma") != 0) {
        buf = strstr(mplayer_output->str, "ANS_gamma");
        sscanf(buf, "ANS_gamma=%i", &idledata->gamma);
    }

    if (strstr(mplayer_output->str, "ANS_hue") != 0) {
        buf = strstr(mplayer_output->str, "ANS_hue");
        sscanf(buf, "ANS_hue=%i", &idledata->hue);
    }

    if (strstr(mplayer_output->str, "ANS_saturation") != 0) {
        buf = strstr(mplayer_output->str, "ANS_saturation");
        sscanf(buf, "ANS_saturation=%i", &idledata->saturation);
    }

    if (strstr(mplayer_output->str, "Cache fill") != 0) {
        buf = strstr(mplayer_output->str, "Cache fill");
        sscanf(buf, "Cache fill: %f%%", &percent);
        //printf("Percent = %f\n",percent);
        //printf("Buffer = %s\n",buf);
        buf = g_strdup_printf(_("Cache fill: %2.2f%%"), percent);
        g_strlcpy(idledata->progress_text, buf, 1024);
        g_idle_add(set_progress_text, idledata);
        idledata->percent = percent / 100.0;
        g_idle_add(set_progress_value, idledata);
    }

    if (strstr(mplayer_output->str, "Connecting") != 0) {
        buf = g_strdup_printf(_("Connecting"));
        g_strlcpy(idledata->progress_text, buf, 1024);
        g_idle_add(set_progress_text, idledata);
        idledata->percent = 0.0;
        g_idle_add(set_progress_value, idledata);
    }
	
    if (strstr(mplayer_output->str, "ID_VIDEO_FORMAT") != 0) {
        g_string_truncate(mplayer_output, mplayer_output->len - 1);
        buf = strstr(mplayer_output->str, "ID_VIDEO_FORMAT") + strlen("ID_VIDEO_FORMAT=");
        g_strlcpy(idledata->video_format, buf, 64);
    }

    if (strstr(mplayer_output->str, "ID_VIDEO_CODEC") != 0) {
        g_string_truncate(mplayer_output, mplayer_output->len - 1);
        buf = strstr(mplayer_output->str, "ID_VIDEO_CODEC") + strlen("ID_VIDEO_CODEC=");
        g_strlcpy(idledata->video_codec, buf, 16);
    }

    if (strstr(mplayer_output->str, "ID_VIDEO_FPS") != 0) {
        g_string_truncate(mplayer_output, mplayer_output->len - 1);
        buf = strstr(mplayer_output->str, "ID_VIDEO_FPS") + strlen("ID_VIDEO_FPS=");
        g_strlcpy(idledata->video_fps, buf, 16);
    }

    if (strstr(mplayer_output->str, "ID_VIDEO_BITRATE") != 0) {
        g_string_truncate(mplayer_output, mplayer_output->len - 1);
        buf = strstr(mplayer_output->str, "ID_VIDEO_BITRATE") + strlen("ID_VIDEO_BITRATE=");
        g_strlcpy(idledata->video_bitrate, buf, 16);
    }

    if (strstr(mplayer_output->str, "ID_AUDIO_CODEC") != 0) {
        g_string_truncate(mplayer_output, mplayer_output->len - 1);
        buf = strstr(mplayer_output->str, "ID_AUDIO_CODEC") + strlen("ID_AUDIO_CODEC=");
        g_strlcpy(idledata->audio_codec, buf, 16);
    }

    if (strstr(mplayer_output->str, "ID_AUDIO_BITRATE") != 0) {
        g_string_truncate(mplayer_output, mplayer_output->len - 1);
        buf = strstr(mplayer_output->str, "ID_AUDIO_BITRATE") + strlen("ID_AUDIO_BITRATE=");
        g_strlcpy(idledata->audio_bitrate, buf, 16);
    }

    if (strstr(mplayer_output->str, "ID_AUDIO_RATE") != 0) {
        g_string_truncate(mplayer_output, mplayer_output->len - 1);
        buf = strstr(mplayer_output->str, "ID_AUDIO_RATE") + strlen("ID_AUDIO_RATE=");
        g_strlcpy(idledata->audio_samplerate, buf, 16);
    }

    if (strstr(mplayer_output->str, "File not found") != 0) {
    }

    if (strstr(mplayer_output->str, "Couldn't open DVD device") != 0) {
        error_msg = g_strdup(mplayer_output->str);
    }

    if (strstr(mplayer_output->str, "ICY Info") != NULL) {
        buf = strstr(mplayer_output->str, "'");
        if (message) {
            g_free(message);
            message = NULL;
        }
        if (buf != NULL) {
            message = g_strdup_printf("%s", buf + 1);
            for (i = 0; i < (int) strlen(message) - 1; i++) {
                if (message[i] == '\'') {
                    message[i] = '\0';
                    break;
                }
            }
        }
        if (message)
            g_strlcpy(idledata->info, message, 1024);
        g_idle_add(set_media_info, idledata);
    }

    if (error_msg != NULL) {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK, error_msg);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(error_msg);
    }

    g_string_free(mplayer_output, TRUE);
    return TRUE;

}

gboolean thread_query(gpointer data)
{
    int size;

    //printf("thread_query state = %i\n",state);

    // this function wakes up every 1/2 second and so 
    // this is where we should put in some code to detect if the media is getting 
    // to close to the amount cached and if so pause until we have a 5% gap or something

    // idledata->percent > (idledata->cachepercent - 0.05)
    //              autopause
    // else
    // 

    if (state == PLAYING) {
        size = write(std_in, "get_percent_pos\n", strlen("get_percent_pos\n"));
        if (size == -1) {
            shutdown();
            return FALSE;
        } else {
            send_command("get_time_length\n");
            send_command("get_time_pos\n");
            return TRUE;
        }
    } else {
        if (state == QUIT) {
            return FALSE;
        } else {
            return TRUE;
        }
    }
}

gpointer launch_player(gpointer data)
{

    gint ok;
    gint player_window;
    char *argv[255];
    gint arg = 0;
	gchar *filename;
	gint count;
	GtkTreePath *path;

    ThreadData *threaddata = (ThreadData *) data;

    fullscreen = 0;
    videopresent = 1;

    g_mutex_lock(thread_running);

    g_strlcpy(idledata->info, threaddata->filename, 1024);
    idledata->percent = 0.0;
    g_strlcpy(idledata->progress_text, "", 1024);
    idledata->width = 1;
    idledata->height = 1;
    idledata->videopresent = 1;
    idledata->volume = 100.0;
    idledata->length = 0.0;

    g_idle_add(set_progress_value, idledata);
    g_idle_add(set_progress_text, idledata);
    g_idle_add(set_media_info, idledata);
    //g_idle_add(set_window_visible, idledata);

    while (gtk_events_pending())
        gtk_main_iteration();

    argv[arg++] = g_strdup_printf("mplayer");
    if (vo != NULL) {
        argv[arg++] = g_strdup_printf("-profile");
        argv[arg++] = g_strdup_printf("gnome-mplayer");
    }
    argv[arg++] = g_strdup_printf("-quiet");
    argv[arg++] = g_strdup_printf("-slave");
    argv[arg++] = g_strdup_printf("-identify");
    argv[arg++] = g_strdup_printf("-softvol");
    argv[arg++] = g_strdup_printf("-framedrop");
    argv[arg++] = g_strdup_printf("-noconsolecontrols");
    argv[arg++] = g_strdup_printf("-osdlevel");
    argv[arg++] = g_strdup_printf("%i", osdlevel);
    if (strcmp(threaddata->filename, "dvdnav://") == 0) {
        argv[arg++] = g_strdup_printf("-mouse-movements");
    } else if (strcmp(threaddata->filename, "dvd://") != 0) {
        argv[arg++] = g_strdup_printf("-nomouseinput");
        argv[arg++] = g_strdup_printf("-cache");
        argv[arg++] = g_strdup_printf("%i", cache_size);
    }
    argv[arg++] = g_strdup_printf("-wid");
    player_window = get_player_window();
    argv[arg++] = g_strdup_printf("0x%x", player_window);

	if (control_id == 0) {
		argv[arg++] = g_strdup_printf("-idx");
	} else {
		argv[arg++] = g_strdup_printf("-cookies");
	}

	if (playlist || threaddata->playlist)
        argv[arg++] = g_strdup_printf("-playlist");
    argv[arg] = g_strdup_printf("%s", threaddata->filename);
    argv[arg + 1] = NULL;
    ok = g_spawn_async_with_pipes(NULL, argv, NULL,
                                  G_SPAWN_SEARCH_PATH,
                                  NULL, NULL, NULL, &std_in, &std_out, &std_err, NULL);

    if (ok) {
        printf("Spawn succeeded for filename %s\n", threaddata->filename);
        state = PAUSED;

        if (channel_in != NULL) {
            g_io_channel_unref(channel_in);
            channel_in = NULL;
        }

        if (channel_err != NULL) {
            g_io_channel_unref(channel_err);
            channel_err = NULL;
        }

        channel_in = g_io_channel_unix_new(std_out);
        channel_err = g_io_channel_unix_new(std_err);
        g_io_channel_set_close_on_unref(channel_in, TRUE);
        g_io_channel_set_close_on_unref(channel_err, TRUE);
        watch_in_id = g_io_add_watch(channel_in, G_IO_IN, thread_reader, NULL);
        watch_err_id = g_io_add_watch(channel_err, G_IO_IN | G_IO_ERR | G_IO_HUP, thread_reader_error, NULL);
        watch_in_hup_id = g_io_add_watch(channel_in, G_IO_ERR | G_IO_HUP, thread_complete, NULL);

        g_idle_add(set_play, NULL);
        g_timeout_add(500, thread_query, NULL);

    } else {
        state = QUIT;
        printf("Spawn failed for filename %s\n", threaddata->filename);
    }

    g_mutex_lock(thread_running);
    printf("Thread completing\n");
	g_source_remove(watch_in_id);
	g_source_remove(watch_err_id);
	g_source_remove(watch_in_hup_id);
    idledata->percent = 1.0;
    idledata->position = idledata->length;
    g_idle_add(set_progress_value, idledata);
    g_idle_add(set_progress_time, idledata);

    if (embed_window != 0 || control_id != 0) {
		dbus_send_event("MediaComplete",0);
        dbus_open_next();
	}
	
    g_free(threaddata);
    threaddata = NULL;

    if (channel_in != NULL) {
        g_io_channel_shutdown(channel_in, FALSE, NULL);
        channel_in = NULL;
    }

    if (channel_out != NULL) {
        g_io_channel_shutdown(channel_out, FALSE, NULL);
        channel_out = NULL;
    }

    if (channel_err != NULL) {
        g_io_channel_shutdown(channel_err, FALSE, NULL);
        channel_err = NULL;
    }

    arg = 0;
    while (argv[arg] != NULL) {
        g_free(argv[arg]);
        argv[arg] = NULL;
        arg++;
    }
    g_mutex_unlock(thread_running);
    // printf("Thread done\n");

	if (dontplaynext == FALSE) {
		if (next_item_in_playlist(&iter)) {
			gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN,&filename, COUNT_COLUMN,&count,PLAYLIST_COLUMN,&playlist,-1);
			set_media_info(filename);
			play_file(filename, playlist);
			gtk_list_store_set(playliststore,&iter,COUNT_COLUMN,count+1, -1);
			g_free(filename);
			if (GTK_IS_TREE_SELECTION(selection)) {
				path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore),&iter);
				gtk_tree_selection_select_path(selection,path);	
				gtk_tree_path_free(path);
			}
			
		} else {
			// printf("end of thread playlist is empty\n");
			if (loop) {
				gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore),&iter);
				gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN,&filename, COUNT_COLUMN,&count,PLAYLIST_COLUMN,&playlist,-1);
				set_media_info(filename);
				play_file(filename, playlist);
				gtk_list_store_set(playliststore,&iter,COUNT_COLUMN,count+1, -1);
				g_free(filename);
				if (GTK_IS_TREE_SELECTION(selection)) {
					path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore),&iter);
					gtk_tree_selection_select_path(selection,path);	
					gtk_tree_path_free(path);
				}
			}
		}
	} else {
		dontplaynext = FALSE;
	}

    return NULL;
}
