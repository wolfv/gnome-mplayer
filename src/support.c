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
#ifdef HAVE_ASOUNDLIB
static char *device = "default";
// static char *pcm_mix = "PCM";
static char *master_mix = "Master";
#endif

void strip_unicode(gchar * data, gsize len)
{
    gsize i = 0;

    if (data != NULL) {
        for (i = 0; i < len; i++) {
            if (!g_unichar_validate(data[i])) {
                data[i] = ' ';
            }
        }
    }
}

gint detect_playlist(gchar * uri)
{

    gint playlist = 0;
    gchar buffer[16 * 1024];
    gchar **output;
    GtkTreeViewColumn *column;
    gchar *coltitle;
    gint count;
    gchar *path = NULL;
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


    if (g_strncasecmp(uri, "cdda://", 7) == 0) {
        playlist = 1;
    } else if (g_strncasecmp(uri, "dvd://", 6) == 0) {
        playlist = 1;
    } else if (g_strncasecmp(uri, "vcd://", 6) == 0) {
        playlist = 1;
//      } else if (g_strncasecmp(filename,"dvdnav://",9) == 0) {
//              playlist = 1;
    } else if (device_name(uri)) {
        playlist = 0;
    } else {

#ifdef GIO_ENABLED
        if (!streaming_media(uri)) {
            if (verbose)
                printf("opening playlist\n");
            file = g_file_new_for_uri(uri);
            path = get_path(uri);
            input = g_file_read(file, NULL, NULL);
            if (input != NULL) {
                memset(buffer, 0, sizeof(buffer));
                g_input_stream_read((GInputStream *) input, buffer, sizeof(buffer), NULL, NULL);
                output = g_strsplit(buffer, "\n", 0);
                if (output[0] != NULL) {
                    g_strchomp(output[0]);
                    g_strchug(output[0]);
                }
                // printf("buffer=%s\n",buffer);
                if (strstr(g_strdown(buffer), "[playlist]") != 0) {
                    playlist = 1;
                }

                if (strstr(g_strdown(buffer), "[reference]") != 0) {
                    playlist = 1;
                }

                if (strstr(g_strdown(buffer), "<asx") != 0) {
                    playlist = 1;
                }

                if (strstr(g_strdown(buffer), "<smil>") != 0) {
                    playlist = 1;
                }

                if (strstr(g_strdown(buffer), "#extm3u") != 0) {
                    playlist = 1;
                }

                if (strstr(g_strdown(buffer), "http://") != 0) {
                    playlist = 1;
                }

                if (strstr(g_strdown(buffer), "rtsp://") != 0) {
                    playlist = 1;
                }

                if (strstr(g_strdown(buffer), "pnm://") != 0) {
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
                g_strfreev(output);

                g_input_stream_close((GInputStream *) input, NULL, NULL);
            }
            g_object_unref(file);
            g_free(path);
        }
#else
        filename = g_filename_from_uri(uri, NULL, NULL);
        // printf("filename %s\n",filename);
        if (filename != NULL) {
            fp = fopen(filename, "r");
            if (path != NULL)
                g_free(path);
            path = get_path(filename);

            if (fp != NULL) {
                if (!feof(fp)) {
                    memset(buffer, 0, sizeof(buffer));
                    size = fread(buffer, 1, sizeof(buffer) - 1, fp);
                    output = g_strsplit(buffer, "\n", 0);
                    if (output[0] != NULL) {
                        g_strchomp(output[0]);
                        g_strchug(output[0]);
                    }
                    //printf("buffer=%s\n",buffer);
                    if (strstr(g_strdown(buffer), "[playlist]") != 0) {
                        playlist = 1;
                    }

                    if (strstr(g_strdown(buffer), "[reference]") != 0) {
                        playlist = 1;
                    }

                    if (g_ascii_strncasecmp(buffer, "#EXT", strlen("#EXT")) == 0) {
                        playlist = 1;
                    }

                    if (strstr(g_strdown(buffer), "<asx") != 0) {
                        playlist = 1;
                    }

                    if (strstr(g_strdown(buffer), "http://") != 0) {
                        playlist = 1;
                    }

                    if (strstr(g_strdown(buffer), "rtsp://") != 0) {
                        playlist = 1;
                    }

                    if (strstr(g_strdown(buffer), "pnm://") != 0) {
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

                    g_strfreev(output);
                }
                fclose(fp);
                g_free(path);
            }
            g_free(filename);
        }
#endif
    }
    if (verbose)
        printf("playlist detection = %i\n", playlist);
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

    // try and parse a playlist in various forms
    // if a parsing does not work then, return 0
    count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);

    ret = parse_basic(uri);
    if (ret != 1)
        ret = parse_ram(uri);
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
                gtk_tree_view_column_set_title(column, coltitle);
                g_free(coltitle);
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
    if (verbose)
        printf("parse playlist = %i\n", ret);
    return ret;
}

// parse_basic covers .pls, .m3u and reference playlist types 
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
    path = get_path(uri);
    input = g_file_read(file, NULL, NULL);
    data = g_data_input_stream_new((GInputStream *) input);
    if (data != NULL) {
        line = g_data_input_stream_read_line(data, &length, NULL, NULL);
        while (line != NULL) {
#else
    FILE *fp;
    gchar *file = NULL;

    file = g_strndup(uri, 4);
    if (strcmp(file, "file") != 0)
        return 0;               // FIXME: remote playlists unsuppored
    parse = g_strsplit(uri, "/", 3);
    path = get_path(parse[2]);
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
            //printf("line = %s\n", line);
            newline = g_strdup(line);
            if ((g_ascii_strncasecmp(line, "ref", 3) == 0) ||
                (g_ascii_strncasecmp(line, "file", 4)) == 0) {
                //printf("ref/file\n");
                parse = g_strsplit(line, "=", 2);
                if (parse != NULL && parse[1] != NULL) {
                    g_strstrip(parse[1]);
                    g_free(newline);
                    newline = g_strdup(parse[1]);
                }
                g_strfreev(parse);
            }

            if (g_ascii_strncasecmp(newline, "[playlist]", strlen("[playlist]")) == 0) {
                //printf("playlist\n");
                //continue;
            } else if (g_ascii_strncasecmp(newline, "[reference]", strlen("[reference]")) == 0) {
                //printf("ref\n");
                //continue;
                //playlist = 1;
            } else if (g_strncasecmp(newline, "<asx", strlen("<asx")) == 0) {
                //printf("asx\n");
                idledata->streaming = TRUE;
                g_free(newline);
                break;
            } else if (g_strncasecmp(newline, "<smil", strlen("<smil")) == 0) {
                //printf("asx\n");
                g_free(newline);
                break;
            } else if (g_strncasecmp(newline, "numberofentries", strlen("numberofentries")) == 0) {
                //printf("num\n");
                //continue;
            } else if (g_strncasecmp(newline, "version", strlen("version")) == 0) {
                //printf("ver\n");
                //continue;
            } else if (g_strncasecmp(newline, "title", strlen("title")) == 0) {
                //printf("tit\n");
                //continue;
            } else if (g_strncasecmp(newline, "length", strlen("length")) == 0) {
                //printf("len\n");
                //continue;
            } else if (g_strncasecmp(newline, "#extinf", strlen("#extinf")) == 0) {
                //printf("#extinf\n");
                //continue;
            } else if (g_strncasecmp(newline, "#", strlen("#")) == 0) {
                //printf("comment\n");
                //continue;
            } else {
                line_uri = g_strdup(newline);
                //printf("newline = %s\n", newline);
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
                        //printf("URL %s\n",newline);
                        add_item_to_playlist(newline, playlist);
                        ret = 1;
                    } else {
                        if (uri_exists(line_uri)) {
                            add_item_to_playlist(line_uri, 0);
                            ret = 1;
                        }
                    }
                }
                //printf("line_uri = %s\n", line_uri);
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
    }
    g_free(file);
    fclose(fp);
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
                    if (g_strncasecmp(output[ac], "rtsp://", strlen("rtsp://")) == 0) {
                        ret = 1;
                        add_item_to_playlist(output[ac], 0);
                    } else if (g_strncasecmp(output[ac], "pnm://", strlen("pnm://")) == 0) {
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
    }
    g_free(buffer);
    buffer = NULL;

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

    if (g_strncasecmp(filename, "cdda://", 7) != 0) {
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
        av[ac++] = g_strdup_printf("-frames");
        av[ac++] = g_strdup_printf("0");
        av[ac++] = g_strdup_printf("-identify");
        av[ac++] = g_strdup_printf("cddb://");
        av[ac++] = g_strdup_printf("cdda://");
        av[ac] = NULL;

        error = NULL;

        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err,
                     &exit_status, &error);
        for (i = 0; i < ac; i++) {
            g_free(av[i]);
        }
        if (error != NULL) {
            printf("Error when running: %s\n", error->message);
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

            if (g_strncasecmp(output[ac], " artist", strlen(" artist")) == 0) {
                artist = g_strdup_printf("%s", output[ac] + strlen(" artist=["));
                ptr = g_strrstr(artist, "]");
                if (ptr != NULL)
                    ptr[0] = '\0';
            }

            if (g_strncasecmp(output[ac], " album", strlen(" album")) == 0) {
                playlistname = g_strdup_printf("%s", output[ac] + strlen(" album=["));
                ptr = g_strrstr(playlistname, "]");
                if (ptr != NULL)
                    ptr[0] = '\0';
            }

            if (g_strncasecmp(output[ac], "  #", 3) == 0) {
                sscanf(output[ac], "  #%i", &num);
                track = g_strdup_printf("cdda://%i", num);
                title = g_strdup_printf("%s", output[ac] + 26);
                ptr = g_strrstr(title, "]");
                if (ptr != NULL)
                    ptr[0] = '\0';

                length =
                    g_strdup_printf("%c%c%c%c%c%c%c%c", output[ac][7], output[ac][8], output[ac][9],
                                    output[ac][10], output[ac][11], output[ac][12], output[ac][13],
                                    output[ac][14]);

                ok = FALSE;
                if (strlen(filename) > 7) {
                    if (g_strcasecmp(filename, track) == 0) {
                        ok = TRUE;
                    }
                } else {
                    ok = TRUE;
                }

                if (ok) {
                    //printf("track = %s, artist = %s, album = %s, title = %s, length = %s\n",track,artist,playlistname,title,length);
                    gtk_list_store_append(playliststore, &localiter);
                    gtk_list_store_set(playliststore, &localiter, ITEM_COLUMN, track,
                                       DESCRIPTION_COLUMN, title,
                                       COUNT_COLUMN, 0,
                                       PLAYLIST_COLUMN, 0,
                                       ARTIST_COLUMN, artist,
                                       ALBUM_COLUMN, playlistname,
                                       SUBTITLE_COLUMN, NULL, LENGTH_COLUMN, length, -1);


                    gtk_list_store_append(nonrandomplayliststore, &localiter);
                    gtk_list_store_set(nonrandomplayliststore, &localiter, ITEM_COLUMN, track,
                                       DESCRIPTION_COLUMN, title,
                                       COUNT_COLUMN, 0,
                                       PLAYLIST_COLUMN, 0,
                                       ARTIST_COLUMN, artist,
                                       ALBUM_COLUMN, playlistname,
                                       SUBTITLE_COLUMN, NULL, LENGTH_COLUMN, length, -1);
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
                if (g_strncasecmp(output[ac], "ID_CDDA_TRACK__", strlen("ID_CDDA_TRACK_")) == 0) {
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

    if (g_strncasecmp(filename, "dvd://", strlen("dvd://")) == 0) {     // || g_strncasecmp(filename,"dvdnav://",strlen("dvdnav://")) == 0) {
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
        av[ac++] = g_strdup_printf("-frames");
        av[ac++] = g_strdup_printf("0");
        av[ac++] = g_strdup_printf("-identify");
        if (idledata->device != NULL) {
            av[ac++] = g_strdup_printf("-dvd-device");
            av[ac++] = g_strdup_printf("%s", idledata->device);
        }
        av[ac++] = g_strdup_printf("dvd://");
        av[ac] = NULL;

        error = NULL;

        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err,
                     &exit_status, &error);
        for (i = 0; i < ac; i++) {
            g_free(av[i]);
        }
        if (error != NULL) {
            printf("Error when running: %s\n", error->message);
            g_error_free(error);
            if (out != NULL)
                g_free(out);
            if (err != NULL)
                g_free(err);
            error = NULL;
        }
        output = g_strsplit(out, "\n", 0);
        if (strstr(err, "Couldn't open DVD device:") != NULL) {
            error_msg =
                g_strdup_printf(_("Couldn't open DVD device: %s"),
                                err + strlen("Couldn't open DVD device: "));
        }
        ac = 0;
        while (output[ac] != NULL) {
            if (g_strncasecmp(output[ac], "ID_DVD_TITLE_", strlen("ID_DVD_TITLE_")) == 0) {
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
                                            GTK_BUTTONS_CLOSE, error_msg);
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

    if (g_strncasecmp(filename, "vcd://", strlen("vcd://")) == 0) {
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
        av[ac++] = g_strdup_printf("-frames");
        av[ac++] = g_strdup_printf("0");
        av[ac++] = g_strdup_printf("-identify");
        if (idledata->device != NULL) {
            av[ac++] = g_strdup_printf("-dvd-device");
            av[ac++] = g_strdup_printf("%s", idledata->device);
        }
        av[ac++] = g_strdup_printf("vcd://");
        av[ac] = NULL;

        error = NULL;

        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err,
                     &exit_status, &error);
        for (i = 0; i < ac; i++) {
            g_free(av[i]);
        }
        if (error != NULL) {
            printf("Error when running: %s\n", error->message);
            g_error_free(error);
            if (out != NULL)
                g_free(out);
            if (err != NULL)
                g_free(err);
            error = NULL;
        }
        output = g_strsplit(out, "\n", 0);
        if (strstr(err, "Couldn't open VCD device:") != NULL) {
            error_msg =
                g_strdup_printf(_("Couldn't open VCD device: %s"),
                                err + strlen("Couldn't open VCD device: "));
        }
        ac = 0;
        while (output[ac] != NULL) {
            if (g_strncasecmp(output[ac], "ID_VCD_TRACK_", strlen("ID_VCD_TRACK_")) == 0) {
                sscanf(output[ac], "ID_VCD_TRACK_%i", &num);
                track = g_strdup_printf("vcd://%i", num);
                printf("adding track %s\n", track);
                add_item_to_playlist(track, 0);
                g_free(track);
            }
            ac++;
        }
        g_strfreev(output);

        if (error_msg != NULL) {
            dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_CLOSE, error_msg);
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

gboolean update_mplayer_config()
{

    GKeyFile *config = g_key_file_new();
    gchar *data;
    gchar *def;
    GError *error;
    gchar *filename;

    error = NULL;

    if (g_getenv("HOME") == NULL)
        return FALSE;

    filename = g_strdup_printf("%s/.mplayer/config", g_getenv("HOME"));
    g_key_file_load_from_file(config,
                              filename,
                              G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error);

    if (error != NULL) {
        //printf("%i\n%s", error->code, error->message);
        if (error->code == 4) {
            g_file_get_contents(filename, &data, NULL, NULL);
            //printf("%s\n", data);
            def = g_strconcat("[default]\n", data, NULL);
            g_file_set_contents(filename, def, -1, NULL);
            g_free(data);
            g_free(def);
            g_error_free(error);
            error = NULL;
            g_key_file_load_from_file(config, filename, G_KEY_FILE_KEEP_TRANSLATIONS, &error);
            if (error != NULL) {
                // something bad happened...
                g_error_free(error);
                error = NULL;
            }
        } else {
            g_error_free(error);
            error = NULL;
        }
    }

    if (vo != NULL && strlen(vo) != 0) {
        g_key_file_set_string(config, "gnome-mplayer", "vo", vo);
        if (g_ascii_strcasecmp(vo, "x11") == 0) {
            // make full screen work when using x11
            g_key_file_set_string(config, "gnome-mplayer", "zoom", "1");
            // make advanced video controls work on x11
            g_key_file_set_string(config, "gnome-mplayer", "vf", "eq2");
        }

        if (g_ascii_strcasecmp(vo, "xv") == 0) {
            // make advanced video controls work on xv
            g_key_file_remove_key(config, "gnome-mplayer", "zoom", NULL);
            g_key_file_set_string(config, "gnome-mplayer", "vf", "eq2");
        }

        if (g_ascii_strcasecmp(vo, "xvmc") == 0 && !disable_deinterlace) {
            g_key_file_set_string(config, "gnome-mplayer", "vo", "xvmc:bobdeint:queue");
        }

        if (g_ascii_strcasecmp(vo, "gl") == 0 || g_ascii_strcasecmp(vo, "gl2") == 0
            || g_ascii_strcasecmp(vo, "xvmc") == 0 || g_ascii_strcasecmp(vo, "vdpau") == 0) {
            // if vf=eq2 is set and we use gl, then mplayer crashes
            g_key_file_remove_key(config, "gnome-mplayer", "zoom", NULL);
            g_key_file_remove_key(config, "gnome-mplayer", "vf", NULL);
        }


    } else {
        g_key_file_remove_key(config, "gnome-mplayer", "vo", NULL);
    }

    if (ao != NULL && strlen(ao) != 0) {
        g_key_file_set_string(config, "gnome-mplayer", "ao", ao);
    } else {
        g_key_file_remove_key(config, "gnome-mplayer", "ao", NULL);
    }

    if (alang != NULL && strlen(alang) != 0) {
        g_key_file_set_string(config, "gnome-mplayer", "alang", alang);
    } else {
        g_key_file_remove_key(config, "gnome-mplayer", "alang", NULL);
    }

    if (slang != NULL && strlen(slang) != 0) {
        g_key_file_set_string(config, "gnome-mplayer", "slang", slang);
    } else {
        g_key_file_remove_key(config, "gnome-mplayer", "slang", NULL);
    }

    g_key_file_remove_key(config, "gnome-mplayer", "really-quiet", NULL);
    g_key_file_set_string(config, "gnome-mplayer", "msglevel", "all=5");

    data = g_key_file_to_data(config, NULL, NULL);
    //printf("%i\n%s", strlen(data), data);

    g_file_set_contents(filename, data, -1, NULL);
    g_free(data);
    g_free(filename);
    g_key_file_free(config);

    return TRUE;
}

gboolean read_mplayer_config()
{

    GKeyFile *config = g_key_file_new();
    gchar *data;
    gchar *def;
    GError *error;
    gchar *filename;

    error = NULL;

    if (g_getenv("HOME") == NULL)
        return FALSE;

    // printf("home is set to %s",g_getenv("HOME"));

    filename = g_strdup_printf("%s/.mplayer/config", g_getenv("HOME"));
    g_key_file_load_from_file(config, filename, G_KEY_FILE_KEEP_TRANSLATIONS, &error);

    if (error != NULL) {
        //printf("%i\n%s\n", error->code, error->message);
        if (error->code == 4) {
            g_file_get_contents(filename, &data, NULL, NULL);
            //printf("%s\n", data);
            def = g_strconcat("[default]\n", data, NULL);
            g_file_set_contents(filename, def, -1, NULL);
            g_free(data);

            g_error_free(error);
            error = NULL;
            g_key_file_load_from_data(config,
                                      (const gchar *) def, strlen(def),
                                      G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                                      &error);
            g_free(def);

        }
    }

    if (vo != NULL)
        g_free(vo);
    if (ao != NULL)
        g_free(ao);
    vo = g_key_file_get_string(config, "gnome-mplayer", "vo", NULL);
    ao = g_key_file_get_string(config, "gnome-mplayer", "ao", NULL);
    alang = g_key_file_get_string(config, "gnome-mplayer", "alang", NULL);
    slang = g_key_file_get_string(config, "gnome-mplayer", "slang", NULL);

    if (vo != NULL && g_ascii_strcasecmp(vo, "xvmc:bobdeint:queue") == 0) {
        g_free(vo);
        vo = g_strdup("xvmc");
    }

    g_free(filename);
    g_key_file_free(config);

    if (verbose)
        printf("vo = %s ao = %s\n", vo, ao);

    return TRUE;
}

gboolean streaming_media(gchar * uri)
{
    gboolean ret;
#ifdef GIO_ENABLED
    GFile *file;
    GFileInfo *info;
#else
    gchar *local_file = NULL;
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
        file = g_file_new_for_uri(uri);
        if (file != NULL) {
            info = g_file_query_info(file, "*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
            if (info != NULL) {
                // SMB filesystems seem to give incorrect access answers over GIO, 
                // so if the file has a filesize > 0 we think it is not streaming
                if (g_ascii_strncasecmp(uri, "smb://", strlen("smb://")) == 0) {
                    ret = (g_file_info_get_size(info) > 0) ? FALSE : TRUE;
                } else {
                    ret =
                        !g_file_info_get_attribute_boolean(info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
                }
                g_object_unref(info);
            } else {
                ret = !g_file_test(uri, G_FILE_TEST_EXISTS);
            }
        }
        g_object_unref(file);
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
    if (verbose > 1)
        printf("Streaming media '%s' = %i\n", uri, ret);
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
    if (verbose > 1) {
        if (ret) {
            printf("%s is a device name\n", filename);
        } else {
            printf("%s is not a device name\n", filename);
        }
    }
    return ret;
}

gchar *metadata_to_utf8(gchar * string)
{
    const gchar *lang;
    lang = g_getenv("LANG");

    if (metadata_codepage != NULL && strlen(metadata_codepage) > 1) {
        // zh_TW usually use BIG5 on tags, if the file is from Windows
        if (g_utf8_validate(string, strlen(string), NULL) == FALSE) {
            return g_convert(string, -1, "UTF-8", metadata_codepage, NULL, NULL, NULL);
        }
    }

    return g_locale_to_utf8(string, -1, NULL, NULL, NULL);
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
#ifdef GIO_ENABLED
    GFile *file;
#endif
    if (device_name(uri)) {
        name = g_strdup(uri);
        if (ret == NULL)
            ret = (MetaData *) g_new0(MetaData, 1);

    } else {
#ifdef GIO_ENABLED
        file = g_file_new_for_uri(uri);
        if (file != NULL) {
            name = g_file_get_path(file);
            basename = g_file_get_basename(file);
        }
#else
        name = g_filename_from_uri(uri, NULL, NULL);
        basename = g_filename_display_basename(name);
#endif
    }

    if (name == NULL)
        return NULL;

    if (verbose)
        printf("getting file metadata for %s\n", name);

    if (mplayer_bin == NULL || !g_file_test(mplayer_bin, G_FILE_TEST_EXISTS)) {
        av[ac++] = g_strdup_printf("mplayer");
    } else {
        av[ac++] = g_strdup_printf("%s", mplayer_bin);
    }
    av[ac++] = g_strdup_printf("-vo");
    av[ac++] = g_strdup_printf("null");
    av[ac++] = g_strdup_printf("-ao");
    av[ac++] = g_strdup_printf("null");
    av[ac++] = g_strdup_printf("-frames");
    av[ac++] = g_strdup_printf("0");
    av[ac++] = g_strdup_printf("-noidx");
    av[ac++] = g_strdup_printf("-identify");
    av[ac++] = g_strdup_printf("-nocache");
    if (idledata->device != NULL) {
        av[ac++] = g_strdup_printf("-dvd-device");
        av[ac++] = g_strdup_printf("%s", idledata->device);
    }

    av[ac++] = g_strdup_printf("%s", name);
    av[ac] = NULL;

    error = NULL;

    if (g_strncasecmp(name, "dvb://", strlen("dvb://")) == 0
        || g_strncasecmp(name, "tv://", strlen("tv://")) == 0) {
        if (verbose)
            printf("Skipping gathering metadata for TV channels\n");
    } else {
        g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status,
                     &error);
    }

    for (i = 0; i < ac; i++) {
        g_free(av[i]);
    }

    if (error != NULL) {
        printf("Error when running: %s\n", error->message);
        g_error_free(error);
        error = NULL;
        if (out != NULL)
            g_free(out);
        if (err != NULL)
            g_free(err);
        return NULL;
    }
    if (out != NULL) {
        output = g_strsplit(out, "\n", 0);
    } else {
        output = NULL;
    }
    ac = 0;
    while (output != NULL && output[ac] != NULL) {
        lower = g_ascii_strdown(output[ac], -1);
        if (strstr(output[ac], "_LENGTH") != NULL
            && (g_strncasecmp(name, "dvdnav://", strlen("dvdnav://")) != 0
                || g_strncasecmp(name, "dvd://", strlen("dvd://")) != 0)) {
            localtitle = strstr(output[ac], "=") + 1;
            sscanf(localtitle, "%f", &seconds);
            length = seconds_to_string(seconds);
        }

        if (g_strncasecmp(output[ac], "ID_CLIP_INFO_NAME", strlen("ID_CLIP_INFO_NAME")) == 0) {
            if (strstr(lower, "=title") != NULL || strstr(lower, "=name") != NULL) {
                localtitle = strstr(output[ac + 1], "=") + 1;
                if (localtitle)
                    title = g_strstrip(metadata_to_utf8(localtitle));
                else
                    title = NULL;

                if (title == NULL) {
                    title = g_strdup(localtitle);
                    strip_unicode(title, strlen(title));
                }
            }
            if (strstr(lower, "=artist") != NULL || strstr(lower, "=author") != NULL) {
                localtitle = strstr(output[ac + 1], "=") + 1;
                artist = metadata_to_utf8(localtitle);
                if (artist == NULL) {
                    artist = g_strdup(localtitle);
                    strip_unicode(artist, strlen(artist));
                }
            }
            if (strstr(lower, "=album") != NULL) {
                localtitle = strstr(output[ac + 1], "=") + 1;
                album = metadata_to_utf8(localtitle);
                if (album == NULL) {
                    album = g_strdup(localtitle);
                    strip_unicode(album, strlen(album));
                }
            }
        }

        if (strstr(output[ac], "DVD Title:") != NULL
            && g_strncasecmp(name, "dvdnav://", strlen("dvdnav://")) == 0) {
            localtitle = g_strrstr(output[ac], ":") + 1;
            title = metadata_to_utf8(localtitle);
            if (title == NULL) {
                title = g_strdup(localtitle);
                strip_unicode(title, strlen(title));
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
            if (ret == NULL)
                ret = (MetaData *) g_new0(MetaData, 1);
            localtitle = strstr(output[ac], "=") + 1;
            ret->demuxer = g_strdup(localtitle);
        }
        g_free(lower);
        ac++;
    }

    if (title == NULL || strlen(title) == 0) {
        g_free(title);
        title = g_strdup(basename);
    }

    if (title == NULL && g_strncasecmp(name, "dvd://", strlen("dvd://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        title = g_strdup_printf("DVD Track %s", localtitle);
    }

    if (title == NULL && g_strncasecmp(name, "tv://", strlen("tv://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        title = g_strdup_printf("%s", localtitle);
    }

    if (title == NULL && g_strncasecmp(name, "dvb://", strlen("dvb://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        title = g_strdup_printf("%s", localtitle);
    }

    if (title == NULL && g_strncasecmp(name, "vcd://", strlen("vcd://")) == 0) {
        localtitle = g_strrstr(name, "/") + 1;
        title = g_strdup_printf("%s", localtitle);
    }

    if (title == NULL && g_strncasecmp(name, "dvdnav://", strlen("dvdnav://")) == 0) {
        title = g_strdup_printf("DVD");
    }

    if (ret != NULL) {
        ret->title = g_strdup(title);
        ret->artist = g_strdup(artist);
        ret->album = g_strdup(album);
        ret->length = g_strdup(length);
        ret->length_value = seconds;
        ret->audio_codec = g_strdup(audio_codec);
        ret->video_codec = g_strdup(video_codec);
        ret->width = width;
        ret->height = height;
    }

    g_free(title);
    g_free(artist);
    g_free(album);
    g_free(length);
    g_free(audio_codec);
    g_free(video_codec);
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
    av[ac++] = g_strdup_printf("-frames");
    av[ac++] = g_strdup_printf("0");
    av[ac++] = g_strdup_printf("-identify");
    av[ac++] = g_strdup_printf("-nocache");
    av[ac++] = g_strdup_printf("%s", name);
    av[ac] = NULL;

    error = NULL;

    g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status, &error);
    for (i = 0; i < ac; i++) {
        g_free(av[i]);
    }
    if (error != NULL) {
        printf("Error when running: %s\n", error->message);
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
    //if (verbose)
    //    printf("ss=%i, ep = %i\n", startsec, endpos);

    if (endpos == 0) {
        startsec = 0;
        endpos = 1;
    }

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
        printf("Error when running: %s\n", error->message);
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

    if (vbitrate <= 0 && abitrate <= 0) {
        bitrate = 0;
    } else if (vbitrate <= 0 && abitrate > 0) {
        // mplayer knows there is video, but doesn't know the bit rate
        // so we make the total bitrate 10 times the audio bitrate
        bitrate = abitrate * 10;
    } else {
        bitrate = vbitrate + abitrate;
    }

    if (verbose)
        printf("bitrate = %i\n", bitrate);
    return bitrate;
}


gboolean add_item_to_playlist(const gchar * uri, gint playlist)
{
    GtkTreeIter localiter;
    gchar *local_uri;
    gchar *unescaped = NULL;
    MetaData *data = NULL;

    if (strlen(uri) < 1)
        return FALSE;

    if (verbose)
        printf("adding %s to playlist\n", uri);
    local_uri = strdup(uri);
    if (!device_name(local_uri) && !streaming_media(local_uri)) {
        if (playlist) {
            data = (MetaData *) g_new0(MetaData, 1);
            data->title = g_strdup_printf("%s", uri);
        } else {
            data = get_metadata(local_uri);
        }
    } else if (g_ascii_strncasecmp(uri, "cdda://", strlen("cdda://")) == 0) {
        data = (MetaData *) g_new0(MetaData, 1);
        data->title = g_strdup_printf("CD Track %s", uri + strlen("cdda://"));
    } else if (device_name(local_uri)) {
        if (g_strncasecmp(uri, "dvdnav://", strlen("dvdnav://") == 0)) {
            loop = 1;
        }
        data = get_metadata(local_uri);

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
        data->title = g_strdup_printf("Stream from %s", unescaped);
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
                           VIDEO_WIDTH_COLUMN, data->width, VIDEO_HEIGHT_COLUMN, data->height, -1);


        gtk_list_store_append(nonrandomplayliststore, &localiter);
        gtk_list_store_set(nonrandomplayliststore, &localiter, ITEM_COLUMN, local_uri,
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
                           VIDEO_WIDTH_COLUMN, data->width, VIDEO_HEIGHT_COLUMN, data->height, -1);

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
        return TRUE;
    } else {
        g_free(local_uri);
        return FALSE;
    }

}

gboolean next_item_in_playlist(GtkTreeIter * iter)
{

    if (!gtk_list_store_iter_is_valid(playliststore, iter)) {
        return FALSE;
    } else {
        if (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), iter)) {
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
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN,
                                   &itemname, -1);
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
                                        GTK_BUTTONS_CLOSE,
                                        _("Unable to save playlist, cannot open file for writing"));
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
        fprintf(contents, "NumberOfEntries=%i\n",
                gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL));
        fprintf(contents, "Version=2\n");
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &localiter)) {
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN,
                                   &itemname, -1);
                fprintf(contents, "File%i=%s\n", i++, itemname);
                g_free(itemname);
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
        }

        fclose(contents);
        return TRUE;
    } else {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        _("Unable to save playlist, cannot open file for writing"));
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
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN,
                                   &itemname, -1);
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
                                        GTK_BUTTONS_CLOSE,
                                        _("Unable to save playlist, cannot open file for writing"));
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
                gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN,
                                   &itemname, -1);
                fprintf(contents, "%s\n", itemname);
                g_free(itemname);
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter));
        }

        fclose(contents);
        return TRUE;
    } else {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        _("Unable to save playlist, cannot open file for writing"));
        gtk_window_set_title(GTK_WINDOW(dialog), "GNOME MPlayer Error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        return FALSE;
    }
    g_free(filename);
