/***************************************************************************
 *            thread.c
 *
 *  Fri Feb  2 20:55:20 2007
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
 
#include "thread.h"
#include "common.h"


void shutdown()
{

    if (state != QUIT) {
		idledata->percent = 1.0;
		g_idle_add(set_progress_value,idledata);
        g_idle_add(set_stop,idledata);
        send_command("quit\n");
		
    }
	while(gtk_events_pending()) gtk_main_iteration();

}

gboolean send_command(gchar *command) {
	
	write(std_in, command, strlen(command));
	fsync(std_in);
	return TRUE;

}
	
gboolean thread_reader_error(GIOChannel * source, GIOCondition condition, gpointer data)
{
    GString *mplayer_output;
    GIOStatus status;
	gchar *error_msg = NULL;
	GtkWidget *dialog;

    if (source == NULL) {
        return FALSE;
    }

    mplayer_output = g_string_new("");

    status = g_io_channel_read_line_string(source, mplayer_output, NULL, NULL);
	if (verbose && strstr(mplayer_output->str,"ANS_") == NULL)
		printf("%s",mplayer_output->str);

	if (strstr(mplayer_output->str, "Couldn't open DVD device") != 0) {
		error_msg = g_strdup(mplayer_output->str);
    }

	if (strstr(mplayer_output->str, "Failed to open") != 0) {
		if (strstr(mplayer_output->str, "LIRC") == 0)
			error_msg = g_strdup(mplayer_output->str);
    }
	
	if (error_msg != NULL) {
       dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_CLOSE, error_msg);
	   gtk_window_set_title(GTK_WINDOW(dialog),"GNOME MPlayer Error");
       gtk_dialog_run (GTK_DIALOG (dialog));
       gtk_widget_destroy (dialog);	
	   g_free(error_msg);
	}		
	
    if (status != G_IO_STATUS_NORMAL) {
        g_string_free(mplayer_output, TRUE);
        return FALSE;
    }
	
	while(gtk_events_pending()) gtk_main_iteration();

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
        return FALSE;
    }

    mplayer_output = g_string_new("");

    // printf("thread_reader state = %i\n",state);

    if (state == QUIT) {
		g_string_free(mplayer_output, TRUE);
        g_idle_add(set_stop,idledata);
		state = QUIT;
		g_mutex_unlock(thread_running);
        return FALSE;
    }

	status = g_io_channel_read_line_string(source, mplayer_output, NULL, &error);
	if (status != G_IO_STATUS_NORMAL) {
		if (error != NULL){
			//printf("%i: %s\n",error->code,error->message);
			if (error->code == G_CONVERT_ERROR_ILLEGAL_SEQUENCE) {
				g_error_free(error);
				error = NULL;
				g_io_channel_set_encoding(source,NULL,NULL);
			} else {
				g_string_free(mplayer_output, TRUE);
				g_idle_add(set_stop,idledata);
				state = QUIT;
				g_mutex_unlock(thread_running);
				g_error_free(error);
				error = NULL;
				return FALSE;
			}
		} else {
			g_string_free(mplayer_output, TRUE);
			g_idle_add(set_stop,idledata);
			state = QUIT;
			g_mutex_unlock(thread_running);
			error = NULL;
			return FALSE;
		}
	}
	
	if (verbose && strstr(mplayer_output->str,"ANS_") == NULL)
		printf("%s",mplayer_output->str);

	
    if (strstr(mplayer_output->str, "(Quit)") != NULL) {
        state = QUIT;
        g_idle_add(set_stop,idledata);
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
		g_idle_add(resize_window,idledata);
        videopresent = 1;
		g_idle_add(set_volume_from_slider,NULL);
    }

    if (strstr(mplayer_output->str, "Video: no video") != NULL) {
        actual_x = 150;
        actual_y = 1;

        // printf("Resizing to %i x %i \n", actual_x, actual_y);
		idledata->width = actual_x;
		idledata->height = actual_y;
		idledata->videopresent = 0;
		g_idle_add(resize_window,idledata);
		g_idle_add(set_volume_from_slider,NULL);
		
    }

    if (strstr(mplayer_output->str, "ANS_PERCENT_POSITION") != 0) {
        buf = strstr(mplayer_output->str, "ANS_PERCENT_POSITION");
        sscanf(buf, "ANS_PERCENT_POSITION=%i", &pos);
        //printf("Position = %i\n",pos);
		idledata->percent = pos / 100.0;
		g_idle_add(set_progress_value,idledata);
    }

    if (strstr(mplayer_output->str, "ANS_LENGTH") != 0) {
        buf = strstr(mplayer_output->str, "ANS_LENGTH");
        sscanf(buf, "ANS_LENGTH=%lf", &idledata->length);
		g_idle_add(set_progress_time,idledata);
    }
	
    if (strstr(mplayer_output->str, "ANS_TIME_POSITION") != 0) {
        buf = strstr(mplayer_output->str, "ANS_TIME_POSITION");
        sscanf(buf, "ANS_TIME_POSITION=%lf", &idledata->position);
		g_idle_add(set_progress_time,idledata);
    }
	
    if (strstr(mplayer_output->str, "ANS_volume") != 0) {
        buf = strstr(mplayer_output->str, "ANS_volume");
        sscanf(buf, "ANS_volume=%i", &volume);
		idledata->volume = volume;
        buf = g_strdup_printf(_("Volume %i%%"), volume);
		g_strlcpy(idledata->vol_tooltip,buf,128);
		g_idle_add(set_volume_tip,idledata);
        g_free(buf);
    }

    if (strstr(mplayer_output->str, "Cache fill") != 0) {
        buf = strstr(mplayer_output->str, "Cache fill");
        sscanf(buf, "Cache fill: %f%%", &percent);
        //printf("Percent = %f\n",percent);
        //printf("Buffer = %s\n",buf);
        buf = g_strdup_printf("Cache fill: %2.2f%%", percent);
		g_strlcpy(idledata->progress_text,buf,1024);
		g_idle_add(set_progress_text,idledata);
		idledata->percent = percent / 100.0;
		g_idle_add(set_progress_value,idledata);
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
			g_strlcpy(idledata->info,message,1024);
			g_idle_add(set_media_info,idledata);
    }
	while(gtk_events_pending()) gtk_main_iteration();

	if (error_msg != NULL) {
       dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK, error_msg);
       gtk_dialog_run (GTK_DIALOG (dialog));
       gtk_widget_destroy (dialog);	
	   g_free(error_msg);
	}		
		
    g_string_free(mplayer_output, TRUE);
    return TRUE;

}

gboolean thread_query(gpointer data)
{
    int size;

    //printf("thread_query state = %i\n",state);

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
            shutdown();
            return FALSE;
        } else {
            return TRUE;
        }
    }
}

gpointer launch_player(gpointer data) {
	
	gint ok;
    gint player_window;
    char *argv[255];
    gint arg = 0;

	ThreadData *threaddata = (ThreadData*) data;
	

    fullscreen = 0;
    videopresent = 1;
	
	g_mutex_lock(thread_running);
	
	g_strlcpy(idledata->info,threaddata->filename,1024);
	idledata->percent = 0.0;
	g_strlcpy(idledata->progress_text,"",1024);
	idledata->width = 1;
	idledata->height = 1;
	idledata->videopresent = 1;
	idledata->volume = 100.0;
	idledata->length = 0.0;
	
	g_idle_add(set_progress_value,idledata);
	g_idle_add(set_progress_text,idledata);
	g_idle_add(set_media_info,idledata);
	g_idle_add(set_window_visible,idledata);
	
    while (gtk_events_pending())
        gtk_main_iteration();

    argv[arg++] = g_strdup_printf("mplayer");
	if (vo != NULL) {
		argv[arg++] = g_strdup_printf("-profile");
		argv[arg++] = g_strdup_printf("gnome-mplayer");
	}
    // argv[arg++] = g_strdup_printf("-zoom");
    argv[arg++] = g_strdup_printf("-quiet");
    argv[arg++] = g_strdup_printf("-slave");
    argv[arg++] = g_strdup_printf("-softvol");
    argv[arg++] = g_strdup_printf("-noconsolecontrols");
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
    if (playlist || threaddata->playlist)
        argv[arg++] = g_strdup_printf("-playlist");
    argv[arg] = g_strdup_printf("%s", threaddata->filename);
    argv[arg + 1] = NULL;
    ok = g_spawn_async_with_pipes(NULL, argv, NULL,
                                  G_SPAWN_SEARCH_PATH,
                                  NULL, NULL, NULL, &std_in, &std_out, &std_err, NULL);

    if (ok) {
		printf("Spawn succeeded for filename %s\n",threaddata->filename);		
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
        g_io_add_watch(channel_in, G_IO_IN | G_IO_HUP, thread_reader, NULL);
        g_io_add_watch(channel_err, G_IO_IN | G_IO_ERR | G_IO_HUP, thread_reader_error, NULL);

        g_idle_add(set_play,NULL);
        g_timeout_add(500, thread_query, NULL);

    } else {
        state = QUIT;
		printf("Spawn failed for filename %s\n",threaddata->filename);
    }

	g_mutex_lock(thread_running);
	printf("Thread completing\n");
	idledata->percent = 1.0;
	idledata->position = idledata->length;
	g_idle_add(set_progress_value,idledata);
	g_idle_add(set_progress_time,idledata);

	if (embed_window != 0)
		dbus_open_next();
	
	g_free(threaddata);
	threaddata = NULL;

	if (channel_in != NULL) {
		g_io_channel_unref(channel_in);
		channel_in = NULL;
	}
	
	if (channel_out != NULL) {
		g_io_channel_unref(channel_out);
		channel_out = NULL;
	}

	if (channel_err != NULL) {
		g_io_channel_unref(channel_err);
		channel_err = NULL;
	}
	
	arg = 0;
	while(argv[arg] != NULL){
		g_free(argv[arg]);
		argv[arg] = NULL;
		arg++;
	}
	g_mutex_unlock(thread_running);
	return NULL;
}
