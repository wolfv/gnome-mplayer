/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * callbacks.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * callbacks.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with callbacks.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>

#include "gui.h"
#include "support.h"
#include "common.h"

gint get_player_window() {
	
	if (GTK_IS_WIDGET(drawing_area)) {
		return gtk_socket_get_id(GTK_SOCKET(drawing_area));
	} else {
		return 0;
	}
}

gboolean hide_buttons(void *data) {

	ThreadData *td = (ThreadData*)data;

	
	if (GTK_IS_WIDGET(ff_event_box)) {
		if (td->streaming) {
			gtk_widget_hide(ff_event_box);
			gtk_widget_hide(rew_event_box);
		} else {
			if (embed_window == 0 || window_x > 250) {
				gtk_widget_show(ff_event_box);
				gtk_widget_show(rew_event_box);
			}
		}
	}
	return FALSE;
}

gboolean set_media_info(void *data) {

	IdleData *idle = (IdleData*)data;
	
	if (GTK_IS_WIDGET(song_title)) {
		gtk_entry_set_text(GTK_ENTRY(song_title), idle->info);
	}
	return FALSE;
}

gboolean set_progress_value(void *data) {
	
	IdleData *idle = (IdleData*)data;
	
	
	if (GTK_IS_WIDGET(progress)) {
        gtk_progress_bar_update(progress, idle->percent);
	}
	return FALSE;
}	

gboolean set_progress_text(void *data) {
	
	IdleData *idle = (IdleData*)data;
	
	
	if (GTK_IS_WIDGET(progress)) {
		gtk_progress_bar_set_text(progress, idle->progress_text);
	}
	return FALSE;
}	

gboolean set_progress_time(void *data) {
    int hour = 0, min = 0, length_hour = 0, length_min = 0;
    long int seconds, length_seconds;
	
	IdleData *idle = (IdleData*)data;

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
		length_seconds =
		length_seconds - (length_hour * 3600);
	}
	if (length_seconds >= 60) {
		length_min = length_seconds / 60;
		length_seconds =
		length_seconds - (length_min * 60);
	}

	if (hour == 0 && length_hour == 0) {
		
		if ((int)idle->length == 0) {
			if (idle->cachepercent > 0 && idle->cachepercent < 1.0) {
				g_snprintf(idle->progress_text, 128,
					 _("%2i:%02i | %2i%% \342\226\274"),
					 min, (int) seconds, 
					 (int) (idle->cachepercent * 100));
			} else {
				g_snprintf(idle->progress_text, 128,
					 _("%2i:%02i"),
					 min, (int) seconds);
			}
		} else {
			if (idle->cachepercent > 0 && idle->cachepercent < 1.0) {
				g_snprintf(idle->progress_text, 128,
					 _("%2i:%02i / %2i:%02i | %2i%% \342\226\274"),
					 min, (int) seconds, length_min,
					 (int) length_seconds,
					 (int) (idle->cachepercent * 100));
			} else {
				g_snprintf(idle->progress_text, 128,
					 _("%2i:%02i / %2i:%02i"),
					 min, (int) seconds, length_min,
					 (int) length_seconds);
			}
		}
	} else {
		if ((int)idle->length == 0) {

			if (idle->cachepercent > 0 && idle->cachepercent < 1.0) {
				g_snprintf(idle->progress_text, 128,
					 _("%i:%02i:%02i | %2i%% \342\226\274"),
					 hour, min, (int) seconds,
					 (int) (idle->cachepercent * 100));
			} else {
				g_snprintf(idle->progress_text, 128,
					 _("%i:%02i:%02i"),
					 hour, min, (int) seconds);
			}
			
		} else {

			if (idle->cachepercent > 0 && idle->cachepercent < 1.0) {
				g_snprintf(idle->progress_text, 128,
					 _("%i:%02i:%02i / %i:%02i:%02i | %2i%% \342\226\274"),
					 hour, min, (int) seconds,
					 length_hour, length_min,
					 (int) length_seconds,
					 (int) (idle->cachepercent * 100));
			} else {
				g_snprintf(idle->progress_text, 128,
					 _("%i:%02i:%02i / %i:%02i:%02i"),
					 hour, min, (int) seconds,
					 length_hour, length_min,
					 (int) length_seconds);
			}
			
		}
	}
	
	if (GTK_IS_WIDGET(progress) && idle->position > 0) {
		gtk_progress_bar_set_text(progress, idle->progress_text);
	}
	return FALSE;
}	

gboolean set_volume_from_slider(gpointer data)
{
    gint vol;
    gchar *cmd;

    vol = (gint) gtk_range_get_value(GTK_RANGE(vol_slider));
    cmd = g_strdup_printf("volume %i 1\n", vol);
	send_command(cmd);
    g_free(cmd);
	if (state == PAUSED || state == STOPPED) {
		send_command("pause\n");
	}

	send_command("get_property volume\n");
	
	return FALSE;
}

gboolean set_volume_tip(void *data) {
	
	IdleData *idle = (IdleData*)data;
	
	if (GTK_IS_WIDGET(vol_slider)) {
		gtk_tooltips_set_tip(volume_tip, vol_slider, idle->vol_tooltip, NULL);
	} 
	return FALSE;
}	

