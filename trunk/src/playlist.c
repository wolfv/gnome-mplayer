/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * playlist.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 *
 * playlist.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * playlist.c is distributed in the hope that it will be useful,
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

#include "playlist.h"
#include "common.h"
#include "gui.h"
#include "support.h"

void update_gui()
{
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) < 2) {
        gtk_widget_hide(prev_event_box);
        gtk_widget_hide(next_event_box);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), FALSE);
        gtk_widget_hide(GTK_WIDGET(menuitem_prev));
        gtk_widget_hide(GTK_WIDGET(menuitem_next));
    } else {
        gtk_widget_show(prev_event_box);
        gtk_widget_show(next_event_box);
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), TRUE);
        gtk_widget_show(GTK_WIDGET(menuitem_prev));
        gtk_widget_show(GTK_WIDGET(menuitem_next));
    }

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) > 0) {
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_loop), TRUE);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_loop), FALSE);
    }

    if (use_defaultpl && safe_to_save_default_playlist)
        save_playlist_pls(default_playlist);

}

gboolean playlist_popup_handler(GtkWidget * widget, GdkEvent * event, void *data)
{
    GtkMenu *menu;
    GdkEventButton *event_button;


    menu = GTK_MENU(widget);
    gtk_widget_grab_focus(fixed);
    if (event->type == GDK_BUTTON_PRESS) {

        event_button = (GdkEventButton *) event;

        if (event_button->button == 3) {
            gtk_menu_popup(menu, NULL, NULL, NULL, NULL, event_button->button, event_button->time);
            return TRUE;
        }

    }
    return FALSE;
}

gboolean button_release_callback(GtkWidget * widget, GdkEvent * event, gpointer data)
{
    GtkTreePath *path;
    // GtkTreeSelection *sel;
    gint pos;
    GdkEventButton *event_button;
    gchar *buf;

    event_button = (GdkEventButton *) event;

    gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(list), event_button->x, event_button->y, &path,
                                  NULL, NULL, NULL);
    if (path != NULL) {
        buf = gtk_tree_path_to_string(path);
        pos = (gint) g_strtod(buf, NULL);
        g_free(buf);
        gtk_tree_path_free(path);

        if (pos == 0) {
            gtk_widget_set_sensitive(up, FALSE);
        } else {
            gtk_widget_set_sensitive(up, TRUE);
        }

        if (pos + 1 == gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL)) {
            gtk_widget_set_sensitive(down, FALSE);
        } else {
            gtk_widget_set_sensitive(down, TRUE);
        }
    } else {
        //sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
        //gtk_tree_selection_unselect_all(sel);
        //gtk_widget_set_sensitive(up, FALSE);
        //gtk_widget_set_sensitive(down, FALSE);
    }

    return FALSE;
}

void playlist_set_subtitle_callback(GtkMenuItem * menuitem, void *data)
{
    GtkTreeSelection *sel;
    GtkTreeView *view = (GtkTreeView *) data;
    GtkTreeIter localiter;
    gchar *subtitle;
    GtkWidget *dialog;
    gchar *path;
    gchar *item;
    gchar *p;

    sel = gtk_tree_view_get_selection(view);

    if (gtk_tree_selection_get_selected(sel, NULL, &localiter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN, &item, -1);
        path = g_strdup(item);
        p = g_strrstr(path, "/");
        if (p != NULL)
            p[1] = '\0';

        dialog = gtk_file_chooser_dialog_new(_("Set Subtitle"),
                                             GTK_WINDOW(window),
                                             GTK_FILE_CHOOSER_ACTION_OPEN,
                                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                             GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        gtk_widget_show(dialog);
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), path);
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            subtitle = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            gtk_list_store_set(playliststore, &localiter, SUBTITLE_COLUMN, subtitle, -1);
            g_free(subtitle);
        }
        gtk_widget_destroy(dialog);
    }

}

void playlist_set_audiofile_callback(GtkMenuItem * menuitem, void *data)
{
    GtkTreeSelection *sel;
    GtkTreeView *view = (GtkTreeView *) data;
    GtkTreeIter localiter;
    gchar *audiofile;
    GtkWidget *dialog;
    gchar *path;
    gchar *item;
    gchar *p;

    sel = gtk_tree_view_get_selection(view);

    if (gtk_tree_selection_get_selected(sel, NULL, &localiter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN, &item, -1);
        path = g_strdup(item);
        p = g_strrstr(path, "/");
        if (p != NULL)
            p[1] = '\0';

        dialog = gtk_file_chooser_dialog_new(_("Set Audio"),
                                             GTK_WINDOW(window),
                                             GTK_FILE_CHOOSER_ACTION_OPEN,
                                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                             GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        gtk_widget_show(dialog);
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), path);
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            audiofile = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            gtk_list_store_set(playliststore, &localiter, AUDIOFILE_COLUMN, audiofile, -1);
            g_free(audiofile);
        }
        gtk_widget_destroy(dialog);
    }

}


