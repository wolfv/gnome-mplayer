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
extern GdkWindow *get_window(GtkWidget * widget);
extern void get_allocation(GtkWidget * widget, GtkAllocation * allocation);
/*
void get_allocation(GtkWidget * widget, GtkAllocation * allocation);
GdkWindow *get_window(GtkWidget * widget);

void get_allocation(GtkWidget * widget, GtkAllocation * allocation)
{
#ifdef GTK2_18_ENABLED
    gtk_widget_get_allocation(widget, allocation);
#else
    allocation = &(widget->allocation);
#endif
}

GdkWindow *get_window(GtkWidget * widget)
{
#ifdef GTK2_14_ENABLED
    return gtk_widget_get_window(widget);
#else
    return widget->window;
#endif
}
*/

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
	gtk_widget_set_double_buffered(GTK_WIDGET(meter), FALSE);

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
	GtkAllocation alloc;
	cairo_t *cr;
	cairo_pattern_t *pattern;
	
	get_allocation(meter,&alloc);
	
    if (GMTK_AUDIO_METER(meter)->data == NULL)
        return;

    division_width = alloc.width / GMTK_AUDIO_METER(meter)->divisions;
    if (division_width < 2)
        division_width = 2;
    if (GMTK_AUDIO_METER(meter)->max_division_width > 0
        && division_width > GMTK_AUDIO_METER(meter)->max_division_width)
        division_width = GMTK_AUDIO_METER(meter)->max_division_width;

	cr = gdk_cairo_create(get_window(meter));
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);	
	cairo_set_line_width (cr, 2.0);
	
    for (i = 0; i < GMTK_AUDIO_METER(meter)->divisions; i++) {
        if (GMTK_AUDIO_METER(meter)->max_data) {
            v = g_array_index(GMTK_AUDIO_METER(meter)->max_data, gfloat, i);
            if (v >= 1.0)
                v = 1.0;
            if (v <= 0.0)
                v = 0.0;

			cairo_set_source_rgb(cr, meter->style->dark[0].red / 65535.0, meter->style->dark[0].green / 65535.0,meter->style->dark[0].blue / 65535.0);
			cairo_rectangle(cr, 
			                i * division_width,
                            alloc.height * (1.0 - v) + 3,
                            division_width, alloc.height * v);
			cairo_fill(cr);
			cairo_stroke(cr);
			
			cairo_set_source_rgb(cr, meter->style->mid[3].red / 65535.0, meter->style->mid[3].green / 65535.0,meter->style->mid[3].blue / 65535.0);
			cairo_rectangle(cr, 
			                i * division_width,
                            alloc.height * (1.0 - v) + 3,
                            division_width, alloc.height * v);
			cairo_stroke(cr);

        }
    }

    for (i = 0; i < GMTK_AUDIO_METER(meter)->divisions; i++) {

        v = g_array_index(GMTK_AUDIO_METER(meter)->data, gfloat, i);
        if (v >= 1.0)
            v = 1.0;
        if (v <= 0.0)
            v = 0.00;

		pattern = cairo_pattern_create_linear(0.0,0.0, 1.0, (gdouble)alloc.height);
		cairo_pattern_add_color_stop_rgb(pattern, 0.30, 1.0, 0, 0);
		cairo_pattern_add_color_stop_rgb(pattern, 0.7, 1.0, 1.0, 0);
		cairo_pattern_add_color_stop_rgb(pattern, 1.0, 0, 1.0, 0);
		
		cairo_set_source_rgb(cr, meter->style->mid[3].red / 65535.0, meter->style->mid[3].green / 65535.0,meter->style->mid[3].blue / 65535.0);
		cairo_rectangle(cr, 
		                i * division_width,
                        alloc.height * (1.0 - v) + 3,
                        division_width, alloc.height * v);
		cairo_set_source(cr, pattern);
		cairo_fill(cr);
		cairo_stroke(cr);
		cairo_pattern_destroy(pattern);

		cairo_set_source_rgb(cr, meter->style->fg[0].red / 65535.0, meter->style->fg[0].green / 65535.0,meter->style->fg[0].blue / 65535.0);
		cairo_rectangle(cr, 
		                i * division_width,
                        alloc.height * (1.0 - v) + 3,
                        division_width, alloc.height * v);
		cairo_stroke(cr);
		
    }
/*
    gdk_draw_rectangle(get_window(meter),
                       gtk_widget_get_style(meter)->text_aa_gc[0],
                       FALSE, 0, 0, meter->allocation.width - 1, meter->allocation.height - 1);
*/
	cairo_set_source_rgb(cr, meter->style->text_aa[0].red / 65535.0, meter->style->text_aa[0].green / 65535.0,meter->style->text_aa[0].blue / 65535.0);

	cairo_move_to(cr, 0, alloc.height -1);
	cairo_line_to(cr, alloc.width - 1, alloc.height - 1);

	cairo_destroy(cr);
	gdk_flush();
}

static gboolean gmtk_audio_meter_expose(GtkWidget * meter, GdkEventExpose * event)
{
    PangoLayout *p;

	gdk_window_begin_paint_region(get_window(meter),event->region);
    if (GMTK_AUDIO_METER(meter)->data_valid) {
        draw(meter);
    } else {
        p = gtk_widget_create_pango_layout(meter, "No Data");
        gdk_draw_layout(get_window(meter), gtk_widget_get_style(meter)->black_gc, 0, 0, p);
        g_object_unref(p);
    }
	gdk_window_end_paint(get_window(meter));
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

    if (get_window(GTK_WIDGET(meter)))
        gdk_window_invalidate_rect(get_window(GTK_WIDGET(meter)), NULL, FALSE);
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

    if (get_window(GTK_WIDGET(meter)))
        gdk_window_invalidate_rect(get_window(GTK_WIDGET(meter)), NULL, FALSE);
}

void gmtk_audio_meter_set_max_division_width(GmtkAudioMeter * meter, gint max_division_width)
{
    meter->max_division_width = max_division_width;
}
