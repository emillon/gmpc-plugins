/* gmpc-tagedit (GMPC plugin)
 * Copyright (C) 2008-2009 Qball Cow <qball@sarine.nl>
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

#include <string.h>
#include <unistd.h>
#include <config.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>
#include <libmpd/libmpd.h>
#include <gmpc/plugin.h>
#include <gmpc/playlist3-messages.h>
#include <tag_c.h>
#include "gmpc-mpddata-model-tagedit.h"
#include "tagedit.h"
#include <libmpd/debug_printf.h>

static void __save_myself(void);
/* The plugin structure */
gmpcPlugin plugin;


static GtkTreeRowReference *te_ref = NULL;
static GtkWidget *browser_box = NULL;
static GtkWidget  *browser_tree = NULL;
static GtkTreeModel *browser_model = NULL;
static GtkWidget *entries[7];
static gulong signal_entries[7];

static inline GQuark
tagedit_quark(void)
{
	return g_quark_from_static_string("tagedit_plugin");
}


/**
 * Taglib functions
 */

static mpd_Song * get_song_from_file(const char *root, const char *filename, GError **error)
{
    TagLib_File *file = NULL;
    TagLib_Tag *tag;
    mpd_Song *song = NULL;
    gchar *url = g_build_path(G_DIR_SEPARATOR_S, root, filename,NULL);
    if(!g_file_test(url, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_REGULAR)){
        g_set_error(error, tagedit_quark(), 0, "%s: '%s'", _("File does not exists"), url);
        g_free(url);
        return NULL;
    }
    if(g_access(url, R_OK|W_OK) != 0) {
        g_set_error(error, tagedit_quark(), 0, "%s: '%s'", _("File is read-only"), url);
        g_free(url);
        return NULL;
    }
    file = taglib_file_new(url);
    if(file && taglib_file_is_valid(file))
    {
        char *a = NULL;
        song = mpd_newSong();
        song->file = g_strdup(filename);
        tag = taglib_file_tag(file);
        if(tag)
        {
            if((a = taglib_tag_title(tag)) && a[0] != '\0')
                song->title = g_strdup(a);
            if((a = taglib_tag_album(tag)) && a[0] != '\0')
                song->album = g_strdup(a); 
            if((a = taglib_tag_artist(tag)) && a[0] != '\0')
                song->artist = g_strdup(a); 
            if(taglib_tag_track(tag) > 0)
                song->track = g_strdup_printf("%i", taglib_tag_track(tag)); 
            if((a = taglib_tag_genre(tag)) && a[0] != '\0')
                song->genre =  g_strdup(a);
            if((a = taglib_tag_comment(tag)) && a[0] != '\0')
                song->comment =  g_strdup(a);
            if(taglib_tag_year(tag) > 0)
                song->date =  g_strdup_printf("%i", taglib_tag_year(tag));
        }
  
        taglib_tag_free_strings();
    }
    if(file)
        taglib_file_free(file);
    g_free(url);
    return song;
}
static gboolean __timeout_mpd_update(gchar *url)
{
    printf("update: %s\n",url);
    mpd_database_update_dir(connection, url);

    return FALSE;
}

static void save_song_to_file(const char *root, const mpd_Song *song)
{
    TagLib_File *file;
    TagLib_Tag *tag;

    gchar *url = g_build_path(G_DIR_SEPARATOR_S, root, song->file,NULL);
    file = taglib_file_new(url);
    if(file)
    {
        tag = taglib_file_tag(file);
        if(song->title) taglib_tag_set_title(tag, song->title);
        if(song->artist) taglib_tag_set_artist(tag, song->artist);
        if(song->album) taglib_tag_set_album(tag, song->album);
        if(song->genre) taglib_tag_set_genre(tag, song->genre);
        if(song->comment) taglib_tag_set_comment(tag, song->comment);
        if(song->track){
            guint track = (guint) g_ascii_strtoll(song->track, NULL, 10); 
            taglib_tag_set_track(tag, track);
        }
        if(song->date){
            guint year = (guint) g_ascii_strtoll(song->date, NULL, 10); 
            taglib_tag_set_year(tag, year);
        }
        if(!taglib_file_save(file)){
            gchar *errorstr = g_strdup_printf("%s: %s '%s'", _("Tag Edit"), _("Failed to save song"), url);
            playlist3_show_error_message(errorstr, DEBUG_ERROR);
            g_free(errorstr);
        }else {
            g_timeout_add_seconds_full(G_PRIORITY_DEFAULT ,1, (GSourceFunc)__timeout_mpd_update, g_strdup(song->file), g_free);
        }
        taglib_tag_free_strings();
        taglib_file_free(file);
    }

    g_free(url);
}


