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
    {"softvol", 0, 0, G_OPTION_ARG_NONE, &softvol, N_("Use mplayer software volume control"), NULL},
    {"volume", 0, 0, G_OPTION_ARG_INT, &volume, N_("Set initial volume percentage"), NULL},
    {"showcontrols", 0, 0, G_OPTION_ARG_INT, &showcontrols, N_("Show the controls in window"),
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
    {"new_instance", 0, 0, G_OPTION_ARG_NONE, &new_instance,
     N_("Ignore single instance preference for this instance"),
     NULL},
    {"keep_on_top", 0, 0, G_OPTION_ARG_NONE, &keep_on_top,
     N_("Keep window on top"),
     NULL},
    {NULL}
};



gint play_file(gchar * uri, gint playlist)
{

    ThreadData *thread_data = (ThreadData *) g_new0(ThreadData, 1);
    GtkWidget *dialog;
    gchar *error_msg = NULL;
    gchar *subtitle = NULL;
    GtkTreePath *path;
    gchar *local_file = NULL;

    if (verbose)
        printf("playing - %s\n", uri);

    shutdown();

    gtk_container_forall(GTK_CONTAINER(menu_edit_sub_langs), remove_langs, NULL);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_select_sub_lang), FALSE);
    gtk_container_forall(GTK_CONTAINER(menu_edit_audio_langs), remove_langs, NULL);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_select_audio_lang), FALSE);

    local_file = get_localfile_from_uri(uri);
    if (local_file == NULL)
        return 0;

    g_strlcpy(thread_data->uri, uri, 2048);
    g_strlcpy(thread_data->filename, local_file, 1024);
    g_free(local_file);
    thread_data->done = FALSE;

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, SUBTITLE_COLUMN, &subtitle, -1);
        if (GTK_IS_TREE_SELECTION(selection)) {
            path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &iter);
            gtk_tree_selection_select_path(selection, path);
            if (GTK_IS_WIDGET(list))
                gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list), path, NULL, FALSE, 0, 0);
            gtk_tree_path_free(path);
        }
    }
    if (subtitle != NULL) {
        g_strlcpy(thread_data->subtitle, subtitle, 1024);
        g_free(subtitle);
    }

    if (g_ascii_strcasecmp(thread_data->filename, "") != 0) {
        if (!device_name(thread_data->filename) && !streaming_media(thread_data->filename)) {
            if (!g_file_test(thread_data->filename, G_FILE_TEST_EXISTS)) {
                error_msg = g_strdup_printf("%s not found\n", thread_data->filename);
                dialog =
                    gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, error_msg);
                gtk_window_set_title(GTK_WINDOW(dialog), "GNOME MPlayer Error");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                return 1;
            }
        }
    }
#ifdef GTK2_12_ENABLED
    // don't put it on the recent list, if it is running in plugin mode
    if (control_id == 0) {
        gtk_recent_manager_add_item(recent_manager, uri);
    }