gboolean resize_window(void *data) {
	
	IdleData *idle = (IdleData*)data;
	gint width,height;
	gint total_height =0;
	
	if (GTK_IS_WIDGET(window)) {
		if (window_x == 0 && window_y == 0) {
			gtk_widget_set_size_request(fixed, -1, -1);
			//printf("%i x %i \n",idle->x,idle->y);
			if (idle->width > 0 && idle->height > 0) {
				gtk_widget_set_size_request(fixed, idle->width, idle->height);
				total_height = idle->height;
				gdk_drawable_get_size(GDK_DRAWABLE(hbox->window), &width,&height);
				total_height += height;
				//printf("window: %i x %i \n",idle->x,total_height);
				
				gtk_window_resize(GTK_WINDOW(window),idle->width,total_height);
			}
		} else {
			if (window_x > 0 && window_y > 0)
				gtk_window_resize(GTK_WINDOW(window), window_x, window_y);
		}
		if ((idle->videopresent == 0)) {
			gtk_widget_show(GTK_WIDGET(song_title));
			gtk_widget_set_size_request(drawing_area, -1, -1);
			gtk_window_resize(GTK_WINDOW(window), 1, 1);
		} 
		
		gtk_widget_set_sensitive(GTK_WIDGET(menuitem_fullscreen),idle->videopresent);
		gtk_widget_set_sensitive(GTK_WIDGET(fs_event_box),idle->videopresent);
		
	}
	return FALSE;
}	

gboolean set_play(void *data) {
	
	play_callback(NULL, NULL, NULL); // ok is just not NULL which is what we want
	return FALSE;
}

gboolean set_pause(void *data) {

	pause_callback(NULL, NULL, NULL); // ok is just not NULL which is what we want
	return FALSE;
}

gboolean set_stop(void *data) {

	stop_callback(NULL, NULL, NULL); // ok is just not NULL which is what we want
	return FALSE;
}

gboolean set_ff(void *data) {

	ff_callback(NULL, NULL, NULL); // ok is just not NULL which is what we want
	return FALSE;
}

gboolean set_rew(void *data) {

	rew_callback(NULL, NULL, NULL); // ok is just not NULL which is what we want
	return FALSE;
}

gboolean set_position(void *data) {
    gchar *cmd;
	IdleData *idle = (IdleData*)data;

    cmd = g_strdup_printf("seek %5.0f 2\n", idle->position);
	send_command(cmd);
    g_free(cmd);
	return FALSE;
}



gboolean set_volume(void *data) {
	IdleData *idle = (IdleData*)data;
	gchar *buf;
	
	if (GTK_IS_WIDGET(vol_slider)) {
		printf("setting slider to %f\n",idle->volume);
		gtk_range_set_value(GTK_RANGE(vol_slider),idle->volume);
        buf = g_strdup_printf(_("Volume %i%%"), (gint)idle->volume);
		g_strlcpy(idledata->vol_tooltip,buf,128);
		gtk_tooltips_set_tip(volume_tip, vol_slider, idle->vol_tooltip, NULL);
		g_free(buf);
	} 
	return FALSE;
}	

gboolean set_fullscreen(void *data) {

	IdleData *idle = (IdleData*)data;
	
	// we need to flip the state since the callback reads the value of fullscreen
	// and if fullscreen is 0 it sets it to fullscreen.
	// fullscreen = ! (gint) idle->fullscreen;
	// printf("calling fs_callback with %i\n",fullscreen);
	// fs_callback(NULL, NULL, NULL); // ok is just not NULL which is what we want
	if (idle->videopresent)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen), idle->fullscreen);
	return FALSE;
}

gboolean set_show_controls(void *data) {

	IdleData *idle = (IdleData*)data;
	
	showcontrols = (gint) idle->showcontrols;
	
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), showcontrols);
	return FALSE;
}

gboolean popup_handler(GtkWidget * widget, GdkEvent * event, void *data)
{
    GtkMenu *menu;
    GdkEventButton *event_button;

    menu = GTK_MENU(widget);
    if (event->type == GDK_BUTTON_PRESS) {
        event_button = (GdkEventButton *) event;
        if (event_button->button == 3) {
            gtk_menu_popup(menu, NULL, NULL, NULL, NULL, event_button->button, event_button->time);
            return TRUE;
        }
    }
    return FALSE;
}

gboolean delete_callback(GtkWidget * widget, GdkEvent * event, void *data)
{
    shutdown();
    if (lastfile != NULL) {
        g_free(lastfile);
        lastfile = NULL;
    }
	if (idledata != NULL) {
		g_free(idledata);
		idledata = NULL;
	}
	
	if (control_id != 0)
		dbus_cancel();
	
	while(gtk_events_pending()) gtk_main_iteration();
    gtk_main_quit();
    return TRUE;
}

gboolean move_window(void *data) {

	IdleData *idle = (IdleData*)data;
	
	// printf("moving window to %i x %i\n",idle->x,idle->y);
	if (GTK_IS_WIDGET(drawing_area) && (idle->x != idle->last_x || idle->y != idle->last_y)) {
		gtk_fixed_move(GTK_FIXED(fixed),GTK_WIDGET(drawing_area),idle->x, idle->y);
		idle->last_x = idle->x;
		idle->last_y = idle->y;
	}
	return FALSE;
}

gboolean expose_fixed_callback(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	GdkGC *gc;
	gdouble movie_ratio, window_ratio;
	gint new_width, new_height;
	
	if (GDK_IS_DRAWABLE(fixed->window)) {
		gc = gdk_gc_new(fixed->window);
		// printf("drawing box %i x %i at %i x %i \n",event->area.width,event->area.height, event->area.x, event->area.y );
		gdk_draw_rectangle(fixed->window, gc, TRUE, event->area.x, event->area.y, event->area.width,event->area.height );
		gdk_gc_unref(gc);
	}

	// calculate the new size of the drawing area
	
	if (actual_x > 0 && actual_y > 0) {
	
		movie_ratio = (gdouble)actual_x / (gdouble)actual_y;
		window_ratio = (gdouble)event->area.width / (gdouble)event->area.height;
		
		if (event->area.width == idledata->width && event->area.height == idledata->width) {
			new_width = event->area.width;
			new_height = event->area.height;
		} else {
			if (movie_ratio > window_ratio) {
				//printf("movie %lf > window %lf\n",movie_ratio,window_ratio);
				new_width = event->area.width;
				new_height = event->area.width / movie_ratio;
			} else {
				//printf("movie %lf < window %lf\n",movie_ratio,window_ratio);
				new_height = event->area.height;
				new_width = event->area.height * movie_ratio;
			}
		}
		//printf("new movie size = %i x %i (%i x %i)\n",new_width,new_height,event->area.width, event->area.height);
		gtk_widget_set_usize(drawing_area, new_width, new_height);
		
		idledata->x = (event->area.width - new_width) / 2;
		idledata->y = (event->area.height - new_height) / 2;
		g_idle_add(move_window,idledata);
	}	
	
	return FALSE;
}