gboolean playlist_drop_callback(GtkWidget * widget, GdkDragContext * dc,
                                gint x, gint y, GtkSelectionData * selection_data,
                                guint info, guint t, gpointer data)
{
    gchar **list;
    gint i = 0;
    gint playlist;

    /* Important, check if we actually got data.  Sometimes errors
     * occure and selection_data will be NULL.
     */
    if (selection_data == NULL)
        return FALSE;
#ifdef GTK2_14_ENABLED
    if (gtk_selection_data_get_length(selection_data) < 0)
        return FALSE;
#else
	if (selection_data->length < 0)
 	    return FALSE;
#endif

    if ((info == DRAG_INFO_0) || (info == DRAG_INFO_1) || (info == DRAG_INFO_2)) {
#ifdef GTK2_14_ENABLED
        list = g_uri_list_extract_uris((const gchar *) gtk_selection_data_get_data(selection_data));
#else
		list = g_uri_list_extract_uris((const gchar *) selection_data->data);
#endif
        while (list[i] != NULL) {
            if (strlen(list[i]) > 0) {
                if (is_uri_dir(list[i])) {
                    create_folder_progress_window();
                    add_folder_to_playlist_callback(list[i], NULL);
                    destroy_folder_progress_window();
                } else {
                    playlist = detect_playlist(list[i]);

                    if (!playlist) {
                        add_item_to_playlist(list[i], playlist);
                    } else {
                        if (!parse_playlist(list[i])) {
                            add_item_to_playlist(list[i], playlist);
                        }
                    }
                }
            }
            i++;
        }

        if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) == 1) {
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            play_iter(&iter, 0);

        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), TRUE);
        }

        if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL) > 0) {
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_loop), TRUE);
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_loop), FALSE);
        }

        g_strfreev(list);
    }
    update_gui();
    return FALSE;
}

gboolean playlist_motion_callback(GtkWidget * widget, GdkEventMotion * event, gpointer user_data)
{
    GtkTreePath *localpath;
    GtkTreeIter localiter;
    gchar *iterfilename;
    gchar *itersubtitle;
    gchar *tip;

    gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), event->x, event->y, &localpath, NULL, NULL,
                                  NULL);

    if (localpath != NULL) {
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(playliststore), &localiter, localpath)) {
            gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN,
                               &iterfilename, SUBTITLE_COLUMN, &itersubtitle, -1);
            if (itersubtitle == NULL) {
                tip = g_strdup_printf("%s", iterfilename);
            } else {
                tip = g_strdup_printf(_("Filename: %s\nSubtitle: %s"), iterfilename, itersubtitle);
                g_free(itersubtitle);
            }
#ifdef GTK2_12_ENABLED
            gtk_widget_set_tooltip_text(widget, tip);
#else
            gtk_tooltips_set_tip(playlisttip, widget, tip, NULL);
#endif
            g_free(tip);
            g_free(iterfilename);
        }
        gtk_tree_path_free(localpath);
    }
    return FALSE;

}

gboolean playlist_enter_callback(GtkWidget * widget, GdkEventMotion * event, gpointer user_data)
{
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(list), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(list), DESCRIPTION_COLUMN);
    gtk_widget_grab_focus(list);
    setup_accelerators(FALSE);
    return TRUE;
}

gboolean playlist_leave_callback(GtkWidget * widget, GdkEventMotion * event, gpointer user_data)
{
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(list), FALSE);
    setup_accelerators(TRUE);
    return TRUE;
}

