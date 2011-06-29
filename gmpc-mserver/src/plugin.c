/* gmpc-mserver (GMPC plugin)
 * Copyright (C) 2007-2009 Qball Cow <qball@sarine.nl>
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
#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <libmpd/debug_printf.h>
#include <gmpc/plugin.h>
#include <gmpc/gmpc-profiles.h>
#include <gmpc/gmpc-mpddata-model.h>
#include <gmpc/gmpc-mpddata-treeview.h>
#include <gmpc/playlist3-messages.h>
#include <libmpd/libmpd-internal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <microhttpd.h>
#include <config.h>

#include <tag_c.h>

#define URLS_CLASS "Music"

gmpcPlugin plugin;



static struct MHD_Daemon * d = NULL;

static GtkTreeModel *ls = NULL;
static GtkWidget *mserver_vbox = NULL;
static config_obj *cfg_urls = NULL;

static GtkTreeRowReference *mserver_ref = NULL;
void mserver_browser_activated(GtkWidget *tree,GtkTreePath *path);

void mserver_browser_add(GtkWidget *cat_tree)
{
    GtkTreePath *path;
    GtkTreeIter iter;
    gint pos = cfg_get_single_value_as_int_with_default(config, "mserver","position",20);
    GtkListStore *pl3_tree = (GtkListStore  *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));

    playlist3_insert_browser(&iter, pos);
	gtk_list_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, plugin.id,
			PL3_CAT_TITLE, _("Serve music"),
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "gmpc-mserver",
			-1);
    /**
     * Clean up old row reference if it exists
     */
    if (mserver_ref)
    {
        gtk_tree_row_reference_free(mserver_ref);
        mserver_ref = NULL;
    }
    /**
     * create row reference
     */
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter);
    if (path)
    {
        mserver_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist3_get_category_tree_store()), path);
        gtk_tree_path_free(path);
    }

}

static void mserver_save_myself(void)
{
	if (mserver_ref)
    {
        /** 
         * Store the location in the left pane
         */
        GtkTreePath *path = gtk_tree_row_reference_get_path(mserver_ref);
        if(path)
        {
            gint *indices = gtk_tree_path_get_indices(path);
            debug_printf(DEBUG_INFO,"Saving myself to position: %i\n", indices[0]);
            cfg_set_single_value_as_int(config, "mserver","position",indices[0]);
            gtk_tree_path_free(path);
        }
    }
}



/**
 * Get/Set enabled
 */
int mserver_get_enabled()
{
	return cfg_get_single_value_as_int_with_default(config, "mserver", "enable", TRUE);
}
void mserver_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "mserver", "enable", enabled);
	if (enabled)
	{
		if(mserver_ref == NULL)
		{
			mserver_browser_add(GTK_WIDGET(playlist3_get_category_tree_view()));
		}
	}
	else if (mserver_ref)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(mserver_ref);
		if (path){
			GtkTreeIter iter;

            mserver_save_myself();
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter, path)){
				gtk_list_store_remove(playlist3_get_category_tree_store(), &iter);
			}
			gtk_tree_path_free(path);
			gtk_tree_row_reference_free(mserver_ref);
			mserver_ref = NULL;
		}                                                                                                  	
	}                                                                                                      	
	pl3_update_go_menu();
}

/**
 * Web server part
 **/
typedef struct {
	FILE *fp;
	size_t size;
    size_t offset;
}str_block;

