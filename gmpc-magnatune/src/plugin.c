/* gmpc-magnatune (GMPC plugin)
 * Copyright (C) 2006-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <plugin.h>
#include <gmpc_easy_download.h>
#include <metadata.h>
#include <gmpc-mpddata-model.h>
#include <gmpc-mpddata-treeview.h>
#include <gmpc-extras.h>
#include <config.h>
#include <playlist3-messages.h>
#include "magnatune.h"
#include <libmpd/debug_printf.h>

extern GtkBuilder *pl3_xml;
gmpcPlugin plugin;
/**
 * Function decleration
 */
static void magnatune_selected(GtkWidget *container);
static void magnatune_unselected(GtkWidget *container);
static void magnatune_add(GtkWidget *cat_tree);
static void magnatune_download_xml_callback(int download, int total,gpointer data);
static void magnatune_download();
static void   magnatune_mpd_status_changed(MpdObj *mi, ChangedStatusType what, void *data);
static int magnatune_fetch_cover_priority(void);
static void magnatune_fetch_get_image(mpd_Song *song,MetaDataType type, void (*callback)(GList *list, gpointer data), gpointer user_data);
static int magnatune_cat_menu_popup(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event);

static int magnatune_button_release_event(GtkWidget *tree, GdkEventButton *event,gpointer data);
static int magnatune_button_handle_release_event_tag(GtkWidget *tree, GdkEventButton *event,gpointer data);

static int magnatune_key_press(GtkWidget *tree, GdkEventKey *event);
static void magnatune_show_album_list(GtkTreeSelection *selection, gpointer user_data);
static void magnatune_show_song_list(GtkTreeSelection *selection, gpointer user_data);
static int magnatune_button_handle_release_event_tag_add(GtkWidget *button, gpointer user_data);
static void magnatune_save_myself(void);
static GtkTreeRowReference *magnatune_ref = NULL;
static void magnatune_download_cancel(GtkWidget *button);

static int downloading = FALSE; 
/**
 * Get set enabled 
 */
static int magnatune_get_enabled()
{
	return cfg_get_single_value_as_int_with_default(config, "magnatune", "enable", TRUE);
}
static void magnatune_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "magnatune", "enable", enabled);
	if (enabled)
	{
		if(magnatune_ref == NULL)
		{
			magnatune_add(GTK_WIDGET(playlist3_get_category_tree_view()));
		}
	}
	else if (magnatune_ref)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(magnatune_ref);
		if (path){
			GtkTreeIter iter;
            magnatune_save_myself();
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter, path)){
				gtk_list_store_remove(playlist3_get_category_tree_store(), &iter);
			}
			gtk_tree_path_free(path);
			gtk_tree_row_reference_free(magnatune_ref);
			magnatune_ref = NULL;
		}                                                                                                  	
	}                                                                                                      	
	pl3_update_go_menu();
}





/* Playlist window row reference */

GtkWidget *magnatune_pb = NULL, *magnatune_cancel = NULL;
GtkWidget *magnatune_vbox = NULL;
GtkTreeModel *mt_store = NULL;

GtkWidget *treeviews[3] = {NULL, NULL,NULL};

static GtkWidget *magnatune_logo=NULL;

static void magnatune_buy_album()
{
	gchar *uri;
	mpd_Song *song = NULL;
	gchar *artist;
	gchar *album;

	if(mpd_check_connected(connection)) 
	{
		song = mpd_playlist_get_current_song(connection);
		artist = __magnatune_process_string(song->artist);
		album = __magnatune_process_string(song->album);
		uri = g_strconcat("http://www.magnatune.com/buy/choose?artist=",artist,"&album=",album,NULL);
		open_uri(uri);
		g_free(artist);
		g_free(album);
		g_free(uri);
	}
}

static void magnatune_logo_add()
{
	mpd_Song *song = NULL; 
	GtkWidget *logo, *button;
	GtkWidget *ali;
    if(mpd_check_connected(connection))
        song = mpd_playlist_get_current_song(connection);

	magnatune_logo = gtk_vbox_new(FALSE,6);
	
	button = gtk_button_new_with_label("Buy this album\nfrom magnatune");
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

	ali = gtk_alignment_new(0,0.5,0,1);
	gtk_container_add(GTK_CONTAINER(ali), button);
	logo = gtk_image_new_from_icon_name("magnatune", GTK_ICON_SIZE_DND);
	gtk_button_set_image(GTK_BUTTON(button), logo);
	gtk_box_pack_start(GTK_BOX(magnatune_logo), ali, TRUE, TRUE,0);
	gtk_box_pack_end(GTK_BOX(gtk_builder_get_object(pl3_xml, "vbox5")), magnatune_logo, FALSE,FALSE,0);	
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(magnatune_buy_album), NULL);
	if(song) {
		if(strstr(song->file,"magnatune.com"))
		{
			gtk_widget_show_all(magnatune_logo);
			return;
		}
	}


}

