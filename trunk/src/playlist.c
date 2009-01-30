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
    if (selection_data->length < 0)
        return FALSE;

    if ((info == DRAG_INFO_0) || (info == DRAG_INFO_1) || (info == DRAG_INFO_2)) {
        list = g_uri_list_extract_uris((const gchar *) selection_data->data);

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
            play_iter(&iter);

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
            gtk_tooltips_set_tip(playlisttip, widget, tip, NULL);
            g_free(tip);
            g_free(iterfilename);
        }
        gtk_tree_path_free(localpath);
    }
    return FALSE;

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
    init_preference_store();
    last_dir = read_preference_string(LAST_DIR);
    if (last_dir != NULL)
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
        write_preference_string(LAST_DIR, last_dir);
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
    release_preference_store();
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
    init_preference_store();
    last_dir = read_preference_string(LAST_DIR);
    if (last_dir != NULL)
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
        write_preference_string(LAST_DIR, last_dir);
        g_free(last_dir);

        gtk_list_store_clear(playliststore);
        gtk_list_store_clear(nonrandomplayliststore);

        create_folder_progress_window();
        if (!parse_playlist(filename)) {
            add_item_to_playlist(filename, 1);
        }
        destroy_folder_progress_window();
    }
    release_preference_store();
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
            info = g_file_enumerator_next_file(dir, NULL, NULL);
        }
        list = g_slist_sort(list, (GCompareFunc) compar);
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
        } while (TRUE);
        list = g_slist_sort(list, (GCompareFunc) compar);
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

    init_preference_store();
    last_dir = read_preference_string(LAST_DIR);
    if (last_dir != NULL) {
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), last_dir);
        g_free(last_dir);
    }

    count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));

        last_dir = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
        write_preference_string(LAST_DIR, last_dir);
        g_free(last_dir);

        g_slist_foreach(uris, &add_item_to_playlist_callback, NULL);
        g_slist_free(uris);
    }
    update_gui();
    release_preference_store();
    column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
    if (playlistname != NULL && strlen(playlistname) > 0 && count == 0) {
        coltitle = g_strdup_printf(_("%s items"), playlistname);
    } else {
        coltitle = g_strdup_printf(ngettext("Item to Play", "Items to Play", count));
    }
    gtk_tree_view_column_set_title(column, coltitle);
    g_free(coltitle);

    gtk_widget_destroy(dialog);
}

void add_folder_to_playlist(GtkWidget * widget, void *data)
{

    GtkWidget *dialog;
    GSList *uris;
    gchar *last_dir;
    gchar *message;

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

    init_preference_store();
    last_dir = read_preference_string(LAST_DIR);
    if (last_dir != NULL) {
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), last_dir);
        g_free(last_dir);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));

        last_dir = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
        write_preference_string(LAST_DIR, last_dir);
        g_free(last_dir);

        filecount = 0;
        create_folder_progress_window();
        g_slist_foreach(uris, &add_folder_to_playlist_callback, NULL);
        destroy_folder_progress_window();
        g_slist_free(uris);
    }
    update_gui();
    release_preference_store();
    gtk_widget_destroy(dialog);
    message =
        g_markup_printf_escaped(ngettext("\n\tFound %i file\n", "\n\tFound %i files\n", filecount),
                                filecount);
    g_strlcpy(idledata->media_info, message, 1024);
    g_free(message);
    g_idle_add(set_media_label, idledata);

}

void remove_from_playlist(GtkWidget * widget, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeView *view = (GtkTreeView *) data;
    GtkTreeIter localiter;
    GtkTreePath *path;
    gchar *localfilename;
    gchar *removedfilename;

    sel = gtk_tree_view_get_selection(view);

    if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN, &removedfilename, -1);
        localiter = iter;
        gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore), &localiter);
        if (gtk_list_store_remove(playliststore, &iter)) {
            iter = localiter;
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(nonrandomplayliststore), &localiter);
            do {
                gtk_tree_model_get(GTK_TREE_MODEL(nonrandomplayliststore), &localiter, ITEM_COLUMN,
                                   &localfilename, -1);
                if (localfilename != NULL) {
                    if (g_ascii_strcasecmp(removedfilename, localfilename) == 0) {
                        gtk_list_store_remove(nonrandomplayliststore, &localiter);
                    }
                    g_free(localfilename);
                }
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(nonrandomplayliststore), &localiter));

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
        g_free(removedfilename);
    }
    update_gui();

}