static int file_reader(str_block *file, size_t pos, char * buf, int max) 
{
	int retv;
    /* Set the right position, this is needed because MDH requiries the first block */
    fseek(file->fp, file->offset+pos, SEEK_SET);
    retv = fread(buf,sizeof(char), max, file->fp);
    if(retv == 0)
    {
        if(feof(file->fp))
        {
            return -1;
        }
        if(ferror(file->fp))
        {
            printf("Error: %s\n", strerror(ferror(file->fp)));
            return -1;
        }
    }
	return retv;
}
static int apc_all(void * cls,
		const struct sockaddr * addr,
		socklen_t addrlen) {

	return MHD_YES; /* accept connections from anyone */
}
static void file_close(str_block *str)
{
	fclose(str->fp);
	g_free(str);
}
static int ahc_echo(void *nothing,
		struct MHD_Connection * connection,
		const char * url,
		const char * method,
        const char *version,
		const char * upload_data,
		size_t     * upload_data_size,
        void **con_cls)
{
    gchar *me; 
    struct MHD_Response * response;
	int ret;

	if (0 != strcmp(method, "GET"))
		return MHD_NO; /* unexpected method */

    /* Catch NULL url or a url that is to short */
    if ( url == NULL || strlen(url) < 2) 
        return MHD_NO;
	me = cfg_get_single_value_as_string(cfg_urls, URLS_CLASS, (char *)&url[1]);
	if(me && g_file_test(me, G_FILE_TEST_EXISTS))
	{
        TagLib_File *file = NULL;
        int tag_found = 0;
		/* needs error checking */
		gchar *mime_type = "application/octet-stream";
		gchar  *extention;
		const char *position;
		glong pos=0;
		struct stat stats;
		str_block *str = g_malloc0(sizeof(str_block));
		stat(me, &stats);
		str->size = -1;
		str->fp=g_fopen(me, "r");

		position = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_RANGE);
        /* Seek end position */
        fseek(str->fp, 0, SEEK_END);
        str->size = (size_t)ftell(str->fp);
        /*
        fseek(str->fp, 0, SEEK_SET);
        */
        /* if read with offset, get the offset */
        if(position)
		{
			pos = (long)g_ascii_strtoll(&position[6],NULL,10);
		}
        str->offset = pos;
        /* Create response */
		response = MHD_create_response_from_callback(str->size-pos,
				4048,
				(MHD_ContentReaderCallback)&file_reader, 
				str,
				(MHD_ContentReaderFreeCallback)&file_close);

		/* somehow gcc thinks I am not using this value, stupid gcc */
		for(extention = &me[strlen(me)]; me != extention && extention[0] != '.';*extention--);
		if(0 == strncasecmp(extention, ".flac",5)) {
			mime_type = "audio/x-flac";
		}else if (0 == strncasecmp(extention, ".mp3", 4))
		{
			mime_type = "audio/mpeg";
		}else if (0 == strncasecmp(extention, ".ogg", 4))
		{
			mime_type = "audio/x-vorbis+ogg";
		} else if (0 == strncasecmp(extention, ".wv", 3))
		{
			mime_type = "audio/x-wavpack";
		}
        else if (0 == strncasecmp(extention, ".wav", 3))
        {
			mime_type = "audio/x-wav";
		}

		MHD_add_response_header(response,MHD_HTTP_HEADER_CONTENT_TYPE,mime_type); 
        /* Tell mpd we can seek */
		MHD_add_response_header(response,MHD_HTTP_HEADER_ACCEPT_RANGES,"bytes"); 


		MHD_add_response_header(response,"icy-metaint" , "0");
        /*
         * Try to generate icy-data 
         */
        file = taglib_file_new(me);
        if(file)
        {
            TagLib_Tag * tag;
            tag  = taglib_file_tag(file);
            if(tag)
            {
                gchar *artist, *title,*album;
                album = taglib_tag_album(tag);
                artist = taglib_tag_artist(tag);
                title = taglib_tag_title(tag);
                if(artist && artist && album) {
                    char *data = g_strdup_printf("%s - %s (%s)", title, artist,album);
                    MHD_add_response_header(response,"x-audiocast-name" , data);
                    g_free(data);
                    tag_found = 1;
                }else if(artist && title){
                    char *data = g_strdup_printf("%s - %s", title, artist);
                    MHD_add_response_header(response,"x-audiocast-name" , data);
                    g_free(data);
                    tag_found = 1;
                }

            }
            taglib_tag_free_strings();
            taglib_file_free(file);
        }
         
        if(!tag_found){
            extention = g_path_get_basename(me);
            MHD_add_response_header(response,"x-audiocast-name" , extention);
            g_free(extention);
        }

		ret = MHD_queue_response(connection,
				MHD_HTTP_OK,
				response);
		MHD_destroy_response(response);
		g_free(me);
		return ret;

	}
	if(me)g_free(me);
	return MHD_NO;
}