/*
 */
/**
 * Browser View
 */


static void free_si(gpointer data)
{
    si *i = data;
    printf("free si\n");
    if(i->revert) mpd_freeSong(i->revert);
    g_free(i);
}
static void queue_selected_songs_for_edit(GtkMenuItem *item, GmpcMpdDataTreeview *tree)
{
    const char *root = connection_get_music_directory();
    MpdData *data = NULL;
    GList *selected = NULL;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    if(!browser_model)
        browser_model = (GtkTreeModel *)gmpc_mpddata_model_tagedit_new();

    selected = gtk_tree_selection_get_selected_rows(selection, &model);
    data = gmpc_mpddata_model_steal_mpd_data(GMPC_MPDDATA_MODEL(browser_model));
    while(data && !mpd_data_is_last(data))data = mpd_data_get_next_real(data, FALSE);
    if(selected && root )
    {
        GList *iter = g_list_first(selected);
        for(;iter; iter = g_list_next(iter))
        {
            GtkTreeIter titer;
            
            if(gtk_tree_model_get_iter(model, &titer, iter->data))
            {
                mpd_Song *song = NULL;
                gtk_tree_model_get(model, &titer, MPDDATA_MODEL_COL_MPDSONG, &song, -1);
                if(song  && song->file)
                {
                    GError *error= NULL;
                    mpd_Song *edited = get_song_from_file(root, song->file, &error);
                    printf("adding: %s\n", song->file);
                    if(edited)
                    {
                        si *i = g_malloc0(sizeof(*i));
                        data = mpd_new_data_struct_append(data);
                        data->type = MPD_DATA_TYPE_SONG;
                        data->song = edited;
                        i->changed = 0;
                        i->revert = mpd_songDup(data->song);
                        data->userdata =i;
                        data->freefunc = free_si;
                    }
                    else
                    {
                        gchar *errorstr = NULL;
                        if(error)
                        {
                            errorstr = g_strdup_printf("%s: %s", _("Tag Edit"), error->message);
                            g_error_free(error);
                        }else{
                           errorstr = g_strdup_printf("%s: '%s'",_("TagLib failed to open"), song->file);
                        }

                        playlist3_show_error_message(errorstr, ERROR_CRITICAL);
                        g_free(errorstr);
                        error = NULL;
                    }
                }
            }
        }
        g_list_foreach (selected, (GFunc)gtk_tree_path_free, NULL);
        g_list_free (selected);
    }
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(browser_model),mpd_data_get_first(data));
}

