/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gmtk_output_combo_box.c
 * Copyright (C) Kevin DeKorte 2009 <kdekorte@gmail.com>
 * 
 * gmtk_output_combo_box.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * gmtk_output_combo_box.c is distributed in the hope that it will be useful,
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

#include "gmtk_output_combo_box.h"

static GObjectClass *parent_class = NULL;

gint sort_iter_compare_func(GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer data)
{
    gint sortcol = GPOINTER_TO_INT(data);
    gint ret = 0;
    gchar *a_desc;
    gchar *b_desc;

    switch (sortcol) {
    case OUTPUT_DESCRIPTION_COLUMN:
        gtk_tree_model_get(model, a, OUTPUT_DESCRIPTION_COLUMN, &a_desc, -1);
        gtk_tree_model_get(model, b, OUTPUT_DESCRIPTION_COLUMN, &b_desc, -1);

        if (a_desc == NULL || b_desc == NULL) {
            if (a_desc == NULL && b_desc == NULL)
                break;          /* both equal => ret = 0 */

            ret = (a_desc == NULL) ? -1 : 1;
        } else {
            ret = g_utf8_collate(a_desc, b_desc);
        }

        g_free(a_desc);
        g_free(b_desc);
        break;

    default:
        printf("unimplemented sort routine\n");
    }

    return ret;

}

#ifdef HAVE_PULSEAUDIO

void pa_sink_cb(pa_context * c, const pa_sink_info * i, int eol, gpointer data)
{
    GmtkOutputComboBox *output = GMTK_OUTPUT_COMBO_BOX(data);
    GtkTreeIter iter;
    gchar *name;
    gchar *device;

    if (i) {
        name = g_strdup_printf("%s (PulseAudio)", i->description);
        device = g_strdup_printf("pulse::%i", i->index);
        gtk_list_store_append(output->list, &iter);
        gtk_list_store_set(output->list, &iter, OUTPUT_TYPE_COLUMN, OUTPUT_TYPE_PULSE,
                           OUTPUT_DESCRIPTION_COLUMN, name, OUTPUT_CARD_COLUMN, -1,
                           OUTPUT_DEVICE_COLUMN, -1, OUTPUT_INDEX_COLUMN, i->index,
                           OUTPUT_MPLAYER_DEVICE_COLUMN, device, -1);
        g_free(device);
        g_free(name);

    }
}

void context_state_callback(pa_context * context, gpointer data)
{
    //printf("context state callback\n");
    int i;

    switch (pa_context_get_state(context)) {
    case PA_CONTEXT_READY:{
            for (i = 0; i < 255; i++) {
                pa_context_get_sink_info_by_index(context, i, pa_sink_cb, data);
            }
        }

    default:
        return;
    }

}
#endif



G_DEFINE_TYPE(GmtkOutputComboBox, gmtk_output_combo_box, GTK_TYPE_COMBO_BOX);

static void gmtk_output_combo_box_class_init(GmtkOutputComboBoxClass * class)
{
    GtkWidgetClass *widget_class;
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS(class);
    widget_class = GTK_WIDGET_CLASS(class);

    parent_class = g_type_class_peek_parent(class);
}