static void magnatune_logo_init()
{
    gchar *path = gmpc_plugin_get_data_path(&plugin);
    gchar *url = g_build_path(G_DIR_SEPARATOR_S,path, "magnatune", NULL);
    debug_printf(DEBUG_WARNING,"Found url: %s\n", url);
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default (),url);
    g_free(url);
    g_free(path);


	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");


	gtk_init_add((GtkFunction )magnatune_logo_add, NULL);
	/** 
	 * init the magnatune stuff
	 */
	magnatune_db_init();
	/**
	 * open the db 
	 */
	magnatune_db_open();
    {
        gchar *un = cfg_get_single_value_as_string(config,"magnatune", "username"); 
        gchar *up = cfg_get_single_value_as_string(config,"magnatune", "password"); 
        magnatune_set_user_password(un, up);
        g_free(un);
        g_free(up);
    }
}


static void   magnatune_mpd_status_changed(MpdObj *mi, ChangedStatusType what, void *data)
{
	if(!magnatune_logo)
		return;
	if(what&(MPD_CST_SONGID|MPD_CST_STATE))
	{
		mpd_Song *song = mpd_playlist_get_current_song(mi);
		if(song && mpd_player_get_state(mi) == MPD_STATUS_STATE_PLAY) {
			if(strstr(song->file,"magnatune.com"))
			{
				gtk_widget_show_all(magnatune_logo);
				return;
			}
		}
		gtk_widget_hide(magnatune_logo);
	}
}

void magnatune_get_genre_list()
{
    MpdData *data = NULL;
    GTimer *timer;
	data = magnatune_db_get_genre_list();

    timer = g_timer_new();
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[0]))),data);
    g_debug("%f seconds elapsed filling genre tree", g_timer_elapsed(timer,NULL)); g_timer_destroy(timer);
}

static void magnatune_show_album_list(GtkTreeSelection *selection, gpointer user_data)
{
    MpdData *data = NULL;
    GtkTreeIter piter;
    GTimer *timer;
    GtkTreeModel *model  = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[0]));
    if(gtk_tree_selection_get_selected(selection, &model, &piter))
    {
        gchar *genre;
        gtk_tree_model_get(model, &piter, MPDDATA_MODEL_COL_SONG_TITLE,&genre,-1);
        data= magnatune_db_get_artist_list(genre);
        
        g_free(genre);
    }
    timer = g_timer_new();
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[1]))),data);
    g_debug("%f seconds elapsed filling artist tree", g_timer_elapsed(timer,NULL)); g_timer_destroy(timer);
}

static void magnatune_show_song_list(GtkTreeSelection *selection, gpointer user_data)
{
    MpdData *data = NULL;
    GtkTreeIter piter;
    GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[0]));
    GtkTreeModel *model  = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[0]));
    GTimer *timer;
    gchar *genre = NULL, *artist = NULL, *album = NULL;
    if(gtk_tree_selection_get_selected(select, &model, &piter))
    {

        select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[1]));
        gtk_tree_model_get(model, &piter, MPDDATA_MODEL_COL_SONG_TITLE,&genre,-1);
        /* model */
        model  = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[1]));
        if(gtk_tree_selection_get_selected(select, &model, &piter))
        {
            gtk_tree_model_get(model, &piter, MPDDATA_MODEL_COL_SONG_TITLE,&artist,-1);
            select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[2]));
            model  = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[2]));
            if(gtk_tree_selection_get_selected(select, &model, &piter))
            {
                gtk_tree_model_get(model, &piter, MPDDATA_MODEL_COL_SONG_TITLE,&album,-1);
            }
        }
    }
    data = magnatune_db_get_song_list(genre, artist, album,TRUE);
    timer = g_timer_new();
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(mt_store), data);
    g_debug("%f seconds elapsed filling song tree", g_timer_elapsed(timer,NULL)); g_timer_destroy(timer);
}

