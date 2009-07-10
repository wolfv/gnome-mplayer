/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gmtk_media_tracker.h
 * Copyright (C) Kevin DeKorte 2009 <kdekorte@gmail.com>
 * 
 * gmtk_media_tracker.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * gmtk_media_tracker.h is distributed in the hope that it will be useful,
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

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifndef __GMTK_MEDIA_TRACKER_H__
#define __GMTK_MEDIA_TRACKER_H__

G_BEGIN_DECLS
#define GMTK_TYPE_MEDIA_TRACKER		(gmtk_media_tracker_get_type ())
#define GMTK_MEDIA_TRACKER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GMTK_TYPE_MEDIA_TRACKER, GmtkMediaTracker))
#define GMTK_MEDIA_TRACKER_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), GMTK_MEDIA_TRACKER, GmtkMediaTrackerClass))
#define GMTK_IS_MEDIA_TRACKER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMTK_TYPE_MEDIA_TRACKER))
#define GMTK_IS_MEDIA_TRACKER_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), GMTK_TYPE_MEDIA_TRACKER))
#define GMTK_MEDIA_TRACKER_GET_CLASS	(G_TYPE_INSTANCE_GET_CLASS ((obj), GMTK_TYPE_MEDIA_TRACKER, GmtkMediaTrackerClass))

typedef enum {
	THUMB_HIDDEN,
    THUMB_ON_BOTTOM,
    THUMB_ON_TOP,
    THUMB_ON_TOP_AND_BOTTOM
} GmtkThumbPosition;

typedef struct _GmtkMediaTracker GmtkMediaTracker;
typedef struct _GmtkMediaTrackerClass GmtkMediaTrackerClass;

struct _GmtkMediaTracker {
    GtkVBox parent;

	GtkWidget *GSEAL(scale);
	GtkWidget *GSEAL(hbox);
	GtkWidget *GSEAL(message);
	GtkWidget *GSEAL(timer);
	GtkTooltips *GSEAL(progress_tip);
	
    /* < private > */
    gdouble media_percent;
    gdouble cache_percent;
    gchar *text;
	gchar *timer_text;
	gfloat position;
	gfloat length;
    gboolean mouse_down;
	gboolean allow_expand;
};

struct _GmtkMediaTrackerClass {
    GtkVBoxClass parent_class;
	void (* value_changed)  (GmtkMediaTracker *tracker);

};

GType gmtk_media_tracker_get_type(void);
GtkWidget *gmtk_media_tracker_new();

void gmtk_media_tracker_set_percentage(GmtkMediaTracker * tracker, gdouble percentage);
gdouble gmtk_media_tracker_get_percentage(GmtkMediaTracker * tracker);

void gmtk_media_tracker_set_text(GmtkMediaTracker * tracker, const gchar * text);

void gmtk_media_tracker_set_position(GmtkMediaTracker * tracker, const gfloat seconds);
void gmtk_media_tracker_set_length(GmtkMediaTracker * tracker, const gfloat seconds);

G_END_DECLS
#endif
