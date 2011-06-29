/* gmpc-jamendo (GMPC plugin)
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
#include <glade/glade.h>
#include <plugin.h>
#include <gmpc_easy_download.h>
#include <metadata.h>
#include <gmpc-mpddata-model.h>
#include <gmpc-mpddata-treeview.h>
#include <gmpc-extras.h>
#include <sqlite3.h>
#include "jamendo.h"
#include <libmpd/debug_printf.h>

extern GladeXML *pl3_xml;

gmpcPlugin plugin;
/**
 * Function decleration
 */
static void jamendo_selected(GtkWidget *container);
static void jamendo_unselected(GtkWidget *container);
static void jamendo_add(GtkWidget *cat_tree);
static void jamendo_download_xml_callback(int download, int total,gpointer data);
static void jamendo_download();
static void   jamendo_mpd_status_changed(MpdObj *mi, ChangedStatusType what, void *data);
static int jamendo_fetch_cover_priority(void);
static void jamendo_fetch_get_image(mpd_Song *song,MetaDataType type, void (*callback)(GList *list, gpointer data), gpointer user_data);
static int jamendo_cat_menu_popup(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event);

static int jamendo_button_release_event(GtkWidget *tree, GdkEventButton *event,gpointer data);
static int jamendo_button_handle_release_event_tag(GtkWidget *tree, GdkEventButton *event,gpointer data);

static int jamendo_key_press(GtkWidget *tree, GdkEventKey *event);
static void jamendo_show_album_list(GtkTreeSelection *selection, gpointer user_data);
static void jamendo_show_song_list(GtkTreeSelection *selection, gpointer user_data);
static int jamendo_button_handle_release_event_tag_add(GtkWidget *button, gpointer user_data);
static void jamendo_save_myself(void);

static void jamendo_download_cancel(GtkWidget *button);
static GtkTreeRowReference *jamendo_ref = NULL;


static int downloading = FALSE; 
/**
 * Get set enabled 
 */
static int jamendo_get_enabled()
{
	return cfg_get_single_value_as_int_with_default(config, "jamendo", "enable", TRUE);
}
static void jamendo_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "jamendo", "enable", enabled);
	if (enabled)
	{
		if(jamendo_ref == NULL)
		{
			jamendo_add(GTK_WIDGET(playlist3_get_category_tree_view()));
		}
	}
	else if (jamendo_ref)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(jamendo_ref);
		if (path){
			GtkTreeIter iter;
            jamendo_save_myself();
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter, path)){
				gtk_list_store_remove(playlist3_get_category_tree_store(), &iter);
			}
			gtk_tree_path_free(path);
			gtk_tree_row_reference_free(jamendo_ref);
			jamendo_ref = NULL;
		}                                                                                                  	
	}                                                                                                      	
	pl3_update_go_menu();
}





/* Playlist window row reference */

GtkWidget *jamendo_pb = NULL, *jamendo_cancel = NULL;
GtkWidget *jamendo_vbox = NULL;
GtkTreeModel *mt_store = NULL;
GtkWidget *treeviews[3] = {NULL, NULL,NULL};

static GtkWidget *jamendo_logo=NULL;
extern GladeXML *pl3_xml;

static void jamendo_buy_album()
{
	gchar *uri;
	mpd_Song *song = NULL;
	gchar *artist;
	gchar *album;

	if(mpd_check_connected(connection)) 
	{
		song = mpd_playlist_get_current_song(connection);
        if(song && song->album && song->artist)
        {
            uri = jamendo_get_album_url(song->artist, song->album);
            open_uri(uri);
            g_free(uri);
        }
	}
}