static void magnatune_show_artist_list(GtkTreeSelection *selection, gpointer user_data)
{
    GtkTreeIter piter;
    GTimer *timer;
    MpdData *data = NULL;
    GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[0]));
    GtkTreeModel *model  = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[0]));

    if(gtk_tree_selection_get_selected(select, &model, &piter))
    {
        GList *iter,*list = NULL;
        gchar *genre;

        select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[1]));
        gtk_tree_model_get(model, &piter, MPDDATA_MODEL_COL_SONG_TITLE,&genre,-1);
        /* model */
        model  = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[1]));
        if(gtk_tree_selection_get_selected(select, &model, &piter))
        {
            GList *iter,*list = NULL;

            gchar *artist;
            gtk_tree_model_get(model, &piter, MPDDATA_MODEL_COL_SONG_TITLE,&artist,-1);
            data = magnatune_db_get_album_list(genre ,artist);
            gmpc_mpddata_model_set_request_artist(GMPC_MPDDATA_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[2]))),artist);
            GmpcMpdDataTreeviewTooltip *tool = (GmpcMpdDataTreeviewTooltip *)gtk_widget_get_tooltip_window(GTK_WIDGET(treeviews[2]));
            if(tool->request_artist){
                g_free(tool->request_artist);
            }
            tool->request_artist = g_strdup(artist);

            g_free(artist);
        }
        g_free(genre);
    }
    timer = g_timer_new();
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[2]))),data);
    g_debug("%f seconds elapsed filling album tree", g_timer_elapsed(timer,NULL)); g_timer_destroy(timer);
}

static void magnatune_add_album_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data)
{
    GtkTreeModel *model = gtk_tree_view_get_model(tree);
    GtkTreeIter iter_r;

    if(gtk_tree_model_get_iter(model, &iter_r, path))
    {
        gchar *path;
        gtk_tree_model_get(model, &iter_r, MPDDATA_MODEL_COL_PATH, &path, -1);
        play_path(path);
        g_free(path);
    }
}

static gboolean magnatune_search_equal_func(GtkTreeModel *model, int column, const gchar *key, GtkTreeIter *iter, gpointer data)
{
	gchar *compare = NULL;
	gtk_tree_model_get(model,iter, column, &compare, -1);
	if(compare){
		gchar *a = g_utf8_casefold(key, -1);
		gchar *b = g_utf8_casefold(compare, -1);
		if(strstr(b,a))
		{
			g_free(a);
			g_free(b);
			return FALSE;
		}
		g_free(a);
		g_free(b);
	}
	return TRUE;
}