static gchar *mserver_get_active_ip(void)
{
#ifdef HAVE_SOCKADDR_IN6_STRUCT
		struct sockaddr_in6 name;
#else /* HAVE_SOCKADDR_IN6_STRUCT */
		struct sockaddr_in name;
#endif /* HAVE_SOCKADDR_IN6_STRUCT */


		socklen_t namelen = sizeof(name);
		gchar *peername;
		if(getsockname(connection->connection->sock,(struct sockaddr*)&name, &namelen)<0){
			peername = g_strdup("localhost");
		} else{
			peername = g_strdup(inet_ntoa(name.sin_addr));
		}
    return peername;
}



void mserver_destroy()
{
    /* Destroy the build in webserver */
	if(d) {
		MHD_stop_daemon(d);
		d = NULL;
	}
    /* Destroy the internal list of songs */
	if(ls) {
		g_object_unref(ls);
		ls = NULL;
	}
    /* Destroy the browser */
	if(mserver_vbox) {
		gtk_widget_destroy(mserver_vbox);
	}
    /* Destroy the config file */
	if(cfg_urls) {
		cfg_close(cfg_urls);
		cfg_urls = NULL;
	}
}

static int support_http = -1;
static int support_file = -1;
static gchar *mserver_get_full_serve_path(const gchar *uid)
{
    gchar *retv = NULL;
    gchar *peername = mserver_get_active_ip();
    if(support_file) {
        gchar *temp = cfg_get_single_value_as_string(cfg_urls, URLS_CLASS,uid); 
        retv = g_strdup_printf("file://%s", temp);
        g_free(temp);
    }else if(support_http) {
        retv = g_strdup_printf("http://%s:9876/%s", peername, uid);
    }

    g_free(peername);
    return retv;
}