static void jamendo_logo_add()
{
	mpd_Song *song = NULL; 
	GtkWidget *logo, *button;
	GtkWidget *ali;
    if(mpd_check_connected(connection))
        song = mpd_playlist_get_current_song(connection);

	jamendo_logo = gtk_hbox_new(FALSE,6);
	
	button = gtk_button_new_with_label("Buy this album\nfrom jamendo");
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

	ali = gtk_alignment_new(0,0.5,0,1);
	gtk_container_add(GTK_CONTAINER(ali), button);
	logo = gtk_image_new_from_icon_name("jamendo", GTK_ICON_SIZE_DND);
	gtk_button_set_image(GTK_BUTTON(button), logo);
	gtk_box_pack_start(GTK_BOX(jamendo_logo), ali, TRUE, TRUE,0);
	gtk_box_pack_end(GTK_BOX(glade_xml_get_widget(pl3_xml, "vbox5")), jamendo_logo, FALSE,FALSE,0);	
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(jamendo_buy_album), NULL);
	if(song) {
		if(strstr(song->file,"jamendo.com"))
		{
			gtk_widget_show_all(jamendo_logo);
			return;
		}
	}


}

static void jamendo_logo_init()
{
    gchar *path = gmpc_plugin_get_data_path(&plugin);
    gchar *url = g_build_path(G_DIR_SEPARATOR_S,path, "jamendo", NULL);
    debug_printf(DEBUG_WARNING,"Found url: %s\n", url);
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default (),url);
    g_free(url);
    g_free(path);

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	gtk_init_add((GtkFunction )jamendo_logo_add, NULL);
	/** 
	 * init the jamendo stuff
	 */
	jamendo_db_init();
	/**
	 * open the db 
	 */
	jamendo_db_open();
}


static void   jamendo_mpd_status_changed(MpdObj *mi, ChangedStatusType what, void *data)
{
	if(!jamendo_logo)
		return;
	if(what&(MPD_CST_SONGID|MPD_CST_STATE))
	{
		mpd_Song *song = mpd_playlist_get_current_song(mi);
		if(song && mpd_player_get_state(mi) == MPD_STATUS_STATE_PLAY) {
			if(strstr(song->file,"jamendo.com"))
			{
				gtk_widget_show_all(jamendo_logo);
				return;
			}
		}
		gtk_widget_hide(jamendo_logo);
	}
}

void jamendo_get_genre_list()
{
    MpdData *data = NULL;
	data = jamendo_db_get_genre_list();
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[0]))),data);
}

static void jamendo_show_album_list(GtkTreeSelection *selection, gpointer user_data)
{
    MpdData *data = NULL;
    GtkTreeIter piter;
    GtkTreeModel *model  = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[0]));
    if(gtk_tree_selection_get_selected(selection, &model, &piter))
    {
        gchar *genre;
        gtk_tree_model_get(model, &piter, MPDDATA_MODEL_COL_SONG_TITLE,&genre,-1);
        data= jamendo_db_get_artist_list(genre);
        
        g_free(genre);
    }
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[1]))),data);
}

static void jamendo_show_song_list(GtkTreeSelection *selection, gpointer user_data)
{
    MpdData *data = NULL;
    GtkTreeIter piter;
    GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[0]));
    GtkTreeModel *model  = gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[0]));
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
    data = jamendo_db_get_song_list(genre, artist, album,TRUE);
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(mt_store), data);
}

static void jamendo_show_artist_list(GtkTreeSelection *selection, gpointer user_data)
{
    GtkTreeIter piter;
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
            data = jamendo_db_get_album_list(genre ,artist);
            gmpc_mpddata_model_set_request_artist(GMPC_MPDDATA_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[2]))),artist);

            GmpcMpdDataTreeviewTooltip *tool = gtk_widget_get_tooltip_window(GTK_WIDGET(treeviews[2]));
            if(tool->request_artist){
                g_free(tool->request_artist);
            }
            tool->request_artist = g_strdup(artist);
            g_free(artist);
        }
        g_free(genre);
    }
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[2]))),data);
}

static void jamendo_add_album_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data)
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

static gboolean jamendo_search_equal_func(GtkTreeModel *model, int column, const gchar *key, GtkTreeIter *iter, gpointer data)
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

