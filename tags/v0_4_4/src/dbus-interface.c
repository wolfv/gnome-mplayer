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

#include "dbus-interface.h"
#include "support.h"

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
    int message_type;
    gchar *s = NULL;
	gchar *hrefid = NULL;
	gchar *path = NULL;
	DBusError error;
	DBusMessage *reply_message;
	gchar *path1;
	gchar *path2;
	gchar *path3;
	GString *xml;
	gchar *xml_string;
	
    message_type = dbus_message_get_type(message);
    sender = dbus_message_get_sender(message);
    destination = dbus_message_get_destination(message);
/*
    printf("path=%s; interface=%s; member=%s; data=%s\n",
               dbus_message_get_path(message),
               dbus_message_get_interface(message), dbus_message_get_member(message), s);
*/	
	path1 = g_strdup_printf("/control/%i",control_id);
	path2 = g_strdup_printf("/window/%i",embed_window);
	path3 = g_strdup_printf("/pid/%i",getpid());
	
	if (dbus_message_get_path(message)) {
	
		if (g_ascii_strcasecmp(dbus_message_get_path(message),"/") == 0 ||
			g_ascii_strcasecmp(dbus_message_get_path(message),path1) == 0 ||
			g_ascii_strcasecmp(dbus_message_get_path(message),path2) == 0 ||
			g_ascii_strcasecmp(dbus_message_get_path(message),path3) == 0 ) {
		
			// printf("Path matched %s\n", dbus_message_get_path(message));
			if (message_type == DBUS_MESSAGE_TYPE_SIGNAL) {
				if (g_ascii_strcasecmp(dbus_message_get_member(message),"Open") == 0) {
					shutdown();
					dbus_error_init(&error);
					if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
						play_file(s,0);
					} else {
						dbus_error_free(&error);
					}
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"OpenPlaylist") == 0) {
					shutdown();
					dbus_error_init(&error);
					if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
						play_file(s,1);
					} else {
						dbus_error_free(&error);
					}
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"OpenButton") == 0) {
					shutdown();
					dbus_error_init(&error);
					if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &s, 
											  DBUS_TYPE_STRING, &hrefid,
											  DBUS_TYPE_INVALID)) {
						make_button(s, hrefid);
					} else {
						dbus_error_free(&error);
					}
					return DBUS_HANDLER_RESULT_HANDLED;
				}
				
				if (g_ascii_strcasecmp(dbus_message_get_member(message),"Close") == 0) {
					shutdown();
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"Quit") == 0) {
					shutdown();
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"Play") == 0  && idledata != NULL) {
					g_idle_add(set_play,idledata);
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"Pause") == 0  && idledata != NULL) {
					g_idle_add(set_pause,idledata);
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"Stop") == 0  && idledata != NULL) {
					g_idle_add(set_stop,idledata);
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"FastForward") == 0  && idledata != NULL) {
					g_idle_add(set_ff,idledata);
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"FastReverse") == 0  && idledata != NULL) {
					g_idle_add(set_rew,idledata);
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"Seek") == 0  && idledata != NULL) {
					dbus_error_init(&error);
					if (dbus_message_get_args(message, &error, DBUS_TYPE_DOUBLE, &(idledata->position), DBUS_TYPE_INVALID)) {
						g_idle_add(set_position,idledata);
					} else {
						dbus_error_free(&error);
					}
					return DBUS_HANDLER_RESULT_HANDLED;
				}
				
				if (g_ascii_strcasecmp(dbus_message_get_member(message),"Terminate") == 0) {
					shutdown();
					dbus_connection_flush(connection);
					dbus_connection_remove_filter(connection,filter_func, NULL);
					dbus_connection_unref(connection);
					connection = NULL;
					gtk_main_quit();
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"Volume") == 0  && idledata != NULL) {
					dbus_error_init(&error);
					if (dbus_message_get_args(message, &error, DBUS_TYPE_DOUBLE, &(idledata->volume), DBUS_TYPE_INVALID)) {
						g_idle_add(set_volume,idledata);
					} else {
						dbus_error_free(&error);
					}
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"SetFullScreen") == 0  && idledata != NULL) {
					dbus_error_init(&error);
					if (dbus_message_get_args(message, &error, DBUS_TYPE_BOOLEAN, &(idledata->fullscreen), DBUS_TYPE_INVALID)) {
						g_idle_add(set_fullscreen,idledata);
					} else {
						dbus_error_free(&error);
					}
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"SetPercent") == 0  && idledata != NULL) {
					dbus_error_init(&error);
					if (dbus_message_get_args(message, &error, DBUS_TYPE_DOUBLE, &(idledata->percent), DBUS_TYPE_INVALID)) {
						g_idle_add(set_progress_value,idledata);
					} else {
						dbus_error_free(&error);
					}
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"SetCachePercent") == 0  && idledata != NULL) {
					dbus_error_init(&error);
					if (dbus_message_get_args(message, &error, DBUS_TYPE_DOUBLE, &(idledata->cachepercent), DBUS_TYPE_INVALID)) {
						g_idle_add(set_progress_value,idledata);
					} else {
						dbus_error_free(&error);
					}
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"SetProgressText") == 0  && idledata != NULL) {
					dbus_error_init(&error);
					if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
						g_strlcpy(idledata->progress_text,s,1024);
						g_idle_add(set_progress_text,idledata);
					} else {
						dbus_error_free(&error);
					}
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"SetInfo") == 0  && idledata != NULL) {
					dbus_error_init(&error);
					if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
						g_strlcpy(idledata->info,s,1024);
						g_idle_add(set_media_info,idledata);
					} else {
						dbus_error_free(&error);
					}
					return DBUS_HANDLER_RESULT_HANDLED;
				}
				
				if (g_ascii_strcasecmp(dbus_message_get_member(message),"SetShowControls") == 0  && idledata != NULL) {
					dbus_error_init(&error);
					if (dbus_message_get_args(message, &error, DBUS_TYPE_BOOLEAN, &(idledata->showcontrols), DBUS_TYPE_INVALID)) {
						g_idle_add(set_show_controls,idledata);
					} else {
						dbus_error_free(&error);
					}
					return DBUS_HANDLER_RESULT_HANDLED;
				}

				if (g_ascii_strcasecmp(dbus_message_get_member(message),"ResizeWindow") == 0  && idledata != NULL) {
					dbus_error_init(&error);
					if (dbus_message_get_args(message, &error, DBUS_TYPE_INT32, &window_x, DBUS_TYPE_INT32, &window_y, DBUS_TYPE_INVALID)) {
						g_idle_add(resize_window,idledata);
					} else {
						dbus_error_free(&error);
					}
					return DBUS_HANDLER_RESULT_HANDLED;
				}
				
				if (g_ascii_strcasecmp(dbus_message_get_member(message),"Ping") == 0) {
					if (control_id != 0) {
						path = g_strdup_printf("/control/%i",control_id);
					}
					if (embed_window != 0) {
						path = g_strdup_printf("/window/%i",embed_window);
					}
					if (path == NULL) {
						path = g_strdup_printf("/");
					}
					
					reply_message = dbus_message_new_signal(path,"com.gecko.mediaplayer","Pong");
					dbus_connection_send(connection,reply_message,NULL);
					dbus_message_unref(reply_message);
					g_free(path);
					return DBUS_HANDLER_RESULT_HANDLED;
				}
			} else if (message_type == DBUS_MESSAGE_TYPE_METHOD_CALL) {
				// printf("Got member %s\n",dbus_message_get_member(message));
				if (dbus_message_is_method_call (message, "org.freedesktop.DBus.Introspectable", "Introspect")) {
					
					xml = g_string_new ("<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
								"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
								"<node>\n"
								"  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
								"    <method name=\"Introspect\">\n"
								"      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
								"    </method>\n"
								"  </interface>\n");
					
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
										  "    </signal>\n"
										  "  </interface>\n");
					xml = g_string_append (xml, "</node>\n");
					
					xml_string = g_string_free(xml,FALSE);
					reply_message = dbus_message_new_method_return (message);
					dbus_message_append_args (reply_message,
									  DBUS_TYPE_STRING, &xml_string,
									  DBUS_TYPE_INVALID);
					g_free(xml_string);
					dbus_connection_send (connection, reply_message, NULL);
					dbus_message_unref(reply_message);
					return DBUS_HANDLER_RESULT_HANDLED;
				}
				
				if (dbus_message_is_method_call (message, "com.gnome.mplayer", "Test")) {
					reply_message = dbus_message_new_method_return (message);
					dbus_connection_send (connection, reply_message, NULL);
					dbus_message_unref(reply_message);
					return DBUS_HANDLER_RESULT_HANDLED;
				}
				if (dbus_message_is_method_call (message, "com.gnome.mplayer", "GetVolume")) {
					reply_message = dbus_message_new_method_return (message);
					dbus_message_append_args (reply_message, DBUS_TYPE_DOUBLE, &idledata->volume, DBUS_TYPE_INVALID);
					dbus_connection_send (connection, reply_message, NULL);
					dbus_message_unref(reply_message);
					return DBUS_HANDLER_RESULT_HANDLED;
				}
				if (dbus_message_is_method_call (message, "com.gnome.mplayer", "GetFullScreen")) {
					reply_message = dbus_message_new_method_return (message);
					dbus_message_append_args (reply_message, DBUS_TYPE_BOOLEAN, fullscreen, DBUS_TYPE_INVALID);
					dbus_connection_send (connection, reply_message, NULL);
					dbus_message_unref(reply_message);
					return DBUS_HANDLER_RESULT_HANDLED;
				}
				if (dbus_message_is_method_call (message, "com.gnome.mplayer", "GetShowControls")) {
					reply_message = dbus_message_new_method_return (message);
					dbus_message_append_args (reply_message, DBUS_TYPE_BOOLEAN, fullscreen, DBUS_TYPE_INVALID);
					dbus_connection_send (connection, reply_message, NULL);
					dbus_message_unref(reply_message);
					return DBUS_HANDLER_RESULT_HANDLED;
				}
				if (dbus_message_is_method_call (message, "com.gnome.mplayer", "GetTime")) {
					reply_message = dbus_message_new_method_return (message);
					dbus_message_append_args (reply_message, DBUS_TYPE_DOUBLE, &idledata->position, DBUS_TYPE_INVALID);
					dbus_connection_send (connection, reply_message, NULL);
					dbus_message_unref(reply_message);
					return DBUS_HANDLER_RESULT_HANDLED;
				}
				if (dbus_message_is_method_call (message, "com.gnome.mplayer", "GetDuration")) {
					reply_message = dbus_message_new_method_return (message);
					dbus_message_append_args (reply_message, DBUS_TYPE_DOUBLE, &idledata->length, DBUS_TYPE_INVALID);
					dbus_connection_send (connection, reply_message, NULL);
					dbus_message_unref(reply_message);
					return DBUS_HANDLER_RESULT_HANDLED;
				}
				if (dbus_message_is_method_call (message, "com.gnome.mplayer", "GetPercent")) {
					reply_message = dbus_message_new_method_return (message);
					dbus_message_append_args (reply_message, DBUS_TYPE_DOUBLE, &idledata->percent, DBUS_TYPE_INVALID);
					dbus_connection_send (connection, reply_message, NULL);
					dbus_message_unref(reply_message);
					return DBUS_HANDLER_RESULT_HANDLED;
				}
				if (dbus_message_is_method_call (message, "com.gnome.mplayer", "GetCacheSize")) {
					reply_message = dbus_message_new_method_return (message);
					dbus_message_append_args (reply_message, DBUS_TYPE_INT32, &cache_size, DBUS_TYPE_INVALID);
					dbus_connection_send (connection, reply_message, NULL);
					dbus_message_unref(reply_message);
					return DBUS_HANDLER_RESULT_HANDLED;
				}
			} else {
				// printf("path didn't match\n");
			}
			
		}
    }
	g_free(path1);
	g_free(path2);
	g_free(path3);
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void dbus_open_by_hrefid(gchar *hrefid) {
	gchar *path;
	DBusMessage *message;
	gchar *id;
	
	id = g_strdup(hrefid);
	printf("requesting id = %s\n", id);
	path = g_strdup_printf("/control/%i",control_id);
	message = dbus_message_new_signal(path,"com.gecko.mediaplayer","RequestById");
	dbus_message_append_args(message, DBUS_TYPE_STRING, &id, DBUS_TYPE_INVALID);
	dbus_connection_send(connection,message,NULL);
	dbus_message_unref(message);
	g_free(path);
	
}

