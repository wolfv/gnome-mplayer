/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 *
 * main.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * main.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <mntent_compat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libintl.h>
#include <signal.h>

//#include <bonobo.h>
// #include <gnome.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include "common.h"
#include "support.h"
#include "dbus-interface.h"
#include "thread.h"

GMutex *thread_running = NULL;


static GOptionEntry entries[] = {
    {"window", 0, 0, G_OPTION_ARG_INT, &embed_window, N_("Window to embed in"), "WID"},
    {"width", 'w', 0, G_OPTION_ARG_INT, &window_x, N_("Width of window to embed in"), "X"},
    {"height", 'h', 0, G_OPTION_ARG_INT, &window_y, N_("Height of window to embed in"), "Y"},
    {"controlid", 0, 0, G_OPTION_ARG_INT, &control_id, N_("Unique DBUS controller id"), "CID"},
    {"playlist", 0, 0, G_OPTION_ARG_NONE, &playlist, N_("File Argument is a playlist"), NULL},
    {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, N_("Show more output on the console"), NULL},
    {"reallyverbose", '\0', 0, G_OPTION_ARG_NONE, &reallyverbose,
     N_("Show even more output on the console"), NULL},
    {"fullscreen", 0, 0, G_OPTION_ARG_NONE, &fullscreen, N_("Start in fullscreen mode"), NULL},
    {"softvol", 0, 0, G_OPTION_ARG_NONE, &softvol, N_("Use mplayer software volume control"), NULL},
    {"mixer", 0, 0, G_OPTION_ARG_STRING, &mixer, N_("Mixer to use"), NULL},
    {"volume", 0, 0, G_OPTION_ARG_INT, &volume, N_("Set initial volume percentage"), NULL},
    {"showcontrols", 0, 0, G_OPTION_ARG_INT, &showcontrols, N_("Show the controls in window"),
     "[0|1]"},
    {"showsubtitles", 0, 0, G_OPTION_ARG_INT, &showsubtitles, N_("Show the subtitles if available"),
     "[0|1]"},
    {"autostart", 0, 0, G_OPTION_ARG_INT, &autostart,
     N_("Autostart the media default to 1, set to 0 to load but don't play"), "[0|1]"},
    {"disablecontextmenu", 0, 0, G_OPTION_ARG_NONE, &disable_context_menu,
     N_("Disable popup menu on right click"), NULL},
    {"disablefullscreen", 0, 0, G_OPTION_ARG_NONE, &disable_fullscreen,
     N_("Disable fullscreen options in browser mode"), NULL},
    {"loop", 0, 0, G_OPTION_ARG_NONE, &loop, N_("Play all files on the playlist forever"), NULL},
    {"quit_on_complete", 'q', 0, G_OPTION_ARG_NONE, &quit_on_complete,
     N_("Quit application when last file on playlist is played"), NULL},
    {"random", 0, 0, G_OPTION_ARG_NONE, &random_order, N_("Play items on playlist in random order"),
     NULL},
    {"cache", 0, 0, G_OPTION_ARG_INT, &cache_size, N_("Set cache size"),
     NULL},
    {"ss", 0, 0, G_OPTION_ARG_INT, &start_second, N_("Start Second (default to 0)"),
     NULL},
    {"endpos", 0, 0, G_OPTION_ARG_INT, &play_length, N_("Length of media to play from start second"),
     NULL},
    {"forcecache", 0, 0, G_OPTION_ARG_NONE, &forcecache, N_("Force cache usage on streaming sites"),
     NULL},
    {"disabledeinterlace", 0, 0, G_OPTION_ARG_NONE, &disable_deinterlace,
     N_("Disable the deinterlace filter"),
     NULL},
    {"disableframedrop", 0, 0, G_OPTION_ARG_NONE, &disable_framedrop,
     N_("Don't skip drawing frames to better keep sync"),
     NULL},
    {"disableass", 0, 0, G_OPTION_ARG_NONE, &disable_ass,
     N_("Use the old subtitle rendering system"),
     NULL},
    {"disableembeddedfonts", 0, 0, G_OPTION_ARG_NONE, &disable_embeddedfonts,
     N_("Don't use fonts embedded on matroska files"),
     NULL},
    {"vertical", 0, 0, G_OPTION_ARG_NONE, &vertical_layout, N_("Use Vertical Layout"),
     NULL},
    {"showplaylist", 0, 0, G_OPTION_ARG_NONE, &playlist_visible, N_("Start with playlist open"),
     NULL},
    {"showdetails", 0, 0, G_OPTION_ARG_NONE, &details_visible, N_("Start with details visible"),
     NULL},
    {"rpname", 0, 0, G_OPTION_ARG_STRING, &rpname, N_("Real Player Name"), "NAME"},
    {"rpconsole", 0, 0, G_OPTION_ARG_STRING, &rpconsole, N_("Real Player Console ID"), "CONSOLE"},
    {"rpcontrols", 0, 0, G_OPTION_ARG_STRING, &rpcontrols, N_("Real Player Console Controls"),
     "Control Name,..."},
    {"subtitle", 0, 0, G_OPTION_ARG_STRING, &subtitle, N_("Subtitle file for first media file"),
     "FILENAME"},
    {"tvdevice", 0, 0, G_OPTION_ARG_STRING, &tv_device, N_("TV device name"), "DEVICE"},
    {"tvdriver", 0, 0, G_OPTION_ARG_STRING, &tv_driver, N_("TV driver name (v4l|v4l2)"), "DRIVER"},
    {"tvinput", 0, 0, G_OPTION_ARG_STRING, &tv_input, N_("TV input name"), "INPUT"},
    {"tvwidth", 0, 0, G_OPTION_ARG_INT, &tv_width, N_("Width of TV input"), "WIDTH"},
    {"tvheight", 0, 0, G_OPTION_ARG_INT, &tv_height, N_("Height of TV input"), "HEIGHT"},
    {"tvfps", 0, 0, G_OPTION_ARG_INT, &tv_fps, N_("Frames per second from TV input"), "FPS"},
    {"single_instance", 0, 0, G_OPTION_ARG_NONE, &single_instance, N_("Only allow one instance"),
     NULL},
    {"replace_and_play", 0, 0, G_OPTION_ARG_NONE, &replace_and_play,
     N_("Put single instance mode into replace and play mode"),
     NULL},
    {"large_buttons", 0, 0, G_OPTION_ARG_NONE, &large_buttons,
     N_("Use large control buttons"),
     NULL},
    {"always_hide_after_timeout", 0, 0, G_OPTION_ARG_NONE, &always_hide_after_timeout,
     N_("Hide control panel when mouse is not moving"),
     NULL},
    {"new_instance", 0, 0, G_OPTION_ARG_NONE, &new_instance,
     N_("Ignore single instance preference for this instance"),
     NULL},
    {"keep_on_top", 0, 0, G_OPTION_ARG_NONE, &keep_on_top,
     N_("Keep window on top"),
     NULL},
#ifdef HAVE_GPOD
    {"load_tracks_from_gpod", 0, 0, G_OPTION_ARG_NONE, &load_tracks_from_gpod,
     N_("Load all tracks from media player using gpod"),
     NULL},
#endif
    {"disable_cover_art_fetch", 0, 0, G_OPTION_ARG_NONE, &disable_cover_art_fetch,
     N_("Don't fetch new cover art images"),
     NULL},
    {NULL}
};

