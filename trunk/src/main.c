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
#include <mntent.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libintl.h>

//#include <bonobo.h>
// #include <gnome.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>
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
    {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, N_("Show more ouput on the console"), NULL},
    {"softvol", 0, 0, G_OPTION_ARG_NONE, &softvol, N_("Use mplayer software volume control"), NULL},
    {"volume", 0, 0, G_OPTION_ARG_INT, &volume, N_("Set initial volume percentage"), NULL},
    {"showcontrols", 0, 0, G_OPTION_ARG_INT, &showcontrols, N_("Show the controls in window"),
     "[0|1]"},
    {"autostart", 0, 0, G_OPTION_ARG_INT, &autostart,
     N_("Autostart the media default to 1, set to 0 to load but don't play"), "[0|1]"},
    {"disablecontextmenu", 0, 0, G_OPTION_ARG_NONE, &disable_context_menu,
     N_("Disable popup menu on right click"), NULL},
    {"loop", 0, 0, G_OPTION_ARG_NONE, &loop, N_("Play all files on the playlist forever"), NULL},
    {"random", 0, 0, G_OPTION_ARG_NONE, &random_order, N_("Play items on playlist in random order"),
     NULL},
    {"cache", 0, 0, G_OPTION_ARG_INT, &cache_size, N_("Set cache size"),
     NULL},
    {"forcecache", 0, 0, G_OPTION_ARG_NONE, &forcecache, N_("Force cache usage on streaming sites"),
     NULL},
	{"vertical", 0, 0, G_OPTION_ARG_NONE, &vertical_layout, N_("Use Vertical Layout"),
     NULL},	
	{"showplaylist", 0, 0, G_OPTION_ARG_NONE, &playlist_visible, N_("Start with playlist open"),
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
    {NULL}
};



gint play_file(gchar * filename, gint playlist)
{

    ThreadData *thread_data = (ThreadData *) g_new0(ThreadData, 1);
    GtkWidget *dialog;
    gchar *error_msg = NULL;
    gchar *subtitle = NULL;

    if (verbose)
        printf("playing - %s\n", filename);

    shutdown();
    g_strlcpy(thread_data->filename, filename, 1024);
	thread_data->done = FALSE;
	
    if (gtk_list_store_iter_is_valid(playliststore, &iter))
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, SUBTITLE_COLUMN, &subtitle, -1);

    if (subtitle != NULL) {
        g_strlcpy(thread_data->subtitle, subtitle, 1024);
    }

    if (g_ascii_strcasecmp(filename, "") != 0) {
        if (!device_name(filename)) {
            if (!streaming_media(filename)) {
                if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
                    error_msg = g_strdup_printf("%s not found\n", filename);
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
    }

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

    if (filename != NULL && strlen(filename) != 0) {
        thread_data->player_window = 0;
        thread_data->playlist = playlist;
        thread_data->streaming = streaming_media(thread_data->filename);
        idledata->streaming = thread_data->streaming;
        streaming = thread_data->streaming;

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
    gchar *filename;
    gint fileindex = 1;
    GError *error = NULL;
    GOptionContext *context;
    GConfClient *gconf;
    gint i, count;

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
    videopresent = 1;
    disable_context_menu = 0;
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

    // call g_type_init or otherwise we can crash
    g_type_init();
    gconf = gconf_client_get_default();
    osdlevel = gconf_client_get_int(gconf, OSDLEVEL, NULL);
    softvol = gconf_client_get_int(gconf, SOFTVOL, NULL);
    forcecache = gconf_client_get_bool(gconf, FORCECACHE, NULL);
    vertical_layout = gconf_client_get_bool(gconf, VERTICAL, NULL);
    playlist_visible = gconf_client_get_bool(gconf, SHOWPLAYLIST, NULL);
    qt_disabled = gconf_client_get_bool(gconf, DISABLE_QT, NULL);
    real_disabled = gconf_client_get_bool(gconf, DISABLE_REAL, NULL);
    wmp_disabled = gconf_client_get_bool(gconf, DISABLE_WMP, NULL);
    dvx_disabled = gconf_client_get_bool(gconf, DISABLE_DVX, NULL);

    context = g_option_context_new(_("- GNOME Media player based on MPlayer"));
    g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    g_option_context_parse(context, &argc, &argv, &error);

	if (cache_size == 0)
		cache_size = gconf_client_get_int(gconf, CACHE_SIZE, NULL);
    if (cache_size == 0)
        cache_size = 2000;
    g_object_unref(G_OBJECT(gconf));
	
	if (volume > 0 && volume <= 100) {
		idledata->volume = (gdouble)volume;
	}

	if (error != NULL) {
        printf("%s\n", error->message);
        printf(_
               ("Run 'gnome-mplayer --help' to see a full list of available command line options.\n"));
        return 1;
    }

    if (verbose == 0)
        verbose = gconf_client_get_int(gconf, VERBOSE, NULL);

    if (!g_thread_supported())
        g_thread_init(NULL);

    if (verbose)
        printf(_("GNOME MPlayer v%s\n"), VERSION);

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
                filename = g_strdup_printf("%s/VIDEO_TS", mnt->mnt_dir);
                stat(filename, &buf);
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
                    gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &filename,
                                       COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
                    set_media_info_name(filename);
                    if (verbose)
                        printf("playing - %s is playlist = %i\n", filename, playlist);
                    play_file(filename, playlist);
                    gtk_list_store_set(playliststore, &iter, COUNT_COLUMN, count + 1, -1);
                    g_free(filename);
                }
            }

        } else {
            // local file
            // detect if playlist here, so even if not specified it can be picked up
            i = fileindex;

            while (argv[i] != NULL) {
                if (playlist == 0)
                    playlist = detect_playlist(argv[i]);

                if (!playlist) {
                    add_item_to_playlist(argv[i], playlist);
                } else {
                    if (!parse_playlist(argv[i])) {
                        add_item_to_playlist(argv[i], playlist);
                    }

                }
                i++;
            }

            if (random_order) {
                gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
                randomize_playlist(playliststore);
            }
            if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter)) {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &filename,
                                   COUNT_COLUMN, &count, PLAYLIST_COLUMN, &playlist, -1);
                set_media_info_name(filename);
                if (verbose)
                    printf("playing - %s is playlist = %i\n", filename, playlist);
                play_file(filename, playlist);
                gtk_list_store_set(playliststore, &iter, COUNT_COLUMN, count + 1, -1);
                g_free(filename);
            }
        }
    } else {
		if (playlist_visible == TRUE && embed_window == 0)
			menuitem_view_playlist_callback(NULL,NULL);
	}		

    dbus_hookup(embed_window, control_id);

    gtk_main();

    return 0;
}