void save_playlist(GtkWidget * widget, void *data)
{
    GtkWidget *dialog;
    gchar *uri;
    gchar *new_uri;
    GtkFileFilter *filter;
    gchar *last_dir;

    dialog = gtk_file_chooser_dialog_new(_("Save Playlist"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

#ifdef GIO_ENABLED
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), FALSE);
#endif
    gtk_widget_show(dialog);
    gm_store = gm_pref_store_new("gnome-mplayer");
    last_dir = gm_pref_store_get_string(gm_store, LAST_DIR);
    if (last_dir != NULL && is_uri_dir(last_dir))
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), last_dir);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Playlist (*.pls)"));
    gtk_file_filter_add_pattern(filter, "*.pls");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("MP3 Playlist (*.m3u)"));
    gtk_file_filter_add_pattern(filter, "*.m3u");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
        last_dir = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
        gm_pref_store_set_string(gm_store, LAST_DIR, last_dir);
        g_free(last_dir);

        if (g_strrstr(uri, ".m3u") != NULL) {
            save_playlist_m3u(uri);
        }
        if (g_strrstr(uri, ".pls") != NULL) {
            save_playlist_pls(uri);
        }

        if (g_strrstr(uri, ".") == NULL) {
            new_uri = g_strdup_printf("%s.pls", uri);
            save_playlist_pls(new_uri);
            g_free(new_uri);
        }
        g_free(uri);
    }
    gm_pref_store_free(gm_store);
    gtk_widget_destroy(dialog);

}

void load_playlist(GtkWidget * widget, void *data)
{
    GtkWidget *dialog;
    gchar *filename;
    GtkFileFilter *filter;
    gchar *last_dir;

    dialog = gtk_file_chooser_dialog_new(_("Open Playlist"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_widget_show(dialog);
    gm_store = gm_pref_store_new("gnome-mplayer");
    last_dir = gm_pref_store_get_string(gm_store, LAST_DIR);
    if (last_dir != NULL && is_uri_dir(last_dir))
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), last_dir);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Playlist (*.pls)"));
    gtk_file_filter_add_pattern(filter, "*.pls");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Reference Playlist (*.ref)"));
    gtk_file_filter_add_pattern(filter, "*.ref");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("MP3 Playlist (*.m3u)"));
    gtk_file_filter_add_pattern(filter, "*.m3u");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        filename = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
        last_dir = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
        gm_pref_store_set_string(gm_store, LAST_DIR, last_dir);
        g_free(last_dir);

        gtk_list_store_clear(playliststore);

        create_folder_progress_window();
        if (!parse_playlist(filename)) {
            add_item_to_playlist(filename, 1);
        }
        destroy_folder_progress_window();
    }
    gm_pref_store_free(gm_store);
    update_gui();
    gtk_widget_destroy(dialog);
}

void add_item_to_playlist_callback(gpointer data, gpointer user_data)
{
    gchar *filename = (gchar *) data;
    gint playlist;

    playlist = detect_playlist(filename);
    if (!playlist) {
        add_item_to_playlist(filename, playlist);
    } else {
        if (!parse_playlist(filename)) {
            add_item_to_playlist(filename, playlist);
        }
    }
    set_item_add_info(filename);

    g_free(filename);
}

gint compar(gpointer a, gpointer b)
{
    return strcasecmp((char *) a, (char *) b);
}

void add_folder_to_playlist_callback(gpointer data, gpointer user_data)
{

#ifdef GIO_ENABLED
    gchar *uri = (gchar *) data;
    GFile *file;
    GFileEnumerator *dir;
    GFileInfo *info;
    GError *error;
    gchar *sub_uri;
    GSList *list = NULL;

    error = NULL;
    file = g_file_new_for_uri(uri);
    dir = g_file_enumerate_children(file, "standard::*", G_FILE_QUERY_INFO_NONE, NULL, &error);
    if (error != NULL)
        printf("error message = %s\n", error->message);

    if (dir != NULL) {
        info = g_file_enumerator_next_file(dir, NULL, NULL);
        while (info != NULL) {
            if (g_file_info_get_file_type(info) & G_FILE_TYPE_DIRECTORY) {
                sub_uri = g_strdup_printf("%s/%s", uri, g_file_info_get_name(info));
                add_folder_to_playlist_callback((gpointer) sub_uri, NULL);
                g_free(sub_uri);
            } else {
                sub_uri = g_strdup_printf("%s/%s", uri, g_file_info_get_name(info));
                list = g_slist_prepend(list, sub_uri);
                filecount++;
            }
            g_object_unref(info);
            info = NULL;
            if (!cancel_folder_load)
                info = g_file_enumerator_next_file(dir, NULL, NULL);

        }
        list = g_slist_sort(list, (GCompareFunc) compar);
        if (!cancel_folder_load)
            g_slist_foreach(list, &add_item_to_playlist_callback, NULL);
        g_slist_free(list);
        g_object_unref(dir);
    }
    g_object_unref(file);
#else
    gchar *uri = (gchar *) data;
    gchar *filename;
    GDir *dir;
    gchar *name;
    gchar *subdir = NULL;
    gchar *subdir_uri = NULL;
    GSList *list = NULL;

    filename = g_filename_from_uri(uri, NULL, NULL);
    dir = g_dir_open(filename, 0, NULL);
    if (dir != NULL) {
        do {
            name = g_strdup(g_dir_read_name(dir));
            if (name != NULL) {
                subdir = g_strdup_printf("%s/%s", filename, name);
                subdir_uri = g_filename_to_uri(subdir, NULL, NULL);
                if (g_file_test(subdir, G_FILE_TEST_IS_DIR)) {
                    add_folder_to_playlist_callback((gpointer) subdir_uri, NULL);
                    g_free(subdir);
                    g_free(subdir_uri);
                } else {
                    list = g_slist_prepend(list, subdir_uri);
                    filecount++;
                }
                g_free(name);
            } else {
                break;
            }
            if (cancel_folder_load)
                break;
        } while (TRUE);
        list = g_slist_sort(list, (GCompareFunc) compar);
        if (!cancel_folder_load)
            g_slist_foreach(list, &add_item_to_playlist_callback, NULL);
        g_slist_free(list);
        g_dir_close(dir);
    }
#endif
}

