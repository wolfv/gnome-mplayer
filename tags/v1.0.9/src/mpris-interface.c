/*
 * mpris-interface.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * mpris-interface.c is free software.
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

#include "mpris-interface.h"
#include "support.h"
#include <unistd.h>
#include "mime_types.h"

#ifdef DBUS_ENABLED

static DBusConnection *mpris_connection = NULL;

void append_metadata_array(DBusMessageIter * messageIter)
{
    DBusMessageIter array, dict, dict_entry, dict_val, variant_array;
    gchar *property;
    gint64 i_val;
    gchar *s_val;
    gint i;


    dbus_message_iter_open_container(messageIter, DBUS_TYPE_VARIANT, "a{sv}", &array);
    dbus_message_iter_open_container(&array, DBUS_TYPE_ARRAY, "{sv}", &dict);

    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
    property = g_strdup("mpris:length");
    dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
    dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "x", &dict_val);
    i_val = gmtk_media_player_get_attribute_double(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_LENGTH) * 1000000;
    dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_INT64, &i_val);
    dbus_message_iter_close_container(&dict_entry, &dict_val);
    dbus_message_iter_close_container(&dict, &dict_entry);
    g_free(property);

    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
    property = g_strdup("mpris:trackid");
    dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
    if (gmtk_media_player_get_uri(GMTK_MEDIA_PLAYER(media)) == NULL
        || !gtk_list_store_iter_is_valid(playliststore, &iter)) {
        s_val = g_strdup("/org/mpris/MediaPlayer2/TrackList/NoTrack");
    } else {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ADD_ORDER_COLUMN, &i, -1);
        s_val = g_strdup_printf("/org/mpris/MediaPlayer2/Track/%i", i);
    }
    dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "o", &dict_val);
    dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_OBJECT_PATH, &s_val);
    dbus_message_iter_close_container(&dict_entry, &dict_val);
    dbus_message_iter_close_container(&dict, &dict_entry);
    g_free(property);
    g_free(s_val);

    if (gmtk_media_player_get_uri(GMTK_MEDIA_PLAYER(media)) != NULL) {
        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
        property = g_strdup("xesam:url");
        dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
        s_val = g_strdup(gmtk_media_player_get_uri(GMTK_MEDIA_PLAYER(media)));
        dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "s", &dict_val);
        dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_STRING, &s_val);
        dbus_message_iter_close_container(&dict_entry, &dict_val);
        dbus_message_iter_close_container(&dict, &dict_entry);
        g_free(property);
        g_free(s_val);
    }

    if (cover_art_uri != NULL) {
        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
        property = g_strdup("mpris:artUrl");
        dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
        s_val = g_strdup(gmtk_media_player_get_uri(GMTK_MEDIA_PLAYER(media)));
        dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "s", &dict_val);
        dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_STRING, &cover_art_uri);
        dbus_message_iter_close_container(&dict_entry, &dict_val);
        dbus_message_iter_close_container(&dict, &dict_entry);
        g_free(property);
        g_free(s_val);
    }

    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
    property = g_strdup("xesam:title");
    dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
    if (gmtk_media_player_get_attribute_string(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_TITLE) == NULL) {
        s_val = g_strdup(idledata->display_name);
    } else {
        s_val = g_strdup(gmtk_media_player_get_attribute_string(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_TITLE));
    }
    dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "s", &dict_val);
    dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_STRING, &s_val);
    dbus_message_iter_close_container(&dict_entry, &dict_val);
    dbus_message_iter_close_container(&dict, &dict_entry);
    g_free(property);
    g_free(s_val);

    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
    property = g_strdup("xesam:artist");
    dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
    if (gmtk_media_player_get_attribute_string(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_ARTIST) == NULL) {
        s_val = g_strdup("Unknown Artist");
    } else {
        s_val = g_strdup(gmtk_media_player_get_attribute_string(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_ARTIST));
    }
    dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "as", &dict_val);
    dbus_message_iter_open_container(&dict_val, DBUS_TYPE_ARRAY, "s", &variant_array);

    dbus_message_iter_append_basic(&variant_array, DBUS_TYPE_STRING, &s_val);
    dbus_message_iter_close_container(&dict_val, &variant_array);
    dbus_message_iter_close_container(&dict_entry, &dict_val);
    dbus_message_iter_close_container(&dict, &dict_entry);
    g_free(property);
    g_free(s_val);

    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
    property = g_strdup("xesam:album");
    dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
    if (gmtk_media_player_get_attribute_string(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_ALBUM) == NULL) {
        s_val = g_strdup("Unknown Album");
    } else {
        s_val = g_strdup(gmtk_media_player_get_attribute_string(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_ALBUM));
    }
    dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "s", &dict_val);
    dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_STRING, &s_val);
    dbus_message_iter_close_container(&dict_entry, &dict_val);
    dbus_message_iter_close_container(&dict, &dict_entry);
    g_free(property);
    g_free(s_val);

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
        property = g_strdup("xesam:trackNumber");
        dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ADD_ORDER_COLUMN, &i, -1);
        dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "i", &dict_val);
        dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_INT32, &i);
        dbus_message_iter_close_container(&dict_entry, &dict_val);
        dbus_message_iter_close_container(&dict, &dict_entry);
        g_free(property);
    }

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
        property = g_strdup("xesam:useCount");
        dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, COUNT_COLUMN, &i, -1);
        dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "i", &dict_val);
        dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_INT32, &i);
        dbus_message_iter_close_container(&dict_entry, &dict_val);
        dbus_message_iter_close_container(&dict, &dict_entry);
        g_free(property);
    }


    dbus_message_iter_close_container(&array, &dict);
    dbus_message_iter_close_container(messageIter, &array);
}

static DBusHandlerResult mpris_filter_func(DBusConnection * mpris_connection, DBusMessage * message, void *user_data)
{

    //const gchar *sender;
    //const gchar *destination;
    gint message_type;
    gchar *s = NULL;
    DBusError error;
    DBusMessage *reply_message;
    gchar *path1;
    GString *xml;
    gchar *xml_string;
    DBusMessageIter sub0, sub1, sub2, sub3, sub4;
    gchar *interface;
    gchar *property;
    gboolean b_val;
    gchar *s_val;
    gdouble d_val;
    gint i;
    gint64 x_val;
    GtkTreeIter localiter;

    message_type = dbus_message_get_type(message);
    //sender = dbus_message_get_sender(message);
    //destination = dbus_message_get_destination(message);

    /*
       printf("path=%s; interface=%s; member=%s; data=%s\n",
       dbus_message_get_path(message),
       dbus_message_get_interface(message), dbus_message_get_member(message), s);
     */

    path1 = g_strdup_printf("/org/mpris/MediaPlayer2");

    if (dbus_message_get_path(message)) {

        if (g_ascii_strcasecmp(dbus_message_get_path(message), path1) == 0) {

            gm_log(verbose, G_LOG_LEVEL_DEBUG, "Path matched %s", dbus_message_get_path(message));
            if (message_type == DBUS_MESSAGE_TYPE_SIGNAL) {

                if (g_ascii_strcasecmp(dbus_message_get_member(message), "Add") == 0) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
                        if (strlen(s) > 0) {
                            g_idle_add(add_to_playlist_and_play, g_strdup(s));
                        }
                    } else {
                        dbus_error_free(&error);
                    }
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

            } else if (message_type == DBUS_MESSAGE_TYPE_METHOD_CALL) {
                if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Introspectable", "Introspect")) {

                    xml =
                        g_string_new
                        ("<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN'\n"
                         "'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>\n"
                         "<node>\n" "  <interface name='org.freedesktop.DBus.Introspectable'>\n"
                         "    <method name='Introspect'>\n"
                         "      <arg name='data' direction='out' type='s'/>\n" "    </method>\n" "  </interface>\n");

                    xml = g_string_append(xml,
                                          " <interface name='org.freedesktop.DBus.Properties'>"
                                          "  <method name='Get'>"
                                          "   <arg name='interface' direction='in' type='s'/>"
                                          "   <arg name='property' direction='in' type='s'/>"
                                          "   <arg name='value' direction='out' type='v'/>"
                                          "  </method>"
                                          "  <method name='GetAll'>"
                                          "   <arg name='interface' direction='in' type='s'/>"
                                          "   <arg name='properties' direction='out' type='a{sv}'/>"
                                          "  </method>"
                                          "</interface>"
                                          "<interface name='org.mpris.MediaPlayer2'>\n"
                                          "    <method name='Raise'>\n"
                                          "    </method>\n"
                                          "    <method name='Quit'>\n"
                                          "    </method>\n"
                                          "    <property name='CanQuit' type='b' access='read' />"
                                          "    <property name='Fullscreen' type='b' access='readwrite' />"
                                          "    <property name='CanRaise' type='b' access='read' />"
                                          "    <property name='HasTrackList' type='b' access='read'/>"
                                          "    <property name='Identity' type='s' access='read'/>"
                                          "    <property name='DesktopEntry' type='s' access='read'/>"
                                          "    <property name='SupportedUriSchemes' type='as' access='read'/>"
                                          "    <property name='SupportedMimeTypes' type='as' access='read'/>"
                                          "</interface>\n"
                                          "<interface name='org.mpris.MediaPlayer2.Player'>\n"
                                          "    <method name='Next'/>\n"
                                          "    <method name='Previous'/>\n"
                                          "    <method name='Pause'/>\n"
                                          "    <method name='PlayPause'/>\n"
                                          "    <method name='Stop'/>\n"
                                          "    <method name='Play'/>\n"
                                          "    <method name='Seek'>\n"
                                          "        <arg direction='in' name='Offset' type='x'/>\n"
                                          "    </method>\n"
                                          "    <method name='SetPosition'>\n"
                                          "        <arg direction='in' name='TrackId' type='o'/>\n"
                                          "        <arg direction='in' name='Position' type='x'/>\n"
                                          "    </method>\n"
                                          "    <method name='OpenUri'>\n"
                                          "        <arg direction='in' name='Uri' type='s'/>\n"
                                          "    </method>\n"
                                          "    <signal name='Seeked'>\n"
                                          "        <arg name='Position' type='x'/>\n"
                                          "    </signal>\n"
                                          "    <property name='PlaybackStatus' type='s' access='read'/>\n"
                                          "    <property name='LoopStatus' type='s' access='readwrite'/>\n"
                                          "    <property name='Rate' type='d' access='readwrite'/>\n"
                                          "    <property name='Shuffle' type='b' access='readwrite'/>\n"
                                          "    <property name='Metadata' type='a{sv}' access='read'>\n"
                                          "    </property>\n"
                                          "    <property name='Volume' type='d' access='readwrite'/>\n"
                                          "    <property name='Position' type='x' access='read'/>\n"
                                          "    <property name='MinimumRate' type='d' access='read'/>\n"
                                          "    <property name='MaximumRate' type='d' access='read'/>\n"
                                          "    <property name='CanGoNext' type='b' access='read'/>\n"
                                          "    <property name='CanGoPrevious' type='b' access='read'/>\n"
                                          "    <property name='CanPlay' type='b' access='read'/>\n"
                                          "    <property name='CanPause' type='b' access='read'/>\n"
                                          "    <property name='CanSeek' type='b' access='read'/>\n"
                                          "    <property name='CanControl' type='b' access='read'/>\n"
                                          "</interface>\n"
                                          "<interface name='org.mpris.MediaPlayer2.TrackList'>\n"
                                          "    <method name='GetTracksMetadata'>\n"
                                          "        <arg direction='in' name='TrackIds' type='ao'/>\n"
                                          "	       <arg direction='out' name='Metadata' type='aa{sv}'>\n"
                                          "	       </arg>\n"
                                          "    </method>\n"
                                          "    <method name='AddTrack'>\n"
                                          "		   <arg direction='in' name='Uri' type='s'/>\n"
                                          "		   <arg direction='in' name='AfterTrack' type='o'/>\n"
                                          "		   <arg direction='in' name='SetAsCurrent' type='b'/>\n"
                                          "    </method>\n"
                                          "    <method name='RemoveTrack'>\n"
                                          "		   <arg direction='in' name='TrackId' type='o'/>\n"
                                          "    </method>\n"
                                          "    <method name='GoTo'>\n"
                                          "		   <arg direction='in' name='TrackId' type='o'/>\n"
                                          "    </method>\n"
                                          "    <signal name='TrackListReplaced'>\n"
                                          "		   <arg name='Tracks' type='ao'/>\n"
                                          "		   <arg name='CurrentTrack' type='o'/>\n"
                                          "    </signal>\n"
                                          "    <signal name='TrackAdded'>\n"
                                          "		   <arg name='Metadata' type='a{sv}'>\n"
                                          "		   </arg>\n"
                                          "		   <arg name='AfterTrack' type='o'/>\n"
                                          "    </signal>\n"
                                          "    <signal name='TrackRemoved'>\n"
                                          "		   <arg name='TrackId' type='o'/>\n"
                                          "    </signal>\n"
                                          "    <signal name='TrackMetadataChanged'>\n"
                                          "		   <arg name='TrackId' type='o'/>\n"
                                          "		   <arg name='Metadata' type='a{sv}'>\n"
                                          "		   </arg>\n"
                                          "    </signal>\n"
                                          "    <property name='Tracks' type='ao' access='read'/>\n"
                                          "    <property name='CanEditTracks' type='b' access='read'/>\n"
                                          "</interface>\n");
                    xml = g_string_append(xml, "</node>\n");

                    xml_string = g_string_free(xml, FALSE);
                    reply_message = dbus_message_new_method_return(message);
                    dbus_message_append_args(reply_message, DBUS_TYPE_STRING, &xml_string, DBUS_TYPE_INVALID);
                    g_free(xml_string);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Properties", "GetAll")) {

                    reply_message = dbus_message_new_method_return(message);
                    dbus_message_iter_init_append(reply_message, &sub0);
                    dbus_message_iter_open_container(&sub0, DBUS_TYPE_ARRAY, "{sv}", &sub1);

                    // org.mpris.MediaPlayer2 properties
                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("CanQuit");
                    b_val = TRUE;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("CanRaise");
                    b_val = TRUE;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("CanSetFullscreen");
                    b_val = TRUE;       // gtk_widget_get_sensitive(fs_event_box);
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("Fullscreen");
                    b_val = fullscreen;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("HasTrackList");
                    b_val = TRUE;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("Identity");
                    s_val = g_strdup("GNOME MPlayer");
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "s", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_STRING, &s_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    g_free(s_val);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("DesktopEntry");
                    s_val = g_strdup("gnome-mplayer");
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "s", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_STRING, &s_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    g_free(s_val);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("SupportedUriSchemes");
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "as", &sub3);
                    dbus_message_iter_open_container(&sub3, DBUS_TYPE_ARRAY, "s", &sub4);

                    s_val = g_strdup("file");
                    dbus_message_iter_append_basic(&sub4, DBUS_TYPE_STRING, &s_val);
                    g_free(s_val);
                    s_val = g_strdup("http");
                    dbus_message_iter_append_basic(&sub4, DBUS_TYPE_STRING, &s_val);
                    g_free(s_val);
                    s_val = g_strdup("mms");
                    dbus_message_iter_append_basic(&sub4, DBUS_TYPE_STRING, &s_val);
                    g_free(s_val);
                    s_val = g_strdup("rtsp");
                    dbus_message_iter_append_basic(&sub4, DBUS_TYPE_STRING, &s_val);
                    g_free(s_val);

                    dbus_message_iter_close_container(&sub3, &sub4);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("SupportedMimeTypes");
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "as", &sub3);
                    dbus_message_iter_open_container(&sub3, DBUS_TYPE_ARRAY, "s", &sub4);

                    for (i = 0; i < G_N_ELEMENTS(mime_types); i++) {
                        s_val = g_strdup(mime_types[i]);
                        dbus_message_iter_append_basic(&sub4, DBUS_TYPE_STRING, &s_val);
                        g_free(s_val);
                    }

                    dbus_message_iter_close_container(&sub3, &sub4);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    // org.mpris.MediaPlayer2.Player properties

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("PlaybackStatus");
                    switch (gmtk_media_player_get_media_state(GMTK_MEDIA_PLAYER(media))) {
                    case MEDIA_STATE_PLAY:
                        s_val = g_strdup("Playing");
                        break;
                    case MEDIA_STATE_PAUSE:
                        s_val = g_strdup("Paused");
                        break;
                    default:
                        s_val = g_strdup("Stopped");
                    }
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "s", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_STRING, &s_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    g_free(s_val);
                    dbus_message_iter_close_container(&sub1, &sub2);


                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("Volume");
                    d_val = audio_device.volume;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "d", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_DOUBLE, &d_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("Rate");
                    d_val = gmtk_media_player_get_attribute_double(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_SPEED_SET);
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "d", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_DOUBLE, &d_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("MinimumRate");
                    d_val = 0.1;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "d", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_DOUBLE, &d_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("MaximumRate");
                    d_val = 10.0;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "d", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_DOUBLE, &d_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("Position");
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "x", &sub3);
                    x_val =
                        gmtk_media_player_get_attribute_double(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_POSITION) * 1000000;
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_INT64, &x_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("CanGoNext");
                    b_val = FALSE;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("CanGoPrevious");
                    b_val = FALSE;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("CanPlay");
                    b_val = TRUE;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("CanPause");
                    b_val = TRUE;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("CanSeek");
                    b_val = (gmtk_media_player_get_attribute_double(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_LENGTH) > 1);
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("CanGoNext");
                    b_val = (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) > 1);
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("CanGoPrevious");
                    b_val = (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) > 1);
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("CanControl");
                    b_val = TRUE;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("CanEditTracks");
                    b_val = FALSE;
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "b", &sub3);
                    dbus_message_iter_append_basic(&sub3, DBUS_TYPE_BOOLEAN, &b_val);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("Tracks");
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    dbus_message_iter_open_container(&sub2, DBUS_TYPE_VARIANT, "ao", &sub3);
                    dbus_message_iter_open_container(&sub3, DBUS_TYPE_ARRAY, "o", &sub4);

                    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter);
                    if (gtk_list_store_iter_is_valid(playliststore, &localiter)) {
                        do {
                            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ADD_ORDER_COLUMN, &i, -1);
                            s_val = g_strdup_printf("/org/mpris/MediaPlayer2/Track/%i", i);
                            dbus_message_iter_append_basic(&sub4, DBUS_TYPE_OBJECT_PATH, &s_val);
                            g_free(s_val);
                        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
                    }

                    dbus_message_iter_close_container(&sub3, &sub4);
                    dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);


                    dbus_message_iter_open_container(&sub1, DBUS_TYPE_DICT_ENTRY, NULL, &sub2);
                    property = g_strdup("Metadata");
                    dbus_message_iter_append_basic(&sub2, DBUS_TYPE_STRING, &property);
                    append_metadata_array(&sub2);
                    //dbus_message_iter_close_container(&sub2, &sub3);
                    g_free(property);
                    dbus_message_iter_close_container(&sub1, &sub2);

                    dbus_message_iter_close_container(&sub0, &sub1);

                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;

                }
                // org.mpris.MediaPlayer2 Methods
                if (dbus_message_is_method_call(message, "org.mpris.MediaPlayer2", "Raise")) {
                    g_idle_add(set_raise_window, NULL);
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "org.mpris.MediaPlayer2", "Quit")) {
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    g_idle_add(set_kill_mplayer, NULL);
                    if (gmtk_media_player_get_media_state(GMTK_MEDIA_PLAYER(media)) != MEDIA_STATE_UNKNOWN)
                        dontplaynext = TRUE;
                    g_idle_add(set_quit, idledata);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                // org.mpris.MediaPlayer2.Player Methods
                if (dbus_message_is_method_call(message, "org.mpris.MediaPlayer2.Player", "Pause")) {
                    g_idle_add(set_pause, idledata);
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "org.mpris.MediaPlayer2.Player", "PlayPause")) {
                    g_idle_add(set_pause, idledata);
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "org.mpris.MediaPlayer2.Player", "Stop")) {
                    g_idle_add(set_stop, idledata);
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "org.mpris.MediaPlayer2.Player", "Play")) {
                    g_idle_add(set_play, idledata);
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "org.mpris.MediaPlayer2.Player", "Next")) {
                    g_idle_add(set_next, idledata);
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "org.mpris.MediaPlayer2.Player", "Previous")) {
                    g_idle_add(set_prev, idledata);
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "org.mpris.MediaPlayer2.Player", "Seek")) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args(message, &error, DBUS_TYPE_INT64, &x_val, DBUS_TYPE_INVALID)) {
                        idledata->position = idledata->position + (x_val / 1000000.0);
                        g_idle_add(set_position, idledata);
                    } else {
                        dbus_error_free(&error);
                    }
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if (dbus_message_is_method_call(message, "org.mpris.MediaPlayer2.Player", "SetPosition")) {
                    dbus_error_init(&error);
                    if (dbus_message_get_args
                        (message, &error, DBUS_TYPE_OBJECT_PATH, &s_val, DBUS_TYPE_INT64, &x_val, DBUS_TYPE_INVALID)) {
                        idledata->position = x_val / 1000000.0;
                        g_idle_add(set_position, idledata);
                    } else {
                        dbus_error_free(&error);
                    }
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }



                if (dbus_message_is_method_call(message, "org.mpris.MediaPlayer2.Player", "OpenUri")) {
                    if (gmtk_media_player_get_media_state(GMTK_MEDIA_PLAYER(media)) != MEDIA_STATE_UNKNOWN)
                        dontplaynext = TRUE;
                    gmtk_media_player_set_state(GMTK_MEDIA_PLAYER(media), MEDIA_STATE_QUIT);
                    dbus_error_init(&error);
                    if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
                        if (strlen(s) > 0) {
                            gm_log(verbose, G_LOG_LEVEL_INFO, "opening %s", s);
                            g_idle_add(clear_playlist_and_play, g_strdup(s));
                        } else {
                            gm_log(verbose, G_LOG_LEVEL_INFO, "Open requested, but value is blank");
                        }

                    } else {
                        dbus_error_free(&error);
                    }
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }


                if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Properties", "Set")) {
                    gm_log(verbose, G_LOG_LEVEL_DEBUG, "dbus message signature: %s",
                           dbus_message_get_signature(message));

                    dbus_message_iter_init(message, &sub0);
                    if (dbus_message_iter_get_arg_type(&sub0) == DBUS_TYPE_STRING) {
                        dbus_message_iter_get_basic(&sub0, &interface);
                        if (dbus_message_iter_next(&sub0) && dbus_message_iter_get_arg_type(&sub0) == DBUS_TYPE_STRING) {
                            dbus_message_iter_get_basic(&sub0, &property);
                            gm_log(verbose, G_LOG_LEVEL_MESSAGE, "interface: %s property: %s", interface, property);

                            if (g_ascii_strcasecmp(property, "Fullscreen") == 0) {
                                dbus_message_iter_next(&sub0);
                                // this is a variant, so we have to open its container
                                dbus_message_iter_recurse(&sub0, &sub1);
                                dbus_message_iter_get_basic(&sub1, &b_val);
                                gm_log(verbose, G_LOG_LEVEL_MESSAGE, "value = %i", b_val);
                                idledata->fullscreen = b_val;
                                g_idle_add(set_fullscreen, idledata);
                            }

                            if (g_ascii_strcasecmp(property, "Volume") == 0) {
                                dbus_message_iter_next(&sub0);
                                // this is a variant, so we have to open its container
                                dbus_message_iter_recurse(&sub0, &sub1);
                                dbus_message_iter_get_basic(&sub1, &d_val);
                                audio_device.volume = d_val;
                                g_idle_add(set_volume, idledata);
                            }

                            if (g_ascii_strcasecmp(property, "Rate") == 0) {
                                dbus_message_iter_next(&sub0);
                                // this is a variant, so we have to open its container
                                dbus_message_iter_recurse(&sub0, &sub1);
                                dbus_message_iter_get_basic(&sub1, &d_val);
                                gmtk_media_player_set_attribute_double(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_SPEED_SET,
                                                                       d_val);
                                g_idle_add(set_volume, idledata);
                            }
                        }
                    } else {
                        gm_log(verbose, G_LOG_LEVEL_MESSAGE, "get args failed");
                    }
                    reply_message = dbus_message_new_method_return(message);
                    dbus_connection_send(mpris_connection, reply_message, NULL);
                    dbus_message_unref(reply_message);

                    return DBUS_HANDLER_RESULT_HANDLED;
                }

                if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Properties", "Get")) {
                    gm_log(verbose, G_LOG_LEVEL_DEBUG, "dbus message signature: %s",
                           dbus_message_get_signature(message));

                    dbus_message_iter_init(message, &sub0);
                    if (dbus_message_iter_get_arg_type(&sub0) == DBUS_TYPE_STRING) {
                        dbus_message_iter_get_basic(&sub0, &interface);
                        if (dbus_message_iter_next(&sub0) && dbus_message_iter_get_arg_type(&sub0) == DBUS_TYPE_STRING) {
                            dbus_message_iter_get_basic(&sub0, &property);

                            if (g_ascii_strcasecmp(property, "PlaybackStatus") == 0) {
                                switch (gmtk_media_player_get_media_state(GMTK_MEDIA_PLAYER(media))) {
                                case MEDIA_STATE_PLAY:
                                    s_val = g_strdup("Playing");
                                    break;
                                case MEDIA_STATE_PAUSE:
                                    s_val = g_strdup("Paused");
                                    break;
                                default:
                                    s_val = g_strdup("Stopped");
                                }

                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                dbus_message_iter_open_container(&sub0, DBUS_TYPE_VARIANT, "s", &sub1);
                                dbus_message_iter_append_basic(&sub1, DBUS_TYPE_STRING, &s_val);
                                dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "Position") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                //dbus_message_iter_open_container(&sub0, DBUS_TYPE_VARIANT, "x", &sub1);
                                x_val =
                                    gmtk_media_player_get_attribute_double(GMTK_MEDIA_PLAYER(media),
                                                                           ATTRIBUTE_POSITION) * 1000000;
                                dbus_message_iter_append_basic(&sub0, DBUS_TYPE_INT64, &x_val);
                                //dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "Volume") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                //dbus_message_iter_open_container(&sub0, DBUS_TYPE_VARIANT, "d", &sub1);
                                d_val = audio_device.volume;
                                dbus_message_iter_append_basic(&sub0, DBUS_TYPE_DOUBLE, &d_val);
                                //dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "Rate") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                d_val =
                                    gmtk_media_player_get_attribute_double(GMTK_MEDIA_PLAYER(media),
                                                                           ATTRIBUTE_SPEED_SET);
                                dbus_message_iter_append_basic(&sub0, DBUS_TYPE_DOUBLE, &d_val);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "MinimumRate") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                d_val = 0.1;
                                dbus_message_iter_append_basic(&sub0, DBUS_TYPE_DOUBLE, &d_val);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "MaximumRate") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                d_val = 10.0;
                                dbus_message_iter_append_basic(&sub0, DBUS_TYPE_DOUBLE, &d_val);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "Identity") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                dbus_message_iter_open_container(&sub0, DBUS_TYPE_VARIANT, "s", &sub1);
                                s_val = g_strdup("GNOME MPlayer");
                                dbus_message_iter_append_basic(&sub1, DBUS_TYPE_STRING, &s_val);
                                g_free(s_val);
                                dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "DesktopEntry") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                dbus_message_iter_open_container(&sub0, DBUS_TYPE_VARIANT, "s", &sub1);
                                s_val = g_strdup("gnome-mplayer");
                                dbus_message_iter_append_basic(&sub1, DBUS_TYPE_STRING, &s_val);
                                g_free(s_val);
                                dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "CanRaise") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                dbus_message_iter_open_container(&sub0, DBUS_TYPE_VARIANT, "b", &sub1);
                                b_val = TRUE;
                                dbus_message_iter_append_basic(&sub1, DBUS_TYPE_BOOLEAN, &b_val);
                                dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "CanQuit") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                dbus_message_iter_open_container(&sub0, DBUS_TYPE_VARIANT, "b", &sub1);
                                b_val = TRUE;
                                dbus_message_iter_append_basic(&sub1, DBUS_TYPE_BOOLEAN, &b_val);
                                dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "CanSeek") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                dbus_message_iter_open_container(&sub0, DBUS_TYPE_VARIANT, "b", &sub1);
                                b_val =
                                    (gmtk_media_player_get_attribute_double(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_LENGTH)
                                     > 1);
                                dbus_message_iter_append_basic(&sub1, DBUS_TYPE_BOOLEAN, &b_val);
                                dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "CanGoNext") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                dbus_message_iter_open_container(&sub0, DBUS_TYPE_VARIANT, "b", &sub1);
                                b_val = (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) > 1);
                                dbus_message_iter_append_basic(&sub1, DBUS_TYPE_BOOLEAN, &b_val);
                                dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "CanGoPrevious") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                dbus_message_iter_open_container(&sub0, DBUS_TYPE_VARIANT, "b", &sub1);
                                b_val = (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) > 1);
                                dbus_message_iter_append_basic(&sub1, DBUS_TYPE_BOOLEAN, &b_val);
                                dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "HasTrackList") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                dbus_message_iter_open_container(&sub0, DBUS_TYPE_VARIANT, "b", &sub1);
                                b_val = TRUE;
                                dbus_message_iter_append_basic(&sub1, DBUS_TYPE_BOOLEAN, &b_val);
                                dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "CanEditTracks") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                dbus_message_iter_open_container(&sub0, DBUS_TYPE_VARIANT, "b", &sub1);
                                b_val = FALSE;
                                dbus_message_iter_append_basic(&sub1, DBUS_TYPE_BOOLEAN, &b_val);
                                dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }

                            if (g_ascii_strcasecmp(property, "Tracks") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                dbus_message_iter_open_container(&sub0, DBUS_TYPE_ARRAY, "o", &sub1);

                                gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter);
                                if (gtk_list_store_iter_is_valid(playliststore, &localiter)) {
                                    do {
                                        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ADD_ORDER_COLUMN,
                                                           &i, -1);
                                        s_val = g_strdup_printf("/org/mpris/MediaPlayer2/Track/%i", i);
                                        dbus_message_iter_append_basic(&sub1, DBUS_TYPE_OBJECT_PATH, &s_val);
                                        g_free(s_val);
                                    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
                                }

                                dbus_message_iter_close_container(&sub0, &sub1);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }


                            if (g_ascii_strcasecmp(property, "Metadata") == 0) {
                                reply_message = dbus_message_new_method_return(message);
                                dbus_message_iter_init_append(reply_message, &sub0);
                                append_metadata_array(&sub0);
                                dbus_connection_send(mpris_connection, reply_message, NULL);
                                dbus_message_unref(reply_message);
                                return DBUS_HANDLER_RESULT_HANDLED;
                            }
                        }
                        gm_log(verbose, G_LOG_LEVEL_MESSAGE, "GET not implemented for interface: %s property: %s",
                               interface, property);
                        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
                    }
                }

                gm_log(verbose, G_LOG_LEVEL_MESSAGE, "No handler implemented for interface: %s member: %s",
                       dbus_message_get_interface(message), dbus_message_get_member(message));

            } else {
                gm_log(verbose, G_LOG_LEVEL_INFO, "path didn't match - %s", dbus_message_get_path(message));
            }

        }
    }
    g_free(path1);
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
#endif

