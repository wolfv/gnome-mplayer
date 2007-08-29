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
	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore),NULL) < 2) {
		gtk_widget_hide(prev_event_box);
		gtk_widget_hide(next_event_box);
	} else {
		gtk_widget_show(prev_event_box);
		gtk_widget_show(next_event_box);
	}		
}	
	


gboolean playlist_drop_callback(GtkWidget * widget, GdkDragContext * dc,
                   gint x, gint y, GtkSelectionData * selection_data,
                   guint info, guint t, gpointer data)
{
    gchar *filename;
	GtkTreeIter localiter;
	gchar **list;
	gint i = 0;
    /* Important, check if we actually got data.  Sometimes errors
     * occure and selection_data will be NULL.
     */
    if (selection_data == NULL)
        return FALSE;
    if (selection_data->length < 0)
        return FALSE;

    if ((info == DRAG_INFO_0) || (info == DRAG_INFO_1) || (info == DRAG_INFO_2)) {
        filename = g_filename_from_uri((const gchar *) selection_data->data, NULL, NULL);
		list = g_strsplit(filename,"\n",0);
		
		while(list[i] != NULL) {
			g_strchomp(list[i]);
			if(strlen(list[i]) > 0) {
				if(!parse_playlist(list[i])) {
					localiter = add_item_to_playlist(list[i],0);
				}
			}
			i++;
		}

		if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playliststore),NULL) < 2) {
			iter = localiter;
			shutdown();
			play_file(list[0],0);
		}
		
		g_strfreev(list);
    }
	update_gui();
	return FALSE;
}

void save_playlist(GtkWidget * widget, void *data)
{
    GtkWidget *dialog;
    gchar *filename;
	gchar *new_filename;
	GtkFileFilter *filter;
    GConfClient *gconf;
    gchar *last_dir;

    dialog = gtk_file_chooser_dialog_new(_("Save Playlist"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    gconf = gconf_client_get_default();
    last_dir = gconf_client_get_string(gconf, LAST_DIR, NULL);
    if (last_dir != NULL)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), last_dir);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter,"Playlist (*.pls)");
	gtk_file_filter_add_pattern(filter,"*.pls");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter,"MP3 Playlist (*.m3u)");
	gtk_file_filter_add_pattern(filter,"*.m3u");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),filter);
	
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        last_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
        gconf_client_set_string(gconf, LAST_DIR, last_dir, NULL);
        g_free(last_dir);
		
		if (g_strrstr(filename,".m3u") != NULL) {
			save_playlist_m3u(filename);
		}
		if (g_strrstr(filename,".pls") != NULL) {
			save_playlist_pls(filename);
		}
		
		if (g_strrstr(filename,".") == NULL) {
			new_filename = g_strdup_printf("%s.pls",filename);
			save_playlist_pls(new_filename);
			g_free(new_filename);
		}			
		g_free(filename);
	}
	
	gtk_widget_destroy(dialog);

}

void load_playlist(GtkWidget * widget, void *data)
{
    GtkWidget *dialog;
    gchar *filename;
	GtkFileFilter *filter;
    GConfClient *gconf;
    gchar *last_dir;

    dialog = gtk_file_chooser_dialog_new(_("Open Playlist"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    gconf = gconf_client_get_default();
    last_dir = gconf_client_get_string(gconf, LAST_DIR, NULL);
    if (last_dir != NULL)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), last_dir);
	
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter,"Playlist (*.pls)");
	gtk_file_filter_add_pattern(filter,"*.pls");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter,"Reference Playlist (*.ref)");
	gtk_file_filter_add_pattern(filter,"*.ref");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter,"MP3 Playlist (*.m3u)");
	gtk_file_filter_add_pattern(filter,"*.m3u");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),filter);
	
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        last_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
        gconf_client_set_string(gconf, LAST_DIR, last_dir, NULL);
        g_free(last_dir);
		
		gtk_list_store_clear(playliststore);
		if (!parse_playlist(filename)) {	
			add_item_to_playlist(filename,1);
		}

	}
	update_gui();
	gtk_widget_destroy(dialog);
}