void clear_playlist(GtkWidget * widget, void *data)
{

    dontplaynext = TRUE;
    mplayer_shutdown();
    gtk_list_store_clear(playliststore);
    gtk_list_store_clear(nonrandomplayliststore);
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
                }
            }
            if (gtk_tree_model_iter_nth_child
                (GTK_TREE_MODEL(nonrandomplayliststore), &a, NULL, pos)) {
                if (gtk_tree_model_iter_nth_child
                    (GTK_TREE_MODEL(nonrandomplayliststore), &b, NULL, pos - 1)) {
                    gtk_list_store_swap(nonrandomplayliststore, &a, &b);
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
                }
            }
            if (gtk_tree_model_iter_nth_child
                (GTK_TREE_MODEL(nonrandomplayliststore), &a, NULL, pos)) {
                if (gtk_tree_model_iter_nth_child
                    (GTK_TREE_MODEL(nonrandomplayliststore), &b, NULL, pos + 1)) {
                    gtk_list_store_swap(nonrandomplayliststore, &a, &b);
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
        play_iter(&iter);
        dontplaynext = FALSE;
    }
    return FALSE;
}

void playlist_close(GtkWidget * widget, void *data)
{
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist), FALSE);
}

void menuitem_view_playlist_callback(GtkMenuItem * menuitem, void *data)
{

    GtkWidget *close;
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
    GtkTreePath *path;
    GtkAccelGroup *accel_group;
    GValue value = { 0, };
    GtkTooltips *tooltip;
    gchar *coltitle;
    gint x, y, depth;
    gint count;
    GtkTargetEntry target_entry[3];
    gint i = 0;
    gint handle_size;

    g_value_init(&value, G_TYPE_BOOLEAN);

    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem_view_playlist)) == FALSE) {
        if (GTK_WIDGET_VISIBLE(plvbox)) {
            if (idledata->videopresent == FALSE) {
                gdk_window_get_geometry(window->window, &x, &y, &window_width, &window_height,
                                        &depth);
                stored_window_width = -1;
                stored_window_height = -1;
                gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
                gtk_widget_hide(GTK_WIDGET(fixed));
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem_view_info), TRUE);
                gtk_widget_show(vbox);
                gtk_widget_set_size_request(window, -1, -1);
            } else {
                gtk_widget_style_get(pane, "handle-size", &handle_size, NULL);
                gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
                gtk_widget_set_size_request(window, -1, -1);
                gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, TRUE);
                window_width = -1;
                window_height = -1;
                gtk_window_get_size(GTK_WINDOW(window), &window_width, &window_height);
                gtk_widget_hide(plvbox);
                if (vertical_layout) {
                    gtk_window_resize(GTK_WINDOW(window), window_width,
                                      window_height - plvbox->allocation.height - handle_size);
                } else {
                    gtk_window_resize(GTK_WINDOW(window),
                                      window_width - plvbox->allocation.width - handle_size,
                                      window_height);
                }
            }
            gtk_container_remove(GTK_CONTAINER(pane), plvbox);
            stored_window_width = -1;
            stored_window_height = -1;
            plvbox = NULL;
            selection = NULL;
            playlist_visible = FALSE;
        }

    } else {
        gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
        if (idledata->videopresent == FALSE) {
            if (window_width != -1)
                gtk_window_resize(GTK_WINDOW(window), window_width, window_height);
            if (vertical_layout) {
                gtk_widget_hide(fixed);
            } else {
                gtk_widget_hide(vbox);
            }
        } else {
            // set the window size properly when coming out of fullscreen mode.
            if (restore_playlist) {
                if (vertical_layout) {
                    stored_window_height = restore_pane;
                } else {
                    stored_window_width = restore_pane;
                }
            } else {
                gdk_window_get_geometry(window->window, &x, &y, &stored_window_width,
                                        &stored_window_height, &depth);
            }
        }
        plvbox = gtk_vbox_new(FALSE, 12);
        hbox = gtk_hbox_new(FALSE, 12);
        gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE);
        box = gtk_hbox_new(FALSE, 10);
        ctrlbox = gtk_hbox_new(FALSE, 0);
        closebox = gtk_hbox_new(FALSE, 0);

        list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(playliststore));
        gtk_tree_view_set_reorderable(GTK_TREE_VIEW(list), TRUE);
        gtk_widget_add_events(list, GDK_BUTTON_PRESS_MASK);
        gtk_widget_set_size_request(GTK_WIDGET(list), -1, -1);

        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
        if (gtk_list_store_iter_is_valid(playliststore, &iter)) {
            path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore), &iter);
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list), path, NULL, FALSE, 0, 0);
            gtk_tree_path_free(path);
        }

        g_signal_connect(G_OBJECT(list), "row_activated", G_CALLBACK(playlist_select_callback),
                         NULL);
        g_signal_connect(G_OBJECT(list), "button-release-event",
                         G_CALLBACK(button_release_callback), NULL);
        g_signal_connect(G_OBJECT(list), "motion-notify-event",
                         G_CALLBACK(playlist_motion_callback), NULL);
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
                         GTK_SIGNAL_FUNC(playlist_drop_callback), NULL);


        playlisttip = gtk_tooltips_new();

        count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore), NULL);
        renderer = gtk_cell_renderer_text_new();
        column =
            gtk_tree_view_column_new_with_attributes(ngettext
                                                     ("Item to play", "Items to Play", count),
                                                     renderer, "text", DESCRIPTION_COLUMN, NULL);
        if (playlistname != NULL) {
            coltitle = g_strdup_printf(_("%s items"), playlistname);
            gtk_tree_view_column_set_title(column, coltitle);
            g_free(coltitle);
        }
        gtk_tree_view_column_set_expand(column, TRUE);
        //gtk_tree_view_column_set_max_width(column, 40);
        g_object_set(renderer, "width-chars", 40, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes(_("Artist"),
                                                          renderer, "text", ARTIST_COLUMN, NULL);
        gtk_tree_view_column_set_expand(column, TRUE);
        //gtk_tree_view_column_set_max_width(column, 20);
        g_object_set(renderer, "width-chars", 20, NULL);
        gtk_tree_view_column_set_alignment(column, 0.0);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes(_("Album"),
                                                          renderer, "text", ALBUM_COLUMN, NULL);
        gtk_tree_view_column_set_expand(column, TRUE);
        //gtk_tree_view_column_set_max_width(column, 20);
        g_object_set(renderer, "width-chars", 20, NULL);
        gtk_tree_view_column_set_alignment(column, 0.0);
        gtk_tree_view_column_set_resizable(column, TRUE);
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
        column = gtk_tree_view_column_new_with_attributes("", renderer, "text", COUNT_COLUMN, NULL);
        //gtk_tree_view_column_set_expand(column, FALSE);
        g_object_set(renderer, "xalign", 1.0, NULL);
        gtk_tree_view_column_set_resizable(column, FALSE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);


        close = gtk_button_new();
        tooltip = gtk_tooltips_new();
        gtk_tooltips_set_tip(tooltip, close, _("Close Playlist View"), NULL);
        gtk_container_add(GTK_CONTAINER(close),
                          gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));

        g_signal_connect_swapped(GTK_OBJECT(close), "clicked",
                                 GTK_SIGNAL_FUNC(playlist_close), NULL);


        loadlist = gtk_button_new();
        tooltip = gtk_tooltips_new();
        gtk_tooltips_set_tip(tooltip, loadlist, _("Open Playlist"), NULL);
        gtk_container_add(GTK_CONTAINER(loadlist),
                          gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
        gtk_box_pack_start(GTK_BOX(ctrlbox), loadlist, FALSE, FALSE, 0);
        g_signal_connect(GTK_OBJECT(loadlist), "clicked", GTK_SIGNAL_FUNC(load_playlist), NULL);

        savelist = gtk_button_new();
        tooltip = gtk_tooltips_new();
        gtk_tooltips_set_tip(tooltip, savelist, _("Save Playlist"), NULL);
        gtk_container_add(GTK_CONTAINER(savelist),
                          gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU));
        gtk_box_pack_start(GTK_BOX(ctrlbox), savelist, FALSE, FALSE, 0);
        g_signal_connect(GTK_OBJECT(savelist), "clicked", GTK_SIGNAL_FUNC(save_playlist), NULL);

        add = gtk_button_new();
        tooltip = gtk_tooltips_new();
        gtk_tooltips_set_tip(tooltip, add, _("Add Item to Playlist"), NULL);
        gtk_button_set_image(GTK_BUTTON(add),
                             gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
        gtk_box_pack_start(GTK_BOX(ctrlbox), add, FALSE, FALSE, 0);
        g_signal_connect(GTK_OBJECT(add), "clicked", GTK_SIGNAL_FUNC(add_to_playlist), NULL);

        remove = gtk_button_new();
        tooltip = gtk_tooltips_new();
        gtk_tooltips_set_tip(tooltip, remove, _("Remove Item from Playlist"), NULL);
        gtk_button_set_image(GTK_BUTTON(remove),
                             gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
        gtk_box_pack_start(GTK_BOX(ctrlbox), remove, FALSE, FALSE, 0);
        g_signal_connect(GTK_OBJECT(remove), "clicked", GTK_SIGNAL_FUNC(remove_from_playlist),
                         list);

        add_folder = gtk_button_new();
        tooltip = gtk_tooltips_new();
        gtk_tooltips_set_tip(tooltip, add_folder, _("Add Items in Folder to Playlist"), NULL);
        gtk_button_set_image(GTK_BUTTON(add_folder),
                             gtk_image_new_from_stock(GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_MENU));
        gtk_box_pack_start(GTK_BOX(ctrlbox), add_folder, FALSE, FALSE, 0);
        g_signal_connect(GTK_OBJECT(add_folder), "clicked", GTK_SIGNAL_FUNC(add_folder_to_playlist),
                         list);

        clear = gtk_button_new();
        tooltip = gtk_tooltips_new();
        gtk_tooltips_set_tip(tooltip, clear, _("Clear Playlist"), NULL);
        gtk_button_set_image(GTK_BUTTON(clear),
                             gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU));
        gtk_box_pack_start(GTK_BOX(ctrlbox), clear, FALSE, FALSE, 0);
        g_signal_connect(GTK_OBJECT(clear), "clicked", GTK_SIGNAL_FUNC(clear_playlist), list);

        up = gtk_button_new();
        tooltip = gtk_tooltips_new();
        gtk_tooltips_set_tip(tooltip, up, _("Move Item Up"), NULL);
        gtk_button_set_image(GTK_BUTTON(up),
                             gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU));
        gtk_box_pack_start(GTK_BOX(ctrlbox), up, FALSE, FALSE, 0);
        g_signal_connect(GTK_OBJECT(up), "clicked", GTK_SIGNAL_FUNC(move_item_up), list);
        gtk_widget_set_sensitive(up, FALSE);

        down = gtk_button_new();
        tooltip = gtk_tooltips_new();
        gtk_tooltips_set_tip(tooltip, down, _("Move Item Down"), NULL);
        gtk_button_set_image(GTK_BUTTON(down),
                             gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU));
        gtk_box_pack_start(GTK_BOX(ctrlbox), down, FALSE, FALSE, 0);
        g_signal_connect(GTK_OBJECT(down), "clicked", GTK_SIGNAL_FUNC(move_item_down), list);
        gtk_widget_set_sensitive(down, FALSE);


        accel_group = gtk_accel_group_new();
        gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
        gtk_widget_add_accelerator(GTK_WIDGET(remove), "clicked",
                                   accel_group, GDK_Delete, 0, GTK_ACCEL_VISIBLE);

        GTK_WIDGET_SET_FLAGS(close, GTK_CAN_DEFAULT);
        gtk_box_pack_end(GTK_BOX(closebox), close, FALSE, FALSE, 0);

        scrolled = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scrolled), list);

        gtk_box_pack_start(GTK_BOX(plvbox), scrolled, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ctrlbox, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(hbox), closebox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(plvbox), hbox, FALSE, FALSE, 0);


        gtk_paned_pack2(GTK_PANED(pane), plvbox, TRUE, TRUE);
        adjust_paned_rules();

        gtk_widget_style_get(pane, "handle-size", &handle_size, NULL);
        if (vertical_layout) {
            gtk_widget_set_size_request(plvbox, -1, 150);
            if (idledata->videopresent)
                gtk_window_resize(GTK_WINDOW(window), stored_window_width,
                                  stored_window_height + 150 + handle_size);
        } else {
            gtk_widget_set_size_request(plvbox, 300, -1);
            if (idledata->videopresent) {
                gtk_window_resize(GTK_WINDOW(window), stored_window_width + 300 + handle_size,
                                  stored_window_height);
            }
        }
        gtk_widget_show_all(plvbox);
        if (idledata->videopresent == FALSE) {
            gtk_widget_show(GTK_WIDGET(fixed));
        } else {
            gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, TRUE);
        }

        playlist_popup_menu = GTK_MENU(gtk_menu_new());
        playlist_set_subtitle =
            GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(_("_Set Subtitle")));
        g_signal_connect(GTK_OBJECT(playlist_set_subtitle), "activate",
                         G_CALLBACK(playlist_set_subtitle_callback), list);

        gtk_menu_append(playlist_popup_menu, GTK_WIDGET(playlist_set_subtitle));
        g_signal_connect_swapped(G_OBJECT(list),
                                 "button_press_event",
                                 G_CALLBACK(playlist_popup_handler), G_OBJECT(playlist_popup_menu));
        gtk_widget_show_all(GTK_WIDGET(playlist_popup_menu));
        playlist_visible = TRUE;

        gtk_widget_grab_default(close);
    }
}
