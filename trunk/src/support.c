/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * support.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * support.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * support.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with support.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "support.h"

gint detect_playlist(gchar * uri)
{

    gint playlist = 0;
    gchar buffer[16 * 1024];
    gchar **output;
    GtkTreeViewColumn *column;
    gchar *coltitle;
    gint count;
    gchar *path = NULL;
    gchar *lower;
#ifdef GIO_ENABLED
    GFile *file;
    GFileInputStream *input;
    gchar *newuri;
#else
    FILE *fp;
    gchar *filename;
    gchar *file;
    gint size;

#endif


    if (g_ascii_strncasecmp(uri, "cdda://", strlen("cdda://")) == 0) {
        if (strlen(uri) > strlen("cdda://")) {
            playlist = 0;
        } else {
            playlist = 1;
        }
    } else if (g_ascii_strncasecmp(uri, "dvd://", strlen("dvd://")) == 0) {
        if (strlen(uri) > strlen("dvd://")) {
            playlist = 0;
        } else {
            playlist = 1;
        }
    } else if (g_ascii_strncasecmp(uri, "vcd://", strlen("vcd://")) == 0) {
        if (strlen(uri) > strlen("vcd://")) {
            playlist = 0;
        } else {
            playlist = 1;
        }
    } else if (g_strrstr(uri, ".m3u") != NULL
               || g_strrstr(uri, ".mxu") != NULL || g_strrstr(uri, ".m1u") != NULL || g_strrstr(uri, ".m4u") != NULL) {
        playlist = 1;
    } else if (device_name(uri)) {
        playlist = 0;
    } else {

#ifdef GIO_ENABLED
        if (!streaming_media(uri)) {
            gm_log(verbose, G_LOG_LEVEL_INFO, "opening playlist");
            file = g_file_new_for_uri(uri);
            path = gm_get_path(uri);
            input = g_file_read(file, NULL, NULL);
            if (input != NULL) {
                memset(buffer, 0, sizeof(buffer));
                g_input_stream_read((GInputStream *) input, buffer, sizeof(buffer), NULL, NULL);
                output = g_strsplit(buffer, "\n", 0);
                lower = g_ascii_strdown(buffer, -1);
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "buffer=%s", buffer);
                if (strstr(lower, "[playlist]") != 0
                    || strstr(lower, "[reference]") != 0
                    || strstr(lower, "<asx") != 0
                    || strstr(lower, "<smil>") != 0
                    || strstr(lower, "#extm3u") != 0
                    || strstr(lower, "#extm4u") != 0
                    || strstr(lower, "http://") != 0 || strstr(lower, "rtsp://") != 0 || strstr(lower, "pnm://") != 0) {
                    playlist = 1;
                }

                if (output[0] != NULL && uri_exists(output[0])) {
                    playlist = 1;
                }
                if (output[0] != NULL && playlist == 0) {
                    newuri = g_filename_to_uri(output[0], NULL, NULL);
                    if (newuri != NULL && uri_exists(newuri))
                        playlist = 1;
                    g_free(newuri);
                }

                if (output[0] != NULL && strlen(output[0]) > 0) {
                    newuri = g_strdup_printf("%s/%s", path, output[0]);
                    if (uri_exists(newuri)) {
                        playlist = 1;
                    }
                    g_free(newuri);
                }
                g_free(lower);
                g_strfreev(output);

                g_input_stream_close((GInputStream *) input, NULL, NULL);
            }
            g_object_unref(file);
            g_free(path);
        }
#else
        filename = g_filename_from_uri(uri, NULL, NULL);
        gm_log(verbose, G_LOG_LEVEL_DEBUG, "filename %s", filename);
        if (filename != NULL) {
            fp = fopen(filename, "r");
            if (path != NULL)
                g_free(path);
            path = gm_get_path(filename);

            if (fp != NULL) {
                if (!feof(fp)) {
                    memset(buffer, 0, sizeof(buffer));
                    size = fread(buffer, 1, sizeof(buffer) - 1, fp);
                    output = g_strsplit(buffer, "\n", 0);
                    lower = g_ascii_strdown(buffer, -1);
                    gm_log(verbose, G_LOG_LEVEL_DEBUG, "buffer=%s", buffer);
                    if (strstr(lower, "[playlist]") != 0) {
                        playlist = 1;
                    }

                    if (strstr(lower, "[reference]") != 0) {
                        playlist = 1;
                    }

                    if (g_ascii_strncasecmp(buffer, "#EXT", strlen("#EXT")) == 0) {
                        playlist = 1;
                    }

                    if (strstr(lower, "<asx") != 0) {
                        playlist = 1;
                    }

                    if (strstr(lower, "http://") != 0) {
                        playlist = 1;
                    }

                    if (strstr(lower, "rtsp://") != 0) {
                        playlist = 1;
                    }

                    if (strstr(lower, "pnm://") != 0) {
                        playlist = 1;
                    }
                    if (output[0] != NULL && g_file_test(output[0], G_FILE_TEST_EXISTS)) {
                        playlist = 1;
                    }

                    if (output[0] != NULL && strlen(output[0]) > 0) {
                        file = g_strdup_printf("%s/%s", path, output[0]);
                        if (g_file_test(file, G_FILE_TEST_EXISTS)) {
                            playlist = 1;
                        }
                        g_free(file);
                    }
                    g_free(lower);
                    g_strfreev(output);
                }
                fclose(fp);
                g_free(path);
            }
            g_free(filename);
        }
#endif
    }
    gm_log(verbose, G_LOG_LEVEL_INFO, "playlist detection = %i", playlist);
    if (!playlist) {
        if (playlistname != NULL)
            g_free(playlistname);
        playlistname = NULL;
        if (GTK_WIDGET(list)) {
            column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
            count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
            coltitle = g_strdup_printf(ngettext("Item to Play", "Items to Play", count));
            gtk_tree_view_column_set_title(column, coltitle);
            g_free(coltitle);
        }
    }

    return playlist;
}

gint parse_playlist(gchar * uri)
{
    gint ret = 0;
    GtkTreeViewColumn *column;
    gchar *coltitle;
    gint count = 0;
    gchar *basename;
    gchar **split;
    gchar *joined;

    // try and parse a playlist in various forms
    // if a parsing does not work then, return 0
    count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);

    ret = parse_basic(uri);
    if (ret != 1)
        ret = parse_ram(uri);
    if (ret != 1)
        ret = parse_asx(uri);
    if (ret != 1)
        ret = parse_cdda(uri);
    if (ret != 1)
        ret = parse_dvd(uri);
    if (ret != 1)
        ret = parse_vcd(uri);
    if (ret == 1 && count == 0) {
        if (playlistname != NULL) {
            g_free(playlistname);
            playlistname = NULL;
        }

        basename = g_path_get_basename(uri);
#ifdef GIO_ENABLED
        playlistname = g_uri_unescape_string(basename, NULL);
#else
        playlistname = g_strdup(basename);
#endif
        g_free(basename);

        if (playlistname != NULL && strlen(playlistname) > 0) {
            if (GTK_WIDGET(list)) {
                column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
                coltitle = g_strdup_printf(_("%s items"), playlistname);
                split = g_strsplit(coltitle, "_", 0);
                joined = g_strjoinv("__", split);
                gtk_tree_view_column_set_title(column, joined);
                g_free(coltitle);
                g_free(joined);
                g_strfreev(split);
            }
        }
    } else {
        g_free(playlistname);
        playlistname = NULL;
        if (GTK_WIDGET(list)) {
            column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
            count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
            coltitle = g_strdup_printf(ngettext("Item to Play", "Items to Play", count));
            gtk_tree_view_column_set_title(column, coltitle);
            g_free(coltitle);
        }
    }

#ifdef GTK2_12_ENABLED
    // don't put it on the recent list, if it is running in plugin mode
    if (control_id == 0 && ret == 1) {
        gtk_recent_manager_add_item(recent_manager, uri);
    }
#endif
    gm_log(verbose, G_LOG_LEVEL_INFO, "parse playlist = %i", ret);
    return ret;
}

// parse_basic covers .pls, .m3u, mxu and reference playlist types 
gint parse_basic(gchar * uri)
{

    if (device_name(uri))
        return 0;

    gchar *path = NULL;
    gchar *line = NULL;
    gchar *newline = NULL;
    gchar *line_uri = NULL;
    gchar **parse;
    gint playlist = 0;
    gint ret = 0;

    if (streaming_media(uri))
        return 0;

#ifdef GIO_ENABLED
    GFile *file;
    GFileInputStream *input;
    GDataInputStream *data;

    gsize length;
    file = g_file_new_for_uri(uri);
    path = gm_get_path(uri);
    input = g_file_read(file, NULL, NULL);
    data = g_data_input_stream_new((GInputStream *) input);
    if (data != NULL) {
        line = g_data_input_stream_read_line(data, &length, NULL, NULL);
        while (line != NULL) {
#else
    FILE *fp;
    gchar *file = NULL;

    file = g_strndup(uri, 4);
    if (strcmp(file, "file") != 0) {
        g_free(file);
        return 0;               // FIXME: remote playlists unsuppored
    }
    parse = g_strsplit(uri, "/", 3);
    path = gm_get_path(parse[2]);
    fp = fopen(parse[2], "r");
    g_strfreev(parse);
    line = g_new0(gchar, 1024);

    if (fp != NULL) {
        while (!feof(fp)) {
            memset(line, 0, sizeof(line));
            line = fgets(line, 1024, fp);
            if (line == NULL)
                continue;
#endif
            if (line != NULL)
                g_strstrip(line);
            if (strlen(line) == 0) {
#ifdef GIO_ENABLED
                g_free(line);
                line = g_data_input_stream_read_line(data, &length, NULL, NULL);
#endif
                continue;
            }
            gm_log(verbose, G_LOG_LEVEL_DEBUG, "line = %s", line);
            newline = g_strdup(line);
            if ((g_ascii_strncasecmp(line, "ref", 3) == 0) || (g_ascii_strncasecmp(line, "file", 4)) == 0) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "ref/file");
                parse = g_strsplit(line, "=", 2);
                if (parse != NULL && parse[1] != NULL) {
                    g_strstrip(parse[1]);
                    g_free(newline);
                    newline = g_strdup(parse[1]);
                }
                g_strfreev(parse);
            }
            if (g_ascii_strncasecmp(newline, "[playlist]", strlen("[playlist]")) == 0) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "playlist");
                //continue;
            } else if (g_ascii_strncasecmp(newline, "[reference]", strlen("[reference]")) == 0) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "ref");
                //continue;
                //playlist = 1;
            } else if (g_ascii_strncasecmp(newline, "<asx", strlen("<asx")) == 0) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "asx");
                //idledata->streaming = TRUE;
                g_free(newline);
                break;
            } else if (g_ascii_strncasecmp(newline, "<smil", strlen("<smil")) == 0) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "smil");
                g_free(newline);
                break;
            } else if (g_strrstr(newline, "<asx") != NULL) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "asx");
                //idledata->streaming = TRUE;
                g_free(newline);
                break;
            } else if (g_strrstr(newline, "<smil") != NULL) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "smil");
                g_free(newline);
                break;
            } else if (g_ascii_strncasecmp(newline, "numberofentries", strlen("numberofentries")) == 0) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "num");
                //continue;
            } else if (g_ascii_strncasecmp(newline, "version", strlen("version")) == 0) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "ver");
                //continue;
            } else if (g_ascii_strncasecmp(newline, "title", strlen("title")) == 0) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "title");
                //continue;
            } else if (g_ascii_strncasecmp(newline, "length", strlen("length")) == 0) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "len");
                //continue;
            } else if (g_ascii_strncasecmp(newline, "#extinf", strlen("#extinf")) == 0) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "#extinf");
                //continue;
            } else if (g_ascii_strncasecmp(newline, "#", strlen("#")) == 0) {
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "comment");
                //continue;
            } else {
                line_uri = g_strdup(newline);
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "newline = %s", newline);
                if (strstr(newline, "://") == NULL) {
                    if (newline[0] != '/') {
                        g_free(line_uri);
#ifdef GIO_ENABLED
                        line_uri = g_strdup_printf("%s/%s", path, newline);
#else
                        line_uri = g_strdup_printf("file://%s/%s", path, newline);
#endif
                    } else {
                        g_free(line_uri);
                        line_uri = g_strdup_printf("file://%s", newline);
                    }

                    if (uri_exists(line_uri)) {
                        add_item_to_playlist(line_uri, 0);
                        ret = 1;
                    }

                } else {
                    if (streaming_media(newline) || device_name(newline)) {
                        gm_log(verbose, G_LOG_LEVEL_DEBUG, "URL %s", newline);
                        add_item_to_playlist(newline, playlist);
                        ret = 1;
                    } else {
                        if (uri_exists(line_uri)) {
                            add_item_to_playlist(line_uri, 0);
                            ret = 1;
                        }
                    }
                }
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "line_uri = %s", line_uri);
                g_free(line_uri);
            }
            g_free(newline);