#endif
}

gchar *get_path(gchar * uri)
{
    gchar *path = NULL;
    gchar cwd[1024];
    gchar *tmp = NULL;

    if (g_strrstr(uri, "/") != NULL) {
        path = g_strdup(uri);
        tmp = g_strrstr(path, "/");
        tmp[0] = '\0';
    } else {
        getcwd(cwd, 1024);
        path = g_strdup(cwd);
    }

    return path;
}

void copy_playlist(GtkListStore * source, GtkListStore * dest)
{

    GtkTreeIter sourceiter;
    GtkTreeIter destiter;
    GtkTreeIter a;
    gchar *iterfilename = NULL;
    gchar *localfilename;
    gchar *itemname;
    gchar *desc = NULL;
    gint count;
    gint playlist;
    gchar *artist = NULL;
    gchar *album = NULL;
    gchar *subtitle = NULL;
    gchar *audio_codec = NULL;
    gchar *video_codec = NULL;
    gchar *demuxer = NULL;
    gchar *length = NULL;
    gint width;
    gint height;
    gfloat length_value;

    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(dest), &iter, ITEM_COLUMN, &iterfilename, -1);
    }

    gtk_list_store_clear(dest);
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(source), &sourceiter)) {
        do {

            gtk_tree_model_get(GTK_TREE_MODEL(source), &sourceiter, ITEM_COLUMN, &itemname,
                               DESCRIPTION_COLUMN, &desc,
                               COUNT_COLUMN, &count,
                               PLAYLIST_COLUMN, &playlist,
                               ARTIST_COLUMN, &artist,
                               ALBUM_COLUMN, &album,
                               SUBTITLE_COLUMN, &subtitle,
                               AUDIO_CODEC_COLUMN, &audio_codec,
                               VIDEO_CODEC_COLUMN, &video_codec,
                               DEMUXER_COLUMN, &demuxer,
                               LENGTH_COLUMN, &length, LENGTH_VALUE_COLUMN, &length_value,
                               VIDEO_HEIGHT_COLUMN, &height, VIDEO_WIDTH_COLUMN, &width, -1);

            gtk_list_store_append(dest, &destiter);
            gtk_list_store_set(dest, &destiter, ITEM_COLUMN, itemname,
                               DESCRIPTION_COLUMN, desc,
                               COUNT_COLUMN, count,
                               PLAYLIST_COLUMN, playlist,
                               ARTIST_COLUMN, artist,
                               ALBUM_COLUMN, album,
                               SUBTITLE_COLUMN, subtitle,
                               AUDIO_CODEC_COLUMN, audio_codec,
                               VIDEO_CODEC_COLUMN, video_codec,
                               DEMUXER_COLUMN, demuxer,
                               LENGTH_COLUMN, length, LENGTH_VALUE_COLUMN, length_value,
                               VIDEO_HEIGHT_COLUMN, height, VIDEO_WIDTH_COLUMN, width, -1);

            g_free(desc);
            desc = NULL;
            g_free(artist);
            artist = NULL;
            g_free(album);
            album = NULL;
            g_free(subtitle);
            subtitle = NULL;
            g_free(audio_codec);
            audio_codec = NULL;
            g_free(video_codec);
            video_codec = NULL;
            g_free(length);
            length = NULL;

        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(source), &sourceiter));
    }

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(dest), &a);
    if (gtk_list_store_iter_is_valid(playliststore, &a)) {
        do {
            gtk_tree_model_get(GTK_TREE_MODEL(dest), &a, ITEM_COLUMN, &localfilename, -1);
            // printf("iter = %s   local = %s \n",iterfilename,localfilename);
            if (g_ascii_strcasecmp(iterfilename, localfilename) == 0) {
                // we found the current iter
                break;
            }
            g_free(localfilename);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(dest), &a));
        iter = a;
    }
    g_free(iterfilename);
}

