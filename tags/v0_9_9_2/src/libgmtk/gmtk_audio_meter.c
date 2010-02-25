/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gmtk_audio_meter.c
 * Copyright (C) Kevin DeKorte 2009 <kdekorte@gmail.com>
 * 
 * gmtk_audio_meter.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * gmtk_audio_meter.c is distributed in the hope that it will be useful,
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

#include "gmtk_audio_meter.h"

G_DEFINE_TYPE(GmtkAudioMeter, gmtk_audio_meter, GTK_TYPE_DRAWING_AREA);

static gboolean gmtk_audio_meter_expose(GtkWidget * meter, GdkEventExpose * event);
static void gmtk_audio_meter_dispose(GObject * object);

static void gmtk_audio_meter_class_init(GmtkAudioMeterClass * class)
{
    GtkWidgetClass *widget_class;

    widget_class = GTK_WIDGET_CLASS(class);

    widget_class->expose_event = gmtk_audio_meter_expose;
    G_OBJECT_CLASS(class)->dispose = gmtk_audio_meter_dispose;
}

static void gmtk_audio_meter_init(GmtkAudioMeter * meter)
{
    meter->divisions = 0;
    meter->data = NULL;
    meter->max_data = NULL;
    meter->data_valid = FALSE;
    meter->max_division_width = -1;
	GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET(meter), GTK_DOUBLE_BUFFERED);

}

static void gmtk_audio_meter_dispose(GObject * object)
{

    GmtkAudioMeter *meter;

    meter = GMTK_AUDIO_METER(object);

    meter->data_valid = FALSE;
    if (meter->data) {
        g_array_free(meter->data, TRUE);
        meter->data = NULL;
    }

    if (meter->max_data) {
        g_array_free(meter->max_data, TRUE);
        meter->max_data = NULL;
    }
}


static void draw(GtkWidget * meter)
{
    gint i;
    gfloat v;
    gint division_width;

    if (GMTK_AUDIO_METER(meter)->data == NULL)
        return;

    division_width = meter->allocation.width / GMTK_AUDIO_METER(meter)->divisions;
    if (division_width < 2)
        division_width = 2;
    if (GMTK_AUDIO_METER(meter)->max_division_width > 0
        && division_width > GMTK_AUDIO_METER(meter)->max_division_width)
        division_width = GMTK_AUDIO_METER(meter)->max_division_width;

    for (i = 0; i < GMTK_AUDIO_METER(meter)->divisions; i++) {
        if (GMTK_AUDIO_METER(meter)->max_data) {
            v = g_array_index(GMTK_AUDIO_METER(meter)->max_data, gfloat, i);
            if (v >= 1.0)
                v = 1.0;
            if (v <= 0.0)
                v = 0.0;

            gdk_draw_rectangle(meter->window,
                               meter->style->dark_gc[0],
                               TRUE,
                               i * division_width,
                               meter->allocation.height * (1.0 - v) + 3,
                               division_width, meter->allocation.height * v);

            gdk_draw_rectangle(meter->window,
                               meter->style->mid_gc[3],
                               FALSE,
                               i * division_width,
                               meter->allocation.height * (1.0 - v) + 3,
                               division_width, meter->allocation.height * v);

        }
    }

    for (i = 0; i < GMTK_AUDIO_METER(meter)->divisions; i++) {

        v = g_array_index(GMTK_AUDIO_METER(meter)->data, gfloat, i);
        if (v >= 1.0)
            v = 1.0;
        if (v <= 0.0)
            v = 0.00;

        gdk_draw_rectangle(meter->window,
                           meter->style->mid_gc[3],
                           TRUE,
                           i * division_width,
                           meter->allocation.height * (1.0 - v) + 3,
                           division_width, meter->allocation.height * v);

        gdk_draw_rectangle(meter->window,
                           meter->style->fg_gc[0],
                           FALSE,
                           i * division_width,
                           meter->allocation.height * (1.0 - v) + 3,
                           division_width, meter->allocation.height * v);

    }
/*
    gdk_draw_rectangle(meter->window,
                       meter->style->text_aa_gc[0],
                       FALSE, 0, 0, meter->allocation.width - 1, meter->allocation.height - 1);
*/

    gdk_draw_line(meter->window, meter->style->text_aa_gc[0], 0,
                  meter->allocation.height - 1, meter->allocation.width - 1,
                  meter->allocation.height - 1);
}

static gboolean gmtk_audio_meter_expose(GtkWidget * meter, GdkEventExpose * event)
{
    PangoLayout *p;

	gdk_window_begin_paint_region(meter->window,event->region);
    if (GMTK_AUDIO_METER(meter)->data_valid) {
        draw(meter);
    } else {
        p = gtk_widget_create_pango_layout(meter, "No Data");
        gdk_draw_layout(meter->window, meter->style->black_gc, 0, 0, p);
        g_object_unref(p);
    }
	gdk_window_end_paint(meter->window);
    return FALSE;
}

GtkWidget *gmtk_audio_meter_new(const gint divisions)
{
    GmtkAudioMeter *meter;

    meter = g_object_new(GMTK_TYPE_AUDIO_METER, NULL);
    meter->divisions = divisions;

    return GTK_WIDGET(meter);
}

void gmtk_audio_meter_set_data(GmtkAudioMeter * meter, GArray * data)
{
    gint i;

    meter->data_valid = FALSE;
    if (meter->data) {
        g_array_free(meter->data, TRUE);
        meter->data = NULL;
    }

    if (meter->max_data) {
        g_array_free(meter->max_data, TRUE);
        meter->max_data = NULL;
    }

    if (data != NULL) {
        meter->data = g_array_new(FALSE, TRUE, sizeof(gfloat));

        for (i = 0; i < meter->divisions; i++) {
            g_array_append_val(meter->data, g_array_index(data, gfloat, i));
        }
        meter->data_valid = TRUE;
    }

    if (GTK_WIDGET(meter)->window)
        gdk_window_invalidate_rect(GTK_WIDGET(meter)->window, NULL, FALSE);
}

void gmtk_audio_meter_set_data_full(GmtkAudioMeter * meter, GArray * data, GArray * max_data)
{
    gint i;

    meter->data_valid = FALSE;
    if (meter->data) {
        g_array_free(meter->data, TRUE);
        meter->data = NULL;
    }

    if (meter->max_data) {
        g_array_free(meter->max_data, TRUE);
        meter->max_data = NULL;
    }

    if (data != NULL && max_data != NULL) {
        meter->data = g_array_new(FALSE, TRUE, sizeof(gfloat));
        meter->max_data = g_array_new(FALSE, TRUE, sizeof(gfloat));

        for (i = 0; i < meter->divisions; i++) {
            g_array_append_val(meter->data, g_array_index(data, gfloat, i));
            g_array_append_val(meter->max_data, g_array_index(max_data, gfloat, i));
        }
        meter->data_valid = TRUE;
    }

    if (GTK_WIDGET(meter)->window)
        gdk_window_invalidate_rect(GTK_WIDGET(meter)->window, NULL, FALSE);
}

void gmtk_audio_meter_set_max_division_width(GmtkAudioMeter * meter, gint max_division_width)
{
    meter->max_division_width = max_division_width;
}