#ifdef GIO_ENABLED
            g_free(line);
            line = g_data_input_stream_read_line(data, &length, NULL, NULL);
        }
        g_input_stream_close((GInputStream *) data, NULL, NULL);
        g_input_stream_close((GInputStream *) input, NULL, NULL);
    }
    g_object_unref(file);
#else
        }
        fclose(fp);
    }
    g_free(file);
#endif
    g_free(path);
    return ret;
}

gint parse_ram(gchar * filename)
{

    gint ac = 0;
    gint ret = 0;
    gchar **output;
    FILE *fp;
    gchar *buffer;

    fp = fopen(filename, "r");
    buffer = g_new0(gchar, 1024);

    if (fp != NULL) {
        while (!feof(fp)) {
            memset(buffer, 0, sizeof(buffer));
            buffer = fgets(buffer, 1024, fp);
            if (buffer != NULL) {
                output = g_strsplit(buffer, "\r", 0);
                ac = 0;
                while (output[ac] != NULL) {
                    g_strchomp(output[ac]);
                    g_strchug(output[ac]);
                    if (g_ascii_strncasecmp(output[ac], "rtsp://", strlen("rtsp://")) == 0) {
                        ret = 1;
                        add_item_to_playlist(output[ac], 0);
                    } else if (g_ascii_strncasecmp(output[ac], "pnm://", strlen("pnm://")) == 0) {
                        ret = 1;
                        add_item_to_playlist(output[ac], 0);
                    }
                    ac++;
                }
                g_strfreev(output);
            }
            if (ret != 1)
                break;
        }
        fclose(fp);
    }
    g_free(buffer);
    buffer = NULL;

    return ret;
}

gint parse_asx(gchar * uri)
{
    gint ret = 0;

    if (device_name(uri))
        return 0;

    if (streaming_media(uri))
        return 0;

    if (gm_parse_asx_is_asx(uri)) {
        ret = 1;
#ifdef GIO_ENABLED
        gchar *line;
        GFile *file;
        GFileInputStream *input;
        GDataInputStream *data;
        gchar *asx_data = g_new0(gchar, 16 * 1024);

        gsize length;
        file = g_file_new_for_uri(uri);
        input = g_file_read(file, NULL, NULL);
        data = g_data_input_stream_new((GInputStream *) input);
        if (data != NULL) {
            line = g_data_input_stream_read_line(data, &length, NULL, NULL);
            while (line != NULL) {
                g_strlcat(asx_data, line, 16 * 1024);
                g_free(line);
                line = g_data_input_stream_read_line(data, &length, NULL, NULL);
            }
            g_input_stream_close((GInputStream *) data, NULL, NULL);
            g_input_stream_close((GInputStream *) input, NULL, NULL);
        }
        g_object_unref(file);

        gm_parse_asx(asx_data, &add_item_to_playlist_callback, NULL);
        g_free(asx_data);
#else
        gchar *filename;
        gchar *contents = NULL;

        filename = g_filename_from_uri(uri, NULL, NULL);
        g_file_get_contents(filename, &contents, NULL, NULL);
        if (contents != NULL) {
            gm_parse_asx(contents, &add_item_to_playlist_callback, NULL);
            g_free(contents);
        }
        g_free(filename);
#endif
    }

    return ret;
}

gint parse_cdda(gchar * filename)
{

    GError *error;
    gint exit_status;
    gchar *out = NULL;
    gchar *err = NULL;
    gchar *av[255];
    gint ac = 0, i;
    gint ret = 0;
    gchar **output;
    gchar *track = NULL;
    gchar *title = NULL;
    gchar *artist = NULL;
    gchar *length = NULL;
    gchar *ptr = NULL;
    gint num;
    GtkTreeIter localiter;
    gboolean ok;
    gint addcount = 0;

    if (g_ascii_strncasecmp(filename, "cdda://", 7) != 0) {
        return 0;
    } else {
        playlist = 0;
        if (mplayer_bin == NULL || !g_file_test(mplayer_bin, G_FILE_TEST_EXISTS)) {
            av[ac++] = g_strdup_printf("mplayer");
        } else {
            av[ac++] = g_strdup_printf("%s", mplayer_bin);
        }
        av[ac++] = g_strdup_printf("-vo");
        av[ac++] = g_strdup_printf("null");
        av[ac++] = g_strdup_printf("-ao");
        av[ac++] = g_strdup_printf("null");
        av[ac++] = g_strdup_printf("-nomsgcolor");
        av[ac++] = g_strdup_printf("-nomsgmodule");
        av[ac++] = g_strdup_printf("-frames");
        av[ac++] = g_strdup_printf("0");
        av[ac++] = g_strdup_printf("-identify");
        av[ac++] = g_strdup_printf("cddb://");
        av[ac++] = g_strdup_printf("cdda://");
        if (mplayer_dvd_device != NULL && strlen(mplayer_dvd_device) > 0) {
            av[ac++] = g_strdup_printf("-cdrom-device");
            av[ac++] = g_strdup_printf("%s", mplayer_dvd_device);
        }
        av[ac] = NULL;

        error = NULL;

        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status, &error);
        for (i = 0; i < ac; i++) {
            g_free(av[i]);
        }
        if (error != NULL) {
            gm_log(verbose, G_LOG_LEVEL_MESSAGE, "Error when running: %s", error->message);
            g_error_free(error);
            if (out != NULL)
                g_free(out);
            if (err != NULL)
                g_free(err);
            error = NULL;
        }
        output = g_strsplit(out, "\n", 0);
        ac = 0;
        while (output[ac] != NULL) {

            if (g_ascii_strncasecmp(output[ac], " artist", strlen(" artist")) == 0) {
                artist = g_strdup_printf("%s", output[ac] + strlen(" artist=["));
                ptr = g_strrstr(artist, "]");
                if (ptr != NULL)
                    ptr[0] = '\0';
            }

            if (g_ascii_strncasecmp(output[ac], " album", strlen(" album")) == 0) {
                playlistname = g_strdup_printf("%s", output[ac] + strlen(" album=["));
                ptr = g_strrstr(playlistname, "]");
                if (ptr != NULL)
                    ptr[0] = '\0';
            }

            if (g_ascii_strncasecmp(output[ac], "  #", 3) == 0) {
                sscanf(output[ac], "  #%i", &num);
                track = g_strdup_printf("cdda://%i", num);
                title = g_strdup_printf("%s", output[ac] + 26);
                ptr = g_strrstr(title, "]");
                if (ptr != NULL)
                    ptr[0] = '\0';

                length =
                    g_strdup_printf("%c%c%c%c%c%c%c%c", output[ac][7], output[ac][8], output[ac][9],
                                    output[ac][10], output[ac][11], output[ac][12], output[ac][13], output[ac][14]);

                ok = FALSE;
                if (strlen(filename) > 7) {
                    if (g_strcasecmp(filename, track) == 0) {
                        ok = TRUE;
                    }
                } else {
                    ok = TRUE;
                }

                if (ok) {
                    gm_log(verbose, G_LOG_LEVEL_DEBUG, "track = %s, artist = %s, album = %s, title = %s, length = %s",
                           track, artist, playlistname, title, length);
                    gtk_list_store_append(playliststore, &localiter);
                    gtk_list_store_set(playliststore, &localiter, ITEM_COLUMN, track,
                                       DESCRIPTION_COLUMN, title,
                                       COUNT_COLUMN, 0,
                                       PLAYLIST_COLUMN, 0,
                                       ARTIST_COLUMN, artist,
                                       ALBUM_COLUMN, playlistname,
                                       SUBTITLE_COLUMN, NULL, LENGTH_COLUMN, length,
                                       PLAY_ORDER_COLUMN,
                                       gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore),
                                                                      NULL), ADD_ORDER_COLUMN,
                                       gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL), -1);


                    addcount++;
                }
                if (track != NULL) {
                    g_free(track);
                    track = NULL;
                }
                if (title != NULL) {
                    g_free(title);
                    title = NULL;
                }
                if (length != NULL) {
                    g_free(length);
                    length = NULL;
                }
            }
            ac++;
        }

        if (addcount == 0) {
            ac = 0;
            while (output[ac] != NULL) {
                if (g_ascii_strncasecmp(output[ac], "ID_CDDA_TRACK__", strlen("ID_CDDA_TRACK_")) == 0) {
                    sscanf(output[ac], "ID_CDDA_TRACK_%i", &num);
                    track = g_strdup_printf("cdda://%i", num);
                    add_item_to_playlist(track, 0);
                    g_free(track);
                }
                ac++;
            }
        }

        g_strfreev(output);
        if (out != NULL)
            g_free(out);
        if (err != NULL)
            g_free(err);

        if (artist != NULL) {
            g_free(artist);
            artist = NULL;
        }


        ret = 1;
    }

    return ret;
}