void randomize_playlist(GtkListStore * store)
{

    GtkTreeIter a;
    GtkTreeIter b;
    gint i;
    gint items;
    gint swapid;
    GRand *rand;
    gchar *iterfilename;
    gchar *localfilename;


    items = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
    rand = g_rand_new();

    if (items > 0) {
        if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iterfilename,
                               -1);
        } else {
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &iterfilename,
                               -1);
        }
        for (i = 0; i < items; i++) {
            swapid = g_rand_int_range(rand, 0, items);
            if (i != swapid) {
                if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &a, NULL, i)) {
                    if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &b, NULL, swapid)) {
                        gtk_list_store_swap(store, &a, &b);
                    }
                }
            }
        }

        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &a);
        do {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &a, ITEM_COLUMN, &localfilename, -1);
            // printf("iter = %s   local = %s \n",iterfilename,localfilename);
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

gdouble get_alsa_volume()
{
    gdouble vol = 100.0;

#ifdef HAVE_ASOUNDLIB

    gint err;
    snd_mixer_t *mhandle;
    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *sid;
    glong get_vol, pmin, pmax;
    gfloat f_multi;
    gboolean found = FALSE;
    gchar **local_mixer;

    if ((err = snd_mixer_open(&mhandle, 0)) < 0) {
        if (verbose)
            printf("Mixer open error %s\n", snd_strerror(err));
        return vol;
    }

    if ((err = snd_mixer_attach(mhandle, device)) < 0) {
        if (verbose)
            printf("Mixer attach error %s\n", snd_strerror(err));
        return vol;
    }

    if ((err = snd_mixer_selem_register(mhandle, NULL, NULL)) < 0) {
        if (verbose)
            printf("Mixer register error %s\n", snd_strerror(err));
        return vol;
    }

    if ((err = snd_mixer_load(mhandle)) < 0) {
        if (verbose)
            printf("Mixer load error %s\n", snd_strerror(err));
        return vol;
    }

    if (mixer != NULL && strlen(mixer) > 0) {
        snd_mixer_selem_id_malloc(&sid);
        local_mixer = g_strsplit(mixer, ",", 2);
        if (local_mixer[1] == NULL) {
            snd_mixer_selem_id_set_index(sid, 0);
        } else {
            snd_mixer_selem_id_set_index(sid, (gint) g_strtod(local_mixer[1], NULL));
        }
        if (local_mixer[0] == NULL) {
            snd_mixer_selem_id_set_name(sid, mixer);
        } else {
            snd_mixer_selem_id_set_name(sid, local_mixer[0]);
        }
        if (local_mixer != NULL)
            g_strfreev(local_mixer);

        elem = snd_mixer_find_selem(mhandle, sid);
        if (!elem) {
            if (verbose)
                printf("Unable to find %s Mixer control, trying Master\n", mixer);
        } else {
            snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
            f_multi = (100 / (float) (pmax - pmin));
            snd_mixer_selem_get_playback_volume(elem, 0, &get_vol);
            vol = (gdouble) ((get_vol - pmin) * f_multi);
            if (verbose) {
                printf("%s Range is %li to %li \n", mixer, pmin, pmax);
                printf("%s Current Volume %li, multiplier = %f\n", mixer, get_vol, f_multi);
                printf("Scaled Volume is %lf\n", vol);
            }
            found = TRUE;
        }
        snd_mixer_selem_id_free(sid);
    }

    if (!found) {
        if (mixer != NULL) {
            g_free(mixer);
            mixer = g_strdup("Master");
        }
        snd_mixer_selem_id_malloc(&sid);
        snd_mixer_selem_id_set_index(sid, 0);
        snd_mixer_selem_id_set_name(sid, master_mix);

        elem = snd_mixer_find_selem(mhandle, sid);
        if (!elem) {
            if (verbose)
                printf("Unable to find Master Mixer control, using default of 100%%\n");
        } else {
            snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
            f_multi = (100 / (float) (pmax - pmin));
            snd_mixer_selem_get_playback_volume(elem, 0, &get_vol);
            vol = (gdouble) ((get_vol - pmin) * f_multi);
            if (verbose) {
                printf("Master Range is %li to %li \n", pmin, pmax);
                printf("Master Current Volume %li, multiplier = %f\n", get_vol, f_multi);
                printf("Scaled Volume is %lf\n", vol);
            }
            found = TRUE;
        }
        snd_mixer_selem_id_free(sid);
    }

    snd_mixer_detach(mhandle, device);
    snd_mixer_close(mhandle);
#endif

    if (verbose)
        printf("Using volume of %3.2lf\n", vol);
    return vol;
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
    //printf("downloaded %li of %li bytes\n",(glong)current_num_bytes,(glong)total_num_bytes);
    idledata->cachepercent = (gfloat) current_num_bytes / (gfloat) total_num_bytes;
    g_idle_add(set_progress_value, idledata);
    if (current_num_bytes > (cache_size * 1024))
        g_cond_signal(idledata->caching_complete);

}