void add_to_playlist(GtkWidget * widget, void *data)
{

    GtkWidget *dialog;
    GSList *uris;
    gchar *last_dir;
    GtkTreeViewColumn *column;
    gchar *coltitle;
    gint count;
    gchar **split;
    gchar *joined;

    dialog = gtk_file_chooser_dialog_new(_("Open File"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    /*allow multiple files to be selected */
    gtk_widget_show(dialog);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
#ifdef GIO_ENABLED
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), FALSE);
#endif

    gm_store = gm_pref_store_new("gnome-mplayer");
    last_dir = gm_pref_store_get_string(gm_store, LAST_DIR);
    if (last_dir != NULL && is_uri_dir(last_dir)) {
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), last_dir);
        g_free(last_dir);
    }

    count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));

        last_dir = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
        gm_pref_store_set_string(gm_store, LAST_DIR, last_dir);
        g_free(last_dir);

        g_slist_foreach(uris, &add_item_to_playlist_callback, NULL);
        g_slist_free(uris);
    }
    update_gui();
    gm_pref_store_free(gm_store);
    column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
    if (playlistname != NULL && strlen(playlistname) > 0 && count == 0) {
        coltitle = g_strdup_printf(_("%s items"), playlistname);
    } else {
        coltitle = g_strdup_printf(ngettext("Item to Play", "Items to Play", count));
    }

    split = g_strsplit(coltitle, "_", 0);
    joined = g_strjoinv("__", split);
    gtk_tree_view_column_set_title(column, joined);
    g_free(coltitle);
    g_free(joined);
    g_strfreev(split);

    gtk_widget_destroy(dialog);
}

void add_folder_to_playlist(GtkWidget * widget, void *data)
{

    GtkWidget *dialog;
    GSList *uris;
    gchar *last_dir;
    gchar *message;
    gint count;

    count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);

    dialog = gtk_file_chooser_dialog_new(_("Choose Directory"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    gtk_widget_show(dialog);
    /*allow multiple files to be selected */
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
#ifdef GIO_ENABLED
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), FALSE);
#endif

    gm_store = gm_pref_store_new("gnome-mplayer");
    last_dir = gm_pref_store_get_string(gm_store, LAST_DIR);
    if (last_dir != NULL && is_uri_dir(last_dir)) {
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), last_dir);
        g_free(last_dir);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));

        last_dir = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
        gm_pref_store_set_string(gm_store, LAST_DIR, last_dir);
        g_free(last_dir);

        filecount = 0;
        create_folder_progress_window();
        g_slist_foreach(uris, &add_folder_to_playlist_callback, NULL);
        destroy_folder_progress_window();
        g_slist_free(uris);
    }
    update_gui();
    gm_pref_store_free(gm_store);
    gtk_widget_destroy(dialog);
    message =
        g_markup_printf_escaped(ngettext("\n\tFound %i file\n", "\n\tFound %i files\n", filecount),
                                filecount);
    g_strlcpy(idledata->media_info, message, 1024);
    g_free(message);
    g_idle_add(set_media_label, idledata);

    if (count == 0 && filecount > 0) {
        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
        play_iter(&iter, 0);
        dontplaynext = FALSE;
    }
}