gint play_iter(GtkTreeIter * playiter, gint restart_second)
{

    ThreadData *thread_data = (ThreadData *) g_new0(ThreadData, 1);
    GtkWidget *dialog;
    gchar *error_msg = NULL;
    gchar *subtitle = NULL;
    GtkTreePath *path;
    gchar *local_file = NULL;
    gchar *uri = NULL;
    gint count, pos;
    gint playlist;
    gchar *title = NULL;
    gchar *artist = NULL;
    gchar *album = NULL;
    gchar *audio_codec;
    gchar *video_codec = NULL;
    gchar *demuxer = NULL;
    gint width;
    gint height;
    gint i;
    gpointer pixbuf;
    gchar *buffer = NULL;
    gchar *message = NULL;
    MetaData *metadata;
#ifdef GTK2_12_ENABLED
    GtkRecentData *recent_data;
#ifdef GIO_ENABLED
    GFile *file;
    GFileInfo *file_info;
#endif
#endif

    if (gtk_list_store_iter_is_valid(playliststore, playiter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), playiter, ITEM_COLUMN, &uri,
                           DESCRIPTION_COLUMN, &title,
                           ARTIST_COLUMN, &artist,
                           ALBUM_COLUMN, &album,
                           AUDIO_CODEC_COLUMN, &audio_codec,
                           VIDEO_CODEC_COLUMN, &video_codec,
                           VIDEO_WIDTH_COLUMN, &width,
                           VIDEO_HEIGHT_COLUMN, &height,
                           DEMUXER_COLUMN, &demuxer,
                           COVERART_COLUMN, &pixbuf,
                           SUBTITLE_COLUMN, &subtitle,
                           COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
        if (GTK_IS_TREE_SELECTION(selection)) {
            path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), playiter);
            if (path) {
                gtk_tree_selection_select_path(selection, path);
                if (GTK_IS_WIDGET(list))
                    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list), path, NULL, FALSE, 0, 0);
                buffer = gtk_tree_path_to_string(path);
                pos = (gint) g_strtod(buffer, NULL);
                g_free(buffer);
                gtk_tree_path_free(path);
            }
        }
        gtk_list_store_set(playliststore, playiter, COUNT_COLUMN, count + 1, -1);
    } else {
        if (verbose > 1)
            printf("iter is invalid, nothing to play\n");
        return 0;
    }

    if (verbose) {
        printf("playing - %s\n", uri);
        printf("is playlist %i\n", playlist);
    }

    mplayer_shutdown();

    while (state != QUIT) {
        gtk_main_iteration();
    }

    // wait for metadata to be available on this item
    if (!streaming_media(uri) && !device_name(uri)) {
        i = 0;
        while (demuxer == NULL && i < 10) {
            g_free(title);
            g_free(artist);
            g_free(album);
            g_free(audio_codec);
            g_free(video_codec);
            g_free(demuxer);
            g_free(subtitle);
            if (gtk_list_store_iter_is_valid(playliststore, playiter)) {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), playiter, LENGTH_VALUE_COLUMN, &i,
                                   DESCRIPTION_COLUMN, &title,
                                   ARTIST_COLUMN, &artist,
                                   ALBUM_COLUMN, &album,
                                   AUDIO_CODEC_COLUMN, &audio_codec,
                                   VIDEO_CODEC_COLUMN, &video_codec,
                                   VIDEO_WIDTH_COLUMN, &width,
                                   VIDEO_HEIGHT_COLUMN, &height,
                                   DEMUXER_COLUMN, &demuxer,
                                   COVERART_COLUMN, &pixbuf,
                                   SUBTITLE_COLUMN, &subtitle,
                                   COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
            } else {
                return 1;       // error condition
            }
            gtk_main_iteration();
            i++;
            if (demuxer == NULL)
                g_usleep(10000);
        }
    }
    // reset audio meter
    for (i = 0; i < METER_BARS; i++) {
        buckets[i] = 0;
        max_buckets[i] = 0;
    }

    message = g_strdup_printf("<small>\n");
    if (title == NULL) {
        title = g_filename_display_basename(uri);
    }
    buffer = g_markup_printf_escaped("\t<big><b>%s</b></big>\n", title);
    message = g_strconcat(message, buffer, NULL);
    g_free(buffer);

    if (artist != NULL) {
        buffer = g_markup_printf_escaped("\t<i>%s</i>\n", artist);
        message = g_strconcat(message, buffer, NULL);
        g_free(buffer);
    }
    if (album != NULL) {
        buffer = g_markup_printf_escaped("\t%s\n", album);
        message = g_strconcat(message, buffer, NULL);
        g_free(buffer);
    }
    //buffer = g_markup_printf_escaped("\n\t%s\n", uri);
    //message = g_strconcat(message, buffer, NULL);
    //g_free(buffer);

    message = g_strconcat(message, "</small>", NULL);

    // probably not much cover art for random video files
    if (pixbuf == NULL && video_codec == NULL && !streaming_media(uri) && control_id == 0
        && !playlist) {
        metadata = (MetaData *) g_new0(MetaData, 1);
        metadata->uri = g_strdup(uri);
        if (title != NULL)
            metadata->title = g_strstrip(g_strdup(title));
        if (artist != NULL)
            metadata->artist = g_strstrip(g_strdup(artist));
        if (album != NULL)
            metadata->album = g_strstrip(g_strdup(album));
        g_thread_create(get_cover_art, metadata, FALSE, NULL);
    } else {
        gtk_image_clear(GTK_IMAGE(cover_art));
    }

    g_strlcpy(idledata->media_info, message, 1024);
    g_strlcpy(idledata->display_name, title, 1024);
    g_free(message);

    message = gm_tempname(NULL, "mplayer-af_exportXXXXXX");
    g_strlcpy(idledata->af_export, message, 1024);
    g_free(message);

    message = g_strdup("");
    if (title == NULL) {
        title = g_filename_display_basename(uri);
    }
    buffer = g_markup_printf_escaped("\t<b>%s</b>\n", title);
    message = g_strconcat(message, buffer, NULL);
    g_free(buffer);

    if (artist != NULL) {
        buffer = g_markup_printf_escaped("\t<i>%s</i>\n", artist);
        message = g_strconcat(message, buffer, NULL);
        g_free(buffer);
    }
    if (album != NULL) {
        buffer = g_markup_printf_escaped("\t%s\n", album);
        message = g_strconcat(message, buffer, NULL);
        g_free(buffer);
    }
    g_strlcpy(idledata->media_notification, message, 1024);
    g_free(message);

    if (control_id == 0) {
        set_media_label(idledata);
    } else {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), FALSE);
    }

    gtk_container_forall(GTK_CONTAINER(menu_edit_sub_langs), remove_langs, NULL);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_select_sub_lang), FALSE);
    gtk_container_forall(GTK_CONTAINER(menu_edit_audio_langs), remove_langs, NULL);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_select_audio_lang), FALSE);
    lang_group = NULL;
    audio_group = NULL;

    local_file = get_localfile_from_uri(uri);
    if (local_file == NULL)
        return 0;

    g_strlcpy(thread_data->uri, uri, 2048);
    g_strlcpy(thread_data->filename, local_file, 1024);
    g_free(local_file);
    thread_data->done = FALSE;

    if (subtitle != NULL) {
        g_strlcpy(thread_data->subtitle, subtitle, 1024);
        g_free(subtitle);
    }