gboolean play_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    gtk_container_remove(GTK_CONTAINER(play_event_box), image_play);
    gtk_container_remove(GTK_CONTAINER(pause_event_box), image_pause);
    gtk_container_remove(GTK_CONTAINER(stop_event_box), image_stop);
    image_play = gtk_image_new_from_pixbuf(pb_play);
    image_pause = gtk_image_new_from_pixbuf(pb_pause);
    image_stop = gtk_image_new_from_pixbuf(pb_stop);
    gtk_container_add(GTK_CONTAINER(play_event_box), image_play);
    gtk_container_add(GTK_CONTAINER(pause_event_box), image_pause);
    gtk_container_add(GTK_CONTAINER(stop_event_box), image_stop);
    gtk_widget_show(image_play);
    gtk_widget_show(image_pause);
    gtk_widget_show(image_stop);

    gtk_widget_show(play_event_box);
    gtk_widget_show(pause_event_box);
    gtk_widget_show(stop_event_box);
    if (data == NULL) {
        if (state == PAUSED || state == STOPPED) {
            send_command("pause\n");
            state = PLAYING;
        }
        if (state == QUIT) {
			if (lastfile != NULL) {
				play_file(lastfile, 0);
			}
        }
		if (state == PLAYING) {
			g_strlcpy(idledata->progress_text,_("Playing"),1024);
			g_idle_add(set_progress_text,idledata);
		}
    }
    return FALSE;
}

gboolean pause_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    gtk_container_remove(GTK_CONTAINER(play_event_box), image_play);
    gtk_container_remove(GTK_CONTAINER(pause_event_box), image_pause);
    gtk_container_remove(GTK_CONTAINER(stop_event_box), image_stop);
    image_play = gtk_image_new_from_pixbuf(pb_play);
    image_pause = gtk_image_new_from_pixbuf(pb_pause);
    image_stop = gtk_image_new_from_pixbuf(pb_stop);
    gtk_container_add(GTK_CONTAINER(play_event_box), image_play);
    gtk_container_add(GTK_CONTAINER(pause_event_box), image_pause);
    gtk_container_add(GTK_CONTAINER(stop_event_box), image_stop);
    gtk_widget_show(image_play);
    gtk_widget_show(image_pause);
    gtk_widget_show(image_stop);

    gtk_widget_show(play_event_box);
    gtk_widget_show(pause_event_box);
    gtk_widget_show(stop_event_box);
    if (state == PLAYING) {
        send_command("pause\n");
        state = PAUSED;
		g_strlcpy(idledata->progress_text,_("Paused"),1024);
		g_idle_add(set_progress_text,idledata);
    } else if (state == PAUSED) {
        send_command("pause\n");
        state = PLAYING;
		g_strlcpy(idledata->progress_text,_("Playing"),1024);
		g_idle_add(set_progress_text,idledata);
    }
    return FALSE;
}

gboolean stop_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    gtk_container_remove(GTK_CONTAINER(play_event_box), image_play);
    gtk_container_remove(GTK_CONTAINER(pause_event_box), image_pause);
    gtk_container_remove(GTK_CONTAINER(stop_event_box), image_stop);
    image_play = gtk_image_new_from_pixbuf(pb_play);
    image_pause = gtk_image_new_from_pixbuf(pb_pause);
    image_stop = gtk_image_new_from_pixbuf(pb_stop);
    gtk_container_add(GTK_CONTAINER(play_event_box), image_play);
    gtk_container_add(GTK_CONTAINER(pause_event_box), image_pause);
    gtk_container_add(GTK_CONTAINER(stop_event_box), image_stop);
    gtk_widget_show(image_play);
    gtk_widget_show(image_pause);
    gtk_widget_show(image_stop);

    gtk_widget_show(play_event_box);
    gtk_widget_show(pause_event_box);
    gtk_widget_show(stop_event_box);
    if (data == NULL) {
		if (state == PAUSED) {
			send_command("pause\n");
			state = PLAYING;
		}
		if (state == PLAYING) {
			send_command("seek 0 2\npause\n");
			state = STOPPED;
		}
		g_strlcpy(idledata->progress_text,_("Stopped"),1024);
		g_idle_add(set_progress_text,idledata);
		
    }
    return FALSE;

}

gboolean hookup_x11_events(void *data)
{
	// This hookup is needed because as a window is resized and mplayer is running in x11 mode
	// The areas that should be black/empty are filled with left over frame data. So this callback
	// draws over/erases them.
	
	//printf("hooking callbacks\n");
/*    g_signal_connect(G_OBJECT(drawing_area), "expose_event", G_CALLBACK(expose_callback), NULL);
    g_signal_connect(G_OBJECT(drawing_area), "size_allocate", G_CALLBACK(allocate_callback), NULL);
    g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(expose_callback), NULL);
    g_signal_connect(G_OBJECT(window), "window_state_event", G_CALLBACK(window_state_callback), NULL);
	*/
	return FALSE;
}

gboolean ff_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    if (state == PLAYING) {
		send_command("seek +10 0\n");
    }
    return FALSE;
}