static void gmtk_output_combo_box_init(GmtkOutputComboBox * output)
{
    GtkTreeIter iter;
    GtkCellRenderer *renderer;
    GtkTreeSortable *sortable;

    gint card, err, dev;
    gchar *name = NULL;
    gchar *menu;
    gchar *mplayer_device;

#ifdef HAVE_ASOUNDLIB
    snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
    snd_ctl_t *handle;
    snd_ctl_card_info_t *info;
    snd_pcm_info_t *pcminfo;
#endif

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(output), renderer, FALSE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(output), renderer, "text", 0);

    output->list =
        gtk_list_store_new(OUTPUT_N_COLUMNS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT,
                           G_TYPE_STRING);

    gtk_list_store_append(output->list, &iter);
    gtk_list_store_set(output->list, &iter, OUTPUT_TYPE_COLUMN, OUTPUT_TYPE_SOFTVOL,
                       OUTPUT_DESCRIPTION_COLUMN, _("Default"), OUTPUT_CARD_COLUMN, -1,
                       OUTPUT_DEVICE_COLUMN, -1, OUTPUT_MPLAYER_DEVICE_COLUMN, "", -1);
    gtk_list_store_append(output->list, &iter);
    gtk_list_store_set(output->list, &iter, OUTPUT_TYPE_COLUMN, OUTPUT_TYPE_SOFTVOL,
                       OUTPUT_DESCRIPTION_COLUMN, "ARTS", OUTPUT_CARD_COLUMN, -1,
                       OUTPUT_DEVICE_COLUMN, -1, OUTPUT_MPLAYER_DEVICE_COLUMN, "arts", -1);
    gtk_list_store_append(output->list, &iter);
    gtk_list_store_set(output->list, &iter, OUTPUT_TYPE_COLUMN, OUTPUT_TYPE_SOFTVOL,
                       OUTPUT_DESCRIPTION_COLUMN, "ESD", OUTPUT_CARD_COLUMN, -1,
                       OUTPUT_DEVICE_COLUMN, -1, OUTPUT_MPLAYER_DEVICE_COLUMN, "esd", -1);
    gtk_list_store_append(output->list, &iter);
    gtk_list_store_set(output->list, &iter, OUTPUT_TYPE_COLUMN, OUTPUT_TYPE_SOFTVOL,
                       OUTPUT_DESCRIPTION_COLUMN, "JACK", OUTPUT_CARD_COLUMN, -1,
                       OUTPUT_DEVICE_COLUMN, -1, OUTPUT_MPLAYER_DEVICE_COLUMN, "jack", -1);
    gtk_list_store_append(output->list, &iter);
    gtk_list_store_set(output->list, &iter, OUTPUT_TYPE_COLUMN, OUTPUT_TYPE_SOFTVOL,
                       OUTPUT_DESCRIPTION_COLUMN, "OSS", OUTPUT_CARD_COLUMN, -1,
                       OUTPUT_DEVICE_COLUMN, -1, OUTPUT_MPLAYER_DEVICE_COLUMN, "oss", -1);
    gtk_list_store_append(output->list, &iter);
    gtk_list_store_set(output->list, &iter, OUTPUT_TYPE_COLUMN, OUTPUT_TYPE_SOFTVOL,
                       OUTPUT_DESCRIPTION_COLUMN, "ALSA", OUTPUT_CARD_COLUMN, -1,
                       OUTPUT_DEVICE_COLUMN, -1, OUTPUT_MPLAYER_DEVICE_COLUMN, "alsa", -1);
    gtk_list_store_append(output->list, &iter);
    gtk_list_store_set(output->list, &iter, OUTPUT_TYPE_COLUMN, OUTPUT_TYPE_SOFTVOL,
                       OUTPUT_DESCRIPTION_COLUMN, "PulseAudio", OUTPUT_CARD_COLUMN, -1,
                       OUTPUT_DEVICE_COLUMN, -1, OUTPUT_INDEX_COLUMN, -1, OUTPUT_MPLAYER_DEVICE_COLUMN, "pulse", -1);

#ifdef HAVE_ASOUNDLIB
    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo);

    card = -1;

    while (snd_card_next(&card) >= 0) {
        //printf("card = %i\n", card);
        if (card < 0)
            break;
        if (name != NULL) {
            free(name);
            name = NULL;
        }
        name = malloc(32);
        sprintf(name, "hw:%i", card);
        //printf("name = %s\n",name);

        if ((err = snd_ctl_open(&handle, name, 0)) < 0) {
            continue;
        }

        if ((err = snd_ctl_card_info(handle, info)) < 0) {
            snd_ctl_close(handle);
            continue;
        }

        dev = -1;
        while (1) {
            snd_ctl_pcm_next_device(handle, &dev);
            if (dev < 0)
                break;
            snd_pcm_info_set_device(pcminfo, dev);
            snd_pcm_info_set_subdevice(pcminfo, 0);
            snd_pcm_info_set_stream(pcminfo, stream);
            if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0) {
                continue;
            }

            menu = g_strdup_printf("%s (%s) (alsa)", snd_ctl_card_info_get_name(info), snd_pcm_info_get_name(pcminfo));
            mplayer_device = g_strdup_printf("alsa:device=hw=%i.%i", card, dev);

            gtk_list_store_append(output->list, &iter);
            gtk_list_store_set(output->list, &iter, OUTPUT_TYPE_COLUMN, OUTPUT_TYPE_ALSA,
                               OUTPUT_DESCRIPTION_COLUMN, menu, OUTPUT_CARD_COLUMN, card,
                               OUTPUT_DEVICE_COLUMN, dev, OUTPUT_MPLAYER_DEVICE_COLUMN, mplayer_device, -1);
        }

        snd_ctl_close(handle);

    }