#ifdef HAVE_ASOUNDLIB
    if (!softvol && ao != NULL
        && (g_ascii_strcasecmp(ao, "alsa") == 0
            || (use_pulse_flat_volume && g_ascii_strcasecmp(ao, "pulse") == 0))) {
        volume = (gint) get_alsa_volume(TRUE);
        idledata->volume = volume;
#if GTK2_12_ENABLED
        gtk_scale_button_set_value(GTK_SCALE_BUTTON(vol_slider), volume);
#else
        gtk_range_set_value(GTK_RANGE(vol_slider), volume);
#endif
    }
#endif
#if GTK2_12_ENABLED
    volume = gtk_scale_button_get_value(GTK_SCALE_BUTTON(vol_slider));
#else
    volume = gtk_range_get_value(GTK_RANGE(vol_slider));
#endif

    if (g_ascii_strcasecmp(thread_data->filename, "") != 0) {
        if (!device_name(thread_data->filename) && !streaming_media(thread_data->filename)) {
            if (!g_file_test(thread_data->filename, G_FILE_TEST_EXISTS)) {
                error_msg = g_strdup_printf("%s not found\n", thread_data->filename);
                dialog =
                    gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", error_msg);
                gtk_window_set_title(GTK_WINDOW(dialog), "GNOME MPlayer Error");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                return 1;
            }
        }
    }