static void magnatune_init()
{
    int size;
	GtkWidget *tree             = NULL;
	GtkWidget *sw               = NULL;
	GtkCellRenderer *renderer   = NULL;
    GtkWidget *paned            = NULL;
    GtkWidget *hbox             = NULL;
    GtkWidget *vbox             = NULL;
    GtkTreeModel *model         = NULL;
    GtkTreeViewColumn *column   = NULL;
	magnatune_vbox = gtk_hpaned_new();
	gmpc_paned_size_group_add_paned(GMPC_PANED_SIZE_GROUP(paned_size_group), GTK_PANED(magnatune_vbox));
   

    vbox = gtk_vbox_new(FALSE,6);
	/**
	 * Create list store
	 * 1. pointer 
	 * 2. name
	 */
	mt_store =  (GtkTreeModel *)gmpc_mpddata_model_new(); 

    /* Paned window */
    paned = gtk_vbox_new(TRUE,6);
    model = (GtkTreeModel *) gmpc_mpddata_model_new();
    /* Genre list */
    sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    treeviews[0] = gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeviews[0]), TRUE);

    gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeviews[0]), MPDDATA_MODEL_COL_SONG_TITLE);
    g_signal_connect(G_OBJECT(treeviews[0]), "button-press-event", G_CALLBACK(magnatune_button_handle_release_event_tag), GINT_TO_POINTER(0));
    /* Add column */
    column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "Genre");


	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_add_attribute(column, renderer, "icon-name", MPDDATA_MODEL_COL_ICON_ID);
    gtk_tree_view_column_set_sizing(column , GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", MPDDATA_MODEL_COL_SONG_TITLE);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(treeviews[0]),column, -1);

    gtk_container_add(GTK_CONTAINER(sw), treeviews[0]);
    gtk_box_pack_start(GTK_BOX(paned), sw, TRUE, TRUE, 0); 

    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[0]))), "changed", G_CALLBACK(magnatune_show_album_list), NULL);

    /* Artist list */
    model = (GtkTreeModel *) gmpc_mpddata_model_new();
    sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    treeviews[1] =  gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeviews[1]), TRUE);
    gmpc_mpd_data_treeview_tooltip_new(GTK_TREE_VIEW(treeviews[1]), META_ARTIST_ART);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeviews[1]), MPDDATA_MODEL_COL_SONG_TITLE);
	g_signal_connect(G_OBJECT(treeviews[1]), "button-press-event", G_CALLBACK(magnatune_button_handle_release_event_tag), GINT_TO_POINTER(1));
    /* Add column */
    column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "Artist");
    size = cfg_get_single_value_as_int_with_default(config, "gmpc-mpddata-model", "icon-size", 64);
    gtk_tree_view_column_set_sizing(column , GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(treeviews[1]), TRUE);


    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_cell_renderer_set_fixed_size(renderer, size,size);
   
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", MPDDATA_MODEL_META_DATA);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", MPDDATA_MODEL_COL_SONG_TITLE);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(treeviews[1]),column, -1);

    gtk_container_add(GTK_CONTAINER(sw), treeviews[1]);
    gtk_box_pack_start(GTK_BOX(paned), sw, TRUE, TRUE, 0); 

    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[1]))), "changed", G_CALLBACK(magnatune_show_artist_list), NULL);

    /* Album list */
    model = (GtkTreeModel *) gmpc_mpddata_model_new();
    sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    treeviews[2] =  gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeviews[2]), TRUE);
    gmpc_mpd_data_treeview_tooltip_new(GTK_TREE_VIEW(treeviews[2]), META_ALBUM_ART);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeviews[2]), MPDDATA_MODEL_COL_SONG_TITLE);

	g_signal_connect(G_OBJECT(treeviews[2]), "button-press-event", G_CALLBACK(magnatune_button_handle_release_event_tag), GINT_TO_POINTER(2));
    /* Add column */
    column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "Album");
    size = cfg_get_single_value_as_int_with_default(config, "gmpc-mpddata-model", "icon-size", 64);
    gtk_tree_view_column_set_sizing(column , GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(treeviews[2]), TRUE);


	renderer = gtk_cell_renderer_pixbuf_new();
    gtk_cell_renderer_set_fixed_size(renderer, size,size);

	gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", MPDDATA_MODEL_META_DATA);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", MPDDATA_MODEL_COL_SONG_TITLE);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(treeviews[2]),column, -1);

    gtk_container_add(GTK_CONTAINER(sw), treeviews[2]);
    gtk_box_pack_start(GTK_BOX(paned), sw, TRUE, TRUE, 0); 


    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[0]))), "changed", G_CALLBACK(magnatune_show_song_list), NULL);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[1]))), "changed", G_CALLBACK(magnatune_show_song_list), NULL);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[2]))), "changed", G_CALLBACK(magnatune_show_song_list), NULL);

    gtk_paned_add1(GTK_PANED(magnatune_vbox), paned);
    gtk_widget_show_all(paned);
    /**
	 * tree
	 */
    sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
 

	tree = (GtkWidget *)gmpc_mpddata_treeview_new("magnatune",TRUE,GTK_TREE_MODEL(mt_store));
	g_signal_connect(G_OBJECT(tree), "row-activated", G_CALLBACK(magnatune_add_album_row_activated), NULL);

	g_signal_connect(G_OBJECT(tree), "button-press-event", G_CALLBACK(magnatune_button_release_event), tree);
	g_signal_connect(G_OBJECT(tree), "key-press-event", G_CALLBACK(magnatune_key_press), NULL);

    gtk_container_add(GTK_CONTAINER(sw), tree);
    gtk_box_pack_start(GTK_BOX(vbox),sw, TRUE, TRUE,0);	


	gtk_widget_show_all(sw);
    gtk_widget_show(vbox);
	/**
	 * Progress Bar for the bottom 
	 */
    hbox = gtk_hbox_new(FALSE, 6);
    magnatune_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(G_OBJECT(magnatune_cancel), "clicked", G_CALLBACK(magnatune_download_cancel), NULL);
	magnatune_pb = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(hbox), magnatune_pb, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), magnatune_cancel, FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
    gtk_paned_add2(GTK_PANED(magnatune_vbox), vbox);


	g_object_ref(magnatune_vbox);


}
static void magnatune_selected(GtkWidget *container)
{
	if(magnatune_vbox == NULL)
	{
		magnatune_init();
		gtk_container_add(GTK_CONTAINER(container), magnatune_vbox);
		gtk_widget_show(magnatune_vbox);
		if(!magnatune_db_has_data())
		{
			magnatune_download();
		}
        else
            magnatune_get_genre_list();
	} else {
		gtk_container_add(GTK_CONTAINER(container), magnatune_vbox);
		gtk_widget_show(magnatune_vbox);
	}
}
static void magnatune_unselected(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), magnatune_vbox);
}
static void magnatune_download_cancel(GtkWidget *button)
{
    GEADAsyncHandler *handle = g_object_get_data(G_OBJECT(button), "handle");
    if(handle)
    {
        gmpc_easy_async_cancel(handle);
        g_object_set_data(G_OBJECT(button), "handle", NULL);
    }
}
static void magnatune_download_callback(const GEADAsyncHandler *handle, const GEADStatus status, gpointer user_data)
{
    GtkWidget *pb = user_data;
    const gchar *uri = gmpc_easy_handler_get_uri(handle);
    if(status == GEAD_DONE)
    {
        goffset length;
        const char *data = gmpc_easy_handler_get_data(handle, &length);

        magnatune_db_load_data(data, length);
        if(length <= 0 || data == NULL) {
            playlist3_show_error_message("Failed to download magnatune db: size is 0.",ERROR_WARNING);
        }
        gtk_widget_hide(gtk_widget_get_parent(pb));
        magnatune_get_genre_list();
        downloading = FALSE; 
    }
    else if (status == GEAD_CANCELLED)
    {
        gtk_widget_hide(gtk_widget_get_parent(pb));
        magnatune_get_genre_list();
        downloading = FALSE; 
    }
    else if (status == GEAD_PROGRESS)
    {
        goffset length;
        goffset total = gmpc_easy_handler_get_content_size(handle);
        /* Get downloaded length */
        gmpc_easy_handler_get_data(handle, &length);
        if(total > 0)
        {
            double fraction = (100*length)/(total);
            gchar *size = g_format_size_for_display((goffset)length);
            gchar *strtotal = g_format_size_for_display((goffset)total);
            gchar *label = g_strdup_printf("Downloading music catalog (%s of %s done)",size,strtotal);
            g_free(strtotal);
            g_free(size);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pb), label);
            g_free(label);
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pb),fraction/100.0);
        }
        else 
        {
            gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pb));
        }
    }
}
static void magnatune_download()
{
    downloading = TRUE; 
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[0]))),NULL);
	gtk_widget_show_all(gtk_widget_get_parent(magnatune_pb));
    const char *url = "http://he3.magnatune.com/info/sqlite_magnatune.db";
    GEADAsyncHandler *handle = gmpc_easy_async_downloader(url, magnatune_download_callback,magnatune_pb);
    g_object_set_data(G_OBJECT(magnatune_cancel), "handle", handle);
}