gboolean rew_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
    if (state == PLAYING) {
		send_command("seek -10 0\n");
    }
    return FALSE;
}

void vol_slider_callback(GtkRange * range, gpointer user_data)
{
    gint vol;
    gchar *cmd;
	gchar *buf;

    vol = (gint) gtk_range_get_value(range);
    cmd = g_strdup_printf("volume %i 1\n", vol);
	send_command(cmd);
    g_free(cmd);
	if (idledata->volume != vol) { 
		
        buf = g_strdup_printf(_("Volume %i%%"), vol);
		g_strlcpy(idledata->vol_tooltip,buf,128);
		g_idle_add(set_volume_tip,idledata);
        g_free(buf);
	}
	send_command("get_property volume\n");
	
}


gboolean fs_callback(GtkWidget * widget, GdkEventExpose * event, void *data)
{
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen), !fullscreen);
	
	return FALSE;
}


void menuitem_open_callback(GtkMenuItem * menuitem, void *data)
{

	GtkWidget *dialog;
	gchar *filename;

    dialog = gtk_file_chooser_dialog_new (_("Open File"),
				      GTK_WINDOW(window),
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);
					  
	if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		shutdown();
		play_file((gchar *) filename, 0);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
	
}

void menuitem_quit_callback(GtkMenuItem * menuitem, void *data)
{
	delete_callback(NULL,NULL,NULL);
}

void menuitem_about_callback(GtkMenuItem * menuitem, void *data)
{
	gchar *authors[] = { "Kevin DeKorte", NULL };
    gtk_show_about_dialog(GTK_WINDOW(window),"name",_("GNOME MPlayer"),
						  "logo",pb_icon,
						  "authors",authors,
						  "copyright","Copyright © 2007 Kevin DeKorte",
						  "comments", _("A media player for GNOME that uses MPlayer"),
						  "version", VERSION,
						  NULL);
}

void menuitem_play_callback(GtkMenuItem * menuitem, void *data)
{
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

void menuitem_fs_callback(GtkMenuItem * menuitem, void *data)
{
 
	gint width = 0,height = 0;
	GdkGC *gc;
	
	//printf("doing fullscreen callback\n");
    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_fullscreen))) {
		gtk_window_unfullscreen(GTK_WINDOW(window));

		if (embed_window != 0) {
			gdk_window_reparent(window->window,window_container,0,0);
			gtk_widget_map(window);
		} else {
			gtk_widget_show(menubar);
		}
        fullscreen = 0;
		if (GDK_IS_DRAWABLE(window_container))
			gdk_drawable_get_size(GDK_DRAWABLE(window_container),&width,&height);
		if (width > 0 && height > 0)
			gtk_window_resize(GTK_WINDOW(window),width,height);
    } else {
		if (embed_window != 0) {
			gdk_window_reparent(window->window,NULL,0,0);
			gtk_widget_map(window);
		} else {
			gtk_widget_hide(menubar);
		}
		gtk_window_fullscreen(GTK_WINDOW(window));
        fullscreen = 1;
    }
	while(gtk_events_pending()) gtk_main_iteration();
	if (GDK_IS_DRAWABLE(drawing_area->window)) {
		gc = gdk_gc_new(drawing_area->window);
		gdk_drawable_get_size(GDK_DRAWABLE(drawing_area->window),&width,&height);
		//printf("drawing box %i x %i\n",width,height);
		if (width > 0 && height > 0)
			gdk_draw_rectangle(drawing_area->window, gc, TRUE, 0, 0, width,height );
		gdk_gc_unref(gc);
	}
	while(gtk_events_pending()) gtk_main_iteration();
	
}

void menuitem_showcontrols_callback(GtkCheckMenuItem * menuitem, void *data)
{
    if (gtk_check_menu_item_get_active(menuitem)) {
		if (GTK_IS_WIDGET(button_event_box)) {
			gtk_widget_hide_all(button_event_box);
		}
        gtk_widget_show(controls_box);
	} else {
        //gtk_widget_hide_all(hbox);
        //gtk_widget_hide(song_title);
		gtk_widget_hide(controls_box);
    }
}

void config_apply(GtkWidget * widget, void *data){
	GConfClient *gconf;
	
	if (vo != NULL){
		g_free(vo);
		vo = NULL;
	}
	vo = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(config_vo)->child)));
	
	if (ao != NULL){
		g_free(ao);
		ao = NULL;
	}
	ao = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(config_ao)->child)));
	
	update_mplayer_config();
	
	cache_size =	(int) gtk_range_get_value(GTK_RANGE(config_cachesize));
	gconf = gconf_client_get_default();
	gconf_client_set_int(gconf,CACHE_SIZE,cache_size,NULL);
	g_object_unref(G_OBJECT(gconf));
	
	gtk_widget_destroy(widget);
}