gboolean mpris_hookup(gint controlid)
{
#ifdef DBUS_ENABLED

    DBusError error;
    DBusBusType type = DBUS_BUS_SESSION;
    gchar *match;
    gchar *path;
    gboolean ret;

    dbus_error_init(&error);
    mpris_connection = dbus_bus_get(type, &error);
    if (mpris_connection == NULL) {
        gm_log(verbose, G_LOG_LEVEL_MESSAGE, "Failed to open connection to %s message bus: %s",
               (type == DBUS_BUS_SYSTEM) ? "system" : "session", error.message);
        dbus_error_free(&error);
        return FALSE;
    }
    dbus_connection_setup_with_g_main(mpris_connection, NULL);

    match = g_strdup_printf("type='signal',interface='org.mpris.MediaPlayer2'");
    dbus_bus_add_match(mpris_connection, match, &error);
    gm_log(verbose, G_LOG_LEVEL_INFO, "Using match: %s", match);
    g_free(match);
    dbus_error_free(&error);

    match = g_strdup_printf("type='signal',interface='org.mpris.MediaPlayer2.Player'");
    dbus_bus_add_match(mpris_connection, match, &error);
    gm_log(verbose, G_LOG_LEVEL_INFO, "Using match: %s", match);
    g_free(match);
    dbus_error_free(&error);

    dbus_connection_add_filter(mpris_connection, mpris_filter_func, NULL, NULL);

    path = g_strdup_printf("org.mpris.MediaPlayer2.gnome-mplayer");
    ret = dbus_bus_request_name(mpris_connection, path, 0, NULL);
    g_free(path);

    if (control_id != 0) {
        path = g_strdup_printf("org.mpris.MediaPlayer2.gnome-mplayer.cid%i", control_id);
        ret = dbus_bus_request_name(mpris_connection, path, 0, NULL);
        g_free(path);
    }

    gm_log(verbose, G_LOG_LEVEL_INFO, "Proxy connections and Command connected ret = %i", ret);
#endif

    return TRUE;
}

