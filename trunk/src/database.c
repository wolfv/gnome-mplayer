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
    run_sql_non_select(conn, "create table if not exists media_entries (uri string not null primary key, "
                       "title string, artist string, album string, cover_art string, resume boolean, position real)");
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
        g_error("NON SELECT error: %s\n", error && error->message ? error->message : "no detail");
    g_object_unref(stmt);
}


#endif
