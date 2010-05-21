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

G_DEFINE_TYPE(GmtkMediaTracker, gmtk_media_tracker, GTK_TYPE_VBOX);

static gboolean gmtk_media_tracker_button_press(GtkWidget * tracker, GdkEventButton * event);
static gboolean gmtk_media_tracker_button_release(GtkWidget * tracker, GdkEventButton * event);
static gboolean gmtk_media_tracker_motion_notify(GtkWidget * tracker, GdkEventMotion * event);
void gmtk_media_tracker_set_timer(GmtkMediaTracker * tracker, const gchar * text);
static void gmtk_media_tracker_dispose(GObject * object);
gchar *gm_seconds_to_string(gfloat seconds);


static void gmtk_media_tracker_class_init(GmtkMediaTrackerClass * class)
{
    GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass*) class;
	widget_class = GTK_WIDGET_CLASS(class);

    widget_class->button_press_event = gmtk_media_tracker_button_press;
    widget_class->button_release_event = gmtk_media_tracker_button_release;
    widget_class->motion_notify_event = gmtk_media_tracker_motion_notify;
    G_OBJECT_CLASS(class)->dispose = gmtk_media_tracker_dispose;

	g_signal_new ("value-changed",
				  G_OBJECT_CLASS_TYPE (object_class),
				  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
				  G_STRUCT_OFFSET (GmtkMediaTrackerClass, value_changed),
				  NULL, NULL,
				  gtk_marshal_VOID__INT,
				  G_TYPE_NONE, 1, G_TYPE_INT);

	g_signal_new ("difference-changed",
				  G_OBJECT_CLASS_TYPE (object_class),
				  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
				  G_STRUCT_OFFSET (GmtkMediaTrackerClass, difference_changed),
				  NULL, NULL,
				  g_cclosure_marshal_VOID__DOUBLE,
				  G_TYPE_NONE, 1, G_TYPE_DOUBLE);
	
}

static void gmtk_media_tracker_init(GmtkMediaTracker * tracker)
{
    gtk_widget_add_events(GTK_WIDGET(tracker),
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK);


    tracker->text = NULL;
	tracker->timer_text = NULL;
    tracker->media_percent = 0.0;
    tracker->cache_percent = 0.0;

	tracker->position = 0.0;
	tracker->length = 0.0;
	tracker->allow_expand = TRUE;
	

	gtk_widget_push_composite_child();
	
	tracker->scale = gtk_hscale_new_with_range(0,1,0.001);	
	gtk_scale_set_draw_value(GTK_SCALE(tracker->scale),FALSE);
	gtk_widget_set_size_request(tracker->scale,200,-1);
	gtk_widget_show(tracker->scale);
	gtk_box_pack_start(GTK_BOX(tracker),GTK_WIDGET(tracker->scale),TRUE,TRUE,0);
	gtk_widget_set_sensitive(tracker->scale, FALSE);

#ifdef GTK2_12_ENABLED
	gtk_widget_set_tooltip_text(GTK_WIDGET(tracker->scale), _("No Information"));
#else
	tracker->progress_tip = gtk_tooltips_new();
   	gtk_tooltips_set_tip(tracker->progress_tip, GTK_WIDGET(tracker->scale), _("No Information"), NULL);
#endif
	g_signal_connect_swapped(G_OBJECT(tracker->scale),"button-press-event",G_CALLBACK(gmtk_media_tracker_button_press),tracker);
	g_signal_connect_swapped(G_OBJECT(tracker->scale),"button-release-event",G_CALLBACK(gmtk_media_tracker_button_release),tracker);
	g_signal_connect_swapped(G_OBJECT(tracker->scale),"motion-notify-event",G_CALLBACK(gmtk_media_tracker_motion_notify),tracker);

	
	tracker->hbox = gtk_hbox_new(0,FALSE);

	tracker->message = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(tracker->message),"<small> </small>");
	gtk_misc_set_alignment(GTK_MISC(tracker->message),0.0,0.0);
	gtk_label_set_ellipsize(GTK_LABEL(tracker->message),PANGO_ELLIPSIZE_END);
	
	tracker->timer = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(tracker->timer),"<small>0:00</small>");
	gtk_misc_set_alignment(GTK_MISC(tracker->timer),1.0,0.0);

	gtk_box_pack_start(GTK_BOX(tracker->hbox),tracker->message,TRUE,TRUE,5);
	gtk_box_pack_end(GTK_BOX(tracker->hbox),tracker->timer,FALSE,TRUE,0);

	gtk_box_pack_end(GTK_BOX(tracker),tracker->hbox,FALSE,TRUE,0);
	

	gtk_widget_pop_composite_child();
}

static void gmtk_media_tracker_dispose(GObject * object)
{

    GmtkMediaTracker *tracker;

    tracker = GMTK_MEDIA_TRACKER(object);
    if (tracker->text) {
        g_free(tracker->text);
        tracker->text = NULL;
    }

    if (tracker->timer_text) {
        g_free(tracker->timer_text);
        tracker->timer_text = NULL;
    }
	
}


static gboolean gmtk_media_tracker_button_press(GtkWidget * tracker, GdkEventButton * event)
{
    GMTK_MEDIA_TRACKER(tracker)->mouse_down = TRUE;

	return TRUE;
}