void config_close(GtkWidget * widget, void *data){
	gtk_widget_destroy(widget);
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
	
	read_mplayer_config();
	
	config_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable(GTK_WINDOW(config_window), FALSE);
    conf_vbox = gtk_vbox_new(FALSE, 10);
    conf_hbutton_box = gtk_hbutton_box_new();
    conf_table = gtk_table_new(20, 2, FALSE);

    gtk_container_add(GTK_CONTAINER(conf_vbox), conf_table);
    gtk_container_add(GTK_CONTAINER(conf_vbox), conf_hbutton_box);
    gtk_container_add(GTK_CONTAINER(config_window), conf_vbox);
	
	gtk_window_set_title(GTK_WINDOW(config_window), _("GNOME MPlayer Configuration"));
    gtk_container_set_border_width(GTK_CONTAINER(config_window), 5);
	gtk_window_set_default_size(GTK_WINDOW(config_window),300,300);
    conf_ok = gtk_button_new_with_label(_("OK"));
    g_signal_connect_swapped(GTK_OBJECT(conf_ok), "clicked",
		     GTK_SIGNAL_FUNC(config_apply), config_window);

    conf_cancel = gtk_button_new_with_label(_("Cancel"));
    g_signal_connect_swapped(GTK_OBJECT(conf_cancel), "clicked",
		     GTK_SIGNAL_FUNC(config_close),config_window);
	
    config_vo = gtk_combo_box_entry_new_text();
    if (config_vo != NULL) {
	gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "gl");
	gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "x11");
	gtk_combo_box_append_text(GTK_COMBO_BOX(config_vo), "xv");
	if (vo != NULL) {
	    if (strcmp(vo, "gl") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX
					 (config_vo), 0);
	    if (strcmp(vo, "x11") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX
					 (config_vo), 1);
	    if (strcmp(vo, "xv") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX
					 (config_vo), 2);
	    if (gtk_combo_box_get_active(GTK_COMBO_BOX(config_vo))
		== -1) {
		gtk_combo_box_append_text(GTK_COMBO_BOX
					  (config_vo),
					  vo);
		gtk_combo_box_set_active(GTK_COMBO_BOX
					 (config_vo), 3);
	    }
	}
    }

    config_ao = gtk_combo_box_entry_new_text();
    if (config_ao != NULL) {
	gtk_combo_box_append_text(GTK_COMBO_BOX(config_ao),
				  "alsa");
	gtk_combo_box_append_text(GTK_COMBO_BOX(config_ao),
				  "arts");
	gtk_combo_box_append_text(GTK_COMBO_BOX(config_ao), "esd");
	gtk_combo_box_append_text(GTK_COMBO_BOX(config_ao),
				  "jack");
	gtk_combo_box_append_text(GTK_COMBO_BOX(config_ao), "oss");
	if (ao != NULL) {
	    if (strcmp(ao, "alsa") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX
					 (config_ao), 0);
	    if (strcmp(ao, "arts") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX
					 (config_ao), 1);
	    if (strcmp(ao, "esd") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX
					 (config_ao), 2);
	    if (strcmp(ao, "jack") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX
					 (config_ao), 3);
	    if (strcmp(ao, "oss") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX
					 (config_ao), 4);
	    if (gtk_combo_box_get_active(GTK_COMBO_BOX(config_ao))
		== -1) {
		gtk_combo_box_append_text(GTK_COMBO_BOX
					  (config_ao),
					  ao);
		gtk_combo_box_set_active(GTK_COMBO_BOX
					 (config_ao), 4);
	    }
	}
    }

    conf_label = gtk_label_new(_("Video Output:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1,
			      0, 1);
    gtk_widget_show(conf_label);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_vo,
			      1, 2, 0, 1);

    conf_label = gtk_label_new(_("Audio Output:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1,
			      1, 2);
    gtk_widget_show(conf_label);
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), config_ao,
			      1, 2, 1, 2);
	
    conf_label = gtk_label_new(_("Minimum Cache Size:"));
    gtk_misc_set_alignment(GTK_MISC(conf_label), 0.0, 0.0);
    gtk_table_attach_defaults(GTK_TABLE(conf_table), conf_label, 0, 1,
			      2, 3);
    gtk_widget_show(conf_label);
    config_cachesize = gtk_hscale_new_with_range(0, 32767, 512);
    gtk_table_attach_defaults(GTK_TABLE(conf_table),
			      config_cachesize, 1, 2, 2, 3);
    gtk_range_set_value(GTK_RANGE(config_cachesize),
			cache_size);
    gtk_widget_show(config_cachesize);
	
	
	gtk_container_add(GTK_CONTAINER(conf_hbutton_box), conf_ok);
    gtk_container_add(GTK_CONTAINER(conf_hbutton_box), conf_cancel);

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
            gtk_menu_popup(popup_menu, NULL, NULL, NULL, NULL, event_button->button, event_button->time);
            return TRUE;
        } else {
			gdk_drawable_get_size(GDK_DRAWABLE(widget->window),&width,&height);

			percent = event->x / width;
			
			cmd = g_strdup_printf("seek %i 1\n",(gint)(percent * 100));
			send_command(cmd);
			g_free(cmd);
			
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

		if (event_button->button == 1) {
			dbus_open_by_hrefid(hrefid);
			// do this in the plugin when we should
			// gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), TRUE);
			return TRUE;
		}
	}
	
    return FALSE;

}

void make_button(gchar *src, gchar *hrefid) {
	
	GError *error;
    gchar *dirname = NULL;
    gchar *filename = NULL;
    gint exit_status;
    gchar *command;
	
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), FALSE);
	
	error = NULL;
	pb_button = gdk_pixbuf_new_from_file(src, &error);
	if (error != NULL) {
		g_error_free(error);
		error = NULL;
		
		dirname = g_strdup_printf("%s", tempnam("/tmp", "gnome-mplayerXXXXXX"));
		filename = g_strdup_printf("%s/00000001.jpg", dirname);

		// run mplayer and try to get the first frame and convert it to a jpeg
		command = g_strdup_printf("mplayer -vo jpeg:outdir=%s -frames 1 %s", dirname, src);
		if (!g_spawn_command_line_sync(command, NULL, NULL, &exit_status, &error))
			printf("Error when running When running command: %s\n%s\n", command, error->message);

		g_free(command);
		
		if (g_file_test(filename,G_FILE_TEST_EXISTS)) {
			pb_button = gdk_pixbuf_new_from_file(filename, &error);
			if (error != NULL) {
				g_error_free(error);
				error = NULL;			
			}
		}
	}

	if (GDK_IS_PIXBUF(pb_button)){
		
		button_event_box = gtk_event_box_new();
		image_button = gtk_image_new_from_pixbuf(pb_button);
		gtk_container_add(GTK_CONTAINER(button_event_box),image_button);
		gtk_box_pack_start(GTK_BOX(vbox), button_event_box, FALSE, FALSE, 0);
		
		g_signal_connect(G_OBJECT(button_event_box),"button_press_event",G_CALLBACK(load_href_callback),hrefid);
		gtk_widget_show_all(button_event_box);
		
	};
	
	
    if (filename != NULL) {
		g_remove(filename);
		g_free(filename);
    }

    if (dirname != NULL) {
		g_remove(dirname);
		g_free(dirname);
    }	
}


