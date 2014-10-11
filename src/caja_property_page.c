/*
 * caja_property_page.c
 * Copyright (C) Kevin DeKorte 2009 <kdekorte@gmail.com>
 * 
 * caja_property_page.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * caja_property_page.c is distributed in the hope that it will be useful,
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
#include <libcaja-extension/caja-extension-types.h>
#include <libcaja-extension/caja-property-page-provider.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gmlib.h>
#include "property_page_common.h"

#define ENABLE_CAJA_PLUGIN "enable-nautilus-plugin"
#define VERBOSE					"verbose"

gint verbose;

static GType pp_type = 0;





static GList *gnome_mplayer_properties_get_pages(CajaPropertyPageProvider * provider, GList * files)
{
    GList *pages = NULL;
    CajaFileInfo *file;
    GtkWidget *page, *label;
    CajaPropertyPage *property_page;
    guint i;
    gboolean found = FALSE;
    gchar *uri;

    /* only add properties page if a single file is selected */
    if (files == NULL || files->next != NULL)
        return pages;

    file = files->data;

    /* only add the properties page to these mime types */
    for (i = 0; i < G_N_ELEMENTS(mime_types); i++) {
        if (caja_file_info_is_mime_type(file, mime_types[i])) {
            found = TRUE;
            break;
        }
    }

    if (found) {
        uri = caja_file_info_get_uri(file);
        label = gtk_label_new(dgettext(GETTEXT_PACKAGE, "Audio/Video"));
        page = gtk_table_new(20, 2, FALSE);
        gtk_container_set_border_width(GTK_CONTAINER(page), 6);
        if (get_properties(page, uri)) {
            gtk_widget_show_all(page);
            property_page = caja_property_page_new("video-properties", label, page);
            pages = g_list_prepend(pages, property_page);
        }
        g_free(uri);
    }
    return pages;
}

static void property_page_provider_iface_init(CajaPropertyPageProviderIface * iface)
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
    g_type_module_add_interface(module, pp_type, CAJA_TYPE_PROPERTY_PAGE_PROVIDER, &property_page_provider_iface_info);
}




/* --- extension interface --- */
void caja_module_initialize(GTypeModule * module)
{
    GmPrefStore *gm_store;

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    // specify the domain in the translation calls so
    // we don't mess up the translation of the other tabs
    // textdomain(GETTEXT_PACKAGE);
#endif

    verbose = 0;

    gm_store = gm_pref_store_new("gnome-mplayer");
    if (gm_pref_store_get_boolean_with_default(gm_store, ENABLE_CAJA_PLUGIN, TRUE)) {
        verbose = gm_pref_store_get_int(gm_store, VERBOSE);
        gnome_mplayer_properties_plugin_register_type(module);
    }
    gm_pref_store_free(gm_store);
}

void caja_module_shutdown(void)
{
}

void caja_module_list_types(const GType ** types, int *num_types)
{
    static GType type_list[1];

    type_list[0] = pp_type;
    *types = type_list;
    *num_types = G_N_ELEMENTS(type_list);
}