// This function pulls the playlist for dvd and dvdnav
gint parse_dvd(gchar * filename)
{

    GError *error;
    gint exit_status;
    gchar *out;
    gchar *err;
    gchar *av[255];
    gint ac = 0, i;
    gint ret = 0;
    gchar **output;
    gchar *track;
    gint num;
    gchar *error_msg = NULL;
    GtkWidget *dialog;

    if (g_ascii_strncasecmp(filename, "dvd://", strlen("dvd://")) == 0) {       // || g_ascii_strncasecmp(filename,"dvdnav://",strlen("dvdnav://")) == 0) {
        playlist = 0;
        // run mplayer and try to get the first frame and convert it to a jpeg
        if (mplayer_bin == NULL || !g_file_test(mplayer_bin, G_FILE_TEST_EXISTS)) {
            av[ac++] = g_strdup_printf("mplayer");
        } else {
            av[ac++] = g_strdup_printf("%s", mplayer_bin);
        }
        av[ac++] = g_strdup_printf("-vo");
        av[ac++] = g_strdup_printf("null");
        av[ac++] = g_strdup_printf("-ao");
        av[ac++] = g_strdup_printf("null");
        av[ac++] = g_strdup_printf("-nomsgcolor");
        av[ac++] = g_strdup_printf("-nomsgmodule");
        av[ac++] = g_strdup_printf("-frames");
        av[ac++] = g_strdup_printf("0");
        av[ac++] = g_strdup_printf("-identify");
        if (idledata->device != NULL && strlen(idledata->device) > 0) {
            av[ac++] = g_strdup_printf("-dvd-device");
            av[ac++] = g_strdup_printf("%s", idledata->device);
        } else {
            if (mplayer_dvd_device != NULL && strlen(mplayer_dvd_device) > 0) {
                av[ac++] = g_strdup_printf("-dvd-device");
                av[ac++] = g_strdup_printf("%s", mplayer_dvd_device);
            }
        }
        av[ac++] = g_strdup_printf("dvd://");
        av[ac] = NULL;

        error = NULL;

        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status, &error);
        for (i = 0; i < ac; i++) {
            g_free(av[i]);
        }
        if (error != NULL) {
            gm_log(verbose, G_LOG_LEVEL_MESSAGE, "Error when running: %s", error->message);
            g_error_free(error);
            if (out != NULL)
                g_free(out);
            if (err != NULL)
                g_free(err);
            error = NULL;
        }
        output = g_strsplit(out, "\n", 0);
        if (strstr(err, "Couldn't open DVD device:") != NULL) {
            error_msg = g_strdup_printf(_("Couldn't open DVD device: %s"), err + strlen("Couldn't open DVD device: "));
        }
        ac = 0;
        while (output[ac] != NULL) {
            if (g_ascii_strncasecmp(output[ac], "ID_DVD_TITLE_", strlen("ID_DVD_TITLE_")) == 0) {
                if (strstr(output[ac], "LENGTH") != NULL) {
                    sscanf(output[ac], "ID_DVD_TITLE_%i", &num);
                    track = g_strdup_printf("dvd://%i", num);
                    add_item_to_playlist(track, 0);
                    g_free(track);
                }
            }
            ac++;

        }
        g_strfreev(output);
        if (error_msg != NULL) {
            dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_CLOSE, "%s", error_msg);
            gtk_window_set_title(GTK_WINDOW(dialog), _("GNOME MPlayer Error"));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            g_free(error_msg);
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        return 0;
    }

    return ret;
}

// This function pulls the playlist for dvd and dvdnav
gint parse_vcd(gchar * filename)
{

    GError *error;
    gint exit_status;
    gchar *out;
    gchar *err;
    gchar *av[255];
    gint ac = 0, i;
    gint ret = 0;
    gchar **output;
    gchar *track;
    gint num;
    gchar *error_msg = NULL;
    GtkWidget *dialog;

    if (g_ascii_strncasecmp(filename, "vcd://", strlen("vcd://")) == 0) {
        playlist = 0;

        if (mplayer_bin == NULL || !g_file_test(mplayer_bin, G_FILE_TEST_EXISTS)) {
            av[ac++] = g_strdup_printf("mplayer");
        } else {
            av[ac++] = g_strdup_printf("%s", mplayer_bin);
        }
        av[ac++] = g_strdup_printf("-vo");
        av[ac++] = g_strdup_printf("null");
        av[ac++] = g_strdup_printf("-ao");
        av[ac++] = g_strdup_printf("null");
        av[ac++] = g_strdup_printf("-nomsgcolor");
        av[ac++] = g_strdup_printf("-nomsgmodule");
        av[ac++] = g_strdup_printf("-frames");
        av[ac++] = g_strdup_printf("0");
        av[ac++] = g_strdup_printf("-identify");
        if (idledata->device != NULL && strlen(idledata->device) > 0) {
            av[ac++] = g_strdup_printf("-dvd-device");
            av[ac++] = g_strdup_printf("%s", idledata->device);
        }
        av[ac++] = g_strdup_printf("vcd://");
        av[ac] = NULL;

        error = NULL;

        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status, &error);
        for (i = 0; i < ac; i++) {
            g_free(av[i]);
        }
        if (error != NULL) {
            gm_log(verbose, G_LOG_LEVEL_MESSAGE, "Error when running: %s", error->message);
            g_error_free(error);
            if (out != NULL)
                g_free(out);
            if (err != NULL)
                g_free(err);
            error = NULL;
        }
        output = g_strsplit(out, "\n", 0);
        if (strstr(err, "Couldn't open VCD device:") != NULL) {
            error_msg = g_strdup_printf(_("Couldn't open VCD device: %s"), err + strlen("Couldn't open VCD device: "));
        }
        ac = 0;
        while (output[ac] != NULL) {
            if (g_ascii_strncasecmp(output[ac], "ID_VCD_TRACK_", strlen("ID_VCD_TRACK_")) == 0) {
                sscanf(output[ac], "ID_VCD_TRACK_%i", &num);
                track = g_strdup_printf("vcd://%i", num);
                gm_log(verbose, G_LOG_LEVEL_MESSAGE, "adding track %s", track);
                add_item_to_playlist(track, 0);
                g_free(track);
            }
            ac++;
        }
        g_strfreev(output);

        if (error_msg != NULL) {
            dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_CLOSE, "%s", error_msg);
            gtk_window_set_title(GTK_WINDOW(dialog), _("GNOME MPlayer Error"));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            g_free(error_msg);
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        return 0;
    }

    return ret;
}