static MpdData * _add_file(MpdData *data, char *uid, char *filename)
{
    TagLib_File *file;
    TagLib_Tag *tag;
    char *a = NULL;
    mpd_Song *song = mpd_newSong();
    data = mpd_new_data_struct_append(data);
    data->type = MPD_DATA_TYPE_SONG;
    data->song = song;
    song->file = mserver_get_full_serve_path(uid); 
    song->name = g_strdup(uid);

    file = taglib_file_new(filename);
    if(file)
    {
        tag = taglib_file_tag(file);
        if(tag)
        {
            if((a = taglib_tag_title(tag)) && a[0] != '\0')
                song->title = g_strdup(a);
            if((a = taglib_tag_album(tag)) && a[0] != '\0')
                song->album = g_strdup(a); 
            if((a = taglib_tag_artist(tag)) && a[0] != '\0')
                song->artist = g_strdup(a); 
            song->track = g_strdup_printf("%i", taglib_tag_track(tag)); 
            if((a = taglib_tag_genre(tag)) && a[0] != '\0')
                song->genre =  g_strdup(a);
            song->date =  g_strdup_printf("%i", taglib_tag_year(tag));
        }
        taglib_tag_free_strings();
        taglib_file_free(file);
    }
    return data;
}
void mserver_browser_add_file()
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Add File", NULL, 
                GTK_FILE_CHOOSER_ACTION_OPEN,
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
                GTK_STOCK_OK, GTK_RESPONSE_OK,
                NULL);

	GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), FALSE);
	gtk_file_filter_set_name(filter, "all");
	gtk_file_filter_add_pattern(filter,"*.wav");
	gtk_file_filter_add_pattern(filter,"*.ogg");
	gtk_file_filter_add_pattern(filter,"*.mp3");
	gtk_file_filter_add_pattern(filter,"*.flac");
	gtk_file_filter_add_pattern(filter,"*.wv");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "wav");
	gtk_file_filter_add_pattern(filter,"*.wav");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "ogg");
	gtk_file_filter_add_pattern(filter,"*.ogg");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "mp3");
	gtk_file_filter_add_pattern(filter,"*.mp3");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "flac");
	gtk_file_filter_add_pattern(filter,"*.flac");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "wavpack");
	gtk_file_filter_add_pattern(filter,"*.wv");                 	
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog),TRUE);


    gtk_widget_set_size_request(GTK_WIDGET(dialog), 300,400);
	gtk_widget_show(dialog);
	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_OK:
			{
				GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
				if(filenames)
				{
                    MpdData *data = gmpc_mpddata_model_steal_mpd_data(GMPC_MPDDATA_MODEL(ls));
					GSList *iter = filenames; 
                    if(data)
                        while(!mpd_data_is_last(data)) data = mpd_data_get_next(data);
					for(;iter;iter= g_slist_next(iter))
                    {
                        gchar *filename = iter->data;
                        guint id = g_random_int();
                        gchar *name = g_strdup_printf("%u", id);
                        data = _add_file(data,name,filename);

                        cfg_set_single_value_as_string(cfg_urls,URLS_CLASS, name, filename);			
                        g_free(name);
                    }
					g_slist_foreach(filenames, (GFunc)g_free, NULL);
					g_slist_free(filenames);
                    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(ls), mpd_data_get_first(data));
				}
			}
		default:
			break;
	}
	gtk_widget_destroy(dialog);
}
void mserver_browser_activated(GtkWidget *tree, GtkTreePath *path)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter(model, &iter,path))
	{		
        gchar *uid = NULL;
		gchar *url = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, MPDDATA_MODEL_COL_SONG_NAME, &uid, -1);
        if(uid)
        {
            url = mserver_get_full_serve_path(uid); 
            mpd_playlist_add(connection, url);
            g_free(url);
            g_free(uid);
        }
	}
}
static void mserver_browser_remove_files(GtkButton *button, GtkTreeView *tree)
{
    MpdData *data;
    MpdData_real *miter = NULL;
    GtkTreeModel *model = gtk_tree_view_get_model(tree);
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	GList *del_items = NULL;
	GList *iter, *list = gtk_tree_selection_get_selected_rows(sel, &model);
    int not_deleted = 0;

    /**
     * If there is no row selected.
     * Make a list of all rows
     */
    if(list == NULL) {
        GtkTreeIter iter;
        if(gtk_tree_model_get_iter_first(model, &iter))
        {
            do{
                GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
                list = g_list_append(list, path); 
            }while(gtk_tree_model_iter_next(model, &iter));
        }
        list = g_list_first(list);
    }
    /**
     * Remove rows
     */
	for(iter = list; iter;iter = g_list_next(iter))
	{
		GtkTreeIter titer;
		if(gtk_tree_model_get_iter(model, &titer, iter->data))
		{
			gchar *uids=NULL;
            gchar *path=NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(model), &titer, 
                MPDDATA_MODEL_COL_SONG_NAME, &uids,
                MPDDATA_MODEL_COL_PATH, &path,
                -1);
            if(path)
            {
                MpdData *data = NULL;
                /* Check if the song that is going to be deleted is in the playlist */
                mpd_playlist_search_start(connection, TRUE);
                mpd_playlist_search_add_constraint(connection, MPD_TAG_ITEM_FILENAME, path);
                data = mpd_playlist_search_commit(connection);
                /* Delete the songs that are in the playlist from he playlist */
                if (data){
                    /* Make sure it is not deleted */
                    g_free(uids);
                    uids = NULL;
                    not_deleted++;
                    mpd_data_free(data);
                }
                g_free(path);
            }
            if(uids)
            {
                /* Delete the item from the config file */
                cfg_del_single_value(cfg_urls, URLS_CLASS, uids);
                del_items = g_list_append(del_items,uids);
            }

		}
	}
	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);

    /* Remove the items from the tree view */
    data = gmpc_mpddata_model_steal_mpd_data(GMPC_MPDDATA_MODEL(ls));
    if(data) miter =(MpdData_real *)mpd_data_get_first(data);
    iter = g_list_first(del_items);
	for(;iter;iter= g_list_next(iter))
	{
        gchar *uid = iter->data;
        while(strcmp(miter->song->name, uid) != 0 && miter) miter = miter->next; 
        if(miter){
            miter = (MpdData_real *)mpd_data_delete_item((MpdData *)miter);
        }
    }
    data = (MpdData *)miter;
    g_list_foreach(del_items, (GFunc) g_free, NULL);
	g_list_free(del_items);
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(ls), mpd_data_get_first(data));

    /**
     * Tell the user that files where not deleted
     */
    if(not_deleted > 0 )
    {
            gchar *mes = g_markup_printf_escaped("%i %s %s.",
                    not_deleted, 
                    (not_deleted > 1)?_("songs where"):_("song was"),
                    _("not removed because it still exists in the play queue"));
            playlist3_message_show(pl3_messages, mes, ERROR_WARNING); 
            g_free(mes);
    }
}