void dbus_open_next() {
	gchar *path;
	DBusMessage *message;
	
	path = g_strdup_printf("/control/%i",control_id);
	message = dbus_message_new_signal(path,"com.gecko.mediaplayer","Next");
	dbus_connection_send(connection,message,NULL);
	dbus_message_unref(message);
	g_free(path);
	
}

void dbus_cancel() {
	gchar *path;
	DBusMessage *message;
	gint id;
	
	id = control_id;
	path = g_strdup_printf("/control/%i",control_id);
	message = dbus_message_new_signal(path,"com.gecko.mediaplayer","Cancel");
	dbus_message_append_args(message, DBUS_TYPE_INT32, &id, DBUS_TYPE_INVALID);
	dbus_connection_send(connection,message,NULL);
	dbus_message_unref(message);
	g_free(path);
	
}

gboolean GetProperty(gchar *property) {
	
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
    printf("Using match: %s\n", match);
    g_free(match);
	
	dbus_connection_add_filter(connection,filter_func,NULL,NULL);
	
	printf("Proxy connections and Command connected\n");

	if (control_id != 0) {
		path = g_strdup_printf("com.gnome.mplayer.cid%i",control_id);
		dbus_bus_request_name(connection,path,0,NULL);
		g_free(path);
		path = g_strdup_printf("/control/%i",control_id);
		id = control_id;
		reply_message = dbus_message_new_signal(path,"com.gecko.mediaplayer","Ready");
		dbus_message_append_args(reply_message, DBUS_TYPE_INT32, &id, DBUS_TYPE_INVALID);
		dbus_connection_send(connection,reply_message,NULL);
		dbus_message_unref(reply_message);
		g_free(path);
	} else {
		dbus_bus_request_name(connection,"com.gnome.mplayer",0,NULL);
	}	
	
    return TRUE;
}