void mpris_unhook()
{
    dbus_enable_screensaver();
#ifdef DBUS_ENABLED
    if (mpris_connection != NULL) {
        dbus_connection_unref(mpris_connection);
        mpris_connection = NULL;
    }
#endif
}

void mpris_send_signal_PlaybackStatus()
{
#ifdef DBUS_ENABLED
    DBusMessage *message;
    DBusMessageIter iter, dict, dict_entry, dict_val;
    gchar *path;
    gchar *interface;
    gchar *property;
    gchar *s_val;

    if (mpris_connection != NULL) {
        path = g_strdup_printf("/org/mpris/MediaPlayer2");
        interface = g_strdup("org.mpris.MediaPlayer2.Player");
        message = dbus_message_new_signal(path, "org.freedesktop.DBus.Properties", "PropertiesChanged");
        dbus_message_iter_init_append(message, &iter);
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &interface);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
        property = g_strdup("PlaybackStatus");
        switch (gmtk_media_player_get_media_state(GMTK_MEDIA_PLAYER(media))) {
        case MEDIA_STATE_PLAY:
            s_val = g_strdup("Playing");
            break;
        case MEDIA_STATE_PAUSE:
            s_val = g_strdup("Paused");
            break;
        default:
            s_val = g_strdup("Stopped");
        }
        dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
        dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "s", &dict_val);
        dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_STRING, &s_val);
        dbus_message_iter_close_container(&dict_entry, &dict_val);
        g_free(s_val);
        dbus_message_iter_close_container(&dict, &dict_entry);
        dbus_message_iter_close_container(&iter, &dict);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &dict);
        dbus_message_iter_append_basic(&dict, DBUS_TYPE_STRING, &property);
        dbus_message_iter_close_container(&iter, &dict);

        g_free(property);

        dbus_connection_send(mpris_connection, message, NULL);
        dbus_message_unref(message);
        g_free(interface);
        g_free(path);
    }