static void magnatune_add(GtkWidget *cat_tree)
{
	GtkTreePath *path = NULL;
	GtkTreeIter iter,child;
	GtkListStore *pl3_tree = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));	
	gint pos = cfg_get_single_value_as_int_with_default(config, "magnatune","position",20);

	if(!cfg_get_single_value_as_int_with_default(config, "magnatune", "enable", TRUE)) return;

    debug_printf(DEBUG_INFO,"Adding at position: %i", pos);
	playlist3_insert_browser(&iter, pos);
	gtk_list_store_set(GTK_LIST_STORE(pl3_tree), &iter, 
			PL3_CAT_TYPE, plugin.id,
			PL3_CAT_TITLE, _("Magnatune Browser"),
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "magnatune",
            -1);
    /**
	 * Clean up old row reference if it exists
	 */
	if (magnatune_ref)
	{
		gtk_tree_row_reference_free(magnatune_ref);
		magnatune_ref = NULL;
	}
	/**
	 * create row reference
	 */
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter);
	if (path)
	{
		magnatune_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist3_get_category_tree_store()), path);
		gtk_tree_path_free(path);
	}

}
char * __magnatune_process_string(const char *name)
{
	int i = 0;
	int j = 0;
	int depth = 0;
	int spaces = 0;

	/* only gonna be smaller */
	char *result = g_malloc0((strlen(name)+1+spaces*2)*sizeof(char));
	for(i=0; i < strlen(name);i++)
	{
		if(name[i] == '(' || name[i] == '[') depth++;
		else if (name[i] == ')' || name[i] == ']') depth--;
		else if(!depth)
		{
			result[j] = name[i];
			j++;
		}
	}
	for(i=j-1;i>0 && result[i] == ' ';i--)
	{
		result[i] = '\0';
	}
	return result;
}