static void jamendo_init()
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
	jamendo_vbox = gtk_hpaned_new();
	gmpc_paned_size_group_add_paned(paned_size_group, GTK_PANED(jamendo_vbox));
   

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
    g_signal_connect(G_OBJECT(treeviews[0]), "button-press-event", G_CALLBACK(jamendo_button_handle_release_event_tag), GINT_TO_POINTER(0));
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

    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[0]))), "changed", G_CALLBACK(jamendo_show_album_list), NULL);

    /* Artist list */
    model = (GtkTreeModel *) gmpc_mpddata_model_new();
    sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    treeviews[1] =  gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeviews[1]), TRUE);
    gmpc_mpd_data_treeview_tooltip_new(GTK_TREE_VIEW(treeviews[1]), META_ARTIST_ART);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeviews[1]), MPDDATA_MODEL_COL_SONG_TITLE);
	g_signal_connect(G_OBJECT(treeviews[1]), "button-press-event", G_CALLBACK(jamendo_button_handle_release_event_tag), GINT_TO_POINTER(1));
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

    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[1]))), "changed", G_CALLBACK(jamendo_show_artist_list), NULL);

    /* Album list */
    model = (GtkTreeModel *) gmpc_mpddata_model_new();
    sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    treeviews[2] =  gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeviews[2]), TRUE);
    gmpc_mpd_data_treeview_tooltip_new(GTK_TREE_VIEW(treeviews[2]), META_ALBUM_ART);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeviews[2]), MPDDATA_MODEL_COL_SONG_TITLE);

	g_signal_connect(G_OBJECT(treeviews[2]), "button-press-event", G_CALLBACK(jamendo_button_handle_release_event_tag), GINT_TO_POINTER(2));
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


    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[0]))), "changed", G_CALLBACK(jamendo_show_song_list), NULL);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[1]))), "changed", G_CALLBACK(jamendo_show_song_list), NULL);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviews[2]))), "changed", G_CALLBACK(jamendo_show_song_list), NULL);

    gtk_paned_add1(GTK_PANED(jamendo_vbox), paned);
    gtk_widget_show_all(paned);
    /**
	 * tree
	 */
    sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
 

	tree = (GtkWidget *)gmpc_mpddata_treeview_new("jamendo",TRUE,GTK_TREE_MODEL(mt_store));
	g_signal_connect(G_OBJECT(tree), "row-activated", G_CALLBACK(jamendo_add_album_row_activated), NULL);

	g_signal_connect(G_OBJECT(tree), "button-press-event", G_CALLBACK(jamendo_button_release_event), tree);
	g_signal_connect(G_OBJECT(tree), "key-press-event", G_CALLBACK(jamendo_key_press), NULL);

    gtk_container_add(GTK_CONTAINER(sw), tree);
    gtk_box_pack_start(GTK_BOX(vbox),sw, TRUE, TRUE,0);	


	gtk_widget_show_all(sw);
    gtk_widget_show(vbox);
	/**
	 * Progress Bar for the bottom 
	 */
	//jamendo_pb = gtk_progress_bar_new();
	//gtk_box_pack_end(GTK_BOX(vbox), jamendo_pb, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 6);
    jamendo_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(G_OBJECT(jamendo_cancel), "clicked", G_CALLBACK(jamendo_download_cancel), NULL);
	jamendo_pb = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(hbox), jamendo_pb, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), jamendo_cancel, FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);


    gtk_paned_add2(GTK_PANED(jamendo_vbox), vbox);
	g_object_ref(jamendo_vbox);


}
static void jamendo_selected(GtkWidget *container)
{
	if(jamendo_vbox == NULL)
	{
		jamendo_init();
		gtk_container_add(GTK_CONTAINER(container), jamendo_vbox);
		gtk_widget_show(jamendo_vbox);
		if(!jamendo_db_has_data())
		{
			jamendo_download();
		}
        else
            jamendo_get_genre_list();
	} else {
		gtk_container_add(GTK_CONTAINER(container), jamendo_vbox);
		gtk_widget_show(jamendo_vbox);
	}
}
static void jamendo_unselected(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), jamendo_vbox);
}
/*
int jamendo_end_download()
{
    downloading = FALSE;

    jamendo_get_genre_list();
	gtk_widget_hide(jamendo_pb);
	gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "pl3_win"), TRUE);
    return FALSE;
}
typedef struct _Pass{
    GtkWidget *pb;
    int download;
    int total;
}Pass;
*/
//static int jamendo_download_xml_callback_real(Pass *p);
/* Move it to main thread, by pushing in idle */
/*static void jamendo_download_xml_callback(int download, int total,gpointer data)
{
    Pass *p= g_malloc0(sizeof(*p));
    p->pb = data;
    p->download = download;
    p->total =total;
    g_idle_add((GSourceFunc)jamendo_download_xml_callback_real, p);
}

static int jamendo_download_xml_callback_real(Pass *p)
{
	GtkWidget *pb = p->pb;
    int download = p->download;
    int total = p->total;
	gchar *label = NULL;
    gchar *size = NULL;
	if(total > 0)
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pb), download/(float)total);
	else
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pb));

    size = g_format_size_for_display((goffset)download);
    label = g_strdup_printf("Downloading music catalog (%s done)",size);
    g_free(size);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pb), label);
	g_free(label);
    g_free(p);
    return FALSE;
}

static void jamendo_download()
{
    downloading = TRUE; 
    //	gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "pl3_win"), FALSE);
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[0]))),NULL);
	gtk_widget_show(jamendo_pb);
	jamendo_db_download_xml(jamendo_download_xml_callback, jamendo_pb);
//	gtk_widget_hide(jamendo_pb);
//	gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "pl3_win"), TRUE);
}
*/
static void jamendo_download_cancel(GtkWidget *button)
{
    GEADAsyncHandler *handle = g_object_get_data(G_OBJECT(button), "handle");
    if(handle)
    {
        gmpc_easy_async_cancel(handle);
        g_object_set_data(G_OBJECT(button), "handle", NULL);
    }
}
static void jamendo_download_callback(const GEADAsyncHandler *handle, const GEADStatus status, gpointer user_data)
{
    GtkWidget *pb = user_data;
    const gchar *uri = gmpc_easy_handler_get_uri(handle);
    if(status == GEAD_DONE)
    {
        goffset length;
        const char *data = gmpc_easy_handler_get_data(handle, &length);

        jamendo_db_load_data(data, length);
        gtk_widget_hide(gtk_widget_get_parent(pb));
        jamendo_get_genre_list();
        g_object_set_data(G_OBJECT(jamendo_cancel), "handle", NULL);
        downloading = FALSE; 
    }
    else if (status == GEAD_CANCELLED)
    {
        gtk_widget_hide(gtk_widget_get_parent(pb));
        jamendo_get_genre_list();
        g_object_set_data(G_OBJECT(jamendo_cancel), "handle", NULL);
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
static void jamendo_download()
{
    downloading = TRUE; 
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(treeviews[0]))),NULL);
	gtk_widget_show_all(gtk_widget_get_parent(jamendo_pb));
    const char *url = "http://img.jamendo.com/data/dbdump_artistalbumtrack.xml.gz";
    GEADAsyncHandler *handle = gmpc_easy_async_downloader(url, jamendo_download_callback,jamendo_pb);
    g_object_set_data(G_OBJECT(jamendo_cancel), "handle", handle);
}

