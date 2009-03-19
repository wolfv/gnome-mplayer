/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gmtk_media_tracker.c
 * Copyright (C) Kevin DeKorte 2009 <kdekorte@gmail.com>
 * 
 * gmtk_media_tracker.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * gmtk_media_tracker.c is distributed in the hope that it will be useful,
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

#include "gmtk_media_tracker.h"

G_DEFINE_TYPE(GmtkMediaTracker, gmtk_media_tracker, GTK_TYPE_DRAWING_AREA);

static gboolean gmtk_media_tracker_expose(GtkWidget * meter, GdkEventExpose * event);
static gboolean gmtk_media_tracker_button_press(GtkWidget * tracker, GdkEventButton * event);
static gboolean gmtk_media_tracker_button_release(GtkWidget * tracker, GdkEventButton * event);
static gboolean gmtk_media_tracker_motion_notify(GtkWidget * tracker, GdkEventMotion * event);
static void gmtk_media_tracker_dispose(GObject * object);

static void gmtk_media_tracker_class_init(GmtkMediaTrackerClass * class)
{
    GtkWidgetClass *widget_class;

    widget_class = GTK_WIDGET_CLASS(class);

    widget_class->expose_event = gmtk_media_tracker_expose;
    widget_class->button_press_event = gmtk_media_tracker_button_press;
    widget_class->button_release_event = gmtk_media_tracker_button_release;
    widget_class->motion_notify_event = gmtk_media_tracker_motion_notify;
    G_OBJECT_CLASS(class)->dispose = gmtk_media_tracker_dispose;
}