GtkWidget *create_window(gint windowid)
{
	GError *error = NULL;
	GtkIconTheme *icon_theme;
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), _("GNOME MPlayer"));
	
	if (windowid != 0) {
		gtk_window_set_decorated(GTK_WINDOW(window),FALSE);
	}

	gtk_window_set_policy(GTK_WINDOW(window),TRUE,TRUE,TRUE);

    gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(window, GDK_BUTTON_RELEASE_MASK);
    gtk_widget_add_events(window, GDK_ENTER_NOTIFY_MASK);
    gtk_widget_add_events(window, GDK_LEAVE_NOTIFY_MASK);
    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    gtk_widget_add_events(window, GDK_VISIBILITY_NOTIFY_MASK);
    gtk_widget_add_events(window, GDK_STRUCTURE_MASK);

    delete_signal_id =
        g_signal_connect(GTK_OBJECT(window), "delete_event", G_CALLBACK(delete_callback), NULL);

    popup_menu = GTK_MENU(gtk_menu_new());
	menubar = gtk_menu_bar_new();
    menuitem_open = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL));
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
    menuitem_showcontrols = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("S_how Controls")));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_showcontrols));
    gtk_widget_show(GTK_WIDGET(menuitem_showcontrols));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), TRUE);
    menuitem_fullscreen = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(_("_Full Screen")));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_fullscreen));
    gtk_widget_show(GTK_WIDGET(menuitem_fullscreen));
    menuitem_sep2 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_sep2));
    gtk_widget_show(GTK_WIDGET(menuitem_sep2));
    menuitem_config = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, NULL));
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_config));
    gtk_widget_show(GTK_WIDGET(menuitem_config));
    menuitem_sep3 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(popup_menu, GTK_WIDGET(menuitem_sep3));
    gtk_widget_show(GTK_WIDGET(menuitem_sep3));
    menuitem_quit = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL));
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
    g_signal_connect(GTK_OBJECT(menuitem_config), "activate",
                     G_CALLBACK(menuitem_config_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_quit), "activate",
                     G_CALLBACK(menuitem_quit_callback), NULL);

    g_signal_connect_swapped(G_OBJECT(window),
                             "button_press_event",
                             G_CALLBACK(popup_handler), GTK_OBJECT(popup_menu));


    menuitem_file = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_File")));
	menu_file = GTK_MENU(gtk_menu_new());
	gtk_widget_show(GTK_WIDGET(menuitem_file));
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar),GTK_WIDGET(menuitem_file));
	gtk_menu_item_set_submenu(menuitem_file,GTK_WIDGET(menu_file));
    menuitem_file_open = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL));
	gtk_menu_append(menu_file,GTK_WIDGET(menuitem_file_open));
    menuitem_file_sep1 = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_append(menu_file, GTK_WIDGET(menuitem_file_sep1));
    menuitem_file_quit = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL));
	gtk_menu_append(menu_file,GTK_WIDGET(menuitem_file_quit));
    g_signal_connect(GTK_OBJECT(menuitem_file_open), "activate",
                     G_CALLBACK(menuitem_open_callback), NULL);
    g_signal_connect(GTK_OBJECT(menuitem_file_quit), "activate",
                     G_CALLBACK(menuitem_quit_callback), NULL);
					 
					 
    menuitem_edit = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Edit")));
	menu_edit = GTK_MENU(gtk_menu_new());
	gtk_widget_show(GTK_WIDGET(menuitem_edit));
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar),GTK_WIDGET(menuitem_edit));
	gtk_menu_item_set_submenu(menuitem_edit,GTK_WIDGET(menu_edit));
    menuitem_edit_config = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, NULL));
	gtk_menu_append(menu_edit,GTK_WIDGET(menuitem_edit_config));
    g_signal_connect(GTK_OBJECT(menuitem_edit_config), "activate",
                     G_CALLBACK(menuitem_config_callback), NULL);
					 
    menuitem_help = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(_("_Help")));
	menu_help = GTK_MENU(gtk_menu_new());
	gtk_widget_show(GTK_WIDGET(menuitem_help));
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar),GTK_WIDGET(menuitem_help));
	gtk_menu_item_set_submenu(menuitem_help,GTK_WIDGET(menu_help));
    menuitem_help_about = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL));
	gtk_menu_append(menu_help,GTK_WIDGET(menuitem_help_about));
    g_signal_connect(GTK_OBJECT(menuitem_help_about), "activate",
                     G_CALLBACK(menuitem_about_callback), NULL);

	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
	gtk_widget_add_accelerator (GTK_WIDGET(menuitem_fullscreen), "activate",
					    accel_group,'f', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);				   

	gtk_widget_add_accelerator (GTK_WIDGET(menuitem_pause), "activate",
					    accel_group,' ', 0, GTK_ACCEL_VISIBLE);				   
						
    vbox = gtk_vbox_new(FALSE, 0);
    hbox = gtk_hbox_new(FALSE, 0);
	controls_box = gtk_vbox_new(FALSE, 0);
	fixed = gtk_fixed_new();
    drawing_area = gtk_socket_new();
    //gtk_widget_set_size_request(drawing_area, 1, 1);
    song_title = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(song_title), FALSE);
	if (windowid == 0)
		gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
	gtk_fixed_put(GTK_FIXED(fixed),drawing_area,0,0);
    gtk_box_pack_start(GTK_BOX(vbox), fixed, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(controls_box), song_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_box), hbox, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(vbox), controls_box, FALSE, FALSE, 0);

    gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(drawing_area, GDK_BUTTON_RELEASE_MASK);
    // gtk_widget_add_events(drawing_area, GDK_EXPOSURE_MASK);

    g_signal_connect_swapped(G_OBJECT(drawing_area),
                             "button_press_event",
                             G_CALLBACK(popup_handler), GTK_OBJECT(popup_menu));
	
    gtk_widget_add_events(fixed, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(fixed, GDK_BUTTON_RELEASE_MASK);

    g_signal_connect_swapped(G_OBJECT(fixed),
                             "button_press_event",
                             G_CALLBACK(popup_handler), GTK_OBJECT(popup_menu));

    gtk_widget_add_events(song_title, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(song_title, GDK_BUTTON_RELEASE_MASK);

    g_signal_connect_swapped(G_OBJECT(song_title),
                             "button_press_event",
                             G_CALLBACK(popup_handler), GTK_OBJECT(popup_menu));
						 
 	gtk_widget_show(menubar);
    gtk_widget_show(drawing_area);
    gtk_container_add(GTK_CONTAINER(window), vbox);

	error = NULL;
	icon_theme = gtk_icon_theme_get_default ();
	


	pb_icon = gdk_pixbuf_new_from_xpm_data((const char **) gnome_mplayer_xpm);
	
	// ok if the theme has all the icons we need, use them, otherwise use the default GNOME ones
	if (gtk_icon_theme_has_icon(icon_theme, "stock_media-play") 
		&& gtk_icon_theme_has_icon(icon_theme, "stock_media-pause")
		&& gtk_icon_theme_has_icon(icon_theme, "stock_media-stop")
		&& gtk_icon_theme_has_icon(icon_theme, "stock_media-fwd")
		&& gtk_icon_theme_has_icon(icon_theme, "stock_media-rew")
		&& gtk_icon_theme_has_icon(icon_theme, "view-fullscreen")) {
		
		pb_play = gtk_icon_theme_load_icon (icon_theme, "stock_media-play", 16, 0, &error);
		pb_pause = gtk_icon_theme_load_icon (icon_theme, "stock_media-pause", 16, 0, &error);
	    pb_stop = gtk_icon_theme_load_icon (icon_theme, "stock_media-stop", 16, 0, &error);
	    pb_ff = gtk_icon_theme_load_icon (icon_theme, "stock_media-fwd", 16, 0, &error);
	    pb_rew = gtk_icon_theme_load_icon (icon_theme, "stock_media-rew", 16, 0, &error);
		pb_fs = gtk_icon_theme_load_icon (icon_theme, "view-fullscreen", 16, 0, &error);
	
	} else {

		pb_play = gdk_pixbuf_new_from_xpm_data((const char **) media_playback_start_xpm);
		pb_pause = gdk_pixbuf_new_from_xpm_data((const char **) media_playback_pause_xpm);
		pb_stop = gdk_pixbuf_new_from_xpm_data((const char **) media_playback_stop_xpm);
		pb_ff = gdk_pixbuf_new_from_xpm_data((const char **) media_seek_forward_xpm);
		pb_rew = gdk_pixbuf_new_from_xpm_data((const char **) media_seek_backward_xpm);
		pb_fs = gdk_pixbuf_new_from_xpm_data((const char **) view_fullscreen_xpm);
		
	}
	
    image_play = gtk_image_new_from_pixbuf(pb_play);
    image_stop = gtk_image_new_from_pixbuf(pb_stop);
    image_pause = gtk_image_new_from_pixbuf(pb_pause);

    image_ff = gtk_image_new_from_pixbuf(pb_ff);
    image_rew = gtk_image_new_from_pixbuf(pb_rew);
    image_fs = gtk_image_new_from_pixbuf(pb_fs);

	gtk_window_set_icon(GTK_WINDOW(window),pb_icon);

	/*
    
	// REWIND
	rew_button = gtk_button_new ();
	gtk_button_set_image(GTK_BUTTON(rew_button), image_rew);
	GTK_WIDGET_UNSET_FLAGS (rew_button, GTK_CAN_DEFAULT);	
    g_signal_connect(G_OBJECT(rew_button),"clicked", G_CALLBACK(rew_callback), NULL);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, rew_button, _("Rewind"), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), rew_button, FALSE, FALSE, 0);
	
	// PLAY BUTTON
	play_button = gtk_button_new ();
	gtk_button_set_image(play_button, image_play);
    g_signal_connect(G_OBJECT(play_button),"clicked", G_CALLBACK(play_callback), NULL);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, play_button, _("Play"), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), play_button, FALSE, FALSE, 0);

	// PAUSE BUTTON
	pause_button = gtk_button_new ();
	gtk_button_set_image(pause_button, image_pause);
    g_signal_connect(G_OBJECT(pause_button),"clicked", G_CALLBACK(pause_callback), NULL);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, pause_button, _("Pause"), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), pause_button, FALSE, FALSE, 0);

	// STOP BUTTON
	stop_button = gtk_button_new ();
	gtk_button_set_image(stop_button, image_stop);
    g_signal_connect(G_OBJECT(stop_button),"clicked", G_CALLBACK(stop_callback), NULL);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, stop_button, _("Stop"), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), stop_button, FALSE, FALSE, 0);

	// FORWARD BUTTON
	ff_button = gtk_button_new ();
	gtk_button_set_image(ff_button, image_ff);
    g_signal_connect(G_OBJECT(ff_button),"clicked", G_CALLBACK(ff_callback), NULL);
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, ff_button, _("Forward"), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), ff_button, FALSE, FALSE, 0);
	
	*/
	
	rew_event_box = gtk_event_box_new();
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, rew_event_box, _("Rewind"), NULL);
    gtk_widget_set_events(rew_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(rew_event_box), "button_press_event", G_CALLBACK(rew_callback), NULL);
    gtk_widget_set_size_request(GTK_WIDGET(rew_event_box), 22, 16);
	
    gtk_container_add(GTK_CONTAINER(rew_event_box), image_rew);
    gtk_box_pack_start(GTK_BOX(hbox), rew_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_rew);
    gtk_widget_show(rew_event_box);

    play_event_box = gtk_event_box_new();
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, play_event_box, _("Play"), NULL);
    gtk_widget_set_events(play_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(play_event_box),
                     "button_press_event", G_CALLBACK(play_callback), NULL);
    gtk_widget_set_size_request(GTK_WIDGET(play_event_box), 22, 16);

    gtk_container_add(GTK_CONTAINER(play_event_box), image_play);
    gtk_box_pack_start(GTK_BOX(hbox), play_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_play);
    gtk_widget_show(play_event_box);



    pause_event_box = gtk_event_box_new();
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, pause_event_box, _("Pause"), NULL);
    gtk_widget_set_events(pause_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(pause_event_box),
                     "button_press_event", G_CALLBACK(pause_callback), NULL);
    gtk_widget_set_size_request(GTK_WIDGET(pause_event_box), 22, 16);

    gtk_container_add(GTK_CONTAINER(pause_event_box), image_pause);
    gtk_box_pack_start(GTK_BOX(hbox), pause_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_pause);
    gtk_widget_show(pause_event_box);

    stop_event_box = gtk_event_box_new();
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, stop_event_box, _("Stop"), NULL);
    gtk_widget_set_events(stop_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(stop_event_box),
                     "button_press_event", G_CALLBACK(stop_callback), NULL);
    gtk_widget_set_size_request(GTK_WIDGET(stop_event_box), 22, 16);

    gtk_container_add(GTK_CONTAINER(stop_event_box), image_stop);
    gtk_box_pack_start(GTK_BOX(hbox), stop_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_stop);
    gtk_widget_show(stop_event_box);

    ff_event_box = gtk_event_box_new();
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, ff_event_box, _("Fast Forward"), NULL);
    gtk_widget_set_events(ff_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(ff_event_box), "button_press_event", G_CALLBACK(ff_callback), NULL);
    gtk_widget_set_size_request(GTK_WIDGET(ff_event_box), 22, 16);

    gtk_container_add(GTK_CONTAINER(ff_event_box), image_ff);
    gtk_box_pack_start(GTK_BOX(hbox), ff_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_ff);
    gtk_widget_show(ff_event_box);


    progress = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(progress), TRUE, TRUE, 2);
    gtk_widget_set_events(GTK_WIDGET(progress), GDK_BUTTON_PRESS_MASK);
	g_signal_connect(G_OBJECT(progress),"button_press_event",
			     G_CALLBACK(progress_callback), NULL);
    gtk_widget_show(GTK_WIDGET(progress));

    vol_slider = gtk_hscale_new_with_range(0.0, 100.0, 1.0);
    volume_tip = gtk_tooltips_new();
    gtk_tooltips_set_tip(volume_tip, vol_slider, _("Volume 100%"), NULL);
    gtk_widget_set_size_request(vol_slider, 44, 16);
    gtk_box_pack_start(GTK_BOX(hbox), vol_slider, FALSE, FALSE, 0);
    gtk_scale_set_draw_value(GTK_SCALE(vol_slider), FALSE);
    gtk_range_set_value(GTK_RANGE(vol_slider), 100.0);
    g_signal_connect(G_OBJECT(vol_slider), "value_changed", G_CALLBACK(vol_slider_callback), NULL);
    gtk_widget_show(vol_slider);

    fs_event_box = gtk_event_box_new();
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, fs_event_box, _("Full Screen"), NULL);
    gtk_widget_set_events(fs_event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(fs_event_box), "button_press_event", G_CALLBACK(fs_callback), NULL);
    gtk_widget_set_size_request(GTK_WIDGET(fs_event_box), 22, 16);

    gtk_container_add(GTK_CONTAINER(fs_event_box), image_fs);
    gtk_box_pack_start(GTK_BOX(hbox), fs_event_box, FALSE, FALSE, 0);
    gtk_widget_show(image_fs);
    gtk_widget_show(fs_event_box);

    gtk_widget_show(hbox);
    gtk_widget_show(vbox);
    gtk_widget_show_all(window);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_showcontrols), showcontrols);

    if (windowid != 0) {
		while(gtk_events_pending()) gtk_main_iteration();
		
		window_container = gdk_window_foreign_new(windowid);
		if (GTK_WIDGET_MAPPED(window))
			gtk_widget_unmap(window);
		gdk_window_reparent(window->window,window_container,0,0);
		gtk_widget_map(window);
		gtk_widget_hide(GTK_WIDGET(song_title));
		if (window_x < 250) {
			gtk_widget_hide(rew_event_box);
			gtk_widget_hide(ff_event_box);
			gtk_widget_hide(fs_event_box);
		}
	}
	
	// g_signal_connect(G_OBJECT(fixed), "size_allocate", G_CALLBACK(allocate_fixed_callback), NULL);
	g_signal_connect(G_OBJECT(fixed), "expose_event", G_CALLBACK(expose_fixed_callback), NULL);
	
	gtk_widget_set_sensitive(GTK_WIDGET(menuitem_fullscreen),FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(fs_event_box), FALSE);
	
	while(gtk_events_pending()) gtk_main_iteration();
		
    return window;
}