static void mserver_browser_add_files_to_playlist(GtkButton *button, GtkTreeView *tree)
{
	GtkTreeModel *model = gtk_tree_view_get_model(tree);
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	GList *iter, *list = gtk_tree_selection_get_selected_rows(sel, &model);
    int found = 0;
	for(iter = list; iter;iter = g_list_next(iter))
	{
		GtkTreeIter titer;
		if(gtk_tree_model_get_iter(model, &titer, iter->data))
		{
			char *uid = NULL;
			gchar *url = NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(model), &titer, MPDDATA_MODEL_COL_SONG_NAME, &uid, -1);
            if(uid)
            {
                url = mserver_get_full_serve_path(uid); 
                mpd_playlist_queue_add(connection, url);
                g_free(url);
                g_free(uid);
                found = 1;
            }
		}
	}
    if(found) mpd_playlist_queue_commit(connection);
	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);
}

static void mserver_browser_replace_files_to_playlist(GtkButton *button, GtkTreeView *tree)
{
    mpd_playlist_clear(connection);
    mserver_browser_add_files_to_playlist(button, tree);
    mpd_player_play(connection);
}
/* Drag and drop Target table */
static GtkTargetEntry target_table[] =
{
    { (gchar*)"x-url/http", 0, 0 },
	{ (gchar*)"_NETSCAPE_URL", 0, 1},
	{ (gchar*)"text/uri-list", 0, 2},
    { (gchar*)"audio/*",0,3},
    { (gchar*)"audio/x-scpls", 0,4}
};


MpdData *add_url(MpdData *data, char *uri)
{
    gchar *filename = g_filename_from_uri(uri, NULL, NULL);
    if(filename)
    {
        if(g_file_test(filename, G_FILE_TEST_IS_REGULAR))
        {
            if(g_regex_match_simple(".*\\.(flac|mp3|ogg|wv|wav)$", filename, G_REGEX_CASELESS, 0))
            {
                guint id = g_random_int();
                gchar *name = g_strdup_printf("%u", id);
                data = _add_file(data,name,filename);
                cfg_set_single_value_as_string(cfg_urls,URLS_CLASS, name, filename);			
                g_free(name);
            }
        }else if (g_file_test(filename, G_FILE_TEST_IS_DIR)) {
            GDir *dir = g_dir_open(filename, 0, NULL);
            if(dir) {
                const char *filename;
                while((filename = g_dir_read_name(dir))){
                    gchar *ffile = g_build_filename(uri, filename,NULL);
                    data = add_url(data, ffile);
                    g_free(ffile);
                }
                g_dir_close(dir);
            }
        }
        g_free(filename);
    }
    return data;
}