static int __song_list_option_menu ( GmpcMpdDataTreeview *tree, GtkMenu *menu)
{
    int retv = 0;
    GtkWidget *item;
	const char *entry_str = connection_get_music_directory();
    if(plugin.get_enabled() && 
            gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)))
            && entry_str && strlen(entry_str) > 0)
    {
        item = gtk_image_menu_item_new_with_label("Queue songs for tag edit");
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
                gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(queue_selected_songs_for_edit), tree);
        retv++;
    }
    return retv;
}
static void browser_selection_changed(GtkTreeSelection *selec, gpointer data)
{
    GList *selected = gtk_tree_selection_get_selected_rows(selec, &browser_model);
    int i =0;
    for(i=0;i< 6;i++)
    {
        if(signal_entries[i])
            g_signal_handler_block(G_OBJECT(entries[i]), signal_entries[i]);
        if(i<4) gtk_entry_set_text(GTK_ENTRY(entries[i]), "");
        else gtk_spin_button_set_value(GTK_SPIN_BUTTON(entries[i]), 0.0);
    }
    if(selected)
    {
        GList *iter = g_list_first(selected);
        for(;iter; iter = g_list_next(iter))
        {
            GtkTreeIter titer;
            if(gtk_tree_model_get_iter(browser_model, &titer, iter->data))
            {
                mpd_Song *song = NULL;
                gtk_tree_model_get(browser_model, &titer, MPDDATA_MODEL_COL_MPDSONG, &song, -1);
                if(song)
                {
                    if(song->title)
                    {
                        if(strlen(gtk_entry_get_text(GTK_ENTRY(entries[0]))) == 0) 
                            gtk_entry_set_text(GTK_ENTRY(entries[0]), song->title);
                    }
                    if(song->artist)
                    {
                        if(strlen(gtk_entry_get_text(GTK_ENTRY(entries[1]))) == 0) 
                            gtk_entry_set_text(GTK_ENTRY(entries[1]), song->artist);
                    }
                    if(song->album)
                    {
                        if(strlen(gtk_entry_get_text(GTK_ENTRY(entries[2]))) == 0) 
                            gtk_entry_set_text(GTK_ENTRY(entries[2]), song->album);
                    }
                    if(song->genre)
                    {
                        if(strlen(gtk_entry_get_text(GTK_ENTRY(entries[3]))) == 0) 
                            gtk_entry_set_text(GTK_ENTRY(entries[3]), song->genre);
                    }
                    if(song->date)
                    {
                        guint year = (guint) g_ascii_strtoll(song->date, NULL, 10);
                        if(year && gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entries[4])) == 0)
                            gtk_spin_button_set_value(GTK_SPIN_BUTTON(entries[4]), (gdouble) year);
                    }
                    if(song->track)
                    {
                        guint track = (guint) g_ascii_strtoll(song->track, NULL, 10);
                        if(track && gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entries[5])) == 0)
                            gtk_spin_button_set_value(GTK_SPIN_BUTTON(entries[5]), (gdouble) track);
                    }
                }
            }
        }

        g_list_foreach (selected, (GFunc)gtk_tree_path_free, NULL);
        g_list_free (selected);
    }
    for(i=0;i<6;i++)
    {
        if(signal_entries[i])
            g_signal_handler_unblock(G_OBJECT(entries[i]), signal_entries[i]);
    }
}
static void __revert_selected(GtkWidget *button, gpointer data)
{
    GtkTreeSelection *selec = gtk_tree_view_get_selection(GTK_TREE_VIEW(browser_tree));
    GList *selected = gtk_tree_selection_get_selected_rows(selec, &browser_model);
     if(selected)
    {
        GList *iter = g_list_first(selected);
        for(;iter; iter = g_list_next(iter))
        {
            GtkTreeIter titer;
            if(gtk_tree_model_get_iter(browser_model, &titer, iter->data))
            {
                si *ud = NULL;
                mpd_Song *song = NULL;
                gtk_tree_model_get(browser_model, &titer, MPDDATA_MODEL_COL_MPDSONG, &song, MPDDATA_MODEL_USERDATA, &ud,-1);
                gmpc_mpddata_model_tagedit_revert_song(browser_model, &titer);
            }
        }

        g_list_foreach (selected, (GFunc)gtk_tree_path_free, NULL);
        g_list_free (selected);

        browser_selection_changed(selec, NULL);
    }
}