#ifdef GTK2_12_ENABLED
#ifdef GIO_ENABLED
    // don't put it on the recent list, if it is running in plugin mode
    if (control_id == 0 && !streaming_media(uri)) {
        recent_data = (GtkRecentData *) g_new0(GtkRecentData, 1);
        if (artist != NULL && strlen(artist) > 0) {
            recent_data->display_name = g_strdup_printf("%s - %s", artist, title);
        } else {
            recent_data->display_name = g_strdup(title);
        }
        g_strlcpy(idledata->display_name, recent_data->display_name, 1024);


        file = g_file_new_for_uri(uri);
        file_info = g_file_query_info(file,
                                      G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
                                      G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                      G_FILE_QUERY_INFO_NONE, NULL, NULL);


        if (file_info) {
            recent_data->mime_type = g_strdup(g_file_info_get_content_type(file_info));
            g_object_unref(file_info);
        }
        g_object_unref(file);
        recent_data->app_name = g_strdup("gnome-mplayer");
        recent_data->app_exec = g_strdup("gnome-mplayer %u");
        if (recent_data->mime_type != NULL) {
            gtk_recent_manager_add_full(recent_manager, uri, recent_data);
            g_free(recent_data->mime_type);
        }
        g_free(recent_data->app_name);
        g_free(recent_data->app_exec);
        g_free(recent_data);

    }
#endif
#endif
    g_free(title);
    g_free(artist);
    g_free(album);
    if (demuxer != NULL) {
        g_strlcpy(idledata->demuxer, demuxer, 64);
        g_free(demuxer);
    } else {
        g_strlcpy(idledata->demuxer, "", 64);
    }
    if (audio_codec != NULL) {
        g_strlcpy(idledata->audio_codec, audio_codec, 64);
        g_free(audio_codec);
    } else {
        g_strlcpy(idledata->audio_codec, "", 64);
    }
    if (video_codec != NULL) {
        g_strlcpy(idledata->video_codec, video_codec, 64);
        g_free(video_codec);
    } else {
        g_strlcpy(idledata->video_codec, "", 64);
    }

    if (lastfile != NULL) {
        g_free(lastfile);
        lastfile = NULL;
    }

    lastfile = g_strdup(thread_data->filename);

    last_x = 0;
    last_y = 0;
    idledata->width = width;
    idledata->height = height;
    if (width > 0 && height > 0) {
        idledata->videopresent = 1;
    } else {
        idledata->videopresent = 0;
    }

    if (idledata->audio_codec != NULL && strlen(idledata->audio_codec) > 0) {
        idledata->audiopresent = 1;
    } else {
        idledata->audiopresent = 0;
    }

    idledata->x = 0;
    idledata->y = 0;
    idledata->cachepercent = -1.0;
    g_strlcpy(idledata->info, uri, 1024);
    set_media_info(idledata);

    streaming = 0;
    subtitle_delay = 0.0;

    gm_store = gm_pref_store_new("gnome-mplayer");
    forcecache = gm_pref_store_get_boolean(gm_store, FORCECACHE);
    gm_pref_store_free(gm_store);

    if (thread_data->filename != NULL && strlen(thread_data->filename) != 0) {
        thread_data->player_window = 0;
        thread_data->playlist = playlist;
        thread_data->restart_second = restart_second;
		thread_data->start_second = start_second;
		thread_data->play_length = play_length;
        thread_data->streaming = streaming_media(thread_data->uri);
        idledata->streaming = thread_data->streaming;
        streaming = thread_data->streaming;
        g_strlcpy(idledata->video_format, "", 64);
        g_strlcpy(idledata->video_fps, "", 16);
        g_strlcpy(idledata->video_bitrate, "", 16);
        g_strlcpy(idledata->audio_bitrate, "", 16);
        g_strlcpy(idledata->audio_samplerate, "", 16);
        g_strlcpy(idledata->audio_channels, "", 16);
        idledata->has_chapters = FALSE;
        idledata->windowid = get_player_window();
        // these next 3 lines are here to make sure the window is available for mplayer to draw to
        // for some vo's (like xv) if the window is not visible and big enough the vo setup fails
        if (thread_data->streaming || thread_data->playlist) {
            idledata->videopresent = 1;
            gtk_widget_set_size_request(drawing_area, 16, 16);
            gtk_widget_show_all(fixed);
        } else {
            g_idle_add(resize_window, idledata);
        }

        if (g_ascii_strcasecmp(uri, "dvdnav://") == 0) {
            gtk_widget_show(menu_event_box);
        } else {
            gtk_widget_hide(menu_event_box);
        }
        while (gtk_events_pending())
            gtk_main_iteration();

        if (autostart) {
            g_idle_add(hide_buttons, idledata);
            js_state = STATE_PLAYING;
            while (gtk_events_pending())
                gtk_main_iteration();
            thread = g_thread_create(launch_player, thread_data, TRUE, NULL);
        }
        autostart = 1;
    }


    return 0;
}