#endif

    if (lastfile != NULL) {
        g_free(lastfile);
        lastfile = NULL;
    }

    lastfile = g_strdup(thread_data->filename);

    last_x = 0;
    last_y = 0;
    idledata->width = 0;
    idledata->height = 0;
    idledata->x = 0;
    idledata->y = 0;

    streaming = 0;

    if (thread_data->filename != NULL && strlen(thread_data->filename) != 0) {
        thread_data->player_window = 0;
        thread_data->playlist = playlist;
        thread_data->streaming = streaming_media(thread_data->uri);
        idledata->streaming = thread_data->streaming;
        streaming = thread_data->streaming;
        g_strlcpy(idledata->video_format, "", 64);
        g_strlcpy(idledata->video_codec, "", 16);
        g_strlcpy(idledata->video_fps, "", 16);
        g_strlcpy(idledata->video_bitrate, "", 16);
        g_strlcpy(idledata->audio_codec, "", 16);
        g_strlcpy(idledata->audio_bitrate, "", 16);
        g_strlcpy(idledata->audio_samplerate, "", 16);
        g_strlcpy(idledata->audio_channels, "", 16);

        // these next 3 lines are here to make sure the window is available for mplayer to draw to
        // for some vo's (like xv) if the window is not visible and big enough the vo setup fails
        gtk_widget_set_size_request(drawing_area, 16, 16);
        gtk_widget_show_all(fixed);
        gtk_widget_hide(menu_event_box);
        while (gtk_events_pending())
            gtk_main_iteration();

        if (autostart) {
            g_idle_add(hide_buttons, idledata);
            js_state = STATE_PLAYING;
            thread = g_thread_create(launch_player, thread_data, TRUE, NULL);
        }
        autostart = 1;
    }

    return 0;
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
    gint i, count;
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
    autostart = 1;
    videopresent = 0;
    disable_context_menu = FALSE;
    dontplaynext = FALSE;
    idledata = (IdleData *) g_new0(IdleData, 1);
    idledata->videopresent = FALSE;
    idledata->volume = 100.0;
    idledata->length = 0.0;
    idledata->brightness = 0;
    idledata->contrast = 0;
    idledata->gamma = 0;
    idledata->hue = 0;
    idledata->saturation = 0;
    idledata->device = NULL;
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
    volume = 0;
    vertical_layout = FALSE;
    playlist_visible = FALSE;
    disable_fullscreen = FALSE;
    disable_framedrop = FALSE;
    softvol = FALSE;
    subtitlefont = NULL;
    subtitle_codepage = NULL;
    subtitle_color = NULL;
    quit_on_complete = FALSE;
    slide_away = NULL;
    verbose = 0;
    reallyverbose = 0;
    embedding_disabled = FALSE;
    disable_pause_on_click = FALSE;
    mplayer_bin = NULL;
    single_instance = FALSE;
    disable_deinterlace = TRUE;
    details_visible = FALSE;
    replace_and_play = FALSE;
    keep_on_top = FALSE;
    use_pausing_keep_force = FALSE;
    show_notification = TRUE;
    show_status_icon = TRUE;
    lang_group = NULL;
    audio_group = NULL;

    // call g_type_init or otherwise we can crash
    g_type_init();

    init_preference_store();
    osdlevel = read_preference_int(OSDLEVEL);
    pplevel = read_preference_int(PPLEVEL);
    volume = read_preference_int(VOLUME);
    softvol = read_preference_bool(SOFTVOL);
    forcecache = read_preference_bool(FORCECACHE);
    vertical_layout = read_preference_bool(VERTICAL);
    playlist_visible = read_preference_bool(SHOWPLAYLIST);
    details_visible = read_preference_bool(SHOWDETAILS);
    show_notification = read_preference_bool(SHOW_NOTIFICATION);
    show_status_icon = read_preference_bool(SHOW_STATUS_ICON);
    disable_deinterlace = read_preference_bool(DISABLEDEINTERLACE);
    disable_framedrop = read_preference_bool(DISABLEFRAMEDROP);
    disable_fullscreen = read_preference_bool(DISABLEFULLSCREEN);
    disable_context_menu = read_preference_bool(DISABLECONTEXTMENU);
    disable_ass = read_preference_bool(DISABLEASS);
    disable_embeddedfonts = read_preference_bool(DISABLEEMBEDDEDFONTS);
    disable_pause_on_click = read_preference_bool(DISABLEPAUSEONCLICK);
    metadata_codepage = read_preference_string(METADATACODEPAGE);
    subtitlefont = read_preference_string(SUBTITLEFONT);
    subtitle_scale = read_preference_float(SUBTITLESCALE);
    if (subtitle_scale < 0.25) {
        subtitle_scale = 1.0;
    }
    subtitle_codepage = read_preference_string(SUBTITLECODEPAGE);
    subtitle_color = read_preference_string(SUBTITLECOLOR);

    qt_disabled = read_preference_bool(DISABLE_QT);
    real_disabled = read_preference_bool(DISABLE_REAL);
    wmp_disabled = read_preference_bool(DISABLE_WMP);
    dvx_disabled = read_preference_bool(DISABLE_DVX);
    embedding_disabled = read_preference_bool(DISABLE_EMBEDDING);
    single_instance = read_preference_bool(SINGLE_INSTANCE);
    if (single_instance)
        replace_and_play = read_preference_bool(REPLACE_AND_PLAY);

    mplayer_bin = read_preference_string(MPLAYER_BIN);
    if (!g_file_test(mplayer_bin, G_FILE_TEST_EXISTS)) {
        g_free(mplayer_bin);
        mplayer_bin = NULL;
    }
    extraopts = read_preference_string(EXTRAOPTS);

    remember_loc = read_preference_bool(REMEMBER_LOC);
    loc_window_x = read_preference_int(WINDOW_X);
    loc_window_y = read_preference_int(WINDOW_Y);
    keep_on_top = read_preference_bool(KEEP_ON_TOP);

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
        verbose = read_preference_int(VERBOSE);

    if (reallyverbose)
        verbose = 2;

    if (verbose)
        printf(_("GNOME MPlayer v%s\n"), VERSION);

    if (cache_size == 0)
        cache_size = read_preference_int(CACHE_SIZE);
    if (cache_size == 0)
        cache_size = 2000;
    release_preference_store();

    if (verbose && single_instance) {
        printf("Running in single instance mode\n");
    }