static void jamendo_add(GtkWidget *cat_tree)
{
	GtkTreePath *path = NULL;
	GtkTreeIter iter,child;
	GtkListStore *pl3_tree = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));	
	gint pos = cfg_get_single_value_as_int_with_default(config, "jamendo","position",20);

	if(!cfg_get_single_value_as_int_with_default(config, "jamendo", "enable", TRUE)) return;

    debug_printf(DEBUG_INFO,"Adding at position: %i", pos);
	playlist3_insert_browser(&iter, pos);
	gtk_list_store_set(GTK_LIST_STORE(pl3_tree), &iter, 
			PL3_CAT_TYPE, plugin.id,
			PL3_CAT_TITLE, _("Jamendo Browser"),
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "jamendo",
			-1);
	/**
	 * Clean up old row reference if it exists
	 */
	if (jamendo_ref)
	{
		gtk_tree_row_reference_free(jamendo_ref);
		jamendo_ref = NULL;
	}
	/**
	 * create row reference
	 */
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter);
	if (path)
	{
		jamendo_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist3_get_category_tree_store()), path);
		gtk_tree_path_free(path);
	}

}

/*** COVER STUFF */
static int jamendo_fetch_cover_priority(void){
	return cfg_get_single_value_as_int_with_default(config, "jamendo", "priority", 80);
}
static void jamendo_fetch_cover_priority_set(int priority){
	cfg_set_single_value_as_int(config, "jamendo", "priority", priority);
}