static void hup_handler(int signum)
{
    // printf("handling signal %i\n",signum);
    delete_callback(NULL, NULL, NULL);
}

int main(int argc, char *argv[])
{
    struct stat buf;
    struct mntent *mnt = NULL;
    FILE *fp;
    gchar *uri;
    gint fileindex = 1;
    GError *error = NULL;
    GOptionContext *context;
    gint i;
    struct sigaction sa;

#ifdef GIO_ENABLED
    GFile *file;
#endif

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

    playlist = 0;
    embed_window = 0;
    control_id = 0;
    window_x = 0;
    window_y = 0;
    last_window_width = 0;
    last_window_height = 0;
    showcontrols = 1;
    showsubtitles = TRUE;
    autostart = 1;
    videopresent = 0;
    disable_context_menu = FALSE;
    dontplaynext = FALSE;
    idledata = (IdleData *) g_new0(IdleData, 1);
    idledata->videopresent = FALSE;
    idledata->volume = 100.0;
    idledata->mute = FALSE;
    idledata->length = 0.0;
    idledata->brightness = 0;
    idledata->contrast = 0;
    idledata->gamma = 0;
    idledata->hue = 0;
    idledata->saturation = 0;
    idledata->device = NULL;
    idledata->cachepercent = -1.0;
    g_strlcpy(idledata->video_format, "", 64);
    g_strlcpy(idledata->video_codec, "", 64);
    g_strlcpy(idledata->video_fps, "", 64);
    g_strlcpy(idledata->video_bitrate, "", 64);
    g_strlcpy(idledata->audio_codec, "", 64);
    g_strlcpy(idledata->audio_bitrate, "", 64);
    g_strlcpy(idledata->audio_samplerate, "", 64);
    selection = NULL;
    lastfile = NULL;
    path = NULL;
    js_state = STATE_UNDEFINED;
    control_instance = TRUE;
    playlistname = NULL;
    thread = NULL;
    rpconsole = NULL;
    subtitle = NULL;
    tv_device = NULL;
    tv_driver = NULL;
    tv_width = 0;
    tv_height = 0;
    tv_fps = 0;
    ok_to_play = TRUE;
    softvol = 0;
    alang = NULL;
    slang = NULL;
    metadata_codepage = NULL;
    playlistname = NULL;
    window_width = -1;
    window_height = -1;
    stored_window_width = -1;
    stored_window_height = -1;
    cache_size = 0;
    forcecache = FALSE;
    volume = -1;
    use_volume_option = FALSE;
    vertical_layout = FALSE;
    playlist_visible = FALSE;
    disable_fullscreen = FALSE;
    disable_framedrop = FALSE;
    softvol = FALSE;
    subtitlefont = NULL;
    subtitle_codepage = NULL;
    subtitle_color = NULL;
    subtitle_outline = FALSE;
    subtitle_shadow = FALSE;
    quit_on_complete = FALSE;
    slide_away = NULL;
    verbose = 0;
    reallyverbose = 0;
    embedding_disabled = FALSE;
    disable_pause_on_click = FALSE;
    disable_animation = FALSE;
    auto_hide_timeout = 3;
    use_mediakeys = TRUE;
    use_defaultpl = FALSE;
    mplayer_bin = NULL;
    single_instance = FALSE;
    disable_deinterlace = TRUE;
    details_visible = FALSE;
    replace_and_play = FALSE;
    bring_to_front = FALSE;
    keep_on_top = FALSE;
    resize_on_new_media = FALSE;
    use_pausing_keep_force = FALSE;
    show_notification = TRUE;
    show_status_icon = TRUE;
    lang_group = NULL;
    audio_group = NULL;
    gpod_mount_point = NULL;
    load_tracks_from_gpod = FALSE;
    disable_cover_art_fetch = FALSE;
    mixer = NULL;
    fullscreen = 0;
    ao = NULL;
    vo = NULL;
    use_pulse_flat_volume = FALSE;
    dvdnav_title_is_menu = FALSE;
    data = NULL;
    max_data = NULL;
    details_table = NULL;
    large_buttons = FALSE;
    button_size = 16;
    lastguistate = -1;
    non_fs_height = 0;
    non_fs_width = 0;
    use_hw_audio = FALSE;
	start_second = 0;
	play_length = 0;

    sa.sa_handler = hup_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;   /* Restart functions if
                                   interrupted by handler */
    if (sigaction(SIGINT, &sa, NULL) == -1)
        printf("SIGINT signal handler not installed\n");
    if (sigaction(SIGHUP, &sa, NULL) == -1)
        printf("SIGHUP signal handler not installed\n");
    if (sigaction(SIGTERM, &sa, NULL) == -1)
        printf("SIGTERM signal handler not installed\n");

    // call g_type_init or otherwise we can crash
    g_type_init();

    uri = g_strdup_printf("%s/gnome-mplayer/cover_art", g_get_user_config_dir());
    if (!g_file_test(uri, G_FILE_TEST_IS_DIR)) {
        g_mkdir_with_parents(uri, 0775);
    }
    g_free(uri);

    uri = g_strdup_printf("%s/gnome-mplayer/plugin", g_get_user_config_dir());
    if (!g_file_test(uri, G_FILE_TEST_IS_DIR)) {
        g_mkdir_with_parents(uri, 0775);
    }
    g_free(uri);
    uri = NULL;

    default_playlist =
        g_strdup_printf("file://%s/gnome-mplayer/default.pls", g_get_user_config_dir());
    safe_to_save_default_playlist = TRUE;

    gm_store = gm_pref_store_new("gnome-mplayer");
    gmp_store = gm_pref_store_new("gecko-mediaplayer");
    mixer = gm_pref_store_get_string(gm_store, MIXER);
    osdlevel = gm_pref_store_get_int(gm_store, OSDLEVEL);
    pplevel = gm_pref_store_get_int(gm_store, PPLEVEL);
#ifndef HAVE_ASOUNDLIB
    volume = gm_pref_store_get_int(gm_store, VOLUME);
#endif
    audio_channels = gm_pref_store_get_int(gm_store, AUDIO_CHANNELS);
    use_hw_audio = gm_pref_store_get_boolean(gm_store, USE_HW_AUDIO);
    fullscreen = gm_pref_store_get_boolean(gm_store, FULLSCREEN);
    softvol = gm_pref_store_get_boolean(gm_store, SOFTVOL);
    forcecache = gm_pref_store_get_boolean(gm_store, FORCECACHE);
    vertical_layout = gm_pref_store_get_boolean(gm_store, VERTICAL);
    playlist_visible = gm_pref_store_get_boolean(gm_store, SHOWPLAYLIST);
    details_visible = gm_pref_store_get_boolean(gm_store, SHOWDETAILS);
    show_notification = gm_pref_store_get_boolean(gm_store, SHOW_NOTIFICATION);
    show_status_icon = gm_pref_store_get_boolean(gm_store, SHOW_STATUS_ICON);
    disable_deinterlace = gm_pref_store_get_boolean(gm_store, DISABLEDEINTERLACE);
    disable_framedrop = gm_pref_store_get_boolean(gm_store, DISABLEFRAMEDROP);
    disable_fullscreen = gm_pref_store_get_boolean(gm_store, DISABLEFULLSCREEN);
    disable_context_menu = gm_pref_store_get_boolean(gm_store, DISABLECONTEXTMENU);
    disable_ass = gm_pref_store_get_boolean(gm_store, DISABLEASS);
    disable_embeddedfonts = gm_pref_store_get_boolean(gm_store, DISABLEEMBEDDEDFONTS);
    disable_pause_on_click = gm_pref_store_get_boolean(gm_store, DISABLEPAUSEONCLICK);
    disable_animation = gm_pref_store_get_boolean(gm_store, DISABLEANIMATION);
    auto_hide_timeout =
        gm_pref_store_get_int_with_default(gm_store, AUTOHIDETIMEOUT, auto_hide_timeout);
    disable_cover_art_fetch = gm_pref_store_get_boolean(gm_store, DISABLE_COVER_ART_FETCH);
    use_mediakeys = gm_pref_store_get_boolean_with_default(gm_store, USE_MEDIAKEYS, use_mediakeys);
    use_defaultpl = gm_pref_store_get_boolean_with_default(gm_store, USE_DEFAULTPL, use_defaultpl);
    metadata_codepage = gm_pref_store_get_string(gm_store, METADATACODEPAGE);
    subtitlefont = gm_pref_store_get_string(gm_store, SUBTITLEFONT);
    subtitle_scale = gm_pref_store_get_float(gm_store, SUBTITLESCALE);
    if (subtitle_scale < 0.25) {
        subtitle_scale = 1.0;
    }
    subtitle_codepage = gm_pref_store_get_string(gm_store, SUBTITLECODEPAGE);
    subtitle_color = gm_pref_store_get_string(gm_store, SUBTITLECOLOR);
    subtitle_outline = gm_pref_store_get_boolean(gm_store, SUBTITLEOUTLINE);
    subtitle_shadow = gm_pref_store_get_boolean(gm_store, SUBTITLESHADOW);
    subtitle_margin = gm_pref_store_get_int(gm_store, SUBTITLE_MARGIN);
    showsubtitles = gm_pref_store_get_boolean_with_default(gm_store, SHOW_SUBTITLES, TRUE);

    qt_disabled = gm_pref_store_get_boolean(gmp_store, DISABLE_QT);
    real_disabled = gm_pref_store_get_boolean(gmp_store, DISABLE_REAL);
    wmp_disabled = gm_pref_store_get_boolean(gmp_store, DISABLE_WMP);
    dvx_disabled = gm_pref_store_get_boolean(gmp_store, DISABLE_DVX);
    midi_disabled = gm_pref_store_get_boolean(gmp_store, DISABLE_MIDI);
    embedding_disabled = gm_pref_store_get_boolean(gmp_store, DISABLE_EMBEDDING);
    if (embed_window == 0) {
        single_instance = gm_pref_store_get_boolean(gm_store, SINGLE_INSTANCE);
        if (single_instance) {
            replace_and_play = gm_pref_store_get_boolean(gm_store, REPLACE_AND_PLAY);
            bring_to_front = gm_pref_store_get_boolean(gm_store, BRING_TO_FRONT);
        }
    }

    mplayer_bin = gm_pref_store_get_string(gm_store, MPLAYER_BIN);
    if (mplayer_bin != NULL && !g_file_test(mplayer_bin, G_FILE_TEST_EXISTS)) {
        g_free(mplayer_bin);
        mplayer_bin = NULL;
    }
    extraopts = gm_pref_store_get_string(gm_store, EXTRAOPTS);
    use_pulse_flat_volume = gm_pref_store_get_boolean(gm_store, USE_PULSE_FLAT_VOLUME);

    remember_loc = gm_pref_store_get_boolean(gm_store, REMEMBER_LOC);
    loc_window_x = gm_pref_store_get_int(gm_store, WINDOW_X);
    loc_window_y = gm_pref_store_get_int(gm_store, WINDOW_Y);
    loc_window_height = gm_pref_store_get_int(gm_store, WINDOW_HEIGHT);
    loc_window_width = gm_pref_store_get_int(gm_store, WINDOW_WIDTH);
    keep_on_top = gm_pref_store_get_boolean(gm_store, KEEP_ON_TOP);
    resize_on_new_media = gm_pref_store_get_boolean(gm_store, RESIZE_ON_NEW_MEDIA);
    read_mplayer_config();

    context = g_option_context_new(_("[FILES...] - GNOME Media player based on MPlayer"));
#ifdef GTK2_12_ENABLED
    g_option_context_set_translation_domain(context, "UTF-8");
    g_option_context_set_translate_func(context, (GTranslateFunc) gettext, NULL, NULL);
#endif
    g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);

    if (new_instance)
        single_instance = FALSE;

    if (verbose == 0)
        verbose = gm_pref_store_get_int(gm_store, VERBOSE);

    if (reallyverbose)
        verbose = 2;

    if (verbose)
        printf(_("GNOME MPlayer v%s\n"), VERSION);

    if (cache_size == 0)
        cache_size = gm_pref_store_get_int(gm_store, CACHE_SIZE);
    if (cache_size == 0)
        cache_size = 2000;
    gm_pref_store_free(gm_store);
    gm_pref_store_free(gmp_store);

    read_mplayer_config();

    if (verbose && single_instance) {
        printf("Running in single instance mode\n");
    }