static void __field_changed(GtkWidget *entry, gpointer data)
{
    int id = GPOINTER_TO_INT(data);

    const gchar *text = NULL;
    gint  value = 0;
    GList *selected = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(browser_tree)), &browser_model);
    if(id < 4)
    {
        text = gtk_entry_get_text(GTK_ENTRY(entry));
    }
    else if (id < 6)
    {
        value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry));
    }
    if(selected)
    {
        GList *iter = g_list_first(selected);
        for(;iter; iter = g_list_next(iter))
        {
            GtkTreeIter titer;
            if(gtk_tree_model_get_iter(browser_model, &titer, iter->data))
            {
                mpd_Song *song = NULL;
                si *ud = NULL;
                gtk_tree_model_get(browser_model, &titer, MPDDATA_MODEL_COL_MPDSONG, &song,MPDDATA_MODEL_USERDATA, &ud, -1);
                if(song)
                {
                   if(id == 0) {
                        if(song->title == NULL || strcmp(song->title, text) != 0)
                        {
                            if(song->title) g_free(song->title);
                            song->title = g_strdup(text);
                            gtk_tree_model_row_changed(browser_model, (GtkTreePath *)iter->data, &titer);
                            if(ud->revert->title && strcmp(text, ud->revert->title) == 0)
                                ud->changed &= ~1;
                            else
                                ud->changed |= 1;
                        }
                   }
                   else if(id == 1) {
                       if(song->artist == NULL || strcmp(song->artist, text) != 0)
                       {
                           if(song->artist) g_free(song->artist);
                           song->artist = g_strdup(text);
                           gtk_tree_model_row_changed(browser_model, (GtkTreePath *)iter->data, &titer);
                            if(ud->revert->artist && strcmp(text, ud->revert->artist) == 0)
                                ud->changed &= ~(1<<1);
                            else
                                ud->changed |= (1<<1);
                       }
                   }
                   else if(id == 2) {
                       if(song->album == NULL || strcmp(song->album, text) != 0)
                       {
                           if(song->album) g_free(song->album);
                           song->album = g_strdup(text);
                           gtk_tree_model_row_changed(browser_model, (GtkTreePath *)iter->data, &titer);
                           if(ud->revert->album && strcmp(text, ud->revert->album) == 0)
                               ud->changed &= ~(1<<2);
                           else
                               ud->changed |= (1<<2);
                       }
                   }
                   else if(id == 3) {
                       if(song->genre == NULL || strcmp(song->genre, text) != 0)
                       {
                           if(song->genre) g_free(song->genre);
                           song->genre = g_strdup(text);
                           if(ud->revert->genre && strcmp(text, ud->revert->genre) == 0)
                               ud->changed &= ~(1<<3);
                           else
                               ud->changed |= (1<<3);
                           gtk_tree_model_row_changed(browser_model, (GtkTreePath *)iter->data, &titer);
                       }
                   }
                   else if (id == 4) {
                        guint val = 0;
                        if(song->date)
                            val = (guint)g_ascii_strtoll(song->date, NULL, 10);
                        if(val != value)
                        {
                            if(song->date ) g_free(song->date );
                            if(value > 0)
                                song->date = g_strdup_printf("%i", value);
                            else 
                                song->date = NULL;


                            if(ud->revert->date == NULL && song->date == NULL) ud->changed &= ~(1<<4);
                            else if(ud->revert->date && song->date && strcmp(song->date, ud->revert->date) == 0)
                                ud->changed &= ~(1<<4);
                            else
                                ud->changed |= (1<<4);
                            gtk_tree_model_row_changed(browser_model, (GtkTreePath *)iter->data, &titer);
                        }
                   }
                   else if (id == 5) {
                        guint val = 0;
                        if(song->track)
                            val = (guint)g_ascii_strtoll(song->track, NULL, 10);
                        if(val != value)
                        {
                            if(song->track) g_free(song->track);
                            if(value > 0)
                                song->track = g_strdup_printf("%i", value);
                            else 
                                song->track = NULL;
                            if(song->track == NULL && ud->revert->track == NULL)
                                ud->changed &= ~(1<<5);
                            else if(ud->revert->track && song->track && strcmp(song->track, ud->revert->track) == 0)
                                ud->changed &= ~(1<<5);
                            else
                                ud->changed |= (1<<5);
                            gtk_tree_model_row_changed(browser_model, (GtkTreePath *)iter->data, &titer);
                        }
                   }
                }

                printf("changed: %i-%i\n", id, ud->changed);
            }
        }
        g_list_foreach (selected, (GFunc)gtk_tree_path_free, NULL);
        g_list_free (selected);
    }

}
static void save_all(GtkWidget *button, gpointer data)
{
    const char *root = connection_get_music_directory();
    GtkTreeIter iter;
    if(root && gtk_tree_model_get_iter_first(browser_model, &iter))
    {
        do{
            mpd_Song *song = NULL;
            si *ud = NULL;
            gtk_tree_model_get(browser_model, &iter, MPDDATA_MODEL_COL_MPDSONG, &song, MPDDATA_MODEL_USERDATA, &ud,-1);
            if(song)
            {
                /* check if song changed */
                if(ud->changed > 0) 
                {
                    GtkTreePath *path = gtk_tree_model_get_path(browser_model, &iter);
                    printf("saving: %s\n", song->file);
                    save_song_to_file(root, song); 
                    ud->changed = 0;
                    
                    gtk_tree_model_row_changed(browser_model, path, &iter);
                    gtk_tree_path_free(path);
                }
            }
        }while(gtk_tree_model_iter_next(browser_model, &iter));
    }
}
static void clear_all(GtkWidget *button, gpointer data)
{
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(browser_model), NULL);

}
static void __edit_columns(void)
{
  gmpc_mpddata_treeview_edit_columns(GMPC_MPDDATA_TREEVIEW(browser_tree));
}
static int __button_release_event(GtkTreeView *tree, GdkEventButton *event)
{
	if(event->button == 3)
	{
		/* del, crop */
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(browser_tree));
        if(gtk_tree_selection_count_selected_rows(selection) == 1)
        {
                item = gtk_image_menu_item_new_with_label(_("Revert changes"));
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                    gtk_image_new_from_stock(GTK_STOCK_REVERT_TO_SAVED, GTK_ICON_SIZE_MENU));
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                    G_CALLBACK(__revert_selected), NULL);
        }
        item = gtk_image_menu_item_new_with_label(_("Edit Columns"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(__edit_columns), NULL);

        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL,0, event->time);	
		return TRUE;
	}
    return FALSE;
}

