/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gm_file.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * gm_file.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * gm_file.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with playlist.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "gm_file.h"

gchar *gm_tempname(gchar * path, const gchar * name_template)
{
    gchar *result;
    gchar *replace;
    gchar *basename;
    gchar *localpath;

    basename = g_strdup(name_template);

    if (path == NULL && g_getenv("TMPDIR") == NULL) {
        localpath = g_strdup("/tmp");
    } else if (path == NULL && g_getenv("TMPDIR") != NULL) {
        localpath = g_strdup(g_getenv("TMPDIR"));
    } else {
        localpath = g_strdup(path);
    }

    while ((replace = g_strrstr(basename, "X"))) {
        replace[0] = (gchar) g_random_int_range((gint) 'a', (gint) 'z');
    }

    result = g_strdup_printf("%s/%s", localpath, basename);
    g_free(basename);
    g_free(localpath);

    return result;
}