void remove_from_playlist(GtkWidget * widget, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeView *view = (GtkTreeView *) data;
    GtkTreeIter localiter;
    GtkTreePath *path;

    sel = gtk_tree_view_get_selection(view);

    if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
        localiter = iter;
        gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter);
        if (gtk_list_store_remove(playliststore, &iter)) {

            if (!gtk_list_store_iter_is_valid(playliststore, &iter)) {
                gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore), &iter);
            }
            if (GTK_IS_TREE_SELECTION(selection)) {
                path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &iter);
                gtk_tree_selection_select_path(selection, path);
                if (GTK_IS_WIDGET(list))
                    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list), path, NULL, FALSE, 0, 0);
                gtk_tree_path_free(path);
            }
        }
    }
    update_gui();

}

void clear_playlist(GtkWidget * widget, void *data)
{

    dontplaynext = TRUE;
    mplayer_shutdown();
    gtk_list_store_clear(playliststore);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_edit_random), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_loop), FALSE);
}

void move_item_up(GtkWidget * widget, void *data)
{
    GtkTreeSelection *sel;
    GtkTreeIter localiter, a, b;
    GtkTreePath *path;
    gint pos = 0;
    gchar *buf;
    gint a_order, b_order;

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
    if (gtk_tree_selection_get_selected(sel, NULL, &localiter)) {
        path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &localiter);
        if (path != NULL) {
            buf = gtk_tree_path_to_string(path);
            pos = (gint) g_strtod(buf, NULL);
            g_free(buf);
            gtk_tree_path_free(path);

            if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(playliststore), &a, NULL, pos)) {
                if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(playliststore), &b, NULL, pos - 1)) {
                    gtk_list_store_swap(playliststore, &a, &b);
                    gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &a, PLAY_ORDER_COLUMN,
                                       &a_order, -1);
                    gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &b, PLAY_ORDER_COLUMN,
                                       &b_order, -1);
                    gtk_list_store_set(playliststore, &a, PLAY_ORDER_COLUMN, b_order, -1);
                    gtk_list_store_set(playliststore, &b, PLAY_ORDER_COLUMN, a_order, -1);
                }
            }
        }
        if (pos - 1 <= 0) {
            gtk_widget_set_sensitive(up, FALSE);
        } else {
            gtk_widget_set_sensitive(up, TRUE);
        }
        gtk_widget_set_sensitive(down, TRUE);
    }
}

void move_item_down(GtkWidget * widget, void *data)
{
    GtkTreeSelection *sel;
    GtkTreeIter localiter, a, b;
    GtkTreePath *path;
    gint pos = 0;
    gchar *buf;
    gint a_order, b_order;

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
    if (gtk_tree_selection_get_selected(sel, NULL, &localiter)) {
        path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &localiter);
        if (path != NULL) {
            buf = gtk_tree_path_to_string(path);
            pos = (gint) g_strtod(buf, NULL);
            g_free(buf);
            gtk_tree_path_free(path);
            if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(playliststore), &a, NULL, pos)) {
                if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(playliststore), &b, NULL, pos + 1)) {
                    gtk_list_store_swap(playliststore, &a, &b);
                    gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &a, PLAY_ORDER_COLUMN,
                                       &a_order, -1);
                    gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &b, PLAY_ORDER_COLUMN,
                                       &b_order, -1);
                    gtk_list_store_set(playliststore, &a, PLAY_ORDER_COLUMN, b_order, -1);
                    gtk_list_store_set(playliststore, &b, PLAY_ORDER_COLUMN, a_order, -1);

                }
            }
        }
        if (pos + 2 == gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL)) {
            gtk_widget_set_sensitive(down, FALSE);
        } else {
            gtk_widget_set_sensitive(down, TRUE);
        }
        gtk_widget_set_sensitive(up, TRUE);
    }

}

gboolean playlist_select_callback(GtkTreeView * view, GtkTreePath * path,
                                  GtkTreeViewColumn * column, gpointer data)
{

    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(playliststore), &iter, path)) {
        dontplaynext = TRUE;
        mplayer_shutdown();
        play_iter(&iter, 0);
        dontplaynext = FALSE;
    }
    return FALSE;
}

void playlist_close(GtkWidget * widget, void *data)
{
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist), FALSE);
}

void repeat_callback(GtkWidget * widget, void *data)
{
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_edit_loop), !loop);
}

