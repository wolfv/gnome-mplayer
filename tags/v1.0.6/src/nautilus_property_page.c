/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * nautilus_property_page.c
 * Copyright (C) Kevin DeKorte 2009 <kdekorte@gmail.com>
 * 
 * nautilus_property_page.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * nautilus_property_page.c is distributed in the hope that it will be useful,
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "mime_types.h"
#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-property-page-provider.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gmlib.h>

#define ENABLE_NAUTILUS_PLUGIN "enable-nautilus-plugin"

static GType pp_type = 0;

typedef struct _MetaData {
    gchar *title;
    gchar *artist;
    gchar *album;
    gchar *length;
    gfloat length_value;
    gchar *subtitle;
    gchar *audio_codec;
    gchar *video_codec;
    gchar *audio_bitrate;
    gchar *video_bitrate;
    gchar *video_fps;
    gchar *audio_nch;
    gchar *demuxer;
    gint width;
    gint height;
    gboolean video_present;
    gboolean audio_present;
} MetaData;

void strip_unicode(gchar * data, gsize len)
{
    gsize i = 0;

    if (data != NULL) {
        for (i = 0; i < len; i++) {
            if (!g_unichar_validate(data[i])) {
                data[i] = ' ';
            }
        }
    }
}

gchar *seconds_to_string(gfloat seconds)
{
    int hour = 0, min = 0;
    gchar *result = NULL;

    if (seconds >= 3600) {
        hour = seconds / 3600;
        seconds = seconds - (hour * 3600);
    }
    if (seconds >= 60) {
        min = seconds / 60;
        seconds = seconds - (min * 60);
    }

    if (hour == 0) {
        result = g_strdup_printf(dgettext(GETTEXT_PACKAGE, "%2i:%02.0f"), min, seconds);
    } else {
        result = g_strdup_printf(dgettext(GETTEXT_PACKAGE, "%i:%02i:%02.0f"), hour, min, seconds);
    }
    return g_strstrip(result);
}

static MetaData *get_metadata(gchar * filename)
{
    GError *error;
    gint exit_status;
    gchar *out = NULL;
    gchar *err = NULL;
    gchar *av[255];
    gint ac = 0, i;
    gchar **output;
    gchar *ptr;
    gchar *lower;
    gfloat f;

    MetaData *ret;
    ret = g_new0(MetaData, 1);

    av[ac++] = g_strdup_printf("mplayer");
    av[ac++] = g_strdup_printf("-vo");
    av[ac++] = g_strdup_printf("null");
    av[ac++] = g_strdup_printf("-ao");
    av[ac++] = g_strdup_printf("null");
    av[ac++] = g_strdup_printf("-nomsgcolor");
    av[ac++] = g_strdup_printf("-nomsgmodule");
    av[ac++] = g_strdup_printf("-frames");
    av[ac++] = g_strdup_printf("0");
    av[ac++] = g_strdup_printf("-noidx");
    av[ac++] = g_strdup_printf("-identify");
    av[ac++] = g_strdup_printf("-nocache");
    av[ac++] = g_strdup_printf("-noidle");

    av[ac++] = g_strdup_printf("%s", filename);
    av[ac] = NULL;

    error = NULL;

    g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status, &error);

    for (i = 0; i < ac; i++) {
        g_free(av[i]);
    }

    if (error != NULL) {
        printf("Error when running: %s\n", error->message);
        g_error_free(error);
        error = NULL;
        if (out != NULL)
            g_free(out);
        if (err != NULL)
            g_free(err);
        if (ret != NULL)
            g_free(ret);
        return NULL;
    }
    output = g_strsplit(out, "\n", 0);
    ac = 0;
    while (output[ac] != NULL) {

        lower = g_ascii_strdown(output[ac], -1);
        if (strstr(output[ac], "ID_LENGTH") != NULL) {
            ptr = strstr(output[ac], "=") + 1;
            sscanf(ptr, "%f", &f);
            ret->length = seconds_to_string(f);
            ret->length_value = f;
        }

        if (g_ascii_strncasecmp(output[ac], "ID_CLIP_INFO_NAME", strlen("ID_CLIP_INFO_NAME")) == 0) {
            if (strstr(lower, "=title") != NULL || strstr(lower, "=name") != NULL) {
                ptr = strstr(output[ac + 1], "=") + 1;
                if (ptr)
                    ret->title = g_strstrip(g_locale_to_utf8(ptr, -1, NULL, NULL, NULL));
                else
                    ret->title = NULL;

                if (ret->title == NULL) {
                    ret->title = g_strdup(ptr);
                    strip_unicode(ret->title, strlen(ret->title));
                }
            }
            if (strstr(lower, "=artist") != NULL || strstr(lower, "=author") != NULL) {
                ptr = strstr(output[ac + 1], "=") + 1;
                ret->artist = g_strstrip(g_locale_to_utf8(ptr, -1, NULL, NULL, NULL));
                if (ret->artist == NULL) {
                    ret->artist = g_strdup(ptr);
                    strip_unicode(ret->artist, strlen(ret->artist));
                }
            }
            if (strstr(lower, "=album") != NULL) {
                ptr = strstr(output[ac + 1], "=") + 1;
                ret->album = g_strstrip(g_locale_to_utf8(ptr, -1, NULL, NULL, NULL));
                if (ret->album == NULL) {
                    ret->album = g_strdup(ptr);
                    strip_unicode(ret->album, strlen(ret->album));
                }
            }
        }

        if (strstr(output[ac], "ID_AUDIO_CODEC") != NULL) {
            ptr = strstr(output[ac], "=") + 1;
            ret->audio_codec = g_strdup(ptr);
            ret->audio_present = TRUE;
        }

        if (strstr(output[ac], "ID_VIDEO_CODEC") != NULL) {
            ptr = strstr(output[ac], "=") + 1;
            ret->video_codec = g_strdup(ptr);
            ret->video_present = TRUE;
        }

        if (strstr(output[ac], "ID_VIDEO_WIDTH") != NULL) {
            ptr = strstr(output[ac], "=") + 1;
            ret->width = (gint) g_strtod(ptr, NULL);
        }

        if (strstr(output[ac], "ID_VIDEO_HEIGHT") != NULL) {
            ptr = strstr(output[ac], "=") + 1;
            ret->height = (gint) g_strtod(ptr, NULL);
        }

        if (strstr(output[ac], "ID_AUDIO_BITRATE") != NULL) {
            ptr = strstr(output[ac], "=") + 1;
            ret->audio_bitrate = g_strdup(ptr);
        }

        if (strstr(output[ac], "ID_VIDEO_BITRATE") != NULL) {
            ptr = strstr(output[ac], "=") + 1;
            ret->video_bitrate = g_strdup(ptr);
        }

        if (strstr(output[ac], "ID_VIDEO_FPS") != NULL) {
            ptr = strstr(output[ac], "=") + 1;
            ret->video_fps = g_strdup(ptr);
        }

        if (strstr(output[ac], "ID_AUDIO_NCH") != NULL) {
            ptr = strstr(output[ac], "=") + 1;
            ret->audio_nch = g_strdup(ptr);
        }

        if (strstr(output[ac], "ID_DEMUXER") != NULL) {
            ptr = strstr(output[ac], "=") + 1;
            ret->demuxer = g_strdup(ptr);
        }
        g_free(lower);
        ac++;
    }

    g_strfreev(output);
    if (out)
        g_free(out);
    if (err)
        g_free(err);

    return ret;

}


