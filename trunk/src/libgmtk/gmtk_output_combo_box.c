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



gint sort_iter_compare_func(GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer data)
{
    gint sortcol = GPOINTER_TO_INT(data);
    gint ret;
    gchar *a_desc;
    gchar *b_desc;

    switch (sortcol) {
    case 0:
        gtk_tree_model_get(model, a, 0, &a_desc, -1);
        gtk_tree_model_get(model, b, 0, &b_desc, -1);

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


G_DEFINE_TYPE(GmtkOutputComboBox, gmtk_output_combo_box, GTK_TYPE_COMBO_BOX);

static void gmtk_output_combo_box_class_init(GmtkOutputComboBoxClass * class)
{
    GtkWidgetClass *widget_class;
    GtkObjectClass *object_class;

    object_class = (GtkObjectClass *) class;
    widget_class = GTK_WIDGET_CLASS(class);

}

static void gmtk_output_combo_box_init(GmtkOutputComboBox * output)
{
    GtkTreeIter iter;
    GtkCellRenderer *renderer;
    GtkTreeSortable *sortable;

    void **hints;
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

    output->list = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING);

    gtk_list_store_append(output->list, &iter);
    gtk_list_store_set(output->list, &iter, 0, "Default", 1, -1, 2, -1, 3, "", -1);

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

            menu =
                g_strdup_printf("%s (%s)", snd_ctl_card_info_get_name(info),
                                snd_pcm_info_get_name(pcminfo));
            mplayer_device = g_strdup_printf("alsa:device=hw=%i.%i", card, dev);

            //printf("menu = %s\n",menu);
            //printf("device = %s\n",mplayer_device);

            gtk_list_store_append(output->list, &iter);
            gtk_list_store_set(output->list, &iter, 0, menu, 1, card, 2, dev, 3, mplayer_device,
                               -1);
        }

        snd_ctl_close(handle);

    }
#endif

    sortable = GTK_TREE_SORTABLE(output->list);
    gtk_tree_sortable_set_sort_func(sortable, 0, sort_iter_compare_func, GINT_TO_POINTER(0), NULL);
    gtk_tree_sortable_set_sort_column_id(sortable, 0, GTK_SORT_ASCENDING);

    gtk_combo_box_set_model(GTK_COMBO_BOX(output), GTK_TREE_MODEL(output->list));
    gtk_combo_box_set_active(GTK_COMBO_BOX(output), 0);


}

static void gmtk_output_combo_box_dispose(GObject * object)
{


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
    gchar *device = NULL;

    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(output), &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(output->list), &iter, 3, &device, -1);
    }
    return device;

}
