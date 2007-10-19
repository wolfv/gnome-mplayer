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
	    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), FALSE);
	} else {
		gtk_widget_show(prev_event_box);
		gtk_widget_show(next_event_box);
		gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), TRUE);
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
		} else {
		    gtk_widget_set_sensitive(GTK_WIDGET(menuitem_edit_random), TRUE);
		}
		g_strfreev(list);
    }
	update_gui();
	return FALSE;
}

gboolean playlist_motion_callback(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
	GtkTreePath *localpath;
	GtkTreeIter localiter;
	gchar *iterfilename;
	
	gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), event->x, event->y,&localpath, NULL, NULL, NULL);
	
	if (localpath != NULL) {
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(playliststore),&localiter,localpath)) {
			gtk_tree_model_get(GTK_TREE_MODEL(playliststore), &localiter, ITEM_COLUMN,&iterfilename,-1);
			gtk_tooltips_set_tip(playlisttip,widget, iterfilename, NULL);
		}
	}
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
		gtk_list_store_clear(nonrandomplayliststore);
		
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

void playlist_close(GtkWidget * widget, void *data)
{
	GValue value = {0, };

	g_value_init(&value,G_TYPE_BOOLEAN);

	if (idledata->videopresent == FALSE) {
		gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	}
	g_value_set_boolean(&value,TRUE);
	gtk_container_child_set_property(GTK_CONTAINER(pane),plvbox,"shrink",&value);
	gtk_widget_hide_all(plvbox);
	
}