static gboolean get_properties(GtkWidget * page, gchar * uri)
{
    GtkWidget *label;
    gint i = 0;
    MetaData *data;
    gchar *filename;
    gchar *buf;

#ifdef GIO_ENABLED
    GFile *file;

    file = g_file_new_for_uri(uri);
    filename = g_file_get_path(file);
    g_object_unref(file);
#else
    filename = g_filename_from_uri(uri, NULL, NULL);
#endif

    if (filename != NULL) {

        data = get_metadata(filename);

        label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "<span weight=\"bold\">Media Details</span>"));
        gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
        gtk_misc_set_padding(GTK_MISC(label), 0, 6);
        gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
        i++;

        if (data->title && strlen(data->title) > 0) {
            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Title"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            label = gtk_label_new(data->title);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 0, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 1, 2, i, i + 1);
            i++;
        }

        if (data->artist && strlen(data->artist) > 0) {
            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Artist"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            label = gtk_label_new(data->artist);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 0, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 1, 2, i, i + 1);
            i++;
        }

        if (data->album && strlen(data->album) > 0) {
            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Album"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            label = gtk_label_new(data->album);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 0, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 1, 2, i, i + 1);
            i++;
        }

        if (data->length) {
            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Length"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            label = gtk_label_new(data->length);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 0, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 1, 2, i, i + 1);
            i++;
        }

        if (data->demuxer && strlen(data->demuxer) > 0) {
            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Demuxer"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            label = gtk_label_new(data->demuxer);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 0, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 1, 2, i, i + 1);
            i++;
        }

        if (data->video_present) {
            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "<span weight=\"bold\">Video Details</span>"));
            gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 0, 6);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            i++;

            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Video Size:"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            buf = g_strdup_printf("%i x %i", data->width, data->height);
            label = gtk_label_new(buf);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 1, 2, i, i + 1);
            g_free(buf);
            i++;

            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Video Codec:"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            buf = g_ascii_strup(data->video_codec, -1);
            label = gtk_label_new(buf);
            g_free(buf);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 1, 2, i, i + 1);
            i++;

            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Video Bitrate:"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            buf = g_strdup_printf("%i Kb/s", (gint) (g_strtod(data->video_bitrate, NULL) / 1024));
            label = gtk_label_new(buf);
            g_free(buf);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 1, 2, i, i + 1);
            i++;

            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Video Frame Rate:"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            buf = g_strdup_printf("%i fps", (gint) (g_strtod(data->video_fps, NULL)));
            label = gtk_label_new(buf);
            g_free(buf);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 1, 2, i, i + 1);
            i++;

        }

        if (data->audio_present) {
            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "<span weight=\"bold\">Audio Details</span>"));
            gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 0, 6);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            i++;

            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Audio Codec:"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            buf = g_ascii_strup(data->audio_codec, -1);
            label = gtk_label_new(buf);
            g_free(buf);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 1, 2, i, i + 1);
            i++;

            label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Audio Bitrate:"));
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_misc_set_padding(GTK_MISC(label), 12, 0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
            buf = g_strdup_printf("%i Kb/s", (gint) (g_strtod(data->audio_bitrate, NULL) / 1024));
            label = gtk_label_new(buf);
            g_free(buf);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
            gtk_table_attach_defaults(GTK_TABLE(page), label, 1, 2, i, i + 1);
            i++;

            if (g_strtod(data->audio_nch, NULL) > 0) {
                label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Audio Channels:"));
                gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
                gtk_misc_set_padding(GTK_MISC(label), 12, 0);
                gtk_table_attach_defaults(GTK_TABLE(page), label, 0, 1, i, i + 1);
                buf = g_strdup_printf("%i", (gint) g_strtod(data->audio_nch, NULL));
                label = gtk_label_new(buf);
                g_free(buf);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
                gtk_table_attach_defaults(GTK_TABLE(page), label, 1, 2, i, i + 1);
                i++;
            }
        }

        g_free(data->title);
        g_free(data->artist);
        g_free(data->album);
        g_free(data->length);
        g_free(data->subtitle);
        g_free(data->audio_codec);
        g_free(data->video_codec);
        g_free(data->audio_bitrate);
        g_free(data->video_bitrate);
        g_free(data->audio_nch);
        g_free(data->video_fps);
        g_free(data->demuxer);
        g_free(data);
        return TRUE;
    }
    return FALSE;
}

