/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * thread.h
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * thread.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * thread.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with thread.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <glib.h>
#include "support.h"
#include "common.h"
#include <unistd.h>

typedef struct _PlayData {
    gchar filename[4096];
	gint playlist;
} PlayData;

gint std_in;
gint std_out;
gint std_err;

GIOChannel *channel_out;
GIOChannel *channel_in;
GIOChannel *channel_err;

guint watch_in_id;
guint watch_err_id;
guint watch_in_hup_id;

GThread *thread;
extern GMutex *thread_running;

gboolean thread_reader_error(GIOChannel * source, GIOCondition condition, gpointer data);
gboolean thread_reader(GIOChannel * source, GIOCondition condition, gpointer data);
gboolean thread_query(gpointer data);