static int __key_release_event(GtkWidget *box , GdkEventKey *event, gpointer data)
{
    if(event->keyval == GDK_Page_Up||event->keyval == GDK_Page_Down)
    {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(browser_tree));
        if(gtk_tree_selection_count_selected_rows(selection) == 1)
        {
            GList *list = gtk_tree_selection_get_selected_rows(selection, &browser_model);
            if(list)
            {
                GtkTreePath *path = list->data;
                if(event->keyval == GDK_Page_Up)
                {
                    if(gtk_tree_path_prev(path)){
                        gtk_tree_selection_unselect_all(selection);
                        gtk_tree_selection_select_path(selection, path);
                        }
                }else{
                    gtk_tree_path_next(path);
                    gtk_tree_selection_unselect_all(selection);
                    gtk_tree_selection_select_path(selection, path);
                }
            }
            g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
            g_list_free (list);
        }
        return TRUE;
    }
    return FALSE;
}

static void __browser_init ( )
{
    GtkWidget *hbox = NULL;
    GtkWidget *sw = NULL, *label;
    GtkWidget *table = NULL;
    gchar *temp;
    browser_box = gtk_hpaned_new();//(FALSE, 6);
    if(!browser_model)
        browser_model = (GtkTreeModel *)gmpc_mpddata_model_tagedit_new();
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
    browser_tree = gmpc_mpddata_treeview_new(CONFIG_NAME, TRUE, browser_model);
    gmpc_mpddata_treeview_enable_click_fix(GMPC_MPDDATA_TREEVIEW(browser_tree));
    gtk_container_add(GTK_CONTAINER(sw), browser_tree);
//    gtk_box_pack_start(GTK_BOX(browser_box), sw, TRUE, TRUE,0);

    gtk_paned_add1(GTK_PANED(browser_box), sw);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(browser_tree))), 
                "changed", G_CALLBACK(browser_selection_changed), NULL);

    gtk_paned_set_position(GTK_PANED(browser_box), 
            cfg_get_single_value_as_int_with_default(config, CONFIG_NAME, "pane-pos", 150));
    /** Add all the fill in fields */
    table = gtk_table_new(8,2,FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);
    label = gtk_label_new(_("Tags"));
    temp = g_markup_printf_escaped("<b>%s</b>", _("Tags"));
    gtk_label_set_markup(GTK_LABEL(label), temp); 
    g_free(temp);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0,0.5);
    gtk_table_attach(GTK_TABLE(table),label, 0,2,0,1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    /* title */
    label = gtk_label_new(_("Title"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0,0.5);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,1,2,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    entries[0] = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table),entries[0], 1,2,1,2,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    signal_entries[0] = g_signal_connect(G_OBJECT(entries[0]), "changed", G_CALLBACK(__field_changed), GINT_TO_POINTER(0));
   /* artist */
    label = gtk_label_new(_("Artist"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0,0.5);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,2,3,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    entries[1] = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table),entries[1], 1,2,2,3,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    signal_entries[1] = g_signal_connect(G_OBJECT(entries[1]), "changed", G_CALLBACK(__field_changed), GINT_TO_POINTER(1));
    /* album */
    label = gtk_label_new(_("Album"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0,0.5);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,3,4,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    entries[2] = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table),entries[2], 1,2,3,4,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    signal_entries[2] = g_signal_connect(G_OBJECT(entries[2]), "changed", G_CALLBACK(__field_changed), GINT_TO_POINTER(2));
    /* album */
    label = gtk_label_new(_("Genre"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0,0.5);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,4,5,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    entries[3] = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table),entries[3], 1,2,4,5,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    signal_entries[3] = g_signal_connect(G_OBJECT(entries[3]), "changed", G_CALLBACK(__field_changed), GINT_TO_POINTER(3));
    /* album */
    label = gtk_label_new(_("Year"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0,0.5);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,5,6,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    entries[4] = gtk_spin_button_new_with_range(0, 3000,1);
    gtk_table_attach(GTK_TABLE(table),entries[4], 1,2,5,6,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    signal_entries[4] = g_signal_connect(G_OBJECT(entries[4]), "value-changed", G_CALLBACK(__field_changed), GINT_TO_POINTER(4));
	g_signal_connect(G_OBJECT(entries[4]), "key-press-event", G_CALLBACK(__key_release_event), NULL);
    /* album */
    label = gtk_label_new(_("Track"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0,0.5);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,6,7,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    entries[5] = gtk_spin_button_new_with_range(0, 3600,1);
    gtk_table_attach(GTK_TABLE(table),entries[5], 1,2,6,7,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    signal_entries[5] = g_signal_connect(G_OBJECT(entries[5]), "value-changed", G_CALLBACK(__field_changed), GINT_TO_POINTER(5));

	g_signal_connect(G_OBJECT(entries[5]), "key-press-event", G_CALLBACK(__key_release_event), NULL);


    hbox = gtk_hbox_new(FALSE, 6);
    label = gtk_button_new_from_stock(GTK_STOCK_SAVE);
    g_signal_connect(G_OBJECT(label), "clicked", G_CALLBACK(save_all), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, TRUE,0);
    label = gtk_button_new_with_label("Clear tag queue");
    gtk_button_set_image(GTK_BUTTON(label), gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(G_OBJECT(label), "clicked", G_CALLBACK(clear_all), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, TRUE,0);

    gtk_table_attach(GTK_TABLE(table),hbox, 0,2,7,8,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);

    //gtk_box_pack_start(GTK_BOX(browser_box), table, FALSE, TRUE,0);

    gtk_paned_add2(GTK_PANED(browser_box), table);

	g_signal_connect(G_OBJECT(browser_tree), "button-release-event", G_CALLBACK(__button_release_event), NULL);

	g_signal_connect(G_OBJECT(browser_box), "key-press-event", G_CALLBACK(__key_release_event), NULL);
    gtk_widget_show_all(browser_box);
    g_object_ref(browser_box);

    {
        const gchar *root = connection_get_music_directory(); 
        if((root == NULL || strlen(root) ==0) && browser_box)
            gtk_widget_set_sensitive(browser_box, FALSE);
    }
}
static void __browser_add ( GtkWidget *cat_tree)
{
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	GtkListStore *pl3_tree = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));	
	gint pos = cfg_get_single_value_as_int_with_default(config, CONFIG_NAME,"position",20);

	if(!cfg_get_single_value_as_int_with_default(config, CONFIG_NAME, "enable", TRUE)) return;

    debug_printf(DEBUG_INFO,"Adding at position: %i", pos);
	playlist3_insert_browser(&iter, pos);
	gtk_list_store_set(GTK_LIST_STORE(pl3_tree), &iter, 
			PL3_CAT_TYPE, plugin.id,
			PL3_CAT_TITLE, _("Tag Editor"),
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "gtk-edit",
			-1);
	/**
	 * Clean up old row reference if it exists
	 */
	if (te_ref)
	{
		gtk_tree_row_reference_free(te_ref);
		te_ref = NULL;
	}
	/**
	 * create row reference
	 */
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter);
	if (path)
	{
		te_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist3_get_category_tree_store()), path);
		gtk_tree_path_free(path);
	}
}

