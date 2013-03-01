/*
 * database.c
 *
 * Copyright (C) 2012 - Kevin DeKorte
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <database.h>

#ifdef LIBGDA_ENABLED

gchar *escape_sql(const gchar * input)
{
    gchar *local;
    gchar *pos;

    local = g_strdup(input);
    pos = g_strrstr(local, "'");
    while (pos) {
        pos[0] = '\x01';
        pos = g_strrstr(local, "'");
    }
    return local;
}

gchar *unescape_sql(char *input)
{
    gchar *pos;

    pos = g_strrstr(input, "\x01");
    while (pos) {
        pos[0] = '\'';
        pos = g_strrstr(input, "\x01");
    }
    return input;
}

GdaConnection *open_db_connection()
{

    GdaConnection *conn;
    GError *error = NULL;
    GdaSqlParser *parser;

    gchar *db_location = NULL;


    db_location = g_strdup_printf("DB_DIR=%s/gnome-mplayer;DB_NAME=gnome-mplayer", g_get_user_config_dir());

    /* open connection */
    conn = gda_connection_open_from_string("SQLite", db_location, NULL, GDA_CONNECTION_OPTIONS_NONE, &error);

    g_free(db_location);
    db_location = NULL;

    if (!conn) {
        g_print("Could not open connection to SQLite database file: %s\n",
                error && error->message ? error->message : "No detail");
        return conn;
    }

    /* create an SQL parser */
    parser = gda_connection_create_parser(conn);
    if (!parser)                /* @conn does not provide its own parser => use default one */
        parser = gda_sql_parser_new();
    /* attach the parser object to the connection */
    g_object_set_data_full(G_OBJECT(conn), "parser", parser, g_object_unref);

    create_tables(conn);

    return conn;
}

void close_db_connection(GdaConnection * conn)
{
    gda_connection_close(conn);
}

void create_tables(GdaConnection * conn)
{
    run_sql_non_select(conn, "create table if not exists media_entries ("
                       "uri string not null primary key, "
                       "title string, "
                       "artist string, "
                       "album string, "
                       "cover_art string, "
                       "audio_codec string, "
                       "video_codec string, "
                       "demuxer string, "
                       "video_width int, " "video_height int, " "length real, " "resume boolean, " "position real)");
}

void delete_tables(GdaConnection * conn)
{
    run_sql_non_select(conn, "drop table if exists media_entries");
}


void run_sql_non_select(GdaConnection * conn, const gchar * sql)
{
    GdaStatement *stmt;
    GError *error = NULL;
    gint nrows;
    const gchar *remain;
    GdaSqlParser *parser;

    parser = g_object_get_data(G_OBJECT(conn), "parser");
    stmt = gda_sql_parser_parse_string(parser, sql, &remain, &error);
    if (remain)
        g_print("REMAINS: %s\n", remain);

    nrows = gda_connection_statement_execute_non_select(conn, stmt, NULL, NULL, &error);
    if (nrows == -1)
        g_error("NON SELECT error: %s\nsql = %s\n", error && error->message ? error->message : "no detail", sql);
    g_object_unref(stmt);
}