gboolean streaming_media(gchar * uri)
{
    gboolean ret;
    gchar *local_file = NULL;
#ifdef GIO_ENABLED
    GFile *file;
    GFileInfo *info;
#endif

    ret = TRUE;
    if (uri == NULL)
        return FALSE;
    if (device_name(uri)) {
        ret = FALSE;
    } else if (g_ascii_strncasecmp(uri, "http://", strlen("http://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(uri, "mmst://", strlen("mmst://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(uri, "mms://", strlen("mms://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(uri, "mmshttp://", strlen("mmshttp://")) == 0) {
        ret = TRUE;
    } else {
#ifdef GIO_ENABLED
        local_file = g_filename_from_uri(uri, NULL, NULL);
        if (local_file != NULL) {
            ret = !g_file_test(local_file, G_FILE_TEST_EXISTS);
            g_free(local_file);
        } else {
            file = g_file_new_for_uri(uri);
            if (file != NULL) {
                info = g_file_query_info(file, "*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
                if (info != NULL) {
                    // SMB filesystems seem to give incorrect access answers over GIO, 
                    // so if the file has a filesize > 0 we think it is not streaming
                    if (g_ascii_strncasecmp(uri, "smb://", strlen("smb://")) == 0) {
                        ret = (g_file_info_get_size(info) > 0) ? FALSE : TRUE;
                    } else {
                        ret = !g_file_info_get_attribute_boolean(info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
                    }
                    g_object_unref(info);
                } else {
                    ret = !g_file_test(uri, G_FILE_TEST_EXISTS);
                }
            }
            g_object_unref(file);
        }
#else
        local_file = g_filename_from_uri(uri, NULL, NULL);
        if (local_file != NULL) {
            ret = !g_file_test(local_file, G_FILE_TEST_EXISTS);
            g_free(local_file);
        } else {
            ret = !g_file_test(uri, G_FILE_TEST_EXISTS);
        }
#endif
    }
    gm_log(verbose, G_LOG_LEVEL_DEBUG, "Streaming media '%s' = %i", uri, ret);
    return ret;
}

gboolean device_name(gchar * filename)
{
    gboolean ret;

    ret = TRUE;

    if (g_ascii_strncasecmp(filename, "dvd://", strlen("dvd://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(filename, "dvdnav://", strlen("dvdnav://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(filename, "cdda://", strlen("cdda://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(filename, "cddb://", strlen("cddb://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(filename, "tv://", strlen("tv://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(filename, "dvb://", strlen("dvb://")) == 0) {
        ret = TRUE;
    } else if (g_ascii_strncasecmp(filename, "vcd://", strlen("vcd://")) == 0) {
        ret = TRUE;
    } else {
        ret = FALSE;
    }
    if (ret) {
        gm_log(verbose, G_LOG_LEVEL_DEBUG, "%s is a device name", filename);
    } else {
        gm_log(verbose, G_LOG_LEVEL_DEBUG, "%s is not a device name", filename);
    }
    return ret;
}

gchar *metadata_to_utf8(gchar * string)
{
    if (metadata_codepage != NULL && strlen(metadata_codepage) > 1) {
        // zh_TW usually use BIG5 on tags, if the file is from Windows
        if (g_utf8_validate(string, strlen(string), NULL) == FALSE) {
            return g_convert(string, -1, "UTF-8", metadata_codepage, NULL, NULL, NULL);
        }
    }

    return g_locale_to_utf8(string, -1, NULL, NULL, NULL);
}

void retrieve_metadata(gpointer data, gpointer user_data)
{
    gchar *uri = (gchar *) data;
    MetaData *mdata = NULL;

    gm_log_name_this_thread("rm");
    gm_log(FALSE, G_LOG_LEVEL_DEBUG, "retrieve_metadata(%s)", uri);

    gm_log(FALSE, G_LOG_LEVEL_DEBUG, "locking retrieve_mutex");
    g_mutex_lock(retrieve_mutex);
    mdata = get_metadata(uri);
    if (mdata != NULL) {
        mdata->uri = g_strdup(uri);
        g_idle_add(set_metadata, (gpointer) mdata);
    }
    g_free(data);
    gm_log(FALSE, G_LOG_LEVEL_DEBUG, "unlocking retrieve_mutex");
    g_mutex_unlock(retrieve_mutex);
}

MetaData *get_basic_metadata(gchar * uri)
{
    gchar *title = NULL;
    gchar *artist = NULL;
    gchar *album = NULL;
    gchar *length = NULL;
    gchar *name = NULL;
    gchar *basename = NULL;
    gchar *audio_codec = NULL;
    gchar *video_codec = NULL;
    gchar *demuxer = NULL;
    gint width = 0;
    gint height = 0;
    gfloat seconds = 0;
    gchar *localtitle = NULL;
    MetaData *ret = NULL;
    gchar *localuri = NULL;
    gchar *p = NULL;
#ifdef GIO_ENABLED
    GFile *file;
#endif

    if (ret == NULL)
        ret = (MetaData *) g_new0(MetaData, 1);

    if (device_name(uri)) {
        name = g_strdup(uri);
    } else {
#ifdef GIO_ENABLED
        file = g_file_new_for_uri(uri);
        if (file != NULL) {
            name = g_file_get_path(file);
            basename = g_file_get_basename(file);
            g_object_unref(file);
        }
#else
        name = g_filename_from_uri(uri, NULL, NULL);
        basename = g_filename_display_basename(name);
#endif
    }

    if (name == NULL) {
        if (ret != NULL)
            g_free(ret);
        return NULL;
    }

    if (title == NULL || strlen(title) == 0) {
        localuri = g_strdup(uri);
        p = g_strrstr(localuri, ".");
        if (p)
            p[0] = '\0';
        p = g_strrstr(localuri, "/");
        if (p) {
            artist = g_strdup(p + 1);
            p = strstr(artist, " - ");
            if (p) {
                title = g_strdup(p + 3);
                p[0] = '\0';
            }
        }
        g_free(localuri);
    }

    if (title == NULL || strlen(title) == 0) {
        g_free(title);
        title = g_strdup(basename);
    }

    if (title == NULL && g_ascii_strncasecmp(name, "dvd://", strlen("dvd://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        title = g_strdup_printf("DVD Track %s", localtitle);
    }

    if (title == NULL && g_ascii_strncasecmp(name, "tv://", strlen("tv://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        title = g_strdup_printf("%s", localtitle);
    }

    if (title == NULL && g_ascii_strncasecmp(name, "dvb://", strlen("dvb://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        title = g_strdup_printf("%s", localtitle);
    }

    if (title == NULL && g_ascii_strncasecmp(name, "vcd://", strlen("vcd://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        title = g_strdup_printf("%s", localtitle);
    }

    if (title == NULL && g_ascii_strncasecmp(name, "dvdnav://", strlen("dvdnav://")) == 0) {
        title = g_strdup_printf("DVD");
    }

    if (ret != NULL) {
        ret->uri = g_strdup(uri);
        ret->title = g_strdup(title);
        ret->artist = g_strdup(artist);
        ret->album = g_strdup(album);
        ret->length = g_strdup(length);
        ret->length_value = seconds;
        ret->audio_codec = g_strdup(audio_codec);
        ret->video_codec = g_strdup(video_codec);
        ret->demuxer = g_strdup(demuxer);
        ret->width = width;
        ret->height = height;
    }

    g_free(title);
    g_free(artist);
    g_free(album);
    g_free(length);
    g_free(audio_codec);
    g_free(video_codec);
    g_free(demuxer);
    g_free(name);
    g_free(basename);

    return ret;
}

MetaData *get_metadata(gchar * uri)
{
    gchar *title = NULL;
    gchar *artist = NULL;
    gchar *album = NULL;
    gchar *length = NULL;
    gchar *name = NULL;
    gchar *basename = NULL;
    gchar *audio_codec = NULL;
    gchar *video_codec = NULL;
    gchar *demuxer = NULL;
    gint width = 0;
    gint height = 0;
    GError *error;
    gint exit_status;
    gchar *out = NULL;
    gchar *err = NULL;
    gchar *av[255];
    gint ac = 0, i;
    gchar **output;
    gchar *lower;
    gfloat seconds;
    gchar *localtitle = NULL;
    MetaData *ret = NULL;
    gboolean missing_header = FALSE;
    gchar *localuri = NULL;
    gchar *p = NULL;
#ifdef GIO_ENABLED
    GFile *file;
#endif

    if (ret == NULL)
        ret = (MetaData *) g_new0(MetaData, 1);

    if (device_name(uri)) {
        name = g_strdup(uri);
    } else {
#ifdef GIO_ENABLED
        file = g_file_new_for_uri(uri);
        if (file != NULL) {
            name = g_file_get_path(file);
            basename = g_file_get_basename(file);
            g_object_unref(file);
        }
#else
        name = g_filename_from_uri(uri, NULL, NULL);
        basename = g_filename_display_basename(name);
#endif
    }

    if (name == NULL) {
        if (ret != NULL)
            g_free(ret);
        return NULL;
    }

    gm_log(verbose, G_LOG_LEVEL_INFO, "getting file metadata for %s", name);

    if (mplayer_bin == NULL || !g_file_test(mplayer_bin, G_FILE_TEST_EXISTS)) {
        av[ac++] = g_strdup_printf("mplayer");
    } else {
        av[ac++] = g_strdup_printf("%s", mplayer_bin);
    }
    av[ac++] = g_strdup_printf("-vo");
    av[ac++] = g_strdup_printf("null");
    av[ac++] = g_strdup_printf("-ao");
    av[ac++] = g_strdup_printf("null");
    av[ac++] = g_strdup_printf("-nomsgcolor");
    av[ac++] = g_strdup_printf("-nomsgmodule");
    av[ac++] = g_strdup_printf("-frames");
    av[ac++] = g_strdup_printf("0");
    av[ac++] = g_strdup_printf("-noidx");
    av[ac++] = g_strdup_printf("-identify");
    av[ac++] = g_strdup_printf("-nocache");
    av[ac++] = g_strdup_printf("-noidle");

    if (idledata->device != NULL && strlen(idledata->device) > 0) {
        av[ac++] = g_strdup_printf("-dvd-device");
        av[ac++] = g_strdup_printf("%s", idledata->device);
    } else {
        if (mplayer_dvd_device != NULL && strlen(mplayer_dvd_device) > 0) {
            av[ac++] = g_strdup_printf("-dvd-device");
            av[ac++] = g_strdup_printf("%s", mplayer_dvd_device);
        }
    }

    av[ac++] = g_strdup_printf("%s", name);
    av[ac] = NULL;

    gchar *allargs = g_strjoinv(" ", av);
    gm_log(verbose, G_LOG_LEVEL_INFO, "%s", allargs);
    g_free(allargs);

    error = NULL;

    if (g_ascii_strncasecmp(name, "dvb://", strlen("dvb://")) == 0
        || g_ascii_strncasecmp(name, "tv://", strlen("tv://")) == 0) {
        gm_log(verbose, G_LOG_LEVEL_INFO, "Skipping gathering metadata for TV channels");
    } else {
        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status, &error);
    }

    for (i = 0; i < ac; i++) {
        g_free(av[i]);
    }

    if (error != NULL) {
        gm_log(verbose, G_LOG_LEVEL_MESSAGE, "Error when running: %s", error->message);
        g_error_free(error);
        error = NULL;
        if (out != NULL)
            g_free(out);
        if (err != NULL)
            g_free(err);
        if (ret != NULL)
            g_free(ret);
        return NULL;
    }
    if (out != NULL) {
        output = g_strsplit(out, "\n", 0);
    } else {
        output = NULL;
    }
    ac = 0;
    while (output != NULL && output[ac] != NULL) {
        gm_log(verbose, G_LOG_LEVEL_DEBUG, "METADATA:%s", output[ac]);
        lower = g_ascii_strdown(output[ac], -1);
        if (strstr(output[ac], "_LENGTH") != NULL
            && (g_ascii_strncasecmp(name, "dvdnav://", strlen("dvdnav://")) != 0
                || g_ascii_strncasecmp(name, "dvd://", strlen("dvd://")) != 0)) {
            localtitle = strstr(output[ac], "=") + 1;
            sscanf(localtitle, "%f", &seconds);
            length = seconds_to_string(seconds);
        }

        if (g_ascii_strncasecmp(output[ac], "ID_CLIP_INFO_NAME", strlen("ID_CLIP_INFO_NAME")) == 0) {
            if (strstr(lower, "=title") != NULL || strstr(lower, "=name") != NULL) {
                localtitle = strstr(output[ac + 1], "=") + 1;
                if (localtitle)
                    title = metadata_to_utf8(localtitle);
                else
                    title = NULL;

                if (title == NULL) {
                    title = g_strdup(localtitle);
                    gm_str_strip_unicode(title, strlen(title));
                }
            }
            if (strstr(lower, "=artist") != NULL || strstr(lower, "=author") != NULL) {
                localtitle = strstr(output[ac + 1], "=") + 1;
                artist = metadata_to_utf8(localtitle);
                if (artist == NULL) {
                    artist = g_strdup(localtitle);
                    gm_str_strip_unicode(artist, strlen(artist));
                }
            }
            if (strstr(lower, "=album") != NULL) {
                localtitle = strstr(output[ac + 1], "=") + 1;
                album = metadata_to_utf8(localtitle);
                if (album == NULL) {
                    album = g_strdup(localtitle);
                    gm_str_strip_unicode(album, strlen(album));
                }
            }
        }

        if (strstr(output[ac], "DVD Title:") != NULL
            && g_ascii_strncasecmp(name, "dvdnav://", strlen("dvdnav://")) == 0) {
            localtitle = g_strrstr(output[ac], ":") + 1;
            title = metadata_to_utf8(localtitle);
            if (title == NULL) {
                title = g_strdup(localtitle);
                gm_str_strip_unicode(title, strlen(title));
            }
        }

        if (strstr(output[ac], "ID_AUDIO_CODEC") != NULL) {
            localtitle = strstr(output[ac], "=") + 1;
            audio_codec = g_strdup(localtitle);
        }

        if (strstr(output[ac], "ID_VIDEO_CODEC") != NULL) {
            localtitle = strstr(output[ac], "=") + 1;
            video_codec = g_strdup(localtitle);
        }

        if (strstr(output[ac], "ID_VIDEO_WIDTH") != NULL) {
            localtitle = strstr(output[ac], "=") + 1;
            width = (gint) g_strtod(localtitle, NULL);
        }

        if (strstr(output[ac], "ID_VIDEO_HEIGHT") != NULL) {
            localtitle = strstr(output[ac], "=") + 1;
            height = (gint) g_strtod(localtitle, NULL);
        }

        if (strstr(output[ac], "ID_DEMUXER") != NULL) {
            localtitle = strstr(output[ac], "=") + 1;
            demuxer = g_strdup(localtitle);
        }

        g_free(lower);
        ac++;
    }

    g_strfreev(output);

    if (err != NULL) {
        output = g_strsplit(err, "\n", 0);
    } else {
        output = NULL;
    }
    ac = 0;
    while (output != NULL && output[ac] != NULL) {
        gm_log(verbose, G_LOG_LEVEL_DEBUG, "METADATA:%s", output[ac]);

        if (strstr(output[ac], "MOV: missing header (moov/cmov) chunk") != NULL) {
            missing_header = TRUE;
        }

        if (strstr(output[ac], "Title: ") != 0) {
            localtitle = strstr(output[ac], "Title: ") + strlen("Title: ");
            localtitle = g_strchomp(localtitle);
            if (title != NULL) {
                g_free(title);
                title = NULL;
            }
            title = g_strdup(localtitle);
        }

        ac++;
    }


    if (title == NULL || strlen(title) == 0) {
        localuri = g_strdup(uri);
        p = g_strrstr(localuri, ".");
        if (p)
            p[0] = '\0';
        p = g_strrstr(localuri, "/");
        if (p) {
            artist = g_strdup(p + 1);
            p = strstr(artist, " - ");
            if (p) {
                title = g_strdup(p + 3);
                p[0] = '\0';
            }
        }
        g_free(localuri);
    }

    if (title == NULL || strlen(title) == 0) {
        g_free(title);
        title = g_strdup(basename);
    }

    if (title == NULL && g_ascii_strncasecmp(name, "dvd://", strlen("dvd://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        title = g_strdup_printf("DVD Track %s", localtitle);
    }

    if (title == NULL && g_ascii_strncasecmp(name, "tv://", strlen("tv://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        title = g_strdup_printf("%s", localtitle);
    }

    if (title == NULL && g_ascii_strncasecmp(name, "dvb://", strlen("dvb://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        title = g_strdup_printf("%s", localtitle);
    }

    if (title == NULL && g_ascii_strncasecmp(name, "vcd://", strlen("vcd://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        title = g_strdup_printf("%s", localtitle);
    }

    if (title == NULL && g_ascii_strncasecmp(name, "dvdnav://", strlen("dvdnav://")) == 0) {
        title = g_strdup_printf("DVD");
    }

    if (ret != NULL) {
        ret->uri = g_strdup(uri);
        ret->title = g_strdup(title);
        ret->artist = g_strdup(artist);
        ret->album = g_strdup(album);
        ret->length = g_strdup(length);
        ret->length_value = seconds;
        ret->audio_codec = g_strdup(audio_codec);
        ret->video_codec = g_strdup(video_codec);
        ret->demuxer = g_strdup(demuxer);
        ret->width = width;
        ret->height = height;
        ret->playable = (demuxer == NULL && missing_header == FALSE) ? FALSE : TRUE;
    }

    g_free(title);
    g_free(artist);
    g_free(album);
    g_free(length);
    g_free(audio_codec);
    g_free(video_codec);
    g_free(demuxer);
    g_strfreev(output);
    if (out != NULL)
        g_free(out);
    if (err != NULL)
        g_free(err);
    g_free(name);
    g_free(basename);

    return ret;
}

gint get_bitrate(gchar * name)
{

    GError *error;
    gint exit_status;
    gchar *out = NULL;
    gchar *err = NULL;
    gchar *av[255];
    gint ac = 0, i;
    gchar **output;
    gint bitrate = 0;
    gint vbitrate = -1;
    gint abitrate = -1;
    gchar *buf;
    gint startsec = 0, endpos = 0;

    if (name == NULL)
        return 0;

    if (!g_file_test(name, G_FILE_TEST_EXISTS))
        return 0;

    if (mplayer_bin == NULL || !g_file_test(mplayer_bin, G_FILE_TEST_EXISTS)) {
        av[ac++] = g_strdup_printf("mplayer");
    } else {
        av[ac++] = g_strdup_printf("%s", mplayer_bin);
    }
    av[ac++] = g_strdup_printf("-vo");
    av[ac++] = g_strdup_printf("null");
    av[ac++] = g_strdup_printf("-ao");
    av[ac++] = g_strdup_printf("null");
    av[ac++] = g_strdup_printf("-nomsgcolor");
    av[ac++] = g_strdup_printf("-nomsgmodule");
    av[ac++] = g_strdup_printf("-frames");
    av[ac++] = g_strdup_printf("0");
    av[ac++] = g_strdup_printf("-identify");
    av[ac++] = g_strdup_printf("-nocache");
    av[ac++] = g_strdup_printf("-noidle");
    av[ac++] = g_strdup_printf("%s", name);
    av[ac] = NULL;

    error = NULL;

    g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status, &error);
    for (i = 0; i < ac; i++) {
        g_free(av[i]);
    }
    if (error != NULL) {
        gm_log(verbose, G_LOG_LEVEL_MESSAGE, "Error when running: %s", error->message);
        g_error_free(error);
        if (out != NULL)
            g_free(out);
        if (err != NULL)
            g_free(err);
        error = NULL;
        return 0;
    }
    output = g_strsplit(out, "\n", 0);
    ac = 0;
    while (output[ac] != NULL) {
        // find the length      
        if (strstr(output[ac], "ID_LENGTH") != 0) {
            buf = strstr(output[ac], "ID_LENGTH") + strlen("ID_LENGTH=");
            endpos = (gint) g_strtod(buf, NULL) / 6;    // pick a spot early in the file
            startsec = endpos - 1;
        }
        ac++;

    }
    g_strfreev(output);
    if (out != NULL) {
        g_free(out);
        out = NULL;
    }

    if (err != NULL) {
        g_free(err);
        err = NULL;
    }
    gm_log(verbose, G_LOG_LEVEL_DEBUG, "ss=%i, ep = %i", startsec, endpos);

    if (endpos == 0) {
        startsec = 0;
        endpos = 1;
    }

    if (control_id == 0) {
        ac = 0;
        av[ac++] = g_strdup_printf("mencoder");
        av[ac++] = g_strdup_printf("-ovc");
        av[ac++] = g_strdup_printf("copy");
        av[ac++] = g_strdup_printf("-oac");
        av[ac++] = g_strdup_printf("copy");
        av[ac++] = g_strdup_printf("-o");
        av[ac++] = g_strdup_printf("/dev/null");
        av[ac++] = g_strdup_printf("-quiet");
        av[ac++] = g_strdup_printf("-ss");
        av[ac++] = g_strdup_printf("%i", startsec);
        av[ac++] = g_strdup_printf("-endpos");
        av[ac++] = g_strdup_printf("%i", endpos);
        av[ac++] = g_strdup_printf("%s", name);
        av[ac] = NULL;

        error = NULL;

        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status, &error);
        for (i = 0; i < ac; i++) {
            g_free(av[i]);
        }
        if (error != NULL) {
            gm_log(verbose, G_LOG_LEVEL_MESSAGE, "Error when running: %s", error->message);
            g_error_free(error);
            if (out != NULL)
                g_free(out);
            if (err != NULL)
                g_free(err);
            error = NULL;
            return 0;
        }
        output = g_strsplit(out, "\n", 0);
        ac = 0;
        while (output[ac] != NULL) {
            if (strstr(output[ac], "Video stream") != 0) {
                buf = g_strrstr(output[ac], "(");
                if (buf != NULL) {
                    vbitrate = (gint) g_strtod(buf + sizeof(gchar), NULL);
                }
            }
            if (strstr(output[ac], "Audio stream") != 0) {
                buf = g_strrstr(output[ac], "(");
                if (buf != NULL) {
                    abitrate = (gint) g_strtod(buf + sizeof(gchar), NULL);
                }
            }
            ac++;

        }

        g_strfreev(output);
        if (out != NULL)
            g_free(out);
        if (err != NULL)
            g_free(err);
    }

    if (vbitrate <= 0 && abitrate <= 0) {
        bitrate = 0;
    } else if (vbitrate <= 0 && abitrate > 0) {
        // mplayer knows there is video, but doesn't know the bit rate
        // so we make the total bitrate 10 times the audio bitrate
        bitrate = abitrate * 10;
    } else {
        bitrate = vbitrate + abitrate;
    }

    gm_log(verbose, G_LOG_LEVEL_INFO, "bitrate = %i", bitrate);
    return bitrate;
}


gboolean add_item_to_playlist(const gchar * uri, gint playlist)
{
    GtkTreeIter localiter;
    gchar *local_uri;
    gchar *unescaped = NULL;
    gchar *slash;
    MetaData *data = NULL;
    gboolean retrieve = FALSE;

    if (strlen(uri) < 1)
        return FALSE;

    gm_log(verbose, G_LOG_LEVEL_INFO, "adding %s to playlist (cancel = %i)", uri, cancel_folder_load);
    local_uri = g_strdup(uri);
    if (!device_name(local_uri) && !streaming_media(local_uri)) {
        if (playlist) {
            data = (MetaData *) g_new0(MetaData, 1);
            data->title = g_strdup_printf("%s", uri);
        } else {
            retrieve = TRUE;
            data = get_basic_metadata(local_uri);
        }
    } else if (g_ascii_strncasecmp(uri, "cdda://", strlen("cdda://")) == 0) {
        data = (MetaData *) g_new0(MetaData, 1);
        data->title = g_strdup_printf("CD Track %s", uri + strlen("cdda://"));
    } else if (device_name(local_uri)) {
        if (g_ascii_strncasecmp(uri, "dvdnav://", strlen("dvdnav://")) == 0) {
            loop = 1;
        }
        retrieve = TRUE;
        if (g_ascii_strncasecmp(uri, "dvb://", strlen("dvb://")) == 0) {
            retrieve = FALSE;
        }
        if (g_ascii_strncasecmp(uri, "tv://", strlen("tv://")) == 0) {
            retrieve = FALSE;
        }
        data = get_basic_metadata(local_uri);

    } else {

        if (g_str_has_prefix(uri, "http://")) {
            unescaped = switch_protocol(uri, "mmshttp");
            g_free(local_uri);
            local_uri = g_strdup(unescaped);
            g_free(unescaped);
        }
#ifdef GIO_ENABLED
        unescaped = g_uri_unescape_string(uri, NULL);
#else
        unescaped = g_strdup(uri);
#endif
        data = (MetaData *) g_new0(MetaData, 1);
        slash = g_strrstr(unescaped, "/");
        if (slash != NULL && strlen(slash + sizeof(gchar)) > 0) {
            data->title = g_strdup_printf("[Stream] %s", slash + sizeof(gchar));
        } else {
            data->title = g_strdup_printf("[Stream] %s", unescaped);
        }
        g_free(unescaped);
    }

    if (data) {
        gtk_list_store_append(playliststore, &localiter);
        gtk_list_store_set(playliststore, &localiter, ITEM_COLUMN, local_uri,
                           DESCRIPTION_COLUMN, data->title,
                           COUNT_COLUMN, 0,
                           PLAYLIST_COLUMN, playlist,
                           ARTIST_COLUMN, data->artist,
                           ALBUM_COLUMN, data->album,
                           SUBTITLE_COLUMN, data->subtitle,
                           AUDIO_CODEC_COLUMN, data->audio_codec,
                           VIDEO_CODEC_COLUMN, data->video_codec,
                           LENGTH_COLUMN, data->length,
                           DEMUXER_COLUMN, data->demuxer,
                           LENGTH_VALUE_COLUMN, data->length_value,
                           VIDEO_WIDTH_COLUMN, data->width, VIDEO_HEIGHT_COLUMN, data->height,
                           PLAY_ORDER_COLUMN,
                           gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL),
                           ADD_ORDER_COLUMN,
                           gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL),
                           PLAYABLE_COLUMN, TRUE, -1);
        if (retrieve) {
            gm_log(verbose, G_LOG_LEVEL_DEBUG, "waiting for all events to drain");
            while (gtk_events_pending()) {
                gtk_main_iteration();
            }
            gm_log(verbose, G_LOG_LEVEL_DEBUG, "adding retrieve_metadata(%s) to pool", uri);
            g_thread_pool_push(retrieve_metadata_pool, (gpointer) g_strdup(uri), NULL);
        }

        set_item_add_info(local_uri);
        g_free(data->demuxer);
        g_free(data->title);
        g_free(data->artist);
        g_free(data->album);
        g_free(data->length);
        g_free(data->subtitle);
        g_free(data->audio_codec);
        g_free(data->video_codec);
        g_free(data);
        g_free(local_uri);
        g_idle_add(set_title_bar, idledata);

        return TRUE;
    } else {
        g_free(local_uri);
        return FALSE;
    }

}

gboolean first_item_in_playlist(GtkTreeIter * iter)
{
    gint i;

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), iter);
    if (gtk_list_store_iter_is_valid(playliststore, iter)) {
        do {
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), iter, PLAY_ORDER_COLUMN, &i, -1);
            if (i == 1) {
                // we found the current iter
                break;
            }
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), iter));
    }

    if (gtk_list_store_iter_is_valid(playliststore, iter)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

gint find_closest_to_x_in_playlist(gint x, gint delta)
{
    gint j = 0, k = -1;
    GtkTreeIter localiter;

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter);
    if (delta > 0)
        k = G_MAXINT;

    if (gtk_list_store_iter_is_valid(playliststore, &localiter)) {
        do {
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, PLAY_ORDER_COLUMN, &j, -1);
            if (j == (x + delta)) {
                // we found the current iter
                k = j;
                break;
            }
            if (delta < 0) {
                if (j > k && j < x) {
                    k = j;
                }
            } else {
                if (j < k && j > x) {
                    k = j;
                }
            }

        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
    }
    gm_log(verbose, G_LOG_LEVEL_DEBUG, "x = %i, delta = %i, return = %i", x, delta, k);
    return k;
}

gboolean prev_item_in_playlist(GtkTreeIter * iter)
{
    gint i, j, k;

    if (!gtk_list_store_iter_is_valid(playliststore, iter)) {
        return FALSE;
    } else {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), iter, PLAY_ORDER_COLUMN, &i, -1);
        if (i > 1) {
            k = find_closest_to_x_in_playlist(i, -1);
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), iter);
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), iter, PLAY_ORDER_COLUMN, &j, -1);
                if (j == k) {
                    // we found the current iter
                    break;
                }
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), iter));
            return TRUE;
        } else {
            return FALSE;
        }
    }
}


gboolean next_item_in_playlist(GtkTreeIter * iter)
{
    gint i, j, k;

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) == 0
        || !gtk_list_store_iter_is_valid(playliststore, iter)) {
        return FALSE;
    } else {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), iter, PLAY_ORDER_COLUMN, &i, -1);
        k = find_closest_to_x_in_playlist(i, 1);
        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), iter);
        if (gtk_list_store_iter_is_valid(playliststore, iter)) {
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), iter, PLAY_ORDER_COLUMN, &j, -1);
                if (j == k) {
                    // we found the current iter
                    break;
                }
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), iter));
        } else {
            return FALSE;
        }

        if (gtk_list_store_iter_is_valid(playliststore, iter)) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
}