void shuffle_callback(GtkWidget * widget, void *data)
{
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_edit_random),
                                   !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM
                                                                   (menuitem_edit_random)));
}

void menuitem_view_playlist_callback(GtkMenuItem * menuitem, void *data)
{
    playlist_visible = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(list), playlist_visible);
    if (remember_loc)
        use_remember_loc = TRUE;
    adjust_layout();
}

void undo_playlist_sort(GtkWidget * widget, void *data)
{
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(playliststore), PLAY_ORDER_COLUMN,
                                         GTK_SORT_ASCENDING);
}

void create_playlist_widget()
{
    GtkWidget *scrolled;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *box;
    GtkWidget *ctrlbox;
    GtkWidget *closebox;
    GtkWidget *hbox;
    GtkWidget *savelist;
    GtkWidget *loadlist;
    GtkWidget *add;
    GtkWidget *remove;
    GtkWidget *add_folder;
    GtkWidget *clear;
    GtkWidget *undo;
    GtkTreePath *path;
    GtkAccelGroup *accel_group;
#ifdef GTK2_12_ENABLED
#else
    GtkTooltips *tooltip;
#endif
    gchar *coltitle;
    gint count;
    GtkTargetEntry target_entry[3];
    gint i = 0;
    gint handle_size;
    gchar **split;
    gchar *joined;

    plvbox = gtk_vbox_new(FALSE, 12);
    hbox = gtk_hbox_new(FALSE, 12);
    gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE);
    box = gtk_hbox_new(FALSE, 10);
    ctrlbox = gtk_hbox_new(FALSE, 0);
    closebox = gtk_hbox_new(FALSE, 0);

    list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(playliststore));
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(list), FALSE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(list), FALSE);
    gtk_widget_add_events(list, GDK_BUTTON_PRESS_MASK);
    gtk_widget_set_size_request(GTK_WIDGET(list), -1, -1);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
    if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
        path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &iter);
        gtk_tree_selection_select_path(selection, path);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list), path, NULL, FALSE, 0, 0);
        gtk_tree_path_free(path);
    }

    g_signal_connect(G_OBJECT(list), "row_activated", G_CALLBACK(playlist_select_callback), NULL);
    g_signal_connect(G_OBJECT(list), "button-release-event",
                     G_CALLBACK(button_release_callback), NULL);
    g_signal_connect(G_OBJECT(list), "motion-notify-event",
                     G_CALLBACK(playlist_motion_callback), NULL);
    g_signal_connect(G_OBJECT(list), "enter-notify-event",
                     G_CALLBACK(playlist_enter_callback), NULL);
    g_signal_connect(G_OBJECT(list), "leave-notify-event",
                     G_CALLBACK(playlist_leave_callback), NULL);
    // Give the window the property to accept DnD
    target_entry[i].target = DRAG_NAME_0;
    target_entry[i].flags = 0;
    target_entry[i++].info = DRAG_INFO_0;
    target_entry[i].target = DRAG_NAME_1;
    target_entry[i].flags = 0;
    target_entry[i++].info = DRAG_INFO_1;
    target_entry[i].target = DRAG_NAME_2;
    target_entry[i].flags = 0;
    target_entry[i++].info = DRAG_INFO_2;

    gtk_drag_dest_set(plvbox,
                      GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
                      GTK_DEST_DEFAULT_DROP, target_entry, i, GDK_ACTION_LINK);

    //Connect the signal for DnD
    g_signal_connect(GTK_OBJECT(plvbox), "drag_data_received",
                     G_CALLBACK(playlist_drop_callback), NULL);

#ifdef GTK2_12_ENABLED
#else
    playlisttip = gtk_tooltips_new();