static void __browser_selected (GtkWidget *container)
{
    if(browser_box == NULL)
    {
        __browser_init();
    }
    gtk_container_add(GTK_CONTAINER(container), browser_box);
}
static void __browser_unselected ( GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), gtk_bin_get_child(GTK_BIN(container)));
}

static void __destroy()
{
    if(browser_box) {
        g_object_unref(browser_box);
        browser_box = NULL;
    }
    if(browser_model){
        g_object_unref(browser_model);
        browser_model = NULL;
    }
}

/**
 * Get/Set enabled
 */
static int ___get_enabled()
{
	return cfg_get_single_value_as_int_with_default(config, CONFIG_NAME, "enable", TRUE);
}
static void ___set_enabled(int enabled)
{
  cfg_set_single_value_as_int(config, CONFIG_NAME, "enable", enabled);
  if(enabled)
  {
  /* Add the browser to the left pane, if there is none to begin with */
		if(te_ref == NULL)
		{
			__browser_add(GTK_WIDGET(playlist3_get_category_tree_view()));
		}
  }
  else if (te_ref)
  {
      /* Remove it from the left pane */
      GtkTreePath *path = gtk_tree_row_reference_get_path(te_ref);
      if (path){
          GtkTreeIter iter;
          /* for a save of myself */
          __save_myself();
          if (gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter, path)){
              gtk_list_store_remove(playlist3_get_category_tree_store(), &iter);
          }
          gtk_tree_path_free(path);
          gtk_tree_row_reference_free(te_ref);
          te_ref = NULL;
      }                                                                                                  	
  }     
}
static void __save_myself(void)
{
    /* Save the position in the left tree */
	if (te_ref)
    {
        GtkTreePath *path = gtk_tree_row_reference_get_path(te_ref);
        if(path)
        {
            gint *indices = gtk_tree_path_get_indices(path);
            debug_printf(DEBUG_INFO,"Saving myself '%s' to position: %i\n",plugin.name, indices[0]);
            cfg_set_single_value_as_int(config, CONFIG_NAME,"position",indices[0]);
            gtk_tree_path_free(path);
        }
    }
    if(browser_box)
    {
        cfg_set_single_value_as_int(config, CONFIG_NAME, "pane-pos", gtk_paned_get_position(GTK_PANED(browser_box)));
    }
}
static void __init(void)
{
    taglib_id3v2_set_default_text_encoding(TagLib_ID3v2_UTF8);
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
}
static const gchar *__get_translation_domain(void)
{
    return GETTEXT_PACKAGE;
}
int plugin_api_version = PLUGIN_API_VERSION;

gmpcPlBrowserPlugin __browser = {
    /* browser */
    .add                = __browser_add,
    .selected           = __browser_selected,
    .unselected         = __browser_unselected,
    .song_list_option_menu = __song_list_option_menu
};
gmpcPlugin plugin = {
    .name               = N_("Tag Edit"),
	.version            = {PLUGIN_MAJOR_VERSION,PLUGIN_MINOR_VERSION,PLUGIN_MICRO_VERSION},
    .plugin_type        = GMPC_PLUGIN_PL_BROWSER,

    .save_yourself      = __save_myself,
    .init               = __init,
    .destroy            = __destroy,
    .browser            = &__browser,
    .set_enabled        = ___set_enabled,
    .get_enabled        = ___get_enabled,

    .get_translation_domain = __get_translation_domain
};