#endif
}


void mpris_send_signal_Updated_Metadata()
{
#ifdef DBUS_ENABLED
    DBusMessage *message;
    DBusMessageIter iter, dict, dict_entry, dict_val;
    gchar *path;
    gchar *interface;
    gchar *property;
    gboolean b_val;
    gchar *s_val;

    if (mpris_connection != NULL) {
        path = g_strdup_printf("/org/mpris/MediaPlayer2");
        interface = g_strdup("org.mpris.MediaPlayer2.Player");
        message = dbus_message_new_signal(path, "org.freedesktop.DBus.Properties", "PropertiesChanged");
        dbus_message_iter_init_append(message, &iter);
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &interface);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
        property = g_strdup("Metadata");
        dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
        append_metadata_array(&dict_entry);
        dbus_message_iter_close_container(&dict, &dict_entry);

        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
        property = g_strdup("CanSeek");
        dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
        dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "b", &dict_val);
        b_val = (gmtk_media_player_get_attribute_double(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_LENGTH) > 1);
        dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_BOOLEAN, &b_val);
        dbus_message_iter_close_container(&dict_entry, &dict_val);
        dbus_message_iter_close_container(&dict, &dict_entry);

        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
        property = g_strdup("PlaybackStatus");
        switch (gmtk_media_player_get_media_state(GMTK_MEDIA_PLAYER(media))) {
        case MEDIA_STATE_PLAY:
            s_val = g_strdup("Playing");
            break;
        case MEDIA_STATE_PAUSE:
            s_val = g_strdup("Paused");
            break;
        default:
            s_val = g_strdup("Stopped");
        }
        dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
        dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "s", &dict_val);
        dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_STRING, &s_val);
        dbus_message_iter_close_container(&dict_entry, &dict_val);
        g_free(s_val);

        dbus_message_iter_close_container(&dict, &dict_entry);
        dbus_message_iter_close_container(&iter, &dict);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &dict);
        dbus_message_iter_close_container(&iter, &dict);

        g_free(property);

        dbus_connection_send(mpris_connection, message, NULL);
        dbus_message_unref(message);
        g_free(interface);
        g_free(path);
    }