static void gmtk_media_tracker_init(GmtkMediaTracker * tracker)
{
    GdkPixbuf *temp;
    GtkIconTheme *icon_theme;
    PangoLayout *p;
    gint pwidth, pheight;

    gtk_widget_add_events(GTK_WIDGET(tracker),
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK);


    tracker->text = NULL;
    tracker->media_percent = 0.0;
    tracker->cache_percent = 0.0;

    p = gtk_widget_create_pango_layout(GTK_WIDGET(tracker), "/^q");
    pango_layout_get_size(p, &pwidth, &pheight);
    pwidth = pwidth / PANGO_SCALE;
    pheight = pheight / PANGO_SCALE;
    GTK_WIDGET(tracker)->requisition.height = pheight + 16;
    GTK_WIDGET(tracker)->requisition.width = 128;
    g_object_unref(p);

    icon_theme = gtk_icon_theme_get_default();
    temp = gtk_icon_theme_load_icon(icon_theme, "media-playback-start", 16, 0, NULL);
    tracker->thumb_lower = gdk_pixbuf_rotate_simple(temp, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
    tracker->thumb_upper = gdk_pixbuf_flip(tracker->thumb_lower, FALSE);
    gdk_pixbuf_unref(temp);

    tracker->position = THUMB_ON_BOTTOM;
}

static void gmtk_media_tracker_dispose(GObject * object)
{

    GmtkMediaTracker *tracker;

    tracker = GMTK_MEDIA_TRACKER(object);
    if (tracker->text) {
        g_free(tracker->text);
        tracker->text = NULL;
    }

    if (GDK_IS_PIXBUF(tracker->thumb_upper)) {
        gdk_pixbuf_unref(tracker->thumb_upper);
        gdk_pixbuf_unref(tracker->thumb_lower);
    }

}

void draw(GtkWidget * tracker)
{

    gint cache_width = 0;
    gint handle_left = 0;
    PangoLayout *p;
    gint pwidth, pheight;
    gint ptop, pleft;
    gint x;
    gint half_thumb_size;
    gint bar_width;

    half_thumb_size = gdk_pixbuf_get_width(GMTK_MEDIA_TRACKER(tracker)->thumb_lower) / 2;
    bar_width =
        tracker->allocation.width - gdk_pixbuf_get_width(GMTK_MEDIA_TRACKER(tracker)->thumb_lower);

    // draw the cache bar first, everything is over it
    if (GMTK_MEDIA_TRACKER(tracker)->cache_percent > 0.0) {
        cache_width = bar_width * GMTK_MEDIA_TRACKER(tracker)->cache_percent;

        gdk_draw_rectangle(tracker->window,
                           tracker->style->mid_gc[3],
                           TRUE, half_thumb_size, 6, cache_width, tracker->allocation.height - 11);

    }
    // draw the box and tick marks
    gdk_draw_rectangle(tracker->window,
                       tracker->style->dark_gc[0],
                       FALSE, half_thumb_size, 5, bar_width, tracker->allocation.height - 10);

	// if the thumb is hidden we can't seek so tickmarks are useless
	if (GMTK_MEDIA_TRACKER(tracker)->position != THUMB_HIDDEN) {
		for (x = half_thumb_size; x < bar_width; x = x + (bar_width / 10)) {
			gdk_draw_line(tracker->window, tracker->style->dark_gc[0], x, 5, x, 8);
			gdk_draw_line(tracker->window,
						  tracker->style->dark_gc[0], x, tracker->allocation.height - 5, x,
						  tracker->allocation.height - 8);
		}
	}

    // text over the background
    if (GMTK_MEDIA_TRACKER(tracker)->text) {
        p = gtk_widget_create_pango_layout(tracker, GMTK_MEDIA_TRACKER(tracker)->text);
        pango_layout_get_size(p, &pwidth, &pheight);
        pwidth = pwidth / PANGO_SCALE;
        pheight = pheight / PANGO_SCALE;
		if (pwidth > bar_width) {
				gtk_widget_set_size_request(tracker,(pwidth + 2 * half_thumb_size),-1);
		}

        ptop = (tracker->allocation.height - pheight) / 2;
        pleft = (tracker->allocation.width - pwidth) / 2;
        gdk_draw_layout(tracker->window, tracker->style->text_gc[0], pleft, ptop + 1, p);
        g_object_unref(p);
    }
    // draw handle, draw it last so it sits on top
    handle_left = bar_width * GMTK_MEDIA_TRACKER(tracker)->media_percent;
    if (handle_left < 0)
        handle_left = 0;
    if (handle_left > bar_width + half_thumb_size)
        handle_left = bar_width + half_thumb_size;
    if (cache_width > 0)
        if ((handle_left) > cache_width)
            handle_left = cache_width;

    if (GMTK_MEDIA_TRACKER(tracker)->position == THUMB_ON_TOP ||
        GMTK_MEDIA_TRACKER(tracker)->position == THUMB_ON_TOP_AND_BOTTOM) {

        gdk_draw_pixbuf(tracker->window, NULL,
                        GMTK_MEDIA_TRACKER(tracker)->thumb_upper, 0, 0, handle_left,
                        0, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
    }

    if (GMTK_MEDIA_TRACKER(tracker)->position == THUMB_ON_BOTTOM ||
        GMTK_MEDIA_TRACKER(tracker)->position == THUMB_ON_TOP_AND_BOTTOM) {

        gdk_draw_pixbuf(tracker->window, NULL,
                        GMTK_MEDIA_TRACKER(tracker)->thumb_lower, 0, 0, handle_left,
                        tracker->allocation.height -
                        gdk_pixbuf_get_height(GMTK_MEDIA_TRACKER(tracker)->thumb_lower) + 1, -1, -1,
                        GDK_RGB_DITHER_NONE, 0, 0);
    }
}


static gboolean gmtk_media_tracker_expose(GtkWidget * tracker, GdkEventExpose * event)
{
    draw(tracker);
    return FALSE;
}

static gboolean gmtk_media_tracker_button_press(GtkWidget * tracker, GdkEventButton * event)
{
    GMTK_MEDIA_TRACKER(tracker)->mouse_down = TRUE;
    return FALSE;
}

static gboolean gmtk_media_tracker_button_release(GtkWidget * tracker, GdkEventButton * event)
{
    gint half_thumb_size;
    gint bar_width;

    half_thumb_size = gdk_pixbuf_get_width(GMTK_MEDIA_TRACKER(tracker)->thumb_lower) / 2;
    bar_width =
        tracker->allocation.width - gdk_pixbuf_get_width(GMTK_MEDIA_TRACKER(tracker)->thumb_lower);

	if (GMTK_MEDIA_TRACKER(tracker)->position == THUMB_HIDDEN)
		return FALSE;
	
    GMTK_MEDIA_TRACKER(tracker)->media_percent = (event->x - half_thumb_size) / bar_width;

    if (GMTK_MEDIA_TRACKER(tracker)->media_percent > 1.0)
        GMTK_MEDIA_TRACKER(tracker)->media_percent = 1.0;

    if (GMTK_MEDIA_TRACKER(tracker)->cache_percent > 0.0)
        if (GMTK_MEDIA_TRACKER(tracker)->media_percent > GMTK_MEDIA_TRACKER(tracker)->cache_percent)
            GMTK_MEDIA_TRACKER(tracker)->media_percent = GMTK_MEDIA_TRACKER(tracker)->cache_percent;

    if (GTK_WIDGET(tracker)->window)
        gdk_window_invalidate_rect(GTK_WIDGET(tracker)->window, NULL, FALSE);

    GMTK_MEDIA_TRACKER(tracker)->mouse_down = FALSE;
    return FALSE;
}

static gboolean gmtk_media_tracker_motion_notify(GtkWidget * tracker, GdkEventMotion * event)
{
    gint half_thumb_size;
    gint bar_width;

    half_thumb_size = gdk_pixbuf_get_width(GMTK_MEDIA_TRACKER(tracker)->thumb_lower) / 2;
    bar_width =
        tracker->allocation.width - gdk_pixbuf_get_width(GMTK_MEDIA_TRACKER(tracker)->thumb_lower);

	if (GMTK_MEDIA_TRACKER(tracker)->position == THUMB_HIDDEN)
		return FALSE;

	
    if (GMTK_MEDIA_TRACKER(tracker)->mouse_down) {
        GMTK_MEDIA_TRACKER(tracker)->media_percent = (event->x - half_thumb_size) / bar_width;

        if (GMTK_MEDIA_TRACKER(tracker)->media_percent > 1.0)
            GMTK_MEDIA_TRACKER(tracker)->media_percent = 1.0;

        if (GMTK_MEDIA_TRACKER(tracker)->cache_percent > 0.0)
            if (GMTK_MEDIA_TRACKER(tracker)->media_percent >
                GMTK_MEDIA_TRACKER(tracker)->cache_percent)
                GMTK_MEDIA_TRACKER(tracker)->media_percent =
                    GMTK_MEDIA_TRACKER(tracker)->cache_percent;

        if (GTK_WIDGET(tracker)->window)
            gdk_window_invalidate_rect(GTK_WIDGET(tracker)->window, NULL, FALSE);
    }
    return FALSE;
}

GtkWidget *gmtk_media_tracker_new()
{
    return g_object_new(GMTK_TYPE_MEDIA_TRACKER, NULL);
}


void gmtk_media_tracker_set_percentage(GmtkMediaTracker * tracker, gdouble percentage)
{
    tracker->media_percent = percentage;
    if (tracker->media_percent > 1.0)
        tracker->media_percent = 1.0;
    if (tracker->media_percent < 0.0)
        tracker->media_percent = 0.0;

    if (GTK_WIDGET(tracker)->window)
        gdk_window_invalidate_rect(GTK_WIDGET(tracker)->window, NULL, FALSE);
}

gdouble gmtk_media_tracker_get_percentage(GmtkMediaTracker * tracker)
{
    return tracker->media_percent;
}

void gmtk_media_tracker_set_text(GmtkMediaTracker * tracker, const gchar * text)
{
    if (tracker->text) {
        g_free(tracker->text);
        tracker->text = NULL;
    }

    if (text)
        tracker->text = g_strdup(text);

    if (GTK_WIDGET(tracker)->window)
        gdk_window_invalidate_rect(GTK_WIDGET(tracker)->window, NULL, FALSE);
}

void gmtk_media_tracker_set_cache_percentage(GmtkMediaTracker * tracker, gdouble percentage)
{
    tracker->cache_percent = percentage;

    if (tracker->cache_percent > 1.0)
        tracker->cache_percent = 1.0;
    if (tracker->cache_percent < 0.0)
        tracker->cache_percent = 0.0;

    if (GTK_WIDGET(tracker)->window)
        gdk_window_invalidate_rect(GTK_WIDGET(tracker)->window, NULL, FALSE);
}

gdouble gmtk_media_tracker_get_cache_percentage(GmtkMediaTracker * tracker)
{
    return tracker->cache_percent;
}

void gmtk_media_tracker_set_thumb_position(GmtkMediaTracker * tracker, GmtkThumbPosition position)
{
    tracker->position = position;
}
