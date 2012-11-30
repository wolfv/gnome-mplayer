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
                if (value != NULL && G_IS_VALUE(value))
                    ret->title = unescape_sql(g_value_dup_string(value));
                value = gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "artist"), 0, &error);
                if (value != NULL && G_IS_VALUE(value))
                    ret->artist = unescape_sql(g_value_dup_string(value));
                value = gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "album"), 0, &error);
                if (value != NULL && G_IS_VALUE(value))
                    ret->album = unescape_sql(g_value_dup_string(value));
                value =
                    gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "audio_codec"), 0,
                                                &error);
                if (value != NULL && G_IS_VALUE(value))
                    ret->audio_codec = g_value_dup_string(value);
                value =
                    gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "video_codec"), 0,
                                                &error);
                if (value != NULL && G_IS_VALUE(value))
                    ret->video_codec = g_value_dup_string(value);
                value =
                    gda_data_model_get_value_at(model, gda_data_model_get_column_index(model, "demuxer"), 0, &error);
                if (value != NULL && G_IS_VALUE(value))
                    ret->demuxer = g_value_dup_string(value);
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
                if (value != NULL && G_IS_VALUE(value)) {
                    ret->length_value = g_value_get_double(value);
                    ret->length = seconds_to_string(ret->length_value);
                }

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

void insert_update_db_metadata(GdaConnection * conn, const gchar * uri, const MetaData * data)
{
    gchar *remove = NULL;
    gchar *insert = NULL;
    gchar *columns = NULL;
    gchar *values = NULL;
    gchar *valuetmp;
    gchar *oldcolumns;
    gchar *oldvalues;
    gchar *localuri;
    gchar *localvalue;

    columns = g_strdup_printf("uri");

    localuri = escape_sql(uri);
    values = g_strdup_printf("'%s'", localuri);

    if (data->title != NULL) {
        oldcolumns = columns;
        columns = g_strconcat(columns, ",title", NULL);
        g_free(oldcolumns);
        localvalue = escape_sql(data->title);
        valuetmp = g_strdup_printf(",'%s'", localvalue);
        g_free(localvalue);
        oldvalues = values;
        values = g_strconcat(values, valuetmp, NULL);
        g_free(oldvalues);
        g_free(valuetmp);
    }

    if (data->artist != NULL) {
        oldcolumns = columns;
        columns = g_strconcat(columns, ",artist", NULL);
        g_free(oldcolumns);
        localvalue = escape_sql(data->artist);
        valuetmp = g_strdup_printf(",'%s'", localvalue);
        g_free(localvalue);
        oldvalues = values;
        values = g_strconcat(values, valuetmp, NULL);
        g_free(oldvalues);
        g_free(valuetmp);
    }

    if (data->album != NULL) {
        oldcolumns = columns;
        columns = g_strconcat(columns, ",album", NULL);
        g_free(oldcolumns);
        localvalue = escape_sql(data->album);
        valuetmp = g_strdup_printf(",'%s'", localvalue);
        g_free(localvalue);
        oldvalues = values;
        values = g_strconcat(values, valuetmp, NULL);
        g_free(oldvalues);
        g_free(valuetmp);
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
        oldcolumns = columns;
        columns = g_strconcat(columns, ",audio_codec", NULL);
        g_free(oldcolumns);
        valuetmp = g_strdup_printf(",'%s'", data->audio_codec);
        oldvalues = values;
        values = g_strconcat(values, valuetmp, NULL);
        g_free(oldvalues);
        g_free(valuetmp);
    }

    if (data->video_codec != NULL) {
        oldcolumns = columns;
        columns = g_strconcat(columns, ",video_codec", NULL);
        g_free(oldcolumns);
        valuetmp = g_strdup_printf(",'%s'", data->video_codec);
        oldvalues = values;
        values = g_strconcat(values, valuetmp, NULL);
        g_free(oldvalues);
        g_free(valuetmp);
    }

    if (data->demuxer != NULL) {
        oldcolumns = columns;
        columns = g_strconcat(columns, ",demuxer", NULL);
        g_free(oldcolumns);
        valuetmp = g_strdup_printf(",'%s'", data->demuxer);
        oldvalues = values;
        values = g_strconcat(values, valuetmp, NULL);
        g_free(oldvalues);
        g_free(valuetmp);
    }

    oldcolumns = columns;
    columns = g_strconcat(columns, ",video_width, video_height, length", NULL);
    g_free(oldcolumns);
    valuetmp = g_strdup_printf(",%i,%i,%f", data->width, data->height, data->length_value);
    oldvalues = values;
    values = g_strconcat(values, valuetmp, NULL);
    g_free(oldvalues);
    g_free(valuetmp);


    remove = g_strdup_printf("delete from media_entries where uri='%s'", localuri);
    insert = g_strdup_printf("insert into media_entries (%s) values (%s)", columns, values);

    run_sql_non_select(conn, remove);
    run_sql_non_select(conn, insert);

    g_free(columns);
    g_free(values);
    g_free(remove);
    g_free(insert);

}


#endif