void add_to_playlist(GtkWidget * widget, void *data)
{
	
    GtkWidget *dialog;
    gchar *filename;
    GConfClient *gconf;
    gchar *last_dir;
	gint playlist;

    dialog = gtk_file_chooser_dialog_new(_("Open File"),
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    gconf = gconf_client_get_default();
    last_dir = gconf_client_get_string(gconf, LAST_DIR, NULL);
    if (last_dir != NULL)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), last_dir);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        last_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
        gconf_client_set_string(gconf, LAST_DIR, last_dir, NULL);
        g_free(last_dir);

        playlist = detect_playlist(filename);
			
		if (!playlist ) {
			add_item_to_playlist(filename,playlist);
		} else {
			if (!parse_playlist(filename)) {	
				add_item_to_playlist(filename,playlist);
			}
		}
        g_free(filename);
    }
	update_gui();
    g_object_unref(G_OBJECT(gconf));
    gtk_widget_destroy(dialog);
	
}

void remove_from_playlist(GtkWidget * widget, gpointer data)
{
	GtkTreeSelection *sel;
	GtkTreeView *view = (GtkTreeView *)data;
	GtkTreeIter localiter;
	GtkTreePath *path;
	
	sel = gtk_tree_view_get_selection(view);
	
	if (gtk_tree_selection_get_selected(sel,NULL,&iter)) {
		localiter = iter;
		gtk_tree_model_iter_next(GTK_TREE_MODEL(playliststore),&localiter);
		if(gtk_list_store_remove(playliststore, &iter)){
			iter = localiter;
			if (!gtk_list_store_iter_is_valid(playliststore,&iter)) {
				gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playliststore),&iter);
			}
			if (GTK_IS_TREE_SELECTION(selection)) {
				path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore),&iter);
				gtk_tree_selection_select_path(selection,path);	
				gtk_tree_path_free(path);
			}
		} 
	}
	update_gui();
	
}

gboolean playlist_select_callback(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data) {

	gchar *filename;
	gint count;
	gint playlist;

	if( gtk_tree_model_get_iter(GTK_TREE_MODEL(playliststore), &iter, path)) {
		dontplaynext = TRUE;
		gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &iter, ITEM_COLUMN,&filename, COUNT_COLUMN,&count,PLAYLIST_COLUMN,&playlist,-1);
		shutdown();
		set_media_info(filename);
		play_file(filename, playlist);
		gtk_list_store_set(playliststore,&iter,COUNT_COLUMN,count+1, -1);
		g_free(filename);
	}	
	return FALSE;
}