#ifdef GIO_ENABLED
    if (verbose) {
        printf("Running with GIO support\n");
    }
#endif

    if (volume == 0 || volume == 100) {
        volume = (gint) get_alsa_volume();
    } else {
        if (verbose)
            printf("Using volume of %i from gnome-mplayer preference\n", volume);
    }

    if (volume > 0 && volume <= 100) {
        idledata->volume = (gdouble) volume;
    }

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

    if (rpconsole == NULL)
        rpconsole = g_strdup("NONE");

    // setup playliststore
    playliststore =
        gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT,
                           G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    nonrandomplayliststore =
        gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT,
                           G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    create_window(embed_window);

    fullscreen = 0;
    autopause = FALSE;
    state = QUIT;
    channel_in = NULL;
    channel_err = NULL;
    ao = NULL;
    vo = NULL;

    read_mplayer_config();
    thread_running = g_mutex_new();
    slide_away = g_mutex_new();
#ifdef GIO_ENABLED
    idledata->caching = g_mutex_new();
#endif

    if (argv[fileindex] != NULL) {
        g_stat(argv[fileindex], &buf);
        if (verbose) {
            printf("opening %s\n", argv[fileindex]);
            printf("is block %i\n", S_ISBLK(buf.st_mode));
            printf("is character %i\n", S_ISCHR(buf.st_mode));
            printf("is reg %i\n", S_ISREG(buf.st_mode));
            printf("is dir %i\n", S_ISDIR(buf.st_mode));
            printf("playlist %i\n", playlist);
            printf("embedded in window id %i\n", embed_window);
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
                if (S_ISDIR(buf.st_mode)) {
                    set_media_info_name(_("Playing DVD"));
                    play_file("dvd://", playlist);
                }
            } else {
                set_media_info_name(_("Playing Audio CD"));
                parse_cdda("cdda://");
                if (random_order) {
                    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
                    randomize_playlist(playliststore);
                }
                //play_file("cdda://", playlist);
                if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
                    gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &uri,
                                       COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
                    set_media_info_name(uri);
                    if (verbose)
                        printf("playing - %s is playlist = %i\n", uri, playlist);
                    play_file(uri, playlist);
                    gtk_list_store_set(playliststore, &iter, COUNT_COLUMN, count + 1, -1);
                    g_free(uri);
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
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &uri,
                                   COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
                set_media_info_name(uri);
                if (verbose)
                    printf("playing - %s is playlist = %i\n", uri, playlist);
                play_file(uri, playlist);
                gtk_list_store_set(playliststore, &iter, COUNT_COLUMN, count + 1, -1);
                g_free(uri);
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
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &uri,
                                   COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
                set_media_info_name(uri);
                if (verbose)
                    printf("playing - %s is playlist = %i\n", uri, playlist);
                play_file(uri, playlist);
                gtk_list_store_set(playliststore, &iter, COUNT_COLUMN, count + 1, -1);
                g_free(uri);
            }
        }

    } else {
        if (embed_window == 0) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist),
                                           playlist_visible);
        }
    }

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_details), details_visible);

    dbus_hookup(embed_window, control_id);

    gtk_main();

    return 0;
}