gboolean save_playlist_pls(gchar * uri)
{
    gchar *itemname;
    GtkTreeIter localiter;
    gint i = 1;
    GtkWidget *dialog;

#ifdef GIO_ENABLED
    GFile *file;
    GFileOutputStream *output;
    GDataOutputStream *data;
    gchar *buffer;

    file = g_file_new_for_uri(uri);
    output = g_file_replace(file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);
    data = g_data_output_stream_new((GOutputStream *) output);
    if (data != NULL) {
        g_data_output_stream_put_string(data, "[playlist]\n", NULL, NULL);
        buffer =
            g_strdup_printf("NumberOfEntries=%i\n",
                            gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL));
        g_data_output_stream_put_string(data, buffer, NULL, NULL);
        g_free(buffer);
        g_data_output_stream_put_string(data, "Version=2\n", NULL, NULL);
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter)) {
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN, &itemname, -1);
                buffer = g_strdup_printf("File%i=%s\n", i++, itemname);
                g_data_output_stream_put_string(data, buffer, NULL, NULL);
                g_free(buffer);
                g_free(itemname);
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
        }

        g_output_stream_close((GOutputStream *) data, NULL, NULL);
        return TRUE;
    } else {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE, _("Unable to save playlist, cannot open file for writing"));
        gtk_window_set_title(GTK_WINDOW(dialog), "GNOME MPlayer Error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        return FALSE;
    }