#endif
}

void mpris_send_signal_Seeked()
{
#ifdef DBUS_ENABLED
    DBusMessageIter iter;
    DBusMessage *message;
    gchar *path;
    gchar *interface;
    gint64 x_val;

    if (mpris_connection != NULL) {
        path = g_strdup_printf("/org/mpris/MediaPlayer2");
        interface = g_strdup("org.mpris.MediaPlayer2.Player");
        message = dbus_message_new_signal(path, interface, "Seeked");
        dbus_message_iter_init_append(message, &iter);
        x_val = gmtk_media_player_get_attribute_double(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_POSITION) * 1000000;
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT64, &x_val);

        dbus_connection_send(mpris_connection, message, NULL);
        dbus_message_unref(message);
        g_free(interface);
        g_free(path);
    }
#endif
}

void mpris_send_signal_VolumeChanged()
{
#ifdef DBUS_ENABLED
    DBusMessageIter iter, dict, dict_entry, dict_val;;
    DBusMessage *message;
    gchar *path;
    gchar *interface;
    gchar *property;
    gdouble d_val;

    if (mpris_connection != NULL) {
        path = g_strdup_printf("/org/mpris/MediaPlayer2");
        interface = g_strdup("org.mpris.MediaPlayer2.Player");
        message = dbus_message_new_signal(path, "org.freedesktop.DBus.Properties", "PropertiesChanged");
        dbus_message_iter_init_append(message, &iter);
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &interface);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
        property = g_strdup("Volume");
        dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
        dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "d", &dict_val);
        d_val = audio_device.volume;
        dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_DOUBLE, &d_val);
        dbus_message_iter_close_container(&dict_entry, &dict_val);
        dbus_message_iter_close_container(&dict, &dict_entry);
        dbus_message_iter_close_container(&iter, &dict);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &dict);
        // dbus_message_iter_append_basic(&dict, DBUS_TYPE_STRING, &property);
        dbus_message_iter_close_container(&iter, &dict);

        g_free(property);

        dbus_connection_send(mpris_connection, message, NULL);
        dbus_message_unref(message);
        g_free(interface);
        g_free(path);
    }