#ifdef GIO_ENABLED
    if (verbose) {
        printf("Running with GIO support\n");
    }
#endif

    if (ao != NULL && g_ascii_strncasecmp(ao, "pulse", strlen("pulse")) == 0) {
        // do nothing
    } else {
        // we have alsa or pulse flat volume now
        use_pulse_flat_volume = TRUE;
    }

    if (volume == -1) {
        volume = (gint) get_alsa_volume(TRUE);
        if (!use_pulse_flat_volume) {
            if (ao != NULL && g_ascii_strncasecmp(ao, "pulse", strlen("pulse")) == 0) {
                if (verbose)
                    printf
                        ("Using pulse audio in non-flat volume mode, setting volume to max (will be limited by mixer 100%% of %i%%)\n",
                         volume);
                volume = 100;
            }
        }
    } else {
        if (verbose)
            printf("Using volume of %i from gnome-mplayer preference\n", volume);
    }

    if (softvol) {
        if (verbose)
            printf
                ("Using softvol, setting volume to max (will be limited by mixer 100%% of %i%%)\n",
                 volume);
        volume = 100;
    }

    if (volume >= 0 && volume <= 100) {
        idledata->volume = (gdouble) volume;
    }

    use_volume_option = detect_volume_option();

    if (large_buttons)
        button_size = 48;

    if (playlist_visible && control_id != 0)
        playlist_visible = FALSE;

    if (error != NULL) {
        printf("%s\n", error->message);
        printf(_
               ("Run 'gnome-mplayer --help' to see a full list of available command line options.\n"));
        return 1;
    }

    if (!g_thread_supported())
        g_thread_init(NULL);

    // if (verbose)
    //      printf("Threading support enabled = %i\n",g_thread_supported());

    if (rpconsole == NULL)
        rpconsole = g_strdup("NONE");

    // setup playliststore
    playliststore =
        gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT,
                           G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_STRING,
                           G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT,
                           G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_FLOAT, G_TYPE_FLOAT);

    create_window(embed_window);

    autopause = FALSE;
    state = QUIT;
    channel_in = NULL;
    channel_out = NULL;
    channel_err = NULL;

    thread_running = g_mutex_new();
    slide_away = g_mutex_new();
    mplayer_complete_cond = g_cond_new();