void menuitem_view_playlist_callback(GtkMenuItem * menuitem, void *data) {

	GtkWidget *playlist_window;
	GtkWidget *close;
	GtkWidget *list;
	GtkWidget *scrolled;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
    GtkWidget *vbox;
	GtkWidget *box;
	GtkWidget *ctrlbox;
	GtkWidget *closebox;
    GtkWidget *hbox;
	GtkWidget *moveup;
	GtkWidget *movedown;
	GtkWidget *savelist;
	GtkWidget *loadlist;
	GtkWidget *add;
	GtkWidget *remove;
	GtkTargetEntry target_entry[3];
	gint i = 0;
	GtkTreePath *path;
	GdkRectangle rect;
	GtkAccelGroup *accel_group;

	
	if (GTK_IS_TREE_SELECTION(selection)) return;
	
	playlist_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	//gtk_widget_set_size_request(GTK_WIDGET(playlist_window),300,200);
   	gtk_window_set_icon(GTK_WINDOW(playlist_window), pb_icon);
	
    gtk_window_set_type_hint(GTK_WINDOW(playlist_window), GDK_WINDOW_TYPE_HINT_UTILITY);
    gtk_window_set_title(GTK_WINDOW(playlist_window), _("Playlist"));

    vbox = gtk_vbox_new(FALSE, 12);
    hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_set_homogeneous (GTK_BOX(hbox), FALSE);
	box = gtk_hbox_new(FALSE,10);
	ctrlbox = gtk_hbutton_box_new();
	closebox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(closebox),GTK_BUTTONBOX_END);
	
    //gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
    //gtk_button_box_set_layout(GTK_BUTTON_BOX(ctrlbox),GTK_BUTTONBOX_START);

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

    gtk_drag_dest_set(playlist_window,
                      GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
                      GTK_DEST_DEFAULT_DROP, target_entry, i, GDK_ACTION_LINK);
	
    g_signal_connect(GTK_OBJECT(playlist_window), "drag_data_received", GTK_SIGNAL_FUNC(playlist_drop_callback),
                     NULL);

	
	list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(playliststore));
	gtk_widget_set_size_request(GTK_WIDGET(list),-1,300);
	
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
	if(gtk_list_store_iter_is_valid(playliststore,&iter)) {
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore),&iter);
		gtk_tree_selection_select_path(selection,path);	
		gtk_tree_path_free(path);
	}

	g_signal_connect(GTK_OBJECT(list), "row_activated", GTK_SIGNAL_FUNC(playlist_select_callback),
                     NULL);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Items to Play"),
                                                   renderer,
                                                   "text", DESCRIPTION_COLUMN,
                                                   NULL);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
                                                   renderer,
                                                   "text", COUNT_COLUMN,
                                                   NULL);
	gtk_tree_view_column_set_expand(column, FALSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

	
    close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(GTK_OBJECT(close), "clicked",
                             GTK_SIGNAL_FUNC(config_close), playlist_window);


	moveup = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	movedown = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	savelist = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	loadlist = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	add = gtk_button_new_from_stock(GTK_STOCK_ADD);
	remove = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	gtk_widget_set_sensitive(moveup,FALSE);
	gtk_widget_set_sensitive(movedown,FALSE);
    //gtk_container_add(GTK_CONTAINER(ctrlbox), moveup);
    //gtk_container_add(GTK_CONTAINER(ctrlbox), movedown);
    gtk_container_add(GTK_CONTAINER(ctrlbox), savelist);
	g_signal_connect(GTK_OBJECT(savelist),"clicked",GTK_SIGNAL_FUNC(save_playlist),NULL);
    gtk_container_add(GTK_CONTAINER(ctrlbox), loadlist);
	g_signal_connect(GTK_OBJECT(loadlist),"clicked",GTK_SIGNAL_FUNC(load_playlist),NULL);
    gtk_container_add(GTK_CONTAINER(ctrlbox), add);
	g_signal_connect(GTK_OBJECT(add),"clicked",GTK_SIGNAL_FUNC(add_to_playlist),NULL);
    gtk_container_add(GTK_CONTAINER(ctrlbox), remove);
	g_signal_connect(GTK_OBJECT(remove),"clicked",GTK_SIGNAL_FUNC(remove_from_playlist),list);
	
    accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(playlist_window), accel_group);
    gtk_widget_add_accelerator(GTK_WIDGET(remove), "clicked",
                               accel_group, GDK_Delete, 0, GTK_ACCEL_VISIBLE);
	
	GTK_WIDGET_SET_FLAGS(close, GTK_CAN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(closebox), close);

	scrolled = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
      								GTK_POLICY_NEVER,
      								GTK_POLICY_AUTOMATIC);	
	gtk_container_add(GTK_CONTAINER(scrolled),list);
	
	gtk_box_pack_start(GTK_BOX(vbox),scrolled,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),ctrlbox,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),closebox,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
	
	
    gtk_container_add(GTK_CONTAINER(playlist_window), vbox);
	
	gtk_widget_show_all(playlist_window);
	gtk_widget_grab_default(close);
	gdk_window_get_frame_extents(window->window,&rect);
	gtk_window_move(GTK_WINDOW(playlist_window),rect.x + rect.width, rect.y);
	
}
