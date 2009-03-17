/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gmtk_audio_meter.h
 * Copyright (C) Kevin DeKorte 2009 <kdekorte@gmail.com>
 * 
 * gmtk_audio_meter.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * gmtk_audio_meter.h is distributed in the hope that it will be useful,
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

#ifndef __GMTK_AUDIO_METER_H__
#define __GMTK_AUDIO_METER_H__

G_BEGIN_DECLS
#define GMTK_TYPE_AUDIO_METER		(gmtk_audio_meter_get_type ())
#define GMTK_AUDIO_METER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GMTK_TYPE_AUDIO_METER, GmtkAudioMeter))
#define GMTK_AUDIO_METER_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), GMTK_AUDIO_METER, GmtkAudioMeterClass))
#define GMTK_IS_AUDIO_METER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMTK_TYPE_AUDIO_METER))
#define GMTK_IS_AUDIO_METER_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), GMTK_TYPE_AUDIO_METER))
#define GMTK_AUDIO_METER_GET_CLASS	(G_TYPE_INSTANCE_GET_CLASS ((obj), GMTK_TYPE_AUDIO_METER, GmtkAudioMeterClass))
typedef struct _GmtkAudioMeter GmtkAudioMeter;
typedef struct _GmtkAudioMeterClass GmtkAudioMeterClass;

struct _GmtkAudioMeter {
    GtkDrawingArea parent;

    /* < private > */
    gint divisions;
    GArray *data;
    GArray *max_data;
    gboolean data_valid;
};

struct _GmtkAudioMeterClass {
    GtkDrawingAreaClass parent_class;
};
GType gmtk_audio_meter_get_type(void);
GtkWidget *gmtk_audio_meter_new(const gint divisions);
void gmtk_audio_meter_set_data(GmtkAudioMeter * meter, GArray * data);
void gmtk_audio_meter_set_data_full(GmtkAudioMeter * meter, GArray * data, GArray * max_data);

G_END_DECLS
#endif