#endif

    count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
    renderer = gtk_cell_renderer_text_new();
    column =
        gtk_tree_view_column_new_with_attributes(ngettext
                                                 ("Item to play", "Items to Play", count),
                                                 renderer, "text", DESCRIPTION_COLUMN, NULL);
    if (playlistname != NULL) {
        coltitle = g_strdup_printf(_("%s items"), playlistname);
        split = g_strsplit(coltitle, "_", 0);
        joined = g_strjoinv("__", split);
        gtk_tree_view_column_set_title(column, joined);
        g_free(coltitle);
        g_free(joined);
        g_strfreev(split);
    }
    gtk_tree_view_column_set_expand(column, TRUE);
    //gtk_tree_view_column_set_max_width(column, 40);
    g_object_set(renderer, "width-chars", 40, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, DESCRIPTION_COLUMN);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Artist"),
                                                      renderer, "text", ARTIST_COLUMN, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    //gtk_tree_view_column_set_max_width(column, 20);
    g_object_set(renderer, "width-chars", 20, NULL);
    gtk_tree_view_column_set_alignment(column, 0.0);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, ARTIST_COLUMN);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Album"),
                                                      renderer, "text", ALBUM_COLUMN, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    //gtk_tree_view_column_set_max_width(column, 20);
    g_object_set(renderer, "width-chars", 20, NULL);
    gtk_tree_view_column_set_alignment(column, 0.0);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, ALBUM_COLUMN);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);


    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Length"),
                                                      renderer, "text", LENGTH_COLUMN, NULL);
    //gtk_tree_view_column_set_expand(column, FALSE);
    gtk_tree_view_column_set_alignment(column, 1.0);
    g_object_set(renderer, "xalign", 1.0, NULL);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);


    renderer = gtk_cell_renderer_text_new();
    column =
        gtk_tree_view_column_new_with_attributes(_("Count"), renderer, "text", COUNT_COLUMN, NULL);
    //gtk_tree_view_column_set_expand(column, FALSE);
    g_object_set(renderer, "xalign", 1.0, NULL);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_sort_column_id(column, COUNT_COLUMN);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

    renderer = gtk_cell_renderer_text_new();
    column =
        gtk_tree_view_column_new_with_attributes(_("Order"), renderer, "text", PLAY_ORDER_COLUMN,
                                                 NULL);
    //gtk_tree_view_column_set_expand(column, FALSE);
    g_object_set(renderer, "xalign", 1.0, NULL);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_sort_column_id(column, PLAY_ORDER_COLUMN);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);


    plclose = gtk_button_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(plclose, _("Close Playlist View"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, plclose, _("Close Playlist View"), NULL);
#endif
    gtk_container_add(GTK_CONTAINER(plclose),
                      gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));

    g_signal_connect_swapped(GTK_OBJECT(plclose), "clicked", G_CALLBACK(playlist_close), NULL);


    loadlist = gtk_button_new();