static void jamendo_fetch_get_image(mpd_Song *song,MetaDataType type, void (*callback)(GList *list, gpointer data), gpointer user_data)
{
    int result = 0;
    if(type == META_ARTIST_ART && song->artist)
    {
        char *value = jamendo_get_artist_image(song->artist);
        if(value)
        {
            GList *list =NULL;
            MetaData *mtd = meta_data_new();
            mtd->type = type;
            mtd->plugin_name = plugin.name;
            mtd->content_type = META_DATA_CONTENT_URI;
            mtd->content = value;
            mtd->size =  -1;
            list = g_list_append(list, mtd);
            callback(list, user_data);
            return;
        }
    }
    else if(type == META_ALBUM_ART && song->artist && song->album)
    {

        char *value = jamendo_get_album_image(song->artist,song->album);

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
    return; 
}	

static void jamendo_redownload_reload_db()
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_row_reference_get_model(jamendo_ref);
	GtkTreePath *path = gtk_tree_row_reference_get_path(jamendo_ref);
	if(path && gtk_tree_model_get_iter(model, &iter, path))
	{
		GtkTreeIter citer;
		while(gtk_tree_model_iter_children(model, &citer,&iter))
		{
			gtk_list_store_remove(GTK_LIST_STORE(model), &citer);
		}
		jamendo_download();
		//jamendo_get_genre_list();
	}
	if(path)
		gtk_tree_path_free(path);
}
static int jamendo_cat_menu_popup(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event)
{
	GtkWidget *item;
	if(type != plugin.id) return 0;
	else if (!downloading)
    {
        /* add the clear widget */
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(jamendo_redownload_reload_db), NULL);
        return 1;
    }
	return 0;
}

static void jamendo_add_selected(GtkWidget *button, GtkTreeView *tree)
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
static void jamendo_replace_selected(GtkWidget *button , GtkTreeView *tree)
{
	mpd_playlist_clear(connection);
	jamendo_add_selected(button, tree);
	mpd_player_play(connection);

}

static int jamendo_button_handle_release_event_tag_add(GtkWidget *button, gpointer userdata)
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
    data = jamendo_db_get_song_list(genre,artist,album, TRUE);
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

static int jamendo_button_handle_release_event_tag_replace(GtkWidget *button, gpointer userdata)
{
    mpd_playlist_clear(connection);
    jamendo_button_handle_release_event_tag_add(button,userdata);
    mpd_player_play(connection);

}

static int jamendo_button_handle_release_event_tag(GtkWidget *tree, GdkEventButton *event,gpointer data)
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
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK( jamendo_button_handle_release_event_tag_add), data);
		/* Replace */
		item = gtk_image_menu_item_new_with_label("Replace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK( jamendo_button_handle_release_event_tag_replace), data);	
        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
        return TRUE;
    }
    return FALSE;
}