#ifdef GIO_ENABLED
    idledata->caching = g_mutex_new();
    idledata->caching_complete = g_cond_new();
#endif

    retrieve_metadata_pool = g_thread_pool_new(retrieve_metadata, NULL, 10, TRUE, NULL);

    if (argv[fileindex] != NULL) {
        g_stat(argv[fileindex], &buf);
        if (verbose) {
            printf("opening %s\n", argv[fileindex]);
            printf("is block %i\n", S_ISBLK(buf.st_mode));
            printf("is character %i\n", S_ISCHR(buf.st_mode));
            printf("is reg %i\n", S_ISREG(buf.st_mode));
            printf("is dir %i\n", S_ISDIR(buf.st_mode));
            printf("playlist %i\n", playlist);
            printf("embedded in window id 0x%x\n", embed_window);
        }
        if (S_ISBLK(buf.st_mode)) {
            // might have a block device, so could be a DVD

            fp = setmntent("/etc/mtab", "r");
            do {
                mnt = getmntent(fp);
                if (mnt)
                    //printf("%s is at %s\n",mnt->mnt_fsname,mnt->mnt_dir);
                    if (argv[fileindex] != NULL && mnt->mnt_fsname != NULL) {
                        if (strcmp(argv[fileindex], mnt->mnt_fsname) == 0)
                            break;
                    }
            }
            while (mnt);
            endmntent(fp);

            if (mnt) {
                printf("%s is mounted on %s\n", argv[fileindex], mnt->mnt_dir);
                uri = g_strdup_printf("%s/VIDEO_TS", mnt->mnt_dir);
                stat(uri, &buf);
                g_free(uri);
                if (S_ISDIR(buf.st_mode)) {
                    add_item_to_playlist("dvd://", 0);
                    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
                    play_iter(&iter, 0);
                } else {
                    uri = g_strdup_printf("file://%s", mnt->mnt_dir);
                    create_folder_progress_window();
                    add_folder_to_playlist_callback(uri, NULL);
                    g_free(uri);
                    destroy_folder_progress_window();
                    if (random_order) {
                        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
                        randomize_playlist(playliststore);
                    }
                    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
                        play_iter(&iter, 0);
                    }
                }
            } else {
                parse_cdda("cdda://");
                if (random_order) {
                    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
                    randomize_playlist(playliststore);
                }
                //play_file("cdda://", playlist);
                if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
                    play_iter(&iter, 0);
                }
            }
        } else if (S_ISDIR(buf.st_mode)) {
            create_folder_progress_window();
            uri = NULL;
#ifdef GIO_ENABLED
            file = g_file_new_for_commandline_arg(argv[fileindex]);
            if (file != NULL) {
                uri = g_file_get_uri(file);
                g_object_unref(file);
            }
#else
            uri = g_filename_to_uri(argv[fileindex], NULL, NULL);
#endif
            add_folder_to_playlist_callback(uri, NULL);
            g_free(uri);
            destroy_folder_progress_window();
            if (random_order) {
                gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
                randomize_playlist(playliststore);
            }
            if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
                play_iter(&iter, 0);
            }

        } else {
            // local file
            // detect if playlist here, so even if not specified it can be picked up
            i = fileindex;

            while (argv[i] != NULL) {
#ifdef GIO_ENABLED
                if (!device_name(argv[i])) {
                    file = g_file_new_for_commandline_arg(argv[i]);
                    if (file != NULL) {
                        uri = g_file_get_uri(file);
                        g_object_unref(file);
                    } else {
                        uri = g_strdup(argv[i]);
                    }
                } else {
                    uri = g_strdup(argv[i]);
                }
#else
                uri = g_filename_to_uri(argv[i], NULL, NULL);
#endif

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
                    g_free(uri);
                }
                i++;
            }

            if (random_order) {
                gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
                randomize_playlist(playliststore);
            }
            if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
                play_iter(&iter, 0);
            }
        }

    }
#ifdef HAVE_GPOD
    if (load_tracks_from_gpod) {
        gpod_mount_point = find_gpod_mount_point();
        printf("mount point is %s\n", gpod_mount_point);
        if (gpod_mount_point != NULL) {
            gpod_load_tracks(gpod_mount_point);
        } else {
            printf("Unable to find gpod mount point\n");
        }
    }
#endif


    dbus_hookup(embed_window, control_id);
    show_window(embed_window);

    if (argv[fileindex] == NULL && embed_window == 0) {
        use_remember_loc = remember_loc;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist),
                                       playlist_visible);
    } else {
        // prevents saving of a playlist with one item on it
        use_defaultpl = FALSE;
    }

    if (single_instance && embed_window == 0) {
        use_remember_loc = remember_loc;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist),
                                       playlist_visible);
    }

    safe_to_save_default_playlist = FALSE;
    if (use_defaultpl) {
        create_folder_progress_window();
        parse_playlist(default_playlist);
        destroy_folder_progress_window();
    }
    safe_to_save_default_playlist = TRUE;
                       
    gtk_main();

    return 0;
}