void menuitem_view_playlist_callback(GtkMenuItem * menuitem, void *data) {

	GtkWidget *close;
	GtkWidget *list;
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
	GtkTreePath *path;
	GtkAccelGroup *accel_group;
	GValue value = {0, };
	GtkTooltips *tooltip;
	GtkRequisition plreq;	
	GtkRequisition winreq;	
	
	//if (GTK_IS_TREE_SELECTION(selection)){
	//	return;
	//}
	g_value_init(&value,G_TYPE_BOOLEAN);
	
	if (GTK_IS_WIDGET(plvbox)) {
		if (GTK_WIDGET_VISIBLE(plvbox)) {
			if (idledata->videopresent == FALSE) {
				gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
				g_value_set_boolean(&value,TRUE);
				gtk_container_child_set_property(GTK_CONTAINER(pane),plvbox,"shrink",&value);
				gtk_widget_hide_all(plvbox);
				gtk_widget_hide(GTK_WIDGET(fixed));
			} else {
				gtk_widget_size_request(GTK_WIDGET(plvbox), &plreq);
				gtk_widget_size_request(GTK_WIDGET(window), &winreq);
				gtk_widget_hide_all(plvbox);
				//gtk_window_resize(GTK_WINDOW(window),winreq.width - plreq.width,winreq.height - plreq.height);
			}
		} else {
			g_value_set_boolean(&value,FALSE);
			gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
			gtk_container_child_set_property(GTK_CONTAINER(pane),plvbox,"shrink",&value);
			if (idledata->videopresent) {
				gtk_widget_size_request(GTK_WIDGET(plvbox), &plreq);
				gtk_widget_size_request(GTK_WIDGET(window), &winreq);
				//gtk_window_resize(GTK_WINDOW(window),winreq.width + plreq.width,winreq.height + plreq.height);
			}			
			gtk_widget_show_all(plvbox);
			if (idledata->videopresent == FALSE)
				gtk_widget_show(GTK_WIDGET(fixed));

		}
		
	} else {
		gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

	    plvbox = gtk_vbox_new(FALSE, 12);
	    hbox = gtk_hbox_new(FALSE, 12);
		gtk_box_set_homogeneous (GTK_BOX(hbox), FALSE);
		box = gtk_hbox_new(FALSE,10);
		ctrlbox = gtk_hbox_new(FALSE,0);
		closebox = gtk_hbox_new(FALSE,0);
		
	    //gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	    //gtk_button_box_set_layout(GTK_BUTTON_BOX(ctrlbox),GTK_BUTTONBOX_START);
	
		list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(playliststore));
		gtk_widget_set_size_request(GTK_WIDGET(list),-1,-1);
		
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
		if(gtk_list_store_iter_is_valid(playliststore,&iter)) {
			path = gtk_tree_model_get_path(GTK_TREE_MODEL(playliststore),&iter);
			gtk_tree_selection_select_path(selection,path);	
			gtk_tree_path_free(path);
		}

		g_signal_connect(G_OBJECT(list), "row_activated", G_CALLBACK(playlist_select_callback),
	                     NULL);
		g_signal_connect(G_OBJECT(list), "motion-notify-event",G_CALLBACK(playlist_motion_callback), NULL);		
		
		playlisttip = gtk_tooltips_new();
		
		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Items to Play"),
	                                                   renderer,
	                                                   "text", DESCRIPTION_COLUMN,
	                                                   NULL);
		gtk_tree_view_column_set_expand(column, TRUE);
		gtk_tree_view_column_set_max_width(column,40);
		gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes ("",
	                                                   renderer,
	                                                   "text", COUNT_COLUMN,
	                                                   NULL);
		gtk_tree_view_column_set_expand(column, FALSE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

		
	    close = gtk_button_new();
	    tooltip = gtk_tooltips_new();
	    gtk_tooltips_set_tip(tooltip, close, _("Close Playlist View"), NULL);
		gtk_container_add(GTK_CONTAINER(close),gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
		
	    g_signal_connect_swapped(GTK_OBJECT(close), "clicked",
	                             GTK_SIGNAL_FUNC(playlist_close), NULL);


		loadlist = gtk_button_new();
	    tooltip = gtk_tooltips_new();
	    gtk_tooltips_set_tip(tooltip, loadlist, _("Open Playlist"), NULL);
		gtk_container_add(GTK_CONTAINER(loadlist),gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
	    gtk_box_pack_start(GTK_BOX(ctrlbox), loadlist,FALSE,FALSE,0);
		g_signal_connect(GTK_OBJECT(loadlist),"clicked",GTK_SIGNAL_FUNC(load_playlist),NULL);

		savelist = gtk_button_new();
	    tooltip = gtk_tooltips_new();
	    gtk_tooltips_set_tip(tooltip, savelist, _("Save Playlist"), NULL);
		gtk_container_add(GTK_CONTAINER(savelist),gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU));
	    gtk_box_pack_start(GTK_BOX(ctrlbox), savelist,FALSE,FALSE,0);
		g_signal_connect(GTK_OBJECT(savelist),"clicked",GTK_SIGNAL_FUNC(save_playlist),NULL);

		add = gtk_button_new();
	    tooltip = gtk_tooltips_new();
	    gtk_tooltips_set_tip(tooltip, add, _("Add Item to Playlist"), NULL);
		gtk_button_set_image(GTK_BUTTON(add),gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	    gtk_box_pack_start(GTK_BOX(ctrlbox), add,FALSE,FALSE,0);
		g_signal_connect(GTK_OBJECT(add),"clicked",GTK_SIGNAL_FUNC(add_to_playlist),NULL);

		remove = gtk_button_new();
	    tooltip = gtk_tooltips_new();
	    gtk_tooltips_set_tip(tooltip, remove, _("Remove Item from Playlist"), NULL);
		gtk_button_set_image(GTK_BUTTON(remove),gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
	    gtk_box_pack_start(GTK_BOX(ctrlbox), remove,FALSE,FALSE,0);
		g_signal_connect(GTK_OBJECT(remove),"clicked",GTK_SIGNAL_FUNC(remove_from_playlist),list);
		
		accel_group = gtk_accel_group_new();
	    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
	    gtk_widget_add_accelerator(GTK_WIDGET(remove), "clicked",
	                               accel_group, GDK_Delete, 0, GTK_ACCEL_VISIBLE);
		
		GTK_WIDGET_SET_FLAGS(close, GTK_CAN_DEFAULT);
	    gtk_box_pack_end(GTK_BOX(closebox), close, FALSE, FALSE, 0);

		scrolled = gtk_scrolled_window_new(NULL,NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
	      								GTK_POLICY_NEVER,
	      								GTK_POLICY_AUTOMATIC);	
		gtk_container_add(GTK_CONTAINER(scrolled),list);
		
		gtk_box_pack_start(GTK_BOX(plvbox),scrolled,TRUE,TRUE,0);
		gtk_box_pack_start(GTK_BOX(hbox),ctrlbox,FALSE,FALSE,0);
		gtk_box_pack_end(GTK_BOX(hbox),closebox,FALSE,FALSE,0);
	    gtk_box_pack_start(GTK_BOX(plvbox),hbox,FALSE,FALSE,0);
		
		
	    //gtk_container_add(GTK_CONTAINER(vbox), plvbox);
		gtk_paned_pack2(GTK_PANED(pane),plvbox,FALSE,FALSE);
	    //gtk_container_add(GTK_CONTAINER(playlist_window), vbox);
			
		gtk_widget_show_all(plvbox);
		gtk_widget_show(GTK_WIDGET(fixed));
		
		// gtk_widget_show_all(playlist_window);
		gtk_widget_grab_default(close);
		//gdk_window_get_frame_extents(window->window,&rect);
		//gtk_window_move(GTK_WINDOW(playlist_window),rect.x + rect.width, rect.y);
	}	
}