static int jamendo_button_release_event(GtkWidget *tree, GdkEventButton *event,gpointer data)
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
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(jamendo_add_selected), tree);
		/* Replace */
		item = gtk_image_menu_item_new_with_label("Replace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(jamendo_replace_selected), tree);
		
        /* Separator */	
        item = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        /* Edit columns */
        /*
        item = gtk_image_menu_item_new_with_label(("Edit Columns"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(jamendo_list_edit_columns), tree);
*/
        gmpc_mpddata_treeview_right_mouse_intergration(GMPC_MPDDATA_TREEVIEW(tree), GTK_MENU(menu));

        gtk_widget_show_all(menu);
        
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
        return TRUE;
	}
	return FALSE;
}

static int jamendo_key_press(GtkWidget *tree, GdkEventKey *event)
{
	if(event->state&GDK_CONTROL_MASK && event->keyval == GDK_Insert)
	{
		jamendo_replace_selected(NULL, GTK_TREE_VIEW(tree));
	}
	else if(event->keyval == GDK_Insert)
	{
		jamendo_add_selected(NULL, GTK_TREE_VIEW(tree));
	}
	return FALSE;
}
static void jamendo_save_myself(void)
{
	if (jamendo_ref)
    {
        GtkTreePath *path = gtk_tree_row_reference_get_path(jamendo_ref);
        if(path)
        {
            gint *indices = gtk_tree_path_get_indices(path);
            debug_printf(DEBUG_INFO,"Saving myself to position: %i\n", indices[0]);
            cfg_set_single_value_as_int(config, "jamendo","position",indices[0]);
            gtk_tree_path_free(path);
        }
    }
}

static void jamendo_destroy(void)
{
    jamendo_db_destroy();
    if(jamendo_vbox)
        gtk_widget_destroy(jamendo_vbox);
}
static gboolean jamendo_integrate_search_field_supported(const int search_field)
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
static MpdData * jamendo_integrate_search(const int search_field, const gchar *query,GError **error)
{
    const gchar *genre = NULL, *artist=NULL, *album=NULL;
    if(!jamendo_get_enabled()) return NULL;
    if(!jamendo_db_has_data()){
        g_set_error(error, 0,0, "Music catalog not yet available, please open jamendo browser first");
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
            return jamendo_db_title_search(query);
        default:
            g_set_error(error, 0,0, "This type of search query is not supported");
            return NULL;
            break;
    }
    return jamendo_db_get_song_list(genre, artist, album,FALSE);
}

gmpcMetaDataPlugin jamendo_cover = {
	.get_priority = jamendo_fetch_cover_priority,
    .set_priority = jamendo_fetch_cover_priority_set,
	.get_metadata = jamendo_fetch_get_image
};


/* Needed plugin_wp stuff */
gmpcPlBrowserPlugin jamendo_gbp = {
	.add                    		= jamendo_add,
	.selected               		= jamendo_selected,
	.unselected             		= jamendo_unselected,
	.cat_right_mouse_menu   		= jamendo_cat_menu_popup,
	.integrate_search       		= jamendo_integrate_search,
	.integrate_search_field_supported 	= jamendo_integrate_search_field_supported
};

int plugin_api_version = PLUGIN_API_VERSION;
static const gchar *jamendo_get_translation_domain(void)
{
    return GETTEXT_PACKAGE;
}


gmpcPlugin plugin = {
	.name                       = N_("Jamendo Stream Browser"),
	.version                    = {PLUGIN_MAJOR_VERSION,PLUGIN_MINOR_VERSION,PLUGIN_MICRO_VERSION},
	.plugin_type                = GMPC_PLUGIN_PL_BROWSER|GMPC_PLUGIN_META_DATA,
    /* Creation and destruction */
	.init                       = jamendo_logo_init,
    .destroy                    = jamendo_destroy, 
    .save_yourself              = jamendo_save_myself,
    .mpd_status_changed         = jamendo_mpd_status_changed, 

    /* Browser extention */
	.browser                    = &jamendo_gbp,
	.metadata                   = &jamendo_cover,

    /* get/set enabled  */
	.get_enabled                = jamendo_get_enabled,
	.set_enabled                = jamendo_set_enabled,
    .get_translation_domain     = jamendo_get_translation_domain
};