#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(loadlist, _("Open Playlist"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, loadlist, _("Open Playlist"), NULL);
#endif
    gtk_container_add(GTK_CONTAINER(loadlist),
                      gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
    gtk_box_pack_start(GTK_BOX(ctrlbox), loadlist, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(loadlist), "clicked", G_CALLBACK(load_playlist), NULL);

    savelist = gtk_button_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(savelist, _("Save Playlist"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, savelist, _("Save Playlist"), NULL);
#endif
    gtk_container_add(GTK_CONTAINER(savelist),
                      gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU));
    gtk_box_pack_start(GTK_BOX(ctrlbox), savelist, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(savelist), "clicked", G_CALLBACK(save_playlist), NULL);

    add = gtk_button_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(add, _("Add Item to Playlist"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, add, _("Add Item to Playlist"), NULL);
#endif
    gtk_button_set_image(GTK_BUTTON(add),
                         gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
    gtk_box_pack_start(GTK_BOX(ctrlbox), add, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(add), "clicked", G_CALLBACK(add_to_playlist), NULL);

    remove = gtk_button_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(remove, _("Remove Item from Playlist"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, remove, _("Remove Item from Playlist"), NULL);
#endif
    gtk_button_set_image(GTK_BUTTON(remove),
                         gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
    gtk_box_pack_start(GTK_BOX(ctrlbox), remove, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(remove), "clicked", G_CALLBACK(remove_from_playlist), list);

    add_folder = gtk_button_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(add_folder, _("Add Items in Folder to Playlist"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, add_folder, _("Add Items in Folder to Playlist"), NULL);
#endif
    gtk_button_set_image(GTK_BUTTON(add_folder),
                         gtk_image_new_from_stock(GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_MENU));
    gtk_box_pack_start(GTK_BOX(ctrlbox), add_folder, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(add_folder), "clicked", G_CALLBACK(add_folder_to_playlist), list);

    clear = gtk_button_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(clear, _("Clear Playlist"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, clear, _("Clear Playlist"), NULL);
#endif
    gtk_button_set_image(GTK_BUTTON(clear),
                         gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU));
    gtk_box_pack_start(GTK_BOX(ctrlbox), clear, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(clear), "clicked", G_CALLBACK(clear_playlist), list);

    up = gtk_button_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(up, _("Move Item Up"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, up, _("Move Item Up"), NULL);
#endif
    gtk_button_set_image(GTK_BUTTON(up),
                         gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU));
    gtk_box_pack_start(GTK_BOX(ctrlbox), up, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(up), "clicked", G_CALLBACK(move_item_up), list);
    gtk_widget_set_sensitive(up, FALSE);

    down = gtk_button_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(down, _("Move Item Down"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, down, _("Move Item Down"), NULL);
#endif
    gtk_button_set_image(GTK_BUTTON(down),
                         gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU));
    gtk_box_pack_start(GTK_BOX(ctrlbox), down, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(down), "clicked", G_CALLBACK(move_item_down), list);
    gtk_widget_set_sensitive(down, FALSE);


    undo = gtk_button_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(undo, _("UnSort List"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, undo, _("UnSort List"), NULL);
#endif
    gtk_button_set_image(GTK_BUTTON(undo),
                         gtk_image_new_from_stock(GTK_STOCK_UNDO, GTK_ICON_SIZE_MENU));
    gtk_box_pack_start(GTK_BOX(ctrlbox), undo, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(undo), "clicked", G_CALLBACK(undo_playlist_sort), list);
    gtk_widget_set_sensitive(undo, TRUE);


    accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
    gtk_widget_add_accelerator(GTK_WIDGET(remove), "clicked",
                               accel_group, GDK_Delete, 0, GTK_ACCEL_VISIBLE);

    repeat = gtk_button_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(repeat, _("Loop Playlist"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, repeat, _("Loop Playlist"), NULL);
#endif
    gtk_button_set_image(GTK_BUTTON(repeat),
                         gtk_image_new_from_icon_name("media-playlist-repeat", GTK_ICON_SIZE_MENU));
    gtk_box_pack_start(GTK_BOX(ctrlbox), repeat, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(repeat), "clicked", G_CALLBACK(repeat_callback), NULL);
    gtk_widget_set_sensitive(repeat, TRUE);

    shuffle = gtk_button_new();
#ifdef GTK2_12_ENABLED
    gtk_widget_set_tooltip_text(shuffle, _("Shuffle Playlist"));
#else
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip, shuffle, _("Shuffle Playlist"), NULL);
#endif
    gtk_button_set_image(GTK_BUTTON(shuffle),
                         gtk_image_new_from_icon_name("media-playlist-shuffle",
                                                      GTK_ICON_SIZE_MENU));
    gtk_box_pack_start(GTK_BOX(ctrlbox), shuffle, FALSE, FALSE, 0);
    g_signal_connect(GTK_OBJECT(shuffle), "clicked", G_CALLBACK(shuffle_callback), NULL);
    gtk_widget_set_sensitive(shuffle, TRUE);


#ifdef GTK2_18_ENABLED
    gtk_widget_set_can_default(plclose, TRUE);
#else
    GTK_WIDGET_SET_FLAGS(plclose, GTK_CAN_DEFAULT);
#endif
    gtk_box_pack_end(GTK_BOX(closebox), plclose, FALSE, FALSE, 0);

    scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), list);

    gtk_box_pack_start(GTK_BOX(plvbox), scrolled, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), ctrlbox, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), closebox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(plvbox), hbox, FALSE, FALSE, 0);


    gtk_paned_pack2(GTK_PANED(pane), plvbox, FALSE, FALSE);

    gtk_widget_style_get(pane, "handle-size", &handle_size, NULL);
    gtk_widget_set_size_request(plvbox, 350, 200);

    playlist_popup_menu = GTK_MENU(gtk_menu_new());
    playlist_set_subtitle =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Set Subtitle")));
    g_signal_connect(GTK_OBJECT(playlist_set_subtitle), "activate",
                     G_CALLBACK(playlist_set_subtitle_callback), list);

    gtk_menu_append(playlist_popup_menu, GTK_WIDGET(playlist_set_subtitle));
    playlist_set_audiofile =
        GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("Set Audi_o")));
    g_signal_connect(GTK_OBJECT(playlist_set_audiofile), "activate",
                     G_CALLBACK(playlist_set_audiofile_callback), list);

    gtk_menu_append(playlist_popup_menu, GTK_WIDGET(playlist_set_audiofile));
    g_signal_connect_swapped(G_OBJECT(list),
                             "button_press_event",
                             G_CALLBACK(playlist_popup_handler), G_OBJECT(playlist_popup_menu));
    gtk_widget_show_all(GTK_WIDGET(playlist_popup_menu));


}