/*** COVER STUFF */
static int magnatune_fetch_cover_priority(void){
	return cfg_get_single_value_as_int_with_default(config, "magnatune", "priority", 80);
}
static void magnatune_fetch_cover_priority_set(int priority){
	cfg_set_single_value_as_int(config, "magnatune", "priority", priority);
}


static void magnatune_fetch_get_image(mpd_Song *song,MetaDataType type, void (*callback)(GList *list, gpointer data), gpointer user_data)
{
	if(type == META_ARTIST_ART && song->artist)
	{
        char *value = magnatune_get_artist_image(song->artist);
		if(value)
		{
            GList *list = NULL;
            MetaData *mtd = meta_data_new();
            mtd->type = type;
            mtd->plugin_name = plugin.name;
            mtd->content_type = META_DATA_CONTENT_URI;
            mtd->content = value;
            mtd->size = -1;
            list = g_list_append(list, mtd);
            callback(list, user_data);
            return;
		}
	}
    else if(type == META_ALBUM_ART && song->artist && song->album)
	{
        char *value = magnatune_get_album_image(song->artist,song->album);
		if(value)
		{
            GList *list = NULL;
            MetaData *mtd = meta_data_new();
            mtd->type = type;
            mtd->plugin_name = plugin.name;
            mtd->content_type = META_DATA_CONTENT_URI;
            mtd->content = value;
            mtd->size = -1;
            list = g_list_append(list, mtd);
            callback(list, user_data);
            return;
		}
	}
    callback(NULL, user_data);
	return ; 
}	

static void magnatune_redownload_reload_db()
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_row_reference_get_model(magnatune_ref);
	GtkTreePath *path = gtk_tree_row_reference_get_path(magnatune_ref);
	if(path && gtk_tree_model_get_iter(model, &iter, path))
	{
		GtkTreeIter citer;
		while(gtk_tree_model_iter_children(model, &citer,&iter))
		{
			gtk_list_store_remove(GTK_LIST_STORE(model), &citer);
		}
		magnatune_download();
	}
	if(path)
		gtk_tree_path_free(path);
}
static int magnatune_cat_menu_popup(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event)
{
	GtkWidget *item;
	if(type != plugin.id) return 0;
	else if (!downloading)
    {
        /* add the clear widget */
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(magnatune_redownload_reload_db), NULL);
        return 1;
    }
	return 0;
}

static void magnatune_add_selected(GtkWidget *button, GtkTreeView *tree)
{
	GtkTreeModel *model = GTK_TREE_MODEL(mt_store);
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	GList *list = gtk_tree_selection_get_selected_rows(sel,&model);
	if(list)
	{
		GList *node ;
		for(node = list; node; node = g_list_next(node))
		{
			GtkTreeIter iter;
			if(gtk_tree_model_get_iter(model, &iter, node->data))
			{
                gchar *path;
				gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_PATH, &path, -1);
                mpd_playlist_queue_add(connection, path);
                g_free(path);
            }
		}
		mpd_playlist_queue_commit(connection);
		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (list);
	}
}
static void magnatune_replace_selected(GtkWidget *button , GtkTreeView *tree)
{
	mpd_playlist_clear(connection);
	magnatune_add_selected(button, tree);
	mpd_player_play(connection);

}

static int magnatune_button_handle_release_event_tag_add(GtkWidget *button, gpointer userdata)
{
    GList *list;
    MpdData *data;
    int position = GPOINTER_TO_INT(userdata);
    gchar *genre=NULL, *artist=NULL, *album=NULL;
    GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[0]));
    GtkTreeModel *model  = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[0]));
    GtkTreeIter piter;
    if(gtk_tree_selection_get_selected(select, &model, &piter))
    {
        gtk_tree_model_get(model, &piter, MPDDATA_MODEL_COL_SONG_TITLE,&genre,-1);
    }
    if(position >= 1)
    {
        select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[1]));
        model  = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[1]));
        if(gtk_tree_selection_get_selected(select, &model, &piter))
        {
            gtk_tree_model_get(model, &piter, MPDDATA_MODEL_COL_SONG_TITLE,&artist,-1);
        }
    }
    if(position >= 2)
    {
        select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[2]));
        model  = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[2]));
        if(gtk_tree_selection_get_selected(select, &model, &piter))
        {
            gtk_tree_model_get(model, &piter, MPDDATA_MODEL_COL_SONG_TITLE,&album,-1);
        }
    }

    data = magnatune_db_get_song_list(genre,artist,album, TRUE);
    for(data = mpd_data_get_first(data);data;data = mpd_data_get_next(data)) {
        mpd_playlist_queue_add(connection, data->song->file);
    }
    mpd_playlist_queue_commit(connection);
    if(genre)
        g_free(genre);
    if(artist)
        g_free(artist);
    if(album)
        g_free(album);

}