#else

#ifdef __OpenBSD__
    gtk_list_store_append(output->list, &iter);
    gtk_list_store_set(output->list, &iter, OUTPUT_TYPE_COLUMN, OUTPUT_TYPE_SOFTVOL,
                       OUTPUT_DESCRIPTION_COLUMN, "SNDIO", OUTPUT_CARD_COLUMN, -1,
                       OUTPUT_DEVICE_COLUMN, -1, OUTPUT_MPLAYER_DEVICE_COLUMN, "sndio", -1);
    gtk_list_store_append(output->list, &iter);
    gtk_list_store_set(output->list, &iter, OUTPUT_TYPE_COLUMN, OUTPUT_TYPE_SOFTVOL,
                       OUTPUT_DESCRIPTION_COLUMN, "RTunes", OUTPUT_CARD_COLUMN, -1,
                       OUTPUT_DEVICE_COLUMN, -1, OUTPUT_MPLAYER_DEVICE_COLUMN, "rtunes", -1);
#endif

#endif

#ifdef HAVE_PULSEAUDIO

    pa_glib_mainloop *loop = pa_glib_mainloop_new(g_main_context_default());
    pa_context *context = pa_context_new(pa_glib_mainloop_get_api(loop), "gmtk context");
    if (context) {
        pa_context_connect(context, NULL, 0, NULL);
        pa_context_set_state_callback(context, context_state_callback, output);
    }
    // make sure the pulse events are done before we exit this function
    while (gtk_events_pending())
        gtk_main_iteration();


#endif

    sortable = GTK_TREE_SORTABLE(output->list);
    gtk_tree_sortable_set_sort_func(sortable, OUTPUT_DESCRIPTION_COLUMN, sort_iter_compare_func,
                                    GINT_TO_POINTER(OUTPUT_DESCRIPTION_COLUMN), NULL);
    gtk_tree_sortable_set_sort_column_id(sortable, OUTPUT_DESCRIPTION_COLUMN, GTK_SORT_ASCENDING);

    gtk_combo_box_set_model(GTK_COMBO_BOX(output), GTK_TREE_MODEL(output->list));
    //gtk_combo_box_set_active(GTK_COMBO_BOX(output), 0);


}

static void gmtk_output_combo_box_dispose(GObject * object)
{
    GmtkOutputComboBox *output = GMTK_OUTPUT_COMBO_BOX(object);

    gtk_list_store_clear(output->list);
    g_object_unref(output->list);

    G_OBJECT_CLASS(parent_class)->dispose(object);

}



GtkWidget *gmtk_output_combo_box_new()
{
    GtkWidget *output;

    output = g_object_new(GMTK_TYPE_OUTPUT_COMBO_BOX, NULL);


    return output;
}

const gchar *gmtk_output_combo_box_get_active_device(GmtkOutputComboBox * output)
{
    GtkTreeIter iter;
    const gchar *device = NULL;

    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(output), &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(output->list), &iter, OUTPUT_MPLAYER_DEVICE_COLUMN, &device, -1);
    }
    return device;

}

const gchar *gmtk_output_combo_box_get_active_description(GmtkOutputComboBox * output)
{
    GtkTreeIter iter;
    const gchar *desc = NULL;

    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(output), &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(output->list), &iter, OUTPUT_DESCRIPTION_COLUMN, &desc, -1);
    }
    return desc;

}

GmtkOutputType gmtk_output_combo_box_get_active_type(GmtkOutputComboBox * output)
{
    GtkTreeIter iter;
    GmtkOutputType type;

    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(output), &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(output->list), &iter, OUTPUT_TYPE_COLUMN, &type, -1);
    }
    return type;

}

gint gmtk_output_combo_box_get_active_card(GmtkOutputComboBox * output)
{
    GtkTreeIter iter;
    gint card;

    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(output), &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(output->list), &iter, OUTPUT_CARD_COLUMN, &card, -1);
    }
    return card;

}

gint gmtk_output_combo_box_get_active_index(GmtkOutputComboBox * output)
{
    GtkTreeIter iter;
    gint index;

    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(output), &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(output->list), &iter, OUTPUT_INDEX_COLUMN, &index, -1);
    }
    return index;

}

GtkTreeModel *gmtk_output_combo_box_get_tree_model(GmtkOutputComboBox * output)
{
    return GTK_TREE_MODEL(output->list);
}