MetaData *get_db_metadata(GdaConnection * conn, const gchar * uri)
{
    MetaData *ret = NULL;
    GdaSqlParser *parser;
    GdaStatement *stmt;
    GError *error = NULL;
    gchar *sql = NULL;
    const GValue *value;
    gchar *localuri;

    if (ret == NULL)
        ret = (MetaData *) g_new0(MetaData, 1);

    ret->uri = g_strdup(uri);
    ret->playable = FALSE;
    ret->valid = FALSE;

    localuri = escape_sql(uri);

    sql = g_strdup_printf("select * from media_entries where uri = '%s'", localuri);

    parser = gda_connection_create_parser(conn);
    if (!parser) {
        printf("parser was not found\n");
        parser = gda_sql_parser_new();
    }

    stmt = gda_sql_parser_parse_string(parser, sql, NULL, &error);
    g_object_unref(parser);

    if (!stmt) {
        /* there was an error while parsing */
        printf("error parsing statement \"%s\"\n", sql);
    } else {
        GdaDataModel *model;
        model = gda_connection_statement_execute_select(conn, stmt, NULL, &error);
        if (model) {
            if (gda_data_model_get_n_rows(model) == 1) {
                value = gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "title"), 0, &error);
                if (value != NULL && G_IS_VALUE(value) && G_VALUE_HOLDS_STRING(value))
                    ret->title = unescape_sql(g_value_dup_string(value));

                value = gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "artist"), 0, &error);
                if (value != NULL && G_IS_VALUE(value) && G_VALUE_HOLDS_STRING(value))
                    ret->artist = unescape_sql(g_value_dup_string(value));

                value = gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "album"), 0, &error);
                if (value != NULL && G_IS_VALUE(value) && G_VALUE_HOLDS_STRING(value))
                    ret->album = unescape_sql(g_value_dup_string(value));

                value =
                    gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "audio_codec"), 0,
                                                &error);
                if (value != NULL && G_IS_VALUE(value) && G_VALUE_HOLDS_STRING(value))
                    ret->audio_codec = g_value_dup_string(value);

                value =
                    gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "video_codec"), 0,
                                                &error);
                if (value != NULL && G_IS_VALUE(value) && G_VALUE_HOLDS_STRING(value))
                    ret->video_codec = g_value_dup_string(value);

                value =
                    gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "demuxer"), 0, &error);
                if (value != NULL && G_IS_VALUE(value) && G_VALUE_HOLDS_STRING(value))
                    ret->demuxer = g_value_dup_string(value);
                if (ret->demuxer != NULL) {
                    ret->playable = TRUE;
                }

                value =
                    gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "video_width"), 0,
                                                &error);
                if (value != NULL && G_IS_VALUE(value))
                    ret->width = g_value_get_int(value);

                value =
                    gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "video_height"), 0,
                                                &error);
                if (value != NULL && G_IS_VALUE(value))
                    ret->height = g_value_get_int(value);

                value = gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "length"), 0, &error);
                if (value != NULL && G_IS_VALUE(value) && G_VALUE_HOLDS_DOUBLE(value)) {
                    ret->length_value = g_value_get_double(value);
                    ret->length = seconds_to_string(ret->length_value);
                }

                value = gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "resume"), 0, &error);
                if (value != NULL && G_IS_VALUE(value)) {
                    ret->resumable = g_value_get_boolean(value);
                }

                value =
                    gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "position"), 0, &error);
                if (value != NULL && G_IS_VALUE(value) && G_VALUE_HOLDS_DOUBLE(value)) {
                    ret->position = g_value_get_double(value);
                }

                if (ret->demuxer != NULL)
                    ret->valid = TRUE;

            }

            g_object_unref(model);
        } else {
            /* there was an error while executing the statement */
            printf("error executing statement \"%s\"\n", sql);
        }
        g_object_unref(stmt);
    }

    g_free(sql);
    g_free(localuri);

    return ret;
}

gboolean is_uri_in_db_resumable(GdaConnection * conn, const gchar * uri)
{
    GdaSqlParser *parser;
    GdaStatement *stmt;
    GError *error = NULL;
    gchar *sql = NULL;
    const GValue *value;
    gchar *localuri;
    gboolean ret = FALSE;

    localuri = escape_sql(uri);

    sql = g_strdup_printf("select resume from media_entries where uri = '%s'", localuri);

    parser = gda_connection_create_parser(conn);
    if (!parser) {
        printf("parser was not found\n");
        parser = gda_sql_parser_new();
    }

    stmt = gda_sql_parser_parse_string(parser, sql, NULL, &error);
    g_object_unref(parser);

    if (!stmt) {
        /* there was an error while parsing */
        printf("error parsing statement \"%s\"\n", sql);
    } else {
        GdaDataModel *model;
        model = gda_connection_statement_execute_select(conn, stmt, NULL, &error);
        if (model) {
            if (gda_data_model_get_n_rows(model) == 1) {
                value = gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "resume"), 0, &error);
                if (value != NULL && G_IS_VALUE(value)) {
                    ret = g_value_get_boolean(value);
                }

            }

            g_object_unref(model);
        } else {
            /* there was an error while executing the statement */
            printf("error executing statement \"%s\"\n", sql);
        }
        g_object_unref(stmt);
    }

    g_free(sql);
    g_free(localuri);

    return ret;
}