static int magnatune_button_handle_release_event_tag_replace(GtkWidget *button, gpointer userdata)
{
    mpd_playlist_clear(connection);
    magnatune_button_handle_release_event_tag_add(button,userdata);
    mpd_player_play(connection);

}

static int magnatune_button_handle_release_event_tag(GtkWidget *tree, GdkEventButton *event,gpointer data)
{
    int i;
    int position = GPOINTER_TO_INT(data);
	if(event->button != 3) return FALSE;
	GtkWidget *item;

	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	if(gtk_tree_selection_count_selected_rows(sel)> 0)
	{
		GtkWidget *menu = gtk_menu_new();


        /* Add */
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK( magnatune_button_handle_release_event_tag_add), data);
		/* Replace */
		item = gtk_image_menu_item_new_with_label("Replace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK( magnatune_button_handle_release_event_tag_replace), data);	
        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
        return TRUE;
    }
    return FALSE;
}

static int magnatune_button_release_event(GtkWidget *tree, GdkEventButton *event,gpointer data)
{
	if(event->button != 3) return FALSE;
	GtkWidget *item;

	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

	if(gtk_tree_selection_count_selected_rows(sel)> 0)
	{
		GtkWidget *menu = gtk_menu_new();
		GtkTreeModel *model = GTK_TREE_MODEL(mt_store);
		/* Add */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(magnatune_add_selected), tree);
		/* Replace */
		item = gtk_image_menu_item_new_with_label("Replace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(magnatune_replace_selected), tree);
		
        gmpc_mpddata_treeview_right_mouse_intergration(GMPC_MPDDATA_TREEVIEW(tree), GTK_MENU(menu));

        gtk_widget_show_all(menu);
        
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
        return TRUE;
	}
	return FALSE;
}
static int magnatune_key_press(GtkWidget *tree, GdkEventKey *event)
{
	if(event->state&GDK_CONTROL_MASK && event->keyval == GDK_Insert)
	{
		magnatune_replace_selected(NULL, GTK_TREE_VIEW(tree));
	}
	else if(event->keyval == GDK_Insert)
	{
		magnatune_add_selected(NULL, GTK_TREE_VIEW(tree));
	}
	return FALSE;
}
static void magnatune_save_myself(void)
{
	if (magnatune_ref)
    {
        GtkTreePath *path = gtk_tree_row_reference_get_path(magnatune_ref);
        if(path)
        {
            gint *indices = gtk_tree_path_get_indices(path);
            debug_printf(DEBUG_INFO,"Saving myself to position: %i\n", indices[0]);
            cfg_set_single_value_as_int(config, "magnatune","position",indices[0]);
            gtk_tree_path_free(path);
        }
    }
}

static void magnatune_destroy(void)
{
    magnatune_db_destroy();
    if(magnatune_vbox)
        gtk_widget_destroy(magnatune_vbox);
}

static gboolean magnatune_integrate_search_field_supported(const int search_field)
{
	switch(search_field)
	{
		case MPD_TAG_ITEM_ARTIST:
		case MPD_TAG_ITEM_ALBUM:
		case MPD_TAG_ITEM_GENRE:
		case MPD_TAG_ITEM_TITLE:
			return TRUE;
		default:
			return FALSE;
	}
}
static MpdData * magnatune_integrate_search(const int search_field, const gchar *query,GError **error)
{
    const gchar *genre = NULL, *artist=NULL, *album=NULL;
    if(!magnatune_get_enabled()) return NULL;
    if(!magnatune_db_has_data()){
        g_set_error(error, 0,0, "Music catalog not yet available, please open magnatune browser first");
        return NULL;
    }
    switch(search_field){
        case MPD_TAG_ITEM_ARTIST:
            artist = query;
            break;
        case MPD_TAG_ITEM_ALBUM:
            album = query;
            break;
        case MPD_TAG_ITEM_GENRE:
            genre = query;
            break;
        case MPD_TAG_ITEM_TITLE:
            return magnatune_db_search_title(query);
            break;
        default:
            g_set_error(error, 0,0, "This type of search query is not supported");
            return NULL;
            break;
    }
    return magnatune_db_get_song_list(genre, artist, album,FALSE);
}