#else
    gchar *filename;
    FILE *contents;

    filename = g_filename_from_uri(uri, NULL, NULL);
    contents = fopen(filename, "w");
    if (contents != NULL) {
        fprintf(contents, "[playlist]\n");
        fprintf(contents, "NumberOfEntries=%i\n", gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL));
        fprintf(contents, "Version=2\n");
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter)) {
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN, &itemname, -1);
                fprintf(contents, "File%i=%s\n", i++, itemname);
                g_free(itemname);
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
        }

        fclose(contents);
        return TRUE;
    } else {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE, _("Unable to save playlist, cannot open file for writing"));
        gtk_window_set_title(GTK_WINDOW(dialog), "GNOME MPlayer Error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        return FALSE;
    }
    g_free(filename);
#endif
}


gboolean save_playlist_m3u(gchar * uri)
{
    gchar *itemname;
    GtkTreeIter localiter;
    GtkWidget *dialog;

#ifdef GIO_ENABLED
    GFile *file;
    GFileOutputStream *output;
    GDataOutputStream *data;
    gchar *buffer;

    file = g_file_new_for_uri(uri);
    output = g_file_replace(file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);
    data = g_data_output_stream_new((GOutputStream *) output);
    if (data != NULL) {
        g_data_output_stream_put_string(data, "#EXTM3U\n", NULL, NULL);
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter)) {
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN, &itemname, -1);
                buffer = g_strdup_printf("%s\n", itemname);
                g_data_output_stream_put_string(data, buffer, NULL, NULL);
                g_free(buffer);
                g_free(itemname);
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
        }

        g_output_stream_close((GOutputStream *) data, NULL, NULL);
        return TRUE;
    } else {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE, _("Unable to save playlist, cannot open file for writing"));
        gtk_window_set_title(GTK_WINDOW(dialog), "GNOME MPlayer Error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        return FALSE;
    }
#else
    gchar *filename;
    FILE *contents;


    filename = g_filename_from_uri(uri, NULL, NULL);
    contents = fopen(filename, "w");
    if (contents != NULL) {
        fprintf(contents, "#EXTM3U\n");
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter)) {
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN, &itemname, -1);
                fprintf(contents, "%s\n", itemname);
                g_free(itemname);
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
        }

        fclose(contents);
        return TRUE;
    } else {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE, _("Unable to save playlist, cannot open file for writing"));
        gtk_window_set_title(GTK_WINDOW(dialog), "GNOME MPlayer Error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        return FALSE;
    }
    g_free(filename);
#endif
}

void randomize_playlist(GtkListStore * store)
{

    GtkTreeIter a;
    GtkTreeIter b;
    gint i, order_a, order_b;
    gint items;
    gint swapid;
    GRand *rand;
    gchar *iterfilename;
    gchar *localfilename;


    items = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
    rand = g_rand_new();

    if (items > 0) {
        if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iterfilename, -1);
        } else {
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iterfilename, -1);
        }
        for (i = 0; i < items; i++) {
            swapid = g_rand_int_range(rand, 0, items);
            if (i != swapid) {
                if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &a, NULL, i)) {
                    if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &b, NULL, swapid)) {
                        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &a, PLAY_ORDER_COLUMN, &order_a, -1);
                        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &b, PLAY_ORDER_COLUMN, &order_b, -1);
                        gtk_list_store_set(playliststore, &a, PLAY_ORDER_COLUMN, order_b, -1);
                        gtk_list_store_set(playliststore, &b, PLAY_ORDER_COLUMN, order_a, -1);
                    }
                }
            }
        }

        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &a);
        do {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &a, ITEM_COLUMN, &localfilename, -1);
            gm_log(verbose, G_LOG_LEVEL_DEBUG, "iter = %s   local = %s", iterfilename, localfilename);
            if (g_ascii_strcasecmp(iterfilename, localfilename) == 0) {
                // we found the current iter
                break;
            }
            g_free(localfilename);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &a));
        g_free(iterfilename);
        iter = a;
    }
    g_rand_free(rand);
}

void reset_playlist_order(GtkListStore * store)
{

    GtkTreeIter a;
    gint i, j;
    gint items;
    gchar *iterfilename;
    gchar *localfilename;


    items = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);

    if (items > 0) {
        if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iterfilename, -1);
        } else {
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iterfilename, -1);
        }
        for (i = 0; i < items; i++) {
            if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &a, NULL, i)) {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &a, ADD_ORDER_COLUMN, &j, -1);
                gtk_list_store_set(playliststore, &a, PLAY_ORDER_COLUMN, j, -1);
            }
        }

        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &a);
        do {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &a, ITEM_COLUMN, &localfilename, -1);
            gm_log(verbose, G_LOG_LEVEL_DEBUG, "iter = %s   local = %s", iterfilename, localfilename);
            if (g_ascii_strcasecmp(iterfilename, localfilename) == 0) {
                // we found the current iter
                break;
            }
            g_free(localfilename);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &a));
        g_free(iterfilename);
        iter = a;
    }
}

gchar *seconds_to_string(gfloat seconds)
{
    int hour = 0, min = 0, sec = 0;
    gchar *result = NULL;

    if (seconds >= 3600) {
        hour = seconds / 3600;
        seconds = seconds - (hour * 3600);
    }
    if (seconds >= 60) {
        min = seconds / 60;
        seconds = seconds - (min * 60);
    }
    sec = seconds;

    if (hour == 0) {
        result = g_strdup_printf(_("%2i:%02i"), min, sec);
    } else {
        result = g_strdup_printf(_("%i:%02i:%02i"), hour, min, sec);
    }
    return result;
}

#ifdef GIO_ENABLED
void cache_callback(goffset current_num_bytes, goffset total_num_bytes, gpointer data)
{
    gm_log(verbose, G_LOG_LEVEL_DEBUG, "downloaded %li of %li bytes", (glong) current_num_bytes,
           (glong) total_num_bytes);
    idledata->cachepercent = (gfloat) current_num_bytes / (gfloat) total_num_bytes;
    g_idle_add(set_progress_value, idledata);
    if (current_num_bytes > (cache_size * 1024)) {
        gm_log(verbose, G_LOG_LEVEL_DEBUG, "cache callback: signaling GCond idledata->caching_complete");
        g_cond_signal(idledata->caching_complete);
    }

}

void ready_callback(GObject * source_object, GAsyncResult * res, gpointer data)
{
    g_object_unref(idledata->tmp);
    g_object_unref(idledata->src);
    gm_log(verbose, G_LOG_LEVEL_DEBUG, "ready callback: signaling GCond idledata->caching_complete");
    g_cond_signal(idledata->caching_complete);
}
#endif