void ready_callback(GObject * source_object, GAsyncResult * res, gpointer data)
{
    g_object_unref(idledata->tmp);
    g_object_unref(idledata->src);
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
        if (verbose)
            printf("using gio to access file\n");
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
                g_mutex_lock(idledata->caching);
                if (verbose)
                    printf("caching uri to localfile via gio asynchronously\n");
                g_file_copy_async(idledata->src, idledata->tmp, G_FILE_COPY_NONE,
                                  G_PRIORITY_DEFAULT, idledata->cancel, cache_callback, NULL,
                                  ready_callback, NULL);
                g_cond_wait(idledata->caching_complete, idledata->caching);
                g_mutex_unlock(idledata->caching);
                idledata->tmpfile = TRUE;
            }
        } else {
            if (verbose)
                printf("src uri can be accessed via '%s'\n", localfile);
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
            // printf("%s is at %s\n",mnt->mnt_fsname,mnt->mnt_dir);
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
    gpointer pixbuf;
    GtkTreeIter localiter;

    db = itdb_parse(mount_point, NULL);
    if (db != NULL) {
        tracks = db->tracks;
        while (tracks) {
            pixbuf = NULL;
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
                    //printf("%s has a thumbnail\n", ((Itdb_Track *) (tracks->data))->title);
                }
            }
#endif
#ifdef GPOD_07
            if (artwork->thumbnail != NULL) {
                pixbuf = itdb_artwork_get_pixbuf(db->device, artwork, -1, -1);
                //printf("%s has a thumbnail\n", ((Itdb_Track *) (tracks->data))->title);
            }
#endif
            // printf("%s is movie = %i\n",((Itdb_Track *) (tracks->data))->title,((Itdb_Track *) (tracks->data))->movie_flag);

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
                               SUBTITLE_COLUMN, NULL, LENGTH_COLUMN, duration, COVERART_COLUMN,
                               pixbuf, -1);

            gtk_list_store_append(nonrandomplayliststore, &localiter);
            gtk_list_store_set(nonrandomplayliststore, &localiter, ITEM_COLUMN, full_path,
                               DESCRIPTION_COLUMN, ((Itdb_Track *) (tracks->data))->title,
                               COUNT_COLUMN, 0,
                               PLAYLIST_COLUMN, 0,
                               ARTIST_COLUMN, ((Itdb_Track *) (tracks->data))->artist,
                               ALBUM_COLUMN, ((Itdb_Track *) (tracks->data))->album,
                               VIDEO_WIDTH_COLUMN, width,
                               VIDEO_HEIGHT_COLUMN, height,
                               SUBTITLE_COLUMN, NULL, LENGTH_COLUMN, duration, COVERART_COLUMN,
                               pixbuf, -1);

            g_free(duration);
            g_free(full_path);

            tracks = g_list_next(tracks);
            i++;
        }
        if (verbose)
            printf("found %i tracks\n", i);
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
        //printf("items found:  %i\n", mb_result_list_get_size(results));

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
                    //printf("asin = %s score = %i\n",asin,score);
                    if (score > highest_score) {
                        if (ret != NULL)
                            g_free(ret);
                        highest_score = score;
                        ret =
                            g_strdup_printf
                            ("http://images.amazon.com/images/P/%s.01.TZZZZZZZ.jpg\n", asin);
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
    gchar *cache_file = NULL;
    gboolean local_artist = FALSE;
    gboolean local_album = FALSE;
    CURL *curl;
    FILE *art;
    gpointer pixbuf;
    MetaData *metadata = (MetaData *) data;
    gboolean art_found = TRUE;
#ifdef GIO_ENABLED
    GFile *file;
#endif

    if (metadata->artist == NULL || strlen(metadata->artist) == 0) {
        if (metadata->artist)
            g_free(metadata->artist);
        metadata->artist = g_strdup("Unknown");
        local_artist = TRUE;
    }
    if (metadata->album == NULL || strlen(metadata->album) == 0) {
        if (metadata->album)
            g_free(metadata->album);
        metadata->album = g_strdup("Unknown");
        local_album = TRUE;
    }

    path = g_strdup(metadata->uri);
    p = g_strrstr(path, "/");
    if (p != NULL) {
        p[0] = '\0';
        p = g_strconcat(path, "/cover.jpg", NULL);
#ifdef GIO_ENABLED
        file = g_file_new_for_uri(p);
        cache_file = g_file_get_path(file);
        g_object_unref(file);
#else
        cache_file = g_filename_from_uri(p, NULL, NULL);
#endif
        g_free(p);
        if (verbose)
            printf("Looking for cover art at %s\n", cache_file);

    }
    g_free(path);
    path = NULL;
    p = NULL;

    if (cache_file == NULL || !g_file_test(cache_file, G_FILE_TEST_EXISTS)) {
        path = g_strdup(metadata->uri);
        p = g_strrstr(path, "/");
        if (p != NULL) {
            p[0] = '\0';
            p = g_strconcat(path, "/Folder.jpg", NULL);
#ifdef GIO_ENABLED
            file = g_file_new_for_uri(p);
            cache_file = g_file_get_path(file);
            g_object_unref(file);
#else
            cache_file = g_filename_from_uri(p, NULL, NULL);
#endif
            g_free(p);
            if (verbose)
                printf("Looking for cover art at %s\n", cache_file);
        }
        g_free(path);
        path = NULL;
        p = NULL;
    }

    if (cache_file == NULL || !g_file_test(cache_file, G_FILE_TEST_EXISTS)) {
        path =
            g_strdup_printf("%s/gnome-mplayer/cover_art/%s", g_get_user_cache_dir(),
                            metadata->artist);

        cache_file =
            g_strdup_printf("%s/gnome-mplayer/cover_art/%s/%s.jpeg", g_get_user_cache_dir(),
                            metadata->artist, metadata->album);
    }

    if (local_artist) {
        g_free(metadata->artist);
        metadata->artist = NULL;
    }
    if (local_album) {
        g_free(metadata->album);
        metadata->album = NULL;
    }

    if (!g_file_test(cache_file, G_FILE_TEST_EXISTS)) {
        art_found = FALSE;
        if (!disable_cover_art_fetch) {
            url = get_cover_art_url(metadata->artist, metadata->title, metadata->album);
            if (url == NULL && metadata->album != NULL && strlen(metadata->album) > 0) {
                g_free(path);
                path =
                    g_strdup_printf("%s/gnome-mplayer/cover_art/Unknown", g_get_user_cache_dir());
                g_free(cache_file);
                cache_file =
                    g_strdup_printf("%s/gnome-mplayer/cover_art/Unknown/%s.jpeg",
                                    g_get_user_cache_dir(), metadata->album);
                if (!g_file_test(cache_file, G_FILE_TEST_EXISTS))
                    url = get_cover_art_url(NULL, NULL, metadata->album);
            }
            if (url == NULL) {
                if ((p = strstr(metadata->title, " - ")) != NULL) {
                    p[0] = '\0';
                    g_free(path);
                    path =
                        g_strdup_printf("%s/gnome-mplayer/cover_art/Unknown",
                                        g_get_user_cache_dir());
                    g_free(cache_file);
                    cache_file =
                        g_strdup_printf("%s/gnome-mplayer/cover_art/Unknown/%s.jpeg",
                                        g_get_user_cache_dir(), metadata->title);
                    if (!g_file_test(cache_file, G_FILE_TEST_EXISTS))
                        url = get_cover_art_url(metadata->title, NULL, NULL);
                } else {
                    if (metadata->artist != NULL && strlen(metadata->artist) != 0) {
                        g_free(path);
                        path =
                            g_strdup_printf("%s/gnome-mplayer/cover_art/%s",
                                            g_get_user_cache_dir(), metadata->artist);
                        g_free(cache_file);
                        cache_file =
                            g_strdup_printf("%s/gnome-mplayer/cover_art/%s/Unknown.jpeg",
                                            g_get_user_cache_dir(), metadata->artist);
                        if (!g_file_test(cache_file, G_FILE_TEST_EXISTS))
                            url = get_cover_art_url(metadata->artist, NULL, NULL);
                    }
                }
            }

            if (verbose > 2) {
                printf("getting cover art from %s\n", url);
                printf("storing cover art to %s\n", cache_file);
                printf("cache file exists = %i\n", g_file_test(cache_file, G_FILE_TEST_EXISTS));
            }

            if (!g_file_test(cache_file, G_FILE_TEST_EXISTS) && disable_cover_art_fetch == FALSE) {
                if (url != NULL) {
                    if (!g_file_test(path, G_FILE_TEST_IS_DIR)) {
                        g_mkdir_with_parents(path, 0775);
                    }

                    art = fopen(cache_file, "wb");
                    if (art) {
                        curl = curl_easy_init();
                        if (curl) {
                            curl_easy_setopt(curl, CURLOPT_URL, url);
                            curl_easy_setopt(curl, CURLOPT_WRITEDATA, art);
                            curl_easy_perform(curl);
                            curl_easy_cleanup(curl);
                        }
                        fclose(art);
                    }
                    // printf("cover art url is %s\n",url);
                    g_free(url);
                    art_found = TRUE;
                }
            } else {
                art_found = TRUE;
            }
        }
    }

    if (g_file_test(cache_file, G_FILE_TEST_EXISTS) && art_found) {
        pixbuf = gdk_pixbuf_new_from_file(cache_file, NULL);
        g_idle_add(set_cover_art, pixbuf);
    } else {
        pixbuf = NULL;
        g_idle_add(set_cover_art, pixbuf);
    }
    g_free(cache_file);
    g_free(path);

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

    if (verbose)
        printf("libcurl required for cover art retrieval\n");
    g_free(metadata->uri);
    g_free(metadata->title);
    g_free(metadata->artist);
    g_free(metadata->album);
    g_free(metadata);
    return NULL;
}

gchar *get_cover_art_url(gchar * artist, gchar * title, gchar * album)
{
    if (verbose > 1)
        printf("Running without musicbrainz support, unable to fetch url\n");
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
    av[ac++] = g_strdup_printf("100");
    av[ac] = NULL;

    error = NULL;

    g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status, &error);

    for (i = 0; i < ac; i++) {
        g_free(av[i]);
    }

    if (error != NULL) {
        printf("Error when running: %s\n", error->message);
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
        if (g_strncasecmp(output[ac], "Unknown option", strlen("Unknown option")) == 0) {
            ret = FALSE;
        }
        ac++;
    }
    g_strfreev(output);
    g_free(out);
    g_free(err);

    if (!ret)
        printf(_
               ("You might want to consider upgrading mplayer to a newer version, -volume option not supported\n"));
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
        idle->mapped_af_export = g_mapped_file_new((gchar *) idle->af_export, FALSE, NULL);
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
        g_unlink((gchar *) idle->af_export);
    }

    gmtk_audio_meter_set_data(GMTK_AUDIO_METER(audio_meter), NULL);

    return FALSE;
}