gmpcMetaDataPlugin magnatune_cover = {
	.get_priority = magnatune_fetch_cover_priority,
    .set_priority = magnatune_fetch_cover_priority_set,
	.get_metadata = magnatune_fetch_get_image
};


/* Needed plugin_wp stuff */
gmpcPlBrowserPlugin magnatune_gbp = {
	.add                    		= magnatune_add,
	.selected               		= magnatune_selected,
	.unselected             		= magnatune_unselected,
	.cat_right_mouse_menu   		= magnatune_cat_menu_popup,
	.integrate_search       		= magnatune_integrate_search,
	.integrate_search_field_supported 	= magnatune_integrate_search_field_supported
};

int plugin_api_version = PLUGIN_API_VERSION;

static const gchar *magnatune_get_translation_domain(void)
{
    return GETTEXT_PACKAGE;
}

static void magnatune_uentry_changed(GtkEntry *entry)
{
    const gchar *text = gtk_entry_get_text(entry);
    cfg_set_single_value_as_string(config,"magnatune", "username", text); 
}
static void magnatune_pentry_changed(GtkEntry *entry)
{
    const gchar *text = gtk_entry_get_text(entry);
    cfg_set_single_value_as_string(config,"magnatune", "password", text); 
}


void magnatune_pref_construct (GtkWidget *container)
{
    GtkWidget *table;
    GtkWidget *label, *uentry, *pentry;
    gchar *un = cfg_get_single_value_as_string(config,"magnatune", "username"); 
    gchar *up = cfg_get_single_value_as_string(config,"magnatune", "password"); 

    table = gtk_table_new(3, 2, FALSE);

    /* Password */
    label = gtk_label_new(_("Username"));
    gtk_table_attach(GTK_TABLE(table), label, 0,1, 0,1, GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0, 0);
    uentry = gtk_entry_new();
    if(un) gtk_entry_set_text(GTK_ENTRY(uentry), un);
    g_signal_connect(G_OBJECT(uentry), "changed", G_CALLBACK(magnatune_uentry_changed), NULL);
    gtk_table_attach(GTK_TABLE(table), uentry, 1,2, 0,1,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0, 0);


    label = gtk_label_new(_("Password"));
    gtk_table_attach(GTK_TABLE(table), label, 0,1, 1,2,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0, 0);

    pentry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pentry), FALSE);
    if(up) gtk_entry_set_text(GTK_ENTRY(pentry), up);
    g_signal_connect(G_OBJECT(pentry), "changed", G_CALLBACK(magnatune_pentry_changed), NULL);
    gtk_table_attach(GTK_TABLE(table), pentry, 1,2, 1,2, GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0, 0);

    g_free(un);
    g_free(up);
    gtk_container_add(GTK_CONTAINER(container), table);
    gtk_widget_show_all(container);
}
void magnatune_pref_destroy (GtkWidget *container)
{
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(container));
    if(child) gtk_widget_destroy(child);
}

gmpcPrefPlugin magnatune_gpp = {
    magnatune_pref_construct,
    magnatune_pref_destroy
};
gmpcPlugin plugin = {
	.name                       = N_("Magnatune Stream Browser"),
	.version                    = {PLUGIN_MAJOR_VERSION,PLUGIN_MINOR_VERSION,PLUGIN_MICRO_VERSION},
	.plugin_type                = GMPC_PLUGIN_PL_BROWSER|GMPC_PLUGIN_META_DATA,
    /* Creation and destruction */
	.init                       = magnatune_logo_init,
    .destroy                    = magnatune_destroy, 
    .save_yourself              = magnatune_save_myself,
    .mpd_status_changed         = magnatune_mpd_status_changed, 

    /* Browser extention */
	.browser                    = &magnatune_gbp,
	.metadata                   = &magnatune_cover,

    .pref                       = &magnatune_gpp,
    /* get/set enabled  */
	.get_enabled                = magnatune_get_enabled,
	.set_enabled                = magnatune_set_enabled,
    

    .get_translation_domain = magnatune_get_translation_domain
};