static void mserver_drag_data_recieved (GtkWidget          *widget,
		GdkDragContext     *context,
		gint                x,
		gint                y,
		GtkSelectionData   *data,
		guint               info,
		guint               time_recieved)
{
    const gchar *url_data = (gchar *)data->data;
    gchar **url = g_uri_list_extract_uris(url_data);
    if(url) {
        int i;
        MpdData *data = gmpc_mpddata_model_steal_mpd_data(GMPC_MPDDATA_MODEL(ls));
        if(data)
            while(!mpd_data_is_last(data)) data = mpd_data_get_next(data);
        for(i=0;url[i];i++)
        {
            data = add_url(data,url[i]);
        }
        gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(ls), mpd_data_get_first(data));
        g_strfreev(url);
    }
}
static gboolean mserver_right_mouse_menu(GtkWidget *tree, GdkEventButton *event)
{
    if(event->button == 3)
    {
        GtkWidget *menu = NULL,*item;
        menu = gtk_menu_new();

        /* add the delete widget */
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(mserver_browser_add_files_to_playlist), tree);

        /* replace the replace widget */
        item = gtk_image_menu_item_new_with_label(_("Replace"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(mserver_browser_replace_files_to_playlist), tree);

        gmpc_mpddata_treeview_right_mouse_intergration(GMPC_MPDDATA_TREEVIEW(tree), GTK_MENU(menu));
        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, 0, event->time);
        return TRUE;
    }
    return FALSE;
}

GtkWidget* error_label = NULL;

void mserver_init() {
	GtkWidget *sw, *tree,*button,*bbox;
	gchar *fs, *path, *url;

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    /**
     * Add icon 
     */
    path = gmpc_plugin_get_data_path(&plugin);
    url = g_build_path(G_DIR_SEPARATOR_S,path, "gmpc-mserver", NULL);
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default (),url);
    g_free(url);
    g_free(path);

	/* config file */
	fs = gmpc_get_user_path("server_urls.txt");
	cfg_urls = cfg_open(fs);
	g_free(fs);	

	d = MHD_start_daemon(
            MHD_USE_THREAD_PER_CONNECTION,
			9876,	
			&apc_all,
			NULL,
			&ahc_echo,
			NULL,
			MHD_OPTION_END);

	ls = (GtkTreeModel *) gmpc_mpddata_model_new();

	mserver_vbox = gtk_vbox_new(FALSE,6);
	/* Scrolled window */
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	/* TreeView */
	tree = gmpc_mpddata_treeview_new("mserver-plugin", TRUE, GTK_TREE_MODEL(ls));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), GTK_SELECTION_MULTIPLE);
    g_signal_connect(G_OBJECT(tree), "button-press-event", G_CALLBACK(mserver_right_mouse_menu), NULL);



	g_signal_connect(G_OBJECT(tree), "row-activated", G_CALLBACK(mserver_browser_activated), NULL);
	gtk_container_add(GTK_CONTAINER(sw),tree);
	gtk_box_pack_start(GTK_BOX(mserver_vbox), sw, TRUE, TRUE, 0); 

	bbox = gtk_hbutton_box_new();
	
	button = gtk_button_new_with_label(_("Add files"));
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(mserver_browser_add_file), NULL);
	gtk_box_pack_start(GTK_BOX(bbox), button,FALSE,FALSE,0);

	button = gtk_button_new_with_label(_("Add to playlist"));
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(mserver_browser_add_files_to_playlist), tree);
	gtk_box_pack_start(GTK_BOX(bbox), button,FALSE,FALSE,0);

	button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(mserver_browser_remove_files),tree);
	gtk_box_pack_start(GTK_BOX(bbox), button,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(mserver_vbox), bbox,FALSE,FALSE,0);


	/**
	 * Set as drag destination
	 */
	gtk_drag_dest_set(mserver_vbox,
			GTK_DEST_DEFAULT_ALL,
			target_table, 5,
			GDK_ACTION_COPY|GDK_ACTION_LINK|GDK_ACTION_DEFAULT|GDK_ACTION_MOVE);
	g_signal_connect (G_OBJECT (mserver_vbox),"drag_data_received",
			GTK_SIGNAL_FUNC (mserver_drag_data_recieved),
			NULL);

	g_object_ref(mserver_vbox);
	gtk_widget_show_all(mserver_vbox);

    error_label = gtk_label_new(_("The connected MPD does not support reading music from the 'Serve Music' plugin"));
    fs = g_markup_printf_escaped("<span size='xx-large' weight='bold'>%s</span>",
        _("The connected MPD does not support reading music from the 'Serve Music' plugin"));
    gtk_label_set_markup(GTK_LABEL(error_label), fs);    
    g_free(fs);

	gtk_box_pack_start(GTK_BOX(mserver_vbox), error_label,FALSE,FALSE,0);
}