void insert_update_db_metadata(GdaConnection * conn, const gchar * uri, const MetaData * data)
{
    gchar *remove = NULL;
    gchar *localuri;
    gchar *localvalue;

    GValue *uri_value;
    GValue *title;
    GValue *artist;
    GValue *album;
    GValue *audio_codec;
    GValue *video_codec;
    GValue *demuxer;
    GValue *video_width;
    GValue *video_height;
    GValue *length;
    GValue *resume;
    GError *error = NULL;
    gboolean res;


    localuri = escape_sql(uri);
    uri_value = gda_value_new_from_string(localuri, G_TYPE_STRING);

    if (data->title != NULL) {
        localvalue = escape_sql(data->title);
        title = gda_value_new_from_string(localvalue, G_TYPE_STRING);
        g_free(localvalue);
    } else {
        title = gda_value_new_from_string("", G_TYPE_STRING);
    }

    if (data->artist != NULL) {
        localvalue = escape_sql(data->artist);
        artist = gda_value_new_from_string(localvalue, G_TYPE_STRING);
        g_free(localvalue);
    } else {
        artist = gda_value_new_from_string("", G_TYPE_STRING);
    }

    if (data->album != NULL) {
        localvalue = escape_sql(data->album);
        album = gda_value_new_from_string(localvalue, G_TYPE_STRING);
        g_free(localvalue);
    } else {
        album = gda_value_new_from_string("", G_TYPE_STRING);
    }

    /*
       if (data->cover_art != NULL) {
       oldcolumns = columns;
       columns = g_strconcat(columns, ",cover_art", NULL);
       g_free(oldcolumns);
       valuetmp = g_strdup_printf(",'%s'", data->cover_art);
       oldvalues = values;
       values = g_strconcat(values, valuetmp, NULL);
       g_free(oldvalues);
       g_free(valuetmp);
       }
     */

    if (data->audio_codec != NULL) {
        audio_codec = gda_value_new_from_string(data->audio_codec, G_TYPE_STRING);
    } else {
        audio_codec = gda_value_new_from_string("", G_TYPE_STRING);
    }

    if (data->video_codec != NULL) {
        video_codec = gda_value_new_from_string(data->video_codec, G_TYPE_STRING);
    } else {
        video_codec = gda_value_new_from_string("", G_TYPE_STRING);
    }

    if (data->demuxer != NULL) {
        demuxer = gda_value_new_from_string(data->demuxer, G_TYPE_STRING);
    } else {
        demuxer = gda_value_new_from_string("", G_TYPE_STRING);
    }

    video_width = gda_value_new(G_TYPE_INT);
    video_height = gda_value_new(G_TYPE_INT);
    length = gda_value_new(G_TYPE_DOUBLE);
    resume = gda_value_new(G_TYPE_BOOLEAN);
    g_value_set_int(video_width, data->width);
    g_value_set_int(video_height, data->height);
    g_value_set_double(length, data->length_value);
    g_value_set_boolean(resume, FALSE);

    remove = g_strdup_printf("delete from media_entries where uri='%s'", localuri);

    run_sql_non_select(conn, remove);

#ifdef LIBGDA_05
    res =
        gda_connection_insert_row_into_table(conn, "media_entries", &error, "uri", uri_value, "title", title, "artist",
                                             artist, "album", album, "audio_codec", audio_codec, "video_codec",
                                             video_codec, "demuxer", demuxer, "video_width", video_width,
                                             "video_height", video_height, "length", length, "resume", resume, NULL);

#else
    res =
        gda_insert_row_into_table(conn, "media_entries", &error, "uri", uri_value, "title", title, "artist", artist,
                                  "album", album, "audio_codec", audio_codec, "video_codec", video_codec, "demuxer",
                                  demuxer, "video_width", video_width, "video_height", video_height, "length", length,
                                  "resume", resume, NULL);
#endif
    if (!res) {
        g_error("Could not INSERT data into the 'media_entries' table: %s\n",
                error && error->message ? error->message : "No detail");
    }


    g_free(localuri);
    gda_value_free(uri_value);
    gda_value_free(title);
    gda_value_free(artist);
    gda_value_free(album);
    gda_value_free(audio_codec);
    gda_value_free(video_codec);
    gda_value_free(demuxer);
    gda_value_free(video_height);
    gda_value_free(video_width);
    gda_value_free(length);
    gda_value_free(resume);
}

void mark_uri_in_db_as_resumable(GdaConnection * conn, const gchar * uri, gboolean resume, gdouble position)
{
    gchar *localuri;
    GValue *uri_value;
    GValue *position_value;
    GValue *resume_value;
    GError *error = NULL;
    gboolean res;

    localuri = escape_sql(uri);
    uri_value = gda_value_new_from_string(localuri, G_TYPE_STRING);
    resume_value = gda_value_new(G_TYPE_BOOLEAN);
    g_value_set_boolean(resume_value, TRUE);
    position_value = gda_value_new(G_TYPE_DOUBLE);
    g_value_set_double(position_value, position);


#ifdef LIBGDA_05
    res =
        gda_connection_update_row_in_table(conn, "media_entries", "uri", uri_value, &error, "position", position_value,
                                           "resume", resume_value, NULL);

#else
    res =
        gda_update_row_in_table(conn, "media_entries", "uri", uri_value, &error, "position", position_value, "resume",
                                resume_value, NULL);
#endif
    if (!res) {
        g_error("Could not UPDATE data into the 'media_entries' table: %s\n",
                error && error->message ? error->message : "No detail");
    }


    g_free(localuri);
    gda_value_free(uri_value);
    gda_value_free(position_value);
    gda_value_free(resume_value);
}

#endif