#endif
}

void mpris_send_signal_RateChanged()
{
#ifdef DBUS_ENABLED
    DBusMessageIter iter, dict, dict_entry, dict_val;;
    DBusMessage *message;
    gchar *path;
    gchar *interface;
    gchar *property;
    gdouble d_val;

    if (mpris_connection != NULL) {
        path = g_strdup_printf("/org/mpris/MediaPlayer2");
        interface = g_strdup("org.mpris.MediaPlayer2.Player");
        message = dbus_message_new_signal(path, "org.freedesktop.DBus.Properties", "PropertiesChanged");
        dbus_message_iter_init_append(message, &iter);
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &interface);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);
        property = g_strdup("Rate");
        dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &property);
        dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "d", &dict_val);
        d_val = gmtk_media_player_get_attribute_double(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_SPEED_SET);
        dbus_message_iter_append_basic(&dict_val, DBUS_TYPE_DOUBLE, &d_val);
        dbus_message_iter_close_container(&dict_entry, &dict_val);
        dbus_message_iter_close_container(&dict, &dict_entry);
        dbus_message_iter_close_container(&iter, &dict);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &dict);
        // dbus_message_iter_append_basic(&dict, DBUS_TYPE_STRING, &property);
        dbus_message_iter_close_container(&iter, &dict);

        g_free(property);

        dbus_connection_send(mpris_connection, message, NULL);
        dbus_message_unref(message);
        g_free(interface);
        g_free(path);
    }
#endif
}