static GList *gnome_mplayer_properties_get_pages(NautilusPropertyPageProvider * provider, GList * files)
{
    GList *pages = NULL;
    NautilusFileInfo *file;
    GtkWidget *page, *label;
    NautilusPropertyPage *property_page;
    guint i;
    gboolean found = FALSE;
    gchar *uri;

    /* only add properties page if a single file is selected */
    if (files == NULL || files->next != NULL)
        return pages;

    file = files->data;

    /* only add the properties page to these mime types */
    for (i = 0; i < G_N_ELEMENTS(mime_types); i++) {
        if (nautilus_file_info_is_mime_type(file, mime_types[i])) {
            found = TRUE;
            break;
        }
    }

    if (found) {
        uri = nautilus_file_info_get_uri(file);
        label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Audio/Video"));
        page = gtk_table_new(20, 2, FALSE);
        gtk_container_set_border_width(GTK_CONTAINER(page), 6);
        if (get_properties(page, uri)) {
            gtk_widget_show_all(page);
            property_page = nautilus_property_page_new("video-properties", label, page);
            pages = g_list_prepend(pages, property_page);
        }
        g_free(uri);
    }
    return pages;
}

static void property_page_provider_iface_init(NautilusPropertyPageProviderIface * iface)
{
    iface->get_pages = gnome_mplayer_properties_get_pages;
}

static void gnome_mplayer_properties_plugin_register_type(GTypeModule * module)
{
    const GTypeInfo info = {
        sizeof(GObjectClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) NULL,
        NULL,
        NULL,
        sizeof(GObject),
        0,
        (GInstanceInitFunc) NULL
    };
    const GInterfaceInfo property_page_provider_iface_info = {
        (GInterfaceInitFunc) property_page_provider_iface_init,
        NULL,
        NULL
    };

    pp_type = g_type_module_register_type(module, G_TYPE_OBJECT, "GnomeMPlayerPropertiesPlugin", &info, 0);
    g_type_module_add_interface(module,
                                pp_type, NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER, &property_page_provider_iface_info);
}




/* --- extension interface --- */
void nautilus_module_initialize(GTypeModule * module)
{
    GmPrefStore *gm_store;

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    // specify the domain in the translation calls so
    // we don't mess up the translation of the other tabs
    // textdomain(GETTEXT_PACKAGE);
#endif

    gm_store = gm_pref_store_new("gnome-mplayer");
    if (gm_pref_store_get_boolean_with_default(gm_store, ENABLE_NAUTILUS_PLUGIN, TRUE)) {
        gnome_mplayer_properties_plugin_register_type(module);
    }
    gm_pref_store_free(gm_store);
}

void nautilus_module_shutdown(void)
{
}

void nautilus_module_list_types(const GType ** types, int *num_types)
{
    static GType type_list[1];

    type_list[0] = pp_type;
    *types = type_list;
    *num_types = G_N_ELEMENTS(type_list);
}