static gboolean gmtk_media_tracker_button_release(GtkWidget * tracker, GdkEventButton * event)
{
	gdouble position;
	gdouble difference;
	GtkAllocation alloc;

	get_allocation(tracker,&alloc);
	
	if (GMTK_MEDIA_TRACKER(tracker)->mouse_down) {
		position = (gdouble)event->x / alloc.width;
		gtk_range_set_value(GTK_RANGE(GMTK_MEDIA_TRACKER(tracker)->scale),position);
		g_signal_emit_by_name(tracker,"value-changed", (gint)(100 * position));

		difference = (GMTK_MEDIA_TRACKER(tracker)->length * position) - GMTK_MEDIA_TRACKER(tracker)->position;
		g_signal_emit_by_name(tracker,"difference-changed", difference);

		GMTK_MEDIA_TRACKER(tracker)->mouse_down = FALSE;
		
		if (GMTK_MEDIA_TRACKER(tracker)->length > 0.0) {
			gmtk_media_tracker_set_position(GMTK_MEDIA_TRACKER(tracker), GMTK_MEDIA_TRACKER(tracker)->length * position);
		}

	}
    return FALSE;
}

static gboolean gmtk_media_tracker_motion_notify(GtkWidget * tracker, GdkEventMotion * event)
{
	gchar *tip;
	gdouble position;
	gdouble difference;
	GtkAllocation alloc;

	gtk_widget_get_allocation(tracker,&alloc);

	position = (gdouble)event->x / alloc.width;
	if (GMTK_MEDIA_TRACKER(tracker)->mouse_down) {
		gtk_range_set_value(GTK_RANGE(GMTK_MEDIA_TRACKER(tracker)->scale), position);
		g_signal_emit_by_name(tracker,"value-changed", (gint)(100 * position));
		difference = (GMTK_MEDIA_TRACKER(tracker)->length * position) - GMTK_MEDIA_TRACKER(tracker)->position;
		if (ABS(position) > 15)
		    g_signal_emit_by_name(tracker,"difference-changed", difference);
		    
	} else {
		if (GMTK_MEDIA_TRACKER(tracker)->length > 0.0) {
			tip = gm_seconds_to_string(GMTK_MEDIA_TRACKER(tracker)->length * ((gdouble)event->x / alloc.width));
		} else {
			tip = g_strdup(_("No Information"));
		}
#ifdef GTK2_12_ENABLED
		gtk_widget_set_tooltip_text(GMTK_MEDIA_TRACKER(tracker)->scale, tip);
#else		
		gtk_tooltips_set_tip(GMTK_MEDIA_TRACKER(tracker)->progress_tip, GTK_WIDGET(GMTK_MEDIA_TRACKER(tracker)->scale), tip, NULL);
#endif
		if (tip)
			g_free(tip);
	}

	return FALSE;
}

GtkWidget *gmtk_media_tracker_new()
{
	GtkWidget *tracker;

	tracker = g_object_new(GMTK_TYPE_MEDIA_TRACKER,"spacing",0,"homogeneous",FALSE,NULL);
	return tracker;
}


void gmtk_media_tracker_set_percentage(GmtkMediaTracker * tracker, gdouble percentage)
{
    tracker->media_percent = percentage;
    if (tracker->media_percent > 1.0)
        tracker->media_percent = 1.0;
    if (tracker->media_percent < 0.0)
        tracker->media_percent = 0.0;

	gtk_range_set_value(GTK_RANGE(tracker->scale), tracker->media_percent);
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
        tracker->text = g_markup_printf_escaped("<small>%s</small>",text);
	
	gtk_label_set_markup(GTK_LABEL(tracker->message),tracker->text);
	
}

void gmtk_media_tracker_set_timer(GmtkMediaTracker * tracker, const gchar * text)
{
    if (tracker->timer_text) {
        g_free(tracker->timer_text);
        tracker->timer_text = NULL;
    }

    if (text)
        tracker->timer_text = g_markup_printf_escaped("<small>%s</small>",text);
	
	gtk_label_set_markup(GTK_LABEL(tracker->timer),tracker->timer_text);
	
}

void gmtk_media_tracker_set_position(GmtkMediaTracker * tracker, const gfloat seconds)
{
	gchar *time_position;
	gchar *time_length;
	gchar *text;

	tracker->position = seconds;
	
	if (tracker->length > 0.0) {
		gtk_widget_set_sensitive(tracker->scale, TRUE);
		time_position = gm_seconds_to_string(tracker->position);
		time_length = gm_seconds_to_string(tracker->length);
		text = g_strdup_printf("%s / %s",time_position,time_length);
		gmtk_media_tracker_set_timer(tracker,text);
		g_free(text);
		g_free(time_length);
		g_free(time_position);
	} else {
		gtk_widget_set_sensitive(tracker->scale, FALSE);
		time_position = gm_seconds_to_string(tracker->position);
		gmtk_media_tracker_set_timer(tracker,time_position);
		g_free(time_position);
	}	

}

void gmtk_media_tracker_set_length(GmtkMediaTracker * tracker, const gfloat seconds)
{
	
	tracker->length = seconds;
	gmtk_media_tracker_set_position(tracker,tracker->position);

}



gchar *gm_seconds_to_string(gfloat seconds)
{
    guint hour = 0, min = 0, sec = 0;
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
        result = g_strdup_printf("%2i:%02i", min, sec);
    } else {
        result = g_strdup_printf("%i:%02i:%02i", hour, min, sec);
    }
    return result;
}