gchar *get_localfile_from_uri(gchar * uri)
{
    gchar *localfile;
#ifdef GIO_ENABLED
    gchar *tmp;
    gchar *base;
#endif
    if (device_name(uri) || streaming_media(uri)) {
        return g_strdup(uri);
    }
    localfile = g_filename_from_uri(uri, NULL, NULL);
#ifdef GIO_ENABLED
    idledata->tmpfile = FALSE;
    if (localfile == NULL) {
        gm_log(verbose, G_LOG_LEVEL_INFO, "using gio to access file");
        tmp = g_strdup_printf("%s/gnome-mplayer", g_get_user_cache_dir());

        idledata->src = g_file_new_for_uri(uri);
        localfile = g_file_get_path(idledata->src);
        // if gvfs-fuse is not installed then we don't have a local
        // name to use so we need to copy it.

        if (localfile == NULL) {
            base = g_file_get_basename(idledata->src);
            if (base != NULL) {
                localfile = g_strdup_printf("%s/%s", tmp, base);
                g_free(base);
                idledata->tmp = g_file_new_for_path(localfile);
                idledata->cancel = g_cancellable_new();
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "locking idledata->caching");
                g_mutex_lock(idledata->caching);
                gm_log(verbose, G_LOG_LEVEL_INFO, "caching uri to localfile via gio asynchronously");
                g_file_copy_async(idledata->src, idledata->tmp, G_FILE_COPY_NONE,
                                  G_PRIORITY_DEFAULT, idledata->cancel, cache_callback, NULL, ready_callback, NULL);
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "waiting for idledata->caching_complete");
                g_cond_wait(idledata->caching_complete, idledata->caching);
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "unlocking idledata->caching");
                g_mutex_unlock(idledata->caching);
                idledata->tmpfile = TRUE;
            }
        } else {
            gm_log(verbose, G_LOG_LEVEL_INFO, "src uri can be accessed via '%s'", localfile);
        }
        g_free(tmp);
    }
#endif

    return localfile;
}

gboolean is_uri_dir(gchar * uri)
{
    gboolean result = FALSE;

    if (uri == NULL)
        return FALSE;
    if (device_name(uri))
        return result;

#ifdef GIO_ENABLED
    GFile *file;
    GFileInfo *info;

    file = g_file_new_for_uri(uri);
    if (file != NULL) {
        info = g_file_query_info(file, "standard::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
        if (info != NULL) {
            result = (g_file_info_get_file_type(info) & G_FILE_TYPE_DIRECTORY);
            g_object_unref(info);
        }
        g_object_unref(file);
    }
#else
    gchar *filename;
    filename = g_filename_from_uri(uri, NULL, NULL);
    if (filename != NULL) {
        result = g_file_test(filename, G_FILE_TEST_IS_DIR);
        g_free(filename);
    }
#endif

    return result;
}

gboolean uri_exists(gchar * uri)
{
    gboolean result = FALSE;

    if (uri == NULL)
        return FALSE;
    if (device_name(uri))
        return result;

#ifdef GIO_ENABLED
    GFile *file;
    GFileInfo *info;

    file = g_file_new_for_uri(uri);
    if (file != NULL) {
        info = g_file_query_info(file, "standard::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
        if (info != NULL) {
            result = TRUE;
            g_object_unref(info);
        }
        g_object_unref(file);
    }
#else
    gchar *filename;
    filename = g_filename_from_uri(uri, NULL, NULL);
    if (filename != NULL) {
        result = g_file_test(filename, G_FILE_TEST_EXISTS);
        g_free(filename);
    }
#endif
    return result;
}

#ifdef HAVE_GPOD

gchar *find_gpod_mount_point()
{
    struct mntent *mnt = NULL;
    FILE *fp;
    char *ret = NULL;
    char *pathname = NULL;

    fp = setmntent("/etc/mtab", "r");
    do {
        mnt = getmntent(fp);
        if (mnt) {
            gm_log(verbose, G_LOG_LEVEL_DEBUG, "%s is at %s", mnt->mnt_fsname, mnt->mnt_dir);
            pathname = g_strdup_printf("%s/iPod_Control", mnt->mnt_dir);
            if (g_file_test(pathname, G_FILE_TEST_IS_DIR))
                ret = g_strdup(mnt->mnt_dir);
            g_free(pathname);
        }
    }
    while (mnt);
    endmntent(fp);

    return ret;
}

gboolean gpod_load_tracks(gchar * mount_point)
{
    Itdb_iTunesDB *db;
    Itdb_Artwork *artwork;
#ifdef GPOD_06
    Itdb_Thumb *thumb;
#endif
    GList *tracks;
    gint i = 0, width, height;
    gchar *duration;
    gchar *ipod_path;
    gchar *full_path;
    // gpointer pixbuf;
    GtkTreeIter localiter;

    db = itdb_parse(mount_point, NULL);
    if (db != NULL) {
        tracks = db->tracks;
        while (tracks) {
            // pixbuf = NULL;
            duration = seconds_to_string(((Itdb_Track *) (tracks->data))->tracklen / 1000);
            ipod_path = g_strdup(((Itdb_Track *) (tracks->data))->ipod_path);
            while (g_strrstr(ipod_path, ":")) {
                ipod_path[g_strrstr(ipod_path, ":") - ipod_path] = '/';
            }
            full_path = g_strdup_printf("file://%s%s", mount_point, ipod_path);
            g_free(ipod_path);

            artwork = (Itdb_Artwork *) ((Itdb_Track *) (tracks->data))->artwork;

#ifdef GPOD_06
            if (artwork->thumbnails != NULL) {
                thumb = (Itdb_Thumb *) (artwork->thumbnails->data);
                if (thumb != NULL) {
                    pixbuf = itdb_thumb_get_gdk_pixbuf(db->device, thumb);
                    gm_log(verbose, G_LOG_LEVEL_DEBUG, "%s has a thumbnail", ((Itdb_Track *) (tracks->data))->title);
                }
            }
#endif
#ifdef GPOD_07
            if (artwork->thumbnail != NULL) {
                //pixbuf = itdb_artwork_get_pixbuf(db->device, artwork, -1, -1);
                gm_log(verbose, G_LOG_LEVEL_DEBUG, "%s has a thumbnail", ((Itdb_Track *) (tracks->data))->title);
            }
#endif
            gm_log(verbose, G_LOG_LEVEL_DEBUG, "%s is movie = %i", ((Itdb_Track *) (tracks->data))->title,
                   ((Itdb_Track *) (tracks->data))->movie_flag);

            if (((Itdb_Track *) (tracks->data))->movie_flag) {
                width = 640;
                height = 480;
            } else {
                width = 0;
                height = 0;
            }

            gtk_list_store_append(playliststore, &localiter);
            gtk_list_store_set(playliststore, &localiter, ITEM_COLUMN, full_path,
                               DESCRIPTION_COLUMN, ((Itdb_Track *) (tracks->data))->title,
                               COUNT_COLUMN, 0,
                               PLAYLIST_COLUMN, 0,
                               ARTIST_COLUMN, ((Itdb_Track *) (tracks->data))->artist,
                               ALBUM_COLUMN, ((Itdb_Track *) (tracks->data))->album,
                               VIDEO_WIDTH_COLUMN, width,
                               VIDEO_HEIGHT_COLUMN, height,
                               SUBTITLE_COLUMN, NULL, LENGTH_COLUMN, duration,
                               PLAY_ORDER_COLUMN,
                               gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL),
                               ADD_ORDER_COLUMN,
                               gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL),
                               PLAYABLE_COLUMN, TRUE, -1);

            g_free(duration);
            g_free(full_path);

            tracks = g_list_next(tracks);
            i++;
        }
        gm_log(verbose, G_LOG_LEVEL_INFO, "found %i tracks", i);
        if (i > 1) {
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), TRUE);
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_loop), TRUE);
            // gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist), TRUE);
        }
        return TRUE;
    } else {
        return FALSE;
    }


}
#endif

#ifdef HAVE_MUSICBRAINZ
gchar *get_cover_art_url(gchar * artist, gchar * title, gchar * album)
{
    int i;
    MbWebService mb;
    MbQuery query;
    MbReleaseFilter release_filter;
    MbResultList results;
    MbRelease release;
    MbReleaseIncludes includes;

    char id[1024];
    char asin[1024];
    gchar *ret = NULL;
    gint score, highest_score;

    if (disable_cover_art_fetch)
        return ret;

    if (album == NULL && artist == NULL)
        return ret;

    mb = mb_webservice_new();

    query = mb_query_new(mb, "gnome-mplayer");

    release_filter = mb_release_filter_new();
    if (release_filter == NULL)
        return ret;
    if (artist != NULL && strlen(artist) > 0)
        release_filter = mb_release_filter_artist_name(release_filter, artist);
    if (album != NULL && strlen(album) > 0)
        release_filter = mb_release_filter_title(release_filter, album);

    results = mb_query_get_releases(query, release_filter);
    mb_release_filter_free(release_filter);

    if (results != NULL) {
        gm_log(verbose, G_LOG_LEVEL_DEBUG, "items found:  %i", mb_result_list_get_size(results));

        highest_score = -1;
        for (i = 0; i < mb_result_list_get_size(results); i++) {
            score = mb_result_list_get_score(results, i);
            release = mb_result_list_get_release(results, i);
            if (release != NULL) {
                mb_release_get_id(release, id, 1024);
                mb_release_free(release);
                includes = mb_release_includes_new();
                if (includes != NULL) {
                    includes = mb_track_includes_url_relations(includes);
                    release = mb_query_get_release_by_id(query, id, includes);
                    mb_release_includes_free(includes);
                }
            }
            if (release != NULL) {
                mb_release_get_asin(release, asin, 1024);
                mb_release_free(release);
                if (strlen(asin) > 0) {
                    gm_log(verbose, G_LOG_LEVEL_DEBUG, "asin = %s score = %i", asin, score);
                    if (score > highest_score) {
                        if (ret != NULL)
                            g_free(ret);
                        highest_score = score;
                        ret = g_strdup_printf("http://images.amazon.com/images/P/%s.01.TZZZZZZZ.jpg\n", asin);
                    }
                }
            }
            if (score == 100 && ret != NULL)
                break;
        }
        mb_result_list_free(results);
    }

    mb_query_free(query);
    mb_webservice_free(mb);

    return ret;
}

