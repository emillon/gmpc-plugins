/* gmpc-playlistsort (GMPC plugin)
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

#include <gtk/gtk.h>
#include <gmpc/plugin.h>
#include <glade/glade.h>
#include <libmpd/libmpd-internal.h>
#include <gmpc/misc.h>

void playlistsort_set_enabled(int enabled);
int playlistsort_get_enabled(void);

int playlistsort_tool_menu(GtkWidget *menu);

extern GtkTreeModel *playlist;
/* Allow gmpc to check the version the plugin is compiled against */
int plugin_api_version = PLUGIN_API_VERSION;

/** 
 * Define the plugin structure
 */
gmpcPlugin plugin = {
	/* name */
	.name = "Playlist sorter",
	/* version */
	.version = {0,15,0},
	/* type */
	.plugin_type = GMPC_PLUGIN_NO_GUI,
	/** enable/disable */
	.get_enabled = playlistsort_get_enabled,
	.set_enabled = playlistsort_set_enabled,

    .tool_menu_integration = playlistsort_tool_menu
};

GList *items = NULL;
GtkListStore *tag_list = NULL;
typedef struct{
    GtkWidget *combo;
    GtkWidget *box;
    GtkWidget *button;
}Item;





int playlistsort_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, "playlistsort", "enable", TRUE);
}
void playlistsort_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "playlistsort", "enable", enabled);
	pl3_tool_menu_update(); 
}
int playlistsort_sort(gpointer aa, gpointer bb, gpointer data)
{
    MpdData_real *a = *(MpdData_real **)aa;
    MpdData_real *b = *(MpdData_real **)bb;
    int retv =0;
    int i;
	int *field = ((gint *)(data));
    /** Filter */
	if(!a && !b) return 0;
	if(!a) return -1;
	if(!b) return 1;

    for(i=0;field[i] != -1;i++)
    {
        int do_int = 0;
        char *sa= NULL, *sb = NULL;
        switch(field[i])
        {
            
            case MPD_TAG_ITEM_TITLE:
                sa = a->song->title;
                sb = b->song->title;
                break;
            case MPD_TAG_ITEM_ARTIST:
                sa = a->song->artist;
                sb = b->song->artist;
                break;
            case MPD_TAG_ITEM_ALBUM:
                sa = a->song->album;
                sb = b->song->album;
                break;
            case MPD_TAG_ITEM_GENRE:
                sa = a->song->genre;
                sb = b->song->genre;
                break;
            case MPD_TAG_ITEM_TRACK:

                sa = a->song->track;
                sb = b->song->track;
                do_int = 1;
                break;
            case MPD_TAG_ITEM_NAME:
                sa = a->song->name;
                sb = b->song->name;
                break;
            case MPD_TAG_ITEM_COMPOSER:
                sa = a->song->composer;
                sb = b->song->composer;
                break;
            case MPD_TAG_ITEM_PERFORMER:
                sa = a->song->performer;
                sb = b->song->performer;
                break;
            case MPD_TAG_ITEM_COMMENT:
                sa = a->song->comment;
                sb = b->song->comment;
                break;
            case MPD_TAG_ITEM_DISC:
                sa = a->song->disc;
                sb = b->song->disc;
                break;
            case MPD_TAG_ITEM_DATE:
                sa = a->song->date;
                sb = b->song->date;
                break;
            case MPD_TAG_ITEM_FILENAME:
                sa = a->song->file;
                sb = b->song->file;
                break;
            default:
                g_assert(FALSE);
                return 0;
        }
        if(sa && sb){

            if(do_int == 0)
            {
                gchar *aa,*bb;
                aa = g_utf8_strdown(sa, -1);
                bb = g_utf8_strdown(sb, -1);
                retv = g_utf8_collate(aa,bb);
                g_free(aa);
                g_free(bb);
            }else{
                gint64 aa,bb;
                aa = g_ascii_strtoll(sa, NULL, 10);
                bb = g_ascii_strtoll(sb, NULL, 10);
                retv = (gint)aa-bb;
            }
        }
        else if(sa == sb) retv = 0;
        else if (sa == NULL) retv  = -1;
        else retv = 1;

        if(retv != 0)
        {
            return retv;
        }
    }
    return 0;
}
void playlistsort_start_field()
{
	GList *node, *list=NULL;	
	GtkTreeIter iter; 
	int i;
    int *fields= g_malloc0((g_list_length(items)+1)*sizeof(int));
    GList *a;
    MpdData *data2,*data = mpd_playlist_get_changes(connection, -1);

    i=0;
    for(a = g_list_first(items);a;a = g_list_next(a)){
        Item *ii = a->data;
        fields[i] = gtk_combo_box_get_active(GTK_COMBO_BOX(ii->combo));
        fields[i+1] = -1;
        i++;
    }
    data2 = misc_sort_mpddata(data,(GCompareDataFunc)playlistsort_sort,(gpointer)fields);
   
//	mpd_playlist_clear(connection);
    /*
	for(data2 = mpd_data_get_first(data2); data2;data2 = mpd_data_get_next(data2))
	{
        mpd_playlist_queue_add(connection, data2->song->file);
	}
    */
    i=0;
    for(data = mpd_data_get_first(data); data;data = mpd_data_get_next(data))
    {
        mpd_playlist_move_id(connection, data->song->id, i);
        //mpd_playlist_queue_add(connection, data->song->file);
        i++;
    }
    g_free(fields);
/**/


    mpd_playlist_queue_commit(connection);
}
void fancy_remove(GtkWidget *button, Item *ii)
{
    items = g_list_remove(items, ii);
    gtk_widget_destroy(ii->box);
    g_free(ii);
}
void add_fancy(GtkWidget *button, GtkWidget *box)
{
    GtkCellRenderer *renderer;
    GtkWidget *hbox = NULL;
    Item *ii = NULL;
    ii = g_malloc0(sizeof(Item));
    items = g_list_append(items, ii);
   
    ii->combo = gtk_combo_box_new();
    ii->box = gtk_hbox_new(FALSE,6);

    
	gtk_combo_box_set_model(GTK_COMBO_BOX(ii->combo), GTK_TREE_MODEL(tag_list));
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ii->combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ii->combo), renderer, "text", 1, NULL);

    gtk_combo_box_set_active(GTK_COMBO_BOX(ii->combo), 0);
    gtk_box_pack_start(GTK_BOX(ii->box), ii->combo, TRUE, TRUE, 0);
    ii->button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    g_signal_connect(G_OBJECT(ii->button), "clicked", G_CALLBACK(fancy_remove), ii);
    gtk_box_pack_start(GTK_BOX(ii->box), ii->button,FALSE,  TRUE, 0);
    

	gtk_box_pack_start(GTK_BOX(box), ii->box, TRUE, TRUE, 0);
    gtk_widget_show_all(box);
}
extern GladeXML *pl3_xml;
void playlistsort_start()
{
    Item *ii = NULL;
	GtkWidget *dialog = NULL;
	GtkWidget *hbox, *label,*selector, *box;	
    GtkWidget *button;
	GtkCellRenderer *renderer;
	int i;
	dialog = gtk_dialog_new_with_buttons("Sort Playlist",GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), 
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                GTK_STOCK_CANCEL,
                                GTK_RESPONSE_REJECT,
                                GTK_STOCK_OK,
                                GTK_RESPONSE_ACCEPT,
                                NULL);
	

	hbox = gtk_hbox_new(FALSE,6);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 9);
	/** add label */
	label = gtk_label_new("Sort field:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	/** Add selector */

    ii = g_malloc0(sizeof(Item));

    
    ii->combo = gtk_combo_box_new();
    ii->box = gtk_hbox_new(FALSE,6);
	tag_list = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING);
	gtk_combo_box_set_model(GTK_COMBO_BOX(ii->combo), GTK_TREE_MODEL(tag_list));
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ii->combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ii->combo), renderer, "text", 1, NULL);


	for(i=0;i< MPD_TAG_ITEM_ANY;i++)
	{ 
		GtkTreeIter iter;
		gtk_list_store_append(tag_list, &iter);
		gtk_list_store_set(tag_list, &iter, 0, i, 1, mpdTagItemKeys[i],-1);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(ii->combo), 0);
    gtk_box_pack_start(GTK_BOX(ii->box), ii->combo, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), ii->box, TRUE, TRUE, 0);
    items = g_list_append(items, ii);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 0);
    button = gtk_button_new_from_stock(GTK_STOCK_ADD);
    gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->vbox), button, FALSE, TRUE, 0);
    g_signal_connect(button, "clicked", G_CALLBACK(add_fancy), GTK_DIALOG(dialog)->vbox);

    gtk_widget_show_all(dialog);
	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_ACCEPT:
			playlistsort_start_field();
		default:
			break;
	}
    gtk_list_store_clear(tag_list);
    g_object_unref(tag_list);
    tag_list = NULL;
    g_list_foreach(items, (GFunc)g_free, NULL);
    g_list_free(items);
    items = NULL;

	gtk_widget_destroy(dialog);
}

int playlistsort_tool_menu(GtkWidget *menu)
{
    GtkWidget *item;
    if(!playlistsort_get_enabled()) return;
    item = gtk_image_menu_item_new_with_label("Playlist Sort");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
            gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(playlistsort_start), NULL);
    return 1;
}