void mserver_browser_selected(GtkWidget *container)
{
	gtk_container_add(GTK_CONTAINER(container), mserver_vbox);
}

void mserver_browser_unselected(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), mserver_vbox);
}

void mserver_connection_changed(MpdObj *mi, int connect, gpointer data)
{
    /* reset */
    support_http = -1;
    support_file = -1;
    if(connect)
    {
        if(support_http < 0 || support_file < 0){
            gchar **url_handlers = mpd_server_get_url_handlers(connection);
            /* set to not supported */
            support_http = support_file = 0;
            if(url_handlers){
                int i;
                for(i=0;url_handlers[i];i++){
                    if(strcasecmp(url_handlers[i], "http://") == 0) support_http = 1;
                    else if(strcasecmp(url_handlers[i], "file://") == 0) support_file = 1;
                }
                g_strfreev(url_handlers);
            }
        }
    }
    if(mserver_vbox && connect) {
        if(support_http == 0 && support_file == 0){
            gtk_widget_set_sensitive(mserver_vbox,FALSE);
            gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(ls),NULL); 
            gtk_widget_show(error_label);
        }else {

            gtk_widget_hide(error_label);
            gtk_widget_set_sensitive(mserver_vbox,TRUE);
            /**
             * Reload
             */
            conf_mult_obj *cmo,*cmo_iter;
            cmo= cfg_get_key_list(cfg_urls, URLS_CLASS);
            if(cmo) {
                MpdData *data = NULL;
                for( cmo_iter = cmo ;cmo_iter;cmo_iter = cmo_iter->next ) {
                    data = _add_file(data,cmo_iter->key,cmo_iter->value);
                }
                cfg_free_multiple(cmo);
                gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(ls), mpd_data_get_first(data));
            }
        }
    }
}

static const gchar *mserver_get_translation_domain(void)
{
    return GETTEXT_PACKAGE;
}

gmpcPlBrowserPlugin mserver_browser_plugin ={
	.add = mserver_browser_add,
	.selected = mserver_browser_selected,
	.unselected = mserver_browser_unselected,

};

int plugin_api_version = PLUGIN_API_VERSION;
gmpcPlugin plugin = {
	.name = N_("File Serve plugin"),
	.version        = {PLUGIN_MAJOR_VERSION,PLUGIN_MINOR_VERSION,PLUGIN_MICRO_VERSION},
	.plugin_type = GMPC_PLUGIN_PL_BROWSER,
	.init = mserver_init,
	.destroy = mserver_destroy,
    .save_yourself = mserver_save_myself,
	.get_enabled = mserver_get_enabled,
	.set_enabled = mserver_set_enabled,
	.browser = &mserver_browser_plugin,
    .mpd_connection_changed = mserver_connection_changed,
    .get_translation_domain = mserver_get_translation_domain
};