gpointer get_cover_art(gpointer data)
{
    gchar *url;
    gchar *path = NULL;
    gchar *p = NULL;
    gchar *test_file = NULL;
    gchar *cache_file = NULL;
    gboolean found_cached = FALSE;
    const gchar *artist = NULL;
    const gchar *album = NULL;
    CURLcode result;
    FILE *art;
    gpointer pixbuf;
    MetaData *metadata = (MetaData *) data;
    gchar *md5;
    gchar *thumbnail;
#ifdef GIO_ENABLED
    GFile *file;
    GInputStream *stream_in;
    gchar buf[2048];
    gint bytes;
    size_t written;
#else
    CURL *curl;
#endif

    gm_log_name_this_thread("gca");
    gm_log(verbose, G_LOG_LEVEL_DEBUG, "get_cover_art(%s)", metadata->uri);

    artist = gmtk_media_player_get_attribute_string(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_ARTIST);
    album = gmtk_media_player_get_attribute_string(GMTK_MEDIA_PLAYER(media), ATTRIBUTE_ALBUM);

    if (metadata->uri != NULL) {
        md5 = g_compute_checksum_for_string(G_CHECKSUM_MD5, metadata->uri, -1);
        thumbnail = g_strdup_printf("%s/.thumbnails/normal/%s.png", g_get_home_dir(), md5);

        if (g_file_test(thumbnail, G_FILE_TEST_EXISTS)) {
            if (cache_file)
                g_free(cache_file);
            cache_file = g_strdup(thumbnail);
            gm_log(verbose, G_LOG_LEVEL_INFO, "Looking for thumbnail %s ... found", thumbnail);
        } else {
            gm_log(verbose, G_LOG_LEVEL_INFO, "Looking for thumbnail %s ... not found", thumbnail);
        }
        g_free(thumbnail);

        if (cache_file == NULL) {
            thumbnail = g_strdup_printf("%s/.thumbnails/large/%s.png", g_get_home_dir(), md5);

            if (g_file_test(thumbnail, G_FILE_TEST_EXISTS)) {
                if (cache_file)
                    g_free(cache_file);
                cache_file = g_strdup(thumbnail);
                gm_log(verbose, G_LOG_LEVEL_INFO, "Looking for thumbnail %s ... found", thumbnail);
            } else {
                gm_log(verbose, G_LOG_LEVEL_INFO, "Looking for thumbnail %s ... not found", thumbnail);
            }
            g_free(thumbnail);
        }
        g_free(md5);
    }

    if (metadata->uri != NULL) {
        path = g_strdup(metadata->uri);
        if (path != NULL) {
            p = g_strrstr(path, "/");
            if (p != NULL) {
                p[0] = '\0';
                p = g_strconcat(path, "/cover.jpg", NULL);
#ifdef GIO_ENABLED
                file = g_file_new_for_uri(p);
                test_file = g_file_get_path(file);
                g_object_unref(file);
#else
                test_file = g_filename_from_uri(p, NULL, NULL);
#endif

                if (g_file_test(test_file, G_FILE_TEST_EXISTS)) {
                    gm_log(verbose, G_LOG_LEVEL_INFO, "Looking for cover art at %s ... found", test_file);
                    if (cache_file != NULL) {
                        g_free(cache_file);
                        cache_file = NULL;
                    }
                    cache_file = g_strdup(test_file);
                } else {
                    gm_log(verbose, G_LOG_LEVEL_INFO, "Looking for cover art at %s ... not found", test_file);
                }
                g_free(test_file);
            }
            g_free(path);
            path = NULL;
            p = NULL;
        }
    }

    if (metadata->uri != NULL) {
        path = g_strdup(metadata->uri);
        if (path != NULL) {
            p = g_strrstr(path, "/");
            if (p != NULL) {
                p[0] = '\0';
                p = g_strconcat(path, "/Folder.jpg", NULL);
#ifdef GIO_ENABLED
                file = g_file_new_for_uri(p);
                test_file = g_file_get_path(file);
                g_object_unref(file);
#else
                test_file = g_filename_from_uri(p, NULL, NULL);
#endif

                if (g_file_test(test_file, G_FILE_TEST_EXISTS)) {
                    gm_log(verbose, G_LOG_LEVEL_INFO, "Looking for cover art at %s ... found", test_file);
                    if (cache_file != NULL) {
                        g_free(cache_file);
                        cache_file = NULL;
                    }
                    cache_file = g_strdup(test_file);
                } else {
                    gm_log(verbose, G_LOG_LEVEL_INFO, "Looking for cover art at %s ... not found", test_file);
                }
                g_free(test_file);
            }
            g_free(path);
            path = NULL;
            p = NULL;
        }
    }

    if (artist != NULL && album != NULL) {
        path = g_strdup_printf("%s/gnome-mplayer/cover_art/%s/%s.jpeg", g_get_user_cache_dir(), artist, album);
        if (g_file_test(path, G_FILE_TEST_EXISTS)) {
            if (cache_file != NULL) {
                g_free(cache_file);
                cache_file = NULL;
            }
            cache_file = g_strdup(path);
            found_cached = TRUE;
        }
        g_free(path);
    }

    if (!found_cached && !disable_cover_art_fetch && artist != NULL && album != NULL) {
        url = get_cover_art_url((gchar *) artist, NULL, (gchar *) album);

        gm_log(verbose, G_LOG_LEVEL_INFO, "getting cover art from %s", url);

        if (url != NULL) {
            path = g_strdup_printf("%s/gnome-mplayer/cover_art/%s", g_get_user_cache_dir(), artist);

            if (!g_file_test(path, G_FILE_TEST_IS_DIR)) {
                g_mkdir_with_parents(path, 0775);
            }
            g_free(path);
            path = g_strdup_printf("%s/gnome-mplayer/cover_art/%s/%s.jpeg", g_get_user_cache_dir(), artist, album);
            gm_log(verbose, G_LOG_LEVEL_INFO, "storing cover art to %s", path);

            result = 0;


            art = fopen(path, "wb");
            if (art) {
#ifdef GIO_ENABLED
                file = g_file_new_for_uri(url);
                stream_in = (GInputStream *) g_file_read(file, NULL, NULL);
                if (stream_in) {
                    result = 0;
                    while ((bytes = g_input_stream_read(stream_in, buf, 2048, NULL, NULL))) {
                        written = fwrite(buf, sizeof(gchar), bytes, art);
                        if (written != bytes) {
                            result = 1;
                            break;
                        }
                    }
                    g_object_unref(stream_in);
                } else {
                    result = 1; // error condition
                }
#else
                curl = curl_easy_init();
                if (curl) {
                    curl_easy_setopt(curl, CURLOPT_URL, url);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, art);
                    result = curl_easy_perform(curl);
                    curl_easy_cleanup(curl);
                }
#endif
                fclose(art);
            }
            gm_log(verbose, G_LOG_LEVEL_DEBUG, "cover art url is %s", url);
            g_free(url);
            if (result == 0) {
                if (cache_file != NULL) {
                    g_free(cache_file);
                    cache_file = NULL;
                }
                cache_file = g_strdup(path);
            }
            g_free(path);
            path = NULL;
        }
    }

    gm_log(verbose, G_LOG_LEVEL_INFO, "metadata->uri = %s", metadata->uri);
    gm_log(verbose, G_LOG_LEVEL_INFO, "cache_file=%s", cache_file);

    if (cover_art_uri != NULL) {
        g_free(cover_art_uri);
        cover_art_uri = NULL;
    }

    if (cache_file != NULL && g_file_test(cache_file, G_FILE_TEST_EXISTS)) {
        gtk_list_store_set(playliststore, &iter, COVERART_COLUMN, cache_file, -1);
        cover_art_uri = g_strdup_printf("file://%s", cache_file);
        pixbuf = gdk_pixbuf_new_from_file(cache_file, NULL);
        g_idle_add(set_cover_art, pixbuf);
        mpris_send_signal_Updated_Metadata();
    } else {
        pixbuf = NULL;
        g_idle_add(set_cover_art, pixbuf);
    }
    g_free(cache_file);

    g_free(metadata->uri);
    g_free(metadata->title);
    g_free(metadata->artist);
    g_free(metadata->album);
    g_free(metadata);
    return NULL;
}
#else
gpointer get_cover_art(gpointer data)
{
    MetaData *metadata = (MetaData *) data;

    gm_log(verbose, G_LOG_LEVEL_INFO, "libcurl required for cover art retrieval");
    g_free(metadata->uri);
    g_free(metadata->title);
    g_free(metadata->artist);
    g_free(metadata->album);
    g_free(metadata);
    return NULL;
}

gchar *get_cover_art_url(gchar * artist, gchar * title, gchar * album)
{
    gm_log(verbose, G_LOG_LEVEL_DEBUG, "Running without musicbrainz support, unable to fetch url");
    return NULL;
}
#endif


/* 
	older mplayer binaries do not have the -volume option
	So rather than handleing several hundred bug reports since
	Ubuntu has some older mplayer binaries.
	we'll try and detect if we can use it or not.
*/
gboolean detect_volume_option()
{
    gchar *av[255];
    gint ac = 0, i;
    gchar **output;
    GError *error;
    gint exit_status;
    gchar *out = NULL;
    gchar *err = NULL;
    gboolean ret = TRUE;

    if (mplayer_bin == NULL || !g_file_test(mplayer_bin, G_FILE_TEST_EXISTS)) {
        av[ac++] = g_strdup_printf("mplayer");
    } else {
        av[ac++] = g_strdup_printf("%s", mplayer_bin);
    }
    av[ac++] = g_strdup_printf("-volume");
    av[ac++] = g_strdup_printf("%i", (gint) (audio_device.volume * 100.0));
    av[ac++] = g_strdup_printf("-noidle");
    av[ac++] = g_strdup_printf("-softvol");
    av[ac] = NULL;

    error = NULL;

    g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status, &error);

    for (i = 0; i < ac; i++) {
        g_free(av[i]);
    }

    if (error != NULL) {
        gm_log(verbose, G_LOG_LEVEL_MESSAGE, "Error when running: %s", error->message);
        g_error_free(error);
        error = NULL;
        if (out != NULL)
            g_free(out);
        if (err != NULL)
            g_free(err);
        return FALSE;
    }
    output = g_strsplit(err, "\n", 0);
    ac = 0;
    while (output[ac] != NULL) {
        if (g_ascii_strncasecmp(output[ac], "Unknown option", strlen("Unknown option")) == 0) {
            ret = FALSE;
        }
        if (g_ascii_strncasecmp(output[ac], "MPlayer2", strlen("MPlayer2")) == 0) {
            use_mplayer2 = TRUE;
        }
        ac++;
    }
    g_strfreev(output);
    g_free(out);
    g_free(err);

    if (!ret)
        printf(_("You might want to consider upgrading mplayer to a newer version, -volume option not supported\n"));
    return ret;
}

gchar *switch_protocol(const gchar * uri, gchar * new_protocol)
{
    gchar *p;

    p = g_strrstr(uri, "://");

    if (p != NULL)
        return g_strdup_printf("%s%s", new_protocol, p);
    else
        return NULL;
}

gboolean map_af_export_file(gpointer data)
{
    IdleData *idle = (IdleData *) data;

    if (reading_af_export)
        return TRUE;

    if (data != NULL) {
        idle->mapped_af_export =
            g_mapped_file_new(gmtk_media_player_get_attribute_string
                              (GMTK_MEDIA_PLAYER(media), ATTRIBUTE_AF_EXPORT_FILENAME), FALSE, NULL);
    }
    return FALSE;
}

gboolean unmap_af_export_file(gpointer data)
{
    IdleData *idle = (IdleData *) data;

    if (reading_af_export)
        return TRUE;

    if (idle->mapped_af_export) {
        g_mapped_file_free(idle->mapped_af_export);
        idle->mapped_af_export = NULL;
    }

    gmtk_audio_meter_set_data(GMTK_AUDIO_METER(audio_meter), NULL);

    return FALSE;
}
