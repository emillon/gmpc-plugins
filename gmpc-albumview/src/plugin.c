#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gmpc/plugin.h>
#include <gmpc/playlist3-messages.h>
#include <gmpc/gmpc-metaimage.h>
#include <gmpc/misc.h>
#include <libmpd/libmpd-internal.h>
#include <math.h>
#include <gmpc/gmpc-extras.h>
#include "plugin.h"

const GType albumview_plugin_get_type(void);
#define ALBUM_VIEW_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), albumview_plugin_get_type(), AlbumViewPlugin))
#define AV_LOG_DOMAIN "AlbumViewPlugin"

typedef struct _AlbumViewPluginPrivate {
    int supported_columns;
    int supported_rows;
    int album_size;
    GtkWidget *filter_entry;
    GtkWidget *slider_scale;
    GtkWidget *progress_bar;
    GtkWidget *item_table;
    GtkWidget *albumview_box;
    GtkWidget *albumview_main_box;
    GtkWidget *event_bg;
    gboolean  require_scale_update;
    int max_entries;
    int current_entry;

    MpdData *complete_list;
    guint update_timeout;
    /* temp */
    MpdData *data;
    GList *current_item;
    GtkTreeRowReference *albumview_ref;
}_AlbumViewPluginPrivate;



static gchar * albumview_format_time(unsigned long seconds);
static void albumview_browser_save_myself(GmpcPluginBase *plug);
static void position_changed(GtkRange *range, gpointer data);
static void filter_list(GtkEntry *entry, gpointer data);
void update_view(AlbumViewPlugin *self);
static void load_list(AlbumViewPlugin *self);
/**
 * Get/Set enable
 */

static int albumview_get_enabled(GmpcPluginBase *plug)
{
	return cfg_get_single_value_as_int_with_default(config, "albumview", "enable", TRUE);
}

void albumview_set_enabled(GmpcPluginBase *plug, int enabled)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(plug);
	cfg_set_single_value_as_int(config, "albumview", "enable", enabled);
	if(enabled)
	{
		if(self->priv->albumview_ref == NULL)
		{
			albumview_add(GMPC_PLUGIN_BROWSER_IFACE(plug), GTK_WIDGET(playlist3_get_category_tree_view()));
		}
	}
	else
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(self->priv->albumview_ref);
        GtkTreeModel *model = gtk_tree_row_reference_get_model(self->priv->albumview_ref);
		if (path){
			GtkTreeIter iter;
			if (gtk_tree_model_get_iter(model, &iter, path)){
				gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			}
			gtk_tree_path_free(path);
			gtk_tree_row_reference_free(self->priv->albumview_ref);
			self->priv->albumview_ref = NULL;
		}
	}
}

/**
 * Playlist browser functions
 */
static void albumview_add(GmpcPluginBrowserIface *plug, GtkWidget *category_tree)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(plug);
	GtkTreePath *path;
	GtkTreeModel *model = GTK_TREE_MODEL(playlist3_get_category_tree_store());
	GtkTreeIter iter;
    gint pos;
	/**
	 * don't do anything if we are disabled
	 */
	if(!cfg_get_single_value_as_int_with_default(config, "albumview", "enable", TRUE)) return;
	/**
	 * Add ourslef to the list
	 */
	pos = cfg_get_single_value_as_int_with_default(config, "albumview","position",2);
	playlist3_insert_browser(&iter, pos);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			PL3_CAT_TYPE, GMPC_PLUGIN_BASE(plug)->id,
			PL3_CAT_TITLE,"Album View",
			PL3_CAT_ICON_ID, "albumview",
			-1);
	/**
	 * remove odl reference if exists
	 */
	if (self->priv->albumview_ref) {
		gtk_tree_row_reference_free(self->priv->albumview_ref);
		self->priv->albumview_ref = NULL;
	}
	/**
	 * create reference to ourself in the list
	 */
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &iter);
	if (path) {
		self->priv->albumview_ref = gtk_tree_row_reference_new(model, path);
		gtk_tree_path_free(path);
	}
}
static void albumview_browser_save_myself(GmpcPluginBase *plug)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(plug);
	if(self->priv->albumview_ref)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(self->priv->albumview_ref);
		if(path)
		{
			gint *indices = gtk_tree_path_get_indices(path);
			g_log(AV_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Saving myself to position: %i", indices[0]);
			cfg_set_single_value_as_int(config, "albumview","position",indices[0]);
			gtk_tree_path_free(path);
		}
	}
}

void size_changed(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(user_data);
    int columns = (alloc->width-10)/(self->priv->album_size +25);
    int rows = (alloc->height-10)/(self->priv->album_size +40);

    if(columns != self->priv->supported_columns || rows != self->priv->supported_rows)
    {
        self->priv->supported_columns = (columns)?columns:1;
        self->priv->supported_rows = (rows)?rows:1;
        printf("supported rows: %i\n", self->priv->supported_rows);
        g_log(AV_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "update columns: %i %i %i\n",
                alloc->width-20,columns, self->priv->album_size);
        self->priv->require_scale_update = TRUE;
        if(self->priv->filter_entry && GTK_WIDGET_IS_SENSITIVE(self->priv->filter_entry))
        {
            update_view(self);
        }
    }

}

static gboolean albumview_scroll_event(GtkWidget *event_box, GdkEventScroll *event, gpointer data)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(data);
    if(self->priv->current_item == NULL) return FALSE;
    if(event->direction == GDK_SCROLL_UP)
    {
        int value = gtk_range_get_value(GTK_RANGE(self->priv->slider_scale))-1;
        gtk_range_set_value(GTK_RANGE(self->priv->slider_scale), value);
        return TRUE;
    }else if(event->direction == GDK_SCROLL_DOWN) {
        int value = gtk_range_get_value(GTK_RANGE(self->priv->slider_scale))+1;
        gtk_range_set_value(GTK_RANGE(self->priv->slider_scale), value);
        return TRUE;
    }
    return FALSE;
}
static gboolean albumview_key_press_event(GtkWidget *event_box, GdkEventKey *event, gpointer data)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(data);
    if(self->priv->current_item == NULL) return FALSE;

    if(event->keyval == GDK_Up){
        int value = gtk_range_get_value(GTK_RANGE(self->priv->slider_scale))-1;
        gtk_range_set_value(GTK_RANGE(self->priv->slider_scale), value);
        return TRUE;
    }else if (event->keyval == GDK_Down){
        int value = gtk_range_get_value(GTK_RANGE(self->priv->slider_scale))+1;
        gtk_range_set_value(GTK_RANGE(self->priv->slider_scale), value);
        return TRUE;
    }else if (event->keyval == GDK_Page_Up) {
        int value = gtk_range_get_value(GTK_RANGE(self->priv->slider_scale))-5;
        gtk_range_set_value(GTK_RANGE(self->priv->slider_scale), value);
        return TRUE;

    }else if (event->keyval == GDK_Page_Down) {
        int value = gtk_range_get_value(GTK_RANGE(self->priv->slider_scale))+5;
        gtk_range_set_value(GTK_RANGE(self->priv->slider_scale), value);
        return TRUE;

    }

    return FALSE;
}
static gboolean albumview_button_press_event(GtkWidget *event_box, GdkEventButton *event, gpointer data)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(data);
    if(self->priv->current_item == NULL) return FALSE;
    gtk_widget_grab_focus(self->priv->event_bg);
}
static gboolean albumview_focus(GtkWidget *wid,GdkEventFocus *event,  gpointer data)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(data);
    g_log(AV_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "focus in");
    gtk_widget_queue_draw(self->priv->event_bg);
    return TRUE;
}
static gboolean albumview_focus_out(GtkWidget *wid,GdkEventFocus *event,  gpointer data)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(data);
    g_log(AV_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "focus out");

    gtk_widget_queue_draw(self->priv->event_bg);
    return TRUE;
}
static gboolean albumview_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	int width = widget->allocation.width;
	int height = widget->allocation.height;
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(data);

	gtk_paint_flat_box(widget->style,
					widget->window,
					GTK_STATE_NORMAL,
					GTK_SHADOW_NONE,
					NULL,
					widget,
					"entry_bg",
					0,0,width,height);
    if(gtk_widget_is_focus(widget))
    {
        gtk_paint_focus(widget->style, widget->window,
                GTK_STATE_NORMAL,
                NULL,
                widget,
                "entry_bg",
                0,0,width,height);
    }
    return FALSE;
}
#if GTK_CHECK_VERSION(2,16,0)
static void mod_fill_clear_search_entry(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
    if(icon_pos == GTK_ENTRY_ICON_SECONDARY){
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
}
#endif
static void albumview_init(AlbumViewPlugin *self)
{
    /** Get an allready exposed widgets to grab theme colors from. */
    GtkWidget *colw = (GtkWidget *)playlist3_get_category_tree_view();
    GtkWidget *label = NULL;
    GtkWidget *event = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *hbox = NULL;     int i = 0;
    self->priv->event_bg = gtk_event_box_new();
    self->priv->albumview_main_box = gtk_vbox_new(FALSE, 6);

    g_signal_connect(G_OBJECT(event), "size-allocate", G_CALLBACK(size_changed), self);

    GtkWidget *iv = self->priv->albumview_box = gtk_vbox_new(FALSE, 6);
    self->priv->slider_scale = gtk_vscale_new_with_range(0,1,1);
    gtk_scale_set_draw_value(GTK_SCALE(self->priv->slider_scale), FALSE);
    g_signal_connect(G_OBJECT(self->priv->slider_scale), "value-changed", G_CALLBACK(position_changed), self);

    self->priv->filter_entry = gtk_entry_new();

#if GTK_CHECK_VERSION(2,16,0)
    gtk_entry_set_icon_from_stock(GTK_ENTRY(self->priv->filter_entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
    g_signal_connect(GTK_ENTRY(self->priv->filter_entry), "icon-press", G_CALLBACK(mod_fill_clear_search_entry), NULL);
#endif
    g_signal_connect(G_OBJECT(self->priv->filter_entry),"changed", G_CALLBACK(filter_list), self);


    hbox = gtk_hbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new(("Filter")), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),self->priv->filter_entry, TRUE, TRUE, 0);

    gtk_box_pack_end(GTK_BOX(self->priv->albumview_main_box), hbox, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(self->priv->albumview_main_box),hbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),event, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),self->priv->slider_scale, FALSE, FALSE, 0);//gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), event);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(event), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(event), GTK_SHADOW_ETCHED_IN);
    /* setup bg */
    /* TODO draw focus */
//    gtk_widget_modify_bg(self->priv->event_bg, GTK_STATE_NORMAL,&(self->priv->albumview_main_box->style->white));
    gtk_widget_set_app_paintable(self->priv->event_bg, TRUE);
    g_signal_connect(G_OBJECT(self->priv->event_bg), "expose-event", G_CALLBACK(albumview_expose_event), self);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(self->priv->event_bg), TRUE);

    g_object_set(self->priv->event_bg, "can-focus", TRUE,NULL);
    GTK_WIDGET_SET_FLAGS(self->priv->event_bg, GTK_HAS_FOCUS);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(event), self->priv->event_bg);
    gtk_container_add(GTK_CONTAINER(self->priv->event_bg), iv);
    gtk_widget_add_events(self->priv->event_bg, GDK_SCROLL_MASK|GDK_BUTTON_PRESS_MASK|GDK_FOCUS_CHANGE_MASK);
    g_signal_connect_object(G_OBJECT(self->priv->event_bg), "scroll-event", G_CALLBACK(albumview_scroll_event), self,0);
    g_signal_connect_object(G_OBJECT(self->priv->event_bg), "key-press-event", G_CALLBACK(albumview_key_press_event), self,0);
    g_signal_connect_object(G_OBJECT(self->priv->event_bg), "focus-in-event", G_CALLBACK(albumview_focus), self,0);
    g_signal_connect_object(G_OBJECT(self->priv->event_bg), "focus-out-event", G_CALLBACK(albumview_focus_out), self, 0);
    g_signal_connect_object(G_OBJECT(self->priv->filter_entry), "key-press-event", G_CALLBACK(albumview_key_press_event), self,0);
    g_signal_connect_object(G_OBJECT(self->priv->event_bg), "button-press-event", G_CALLBACK(albumview_button_press_event), self,0);

    gtk_widget_show_all(self->priv->albumview_main_box);


    /* maintain my own reference to the widget, so it won't get destroyed removing
     * from view
     */
    g_object_ref_sink(self->priv->albumview_main_box);
}


static void albumview_selected(GmpcPluginBrowserIface *plug, GtkWidget *container)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(plug);
    if(self->priv->albumview_main_box== NULL) {
        albumview_init((AlbumViewPlugin *)plug);
        albumview_connection_changed(gmpcconn, connection,1,self);
    }
    gtk_container_add(GTK_CONTAINER(container), self->priv->albumview_main_box);
    gtk_widget_show(self->priv->albumview_main_box);
    gtk_widget_show(container);
    gtk_widget_grab_focus(self->priv->event_bg);
}

static void albumview_unselected(GmpcPluginBrowserIface *plug,GtkWidget *container)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(plug);
    gtk_container_remove(GTK_CONTAINER(container), self->priv->albumview_main_box);
}

static void status_changed(GmpcConnection *gmpcconnn,MpdObj *mi, ChangedStatusType type, AlbumViewPlugin *self)
{
    if((type&MPD_CST_DATABASE) > 0)
    {

        if(self->priv->albumview_main_box)
            load_list(self);
    }
}

void albumview_plugin_init(AlbumViewPlugin *self)
{
    gchar *path;
    const gchar * const *paths = g_get_system_data_dirs();
    int i;

    /* First try the compile time path */
    path = g_build_filename(PIXMAP_DATA_DIR, NULL);
    if(path){
        if(!g_file_test(path, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_DIR))
        {
            g_free(path);
            path = NULL;
        }
    }

    for(i=0; path == NULL && paths && paths[i]; i++) {
        path = g_build_filename(paths[i], "gmpc-albumview", "icons", NULL);
        if(!g_file_test(path, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_DIR))
        {
            g_free(path);
            path = NULL;
        }
    }
    if(path) {
        gtk_icon_theme_append_search_path(gtk_icon_theme_get_default (),path);
    }
    g_signal_connect_object(G_OBJECT(gmpcconn), "status-changed", G_CALLBACK(status_changed), self, 0);

    g_free(path);
    path = NULL;
}
#define TIMER_SUB(start,stop,diff)  diff.tv_usec = stop.tv_usec - start.tv_usec;\
        diff.tv_sec = stop.tv_sec - start.tv_sec;\
        if(diff.tv_usec < 0) {\
            diff.tv_sec -= 1; \
            diff.tv_usec += G_USEC_PER_SEC; \
        }
static gint __add_sort(gpointer aa, gpointer bb, gpointer data)
{
    MpdData_real *a = *(MpdData_real **)aa;
    MpdData_real *b = *(MpdData_real **)bb;
    if(!a || !b) return 0;
    if(a->type == MPD_DATA_TYPE_SONG && b->type == MPD_DATA_TYPE_SONG)
    {
        if(a->song->artist &&  b->song->artist)
        {
            int val;
            {
                gchar *sa,*sb;
                sa = g_utf8_strdown(a->song->artist, -1);
                sb = g_utf8_strdown(b->song->artist, -1);
                val = g_utf8_collate(sa,sb);
                g_free(sa);
                g_free(sb);
            }/* else {
                val = (a == NULL)?((b==NULL)?0:-1):1;
            }*/
            if(val != 0)
                return val;
            if (a->song->album && b->song->album)
            {
                gchar *sa,*sb;
                sa = g_utf8_strdown(a->song->album, -1);
                sb = g_utf8_strdown(b->song->album, -1);
                val = g_utf8_collate(sa,sb);
                g_free(sa);
                g_free(sb);

            }
            return val;
        }
    }
    return -1;
}

static gboolean update_progressbar(AlbumViewPlugin *self)
{
    gchar *temp = g_strdup_printf("%i of %i albums loaded", self->priv->current_entry, self->priv->max_entries);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->priv->progress_bar), self->priv->current_entry/(double)self->priv->max_entries);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(self->priv->progress_bar), temp);
    g_free(temp);
    return FALSE;
}

static void load_list_itterate(MpdObj *mi, AlbumViewPlugin *self)
{
    do{
    MpdData *data2 = NULL;
    self->priv->current_entry++;
    if(self->priv->max_entries>0 && self->priv->current_entry%25 == 0){
        g_idle_add(update_progressbar, self);
    }
    if(self->priv->data)
    {
        mpd_database_search_field_start(mi, MPD_TAG_ITEM_ARTIST);
        mpd_database_search_add_constraint(mi, MPD_TAG_ITEM_ALBUM, (self->priv->data)->tag);
        data2 = mpd_database_search_commit(mi);
        if(data2)
        {
            mpd_Song *song = mpd_newSong();
            song->album = g_strdup((self->priv->data)->tag);
            song->artist = g_strdup(data2->tag);
            if(!mpd_data_is_last(data2))
            {
                /* test if server supports album artist */
                if(mpd_server_tag_supported(mi, MPD_TAG_ITEM_ALBUM_ARTIST))
                {
                    mpd_database_search_field_start(mi, MPD_TAG_ITEM_ALBUM_ARTIST);
                    mpd_database_search_add_constraint(mi, MPD_TAG_ITEM_ALBUM, (self->priv->data)->tag);
                    MpdData *data3 = mpd_database_search_commit(mi);
                    if(mpd_data_is_last(data3)){
                        if(strlen(data3->tag) > 0)
                        {
                            song->albumartist = g_strdup(data3->tag);
                            if(song->artist) g_free(song->artist);
                            song->artist = g_strdup(data3->tag);
                        }
                    }
                    else{
                        mpd_freeSong(song);
                        song = NULL;
                    }
                    mpd_data_free(data3);
                }
                else {
                    mpd_freeSong(song);
                    song = NULL;
                }
            }
            mpd_data_free(data2);
            if(song){
                self->priv->complete_list = mpd_new_data_struct_append(self->priv->complete_list);
                self->priv->complete_list->song = song;
                self->priv->complete_list->type = MPD_DATA_TYPE_SONG;
                song = NULL;
            }
        }

        (self->priv->data) = mpd_data_get_next((self->priv->data));
    }
    }
    while(self->priv->data != NULL);
    self->priv->complete_list = (MpdData *)misc_sort_mpddata(mpd_data_get_first(self->priv->complete_list), (GCompareDataFunc)__add_sort, NULL);
}

void update_finished(MpdData *data, AlbumViewPlugin *self)
{
    if(self->priv->data == NULL){
        int items = 0;
        MpdData_real *iter;
        g_log(AV_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"update view\n");
        gtk_widget_destroy(self->priv->progress_bar);
        self->priv->progress_bar = NULL;
        for(iter = (MpdData_real*)self->priv->complete_list; iter; iter = iter->next) items++;

        gtk_widget_set_sensitive(self->priv->filter_entry, TRUE);
        filter_list(GTK_ENTRY(self->priv->filter_entry), self);

        gtk_widget_grab_focus(self->priv->event_bg);
    }
}

static void load_list(AlbumViewPlugin *self)
{
    if(self->priv->complete_list)mpd_data_free(self->priv->complete_list);
    self->priv->complete_list = NULL;
    if(self->priv->current_item) g_list_free(self->priv->current_item);
    self->priv->current_item = NULL;


    self->priv->progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(self->priv->albumview_box), self->priv->progress_bar, FALSE, FALSE, 0);
    gtk_widget_show(self->priv->progress_bar);
    mpd_database_search_field_start(connection, MPD_TAG_ITEM_ALBUM);
    MpdData *iter,*data = mpd_database_search_commit(connection);
    self->priv->max_entries= 0;
    self->priv->current_entry = 0;
    gtk_widget_set_sensitive(self->priv->filter_entry, FALSE);
    for(iter = data; iter; iter = mpd_data_get_next_real(iter, FALSE)) self->priv->max_entries++;
    self->priv->data= data;
    mpd_async_request(update_finished, self, load_list_itterate, self);
}
void albumview_connection_changed(GmpcConnection *conn, MpdObj *mi, int connect,void *usedata)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(usedata);

    if(connect && self->priv->albumview_main_box)
    {
        load_list(self);
    }
    else if(self->priv->albumview_main_box){
        mpd_data_free(self->priv->complete_list);
        self->priv->complete_list = NULL;
        if(self->priv->item_table)
            gtk_widget_hide(self->priv->item_table);
    }
}
static void album_add(GtkWidget *button, mpd_Song *song)
{
    mpd_database_search_start(connection,TRUE);

    mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ALBUM, song->album);
    if(song->albumartist && strlen(song->albumartist) >0){
        mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ALBUM_ARTIST, song->albumartist);
    }else{
        mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, song->artist);
    }
    MpdData *data = mpd_database_search_commit(connection);
    /* Sort it before adding */
    data = misc_sort_mpddata_by_album_disc_track(data);
    for(;data;data = mpd_data_get_next(data)){
        mpd_playlist_queue_add(connection, data->song->file);
    }
    mpd_playlist_queue_commit(connection);
}
static void album_view(GtkWidget *button ,mpd_Song *song)
{
	if (song && song->artist && song->album) {
		info2_activate();
		info2_fill_album_view(song->artist, song->album);
	}
}

static void album_replace(GtkWidget *button, mpd_Song *song)
{
    mpd_playlist_clear(connection);
    album_add(button, song);
    mpd_player_play(connection);
}
static gboolean album_button_press(GtkWidget *image, GtkMenu *menu, mpd_Song *song)
{
    GtkWidget *item;

    item = gtk_image_menu_item_new_with_label("Album information");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
            gtk_image_new_from_stock(GTK_STOCK_INFO, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(album_view), song);

    item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(album_add), song);

    /* replace the replace widget */
    item = gtk_image_menu_item_new_with_label("Replace");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
            gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(album_replace), song);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show(item);

    return TRUE;
}
static GtkWidget * create_button(AlbumViewPlugin *self, MpdData_real *complete_list_iter)
{
    GtkWidget *item;
    GtkWidget *vbox;
    GtkWidget *label = NULL;
    gchar *temp = NULL;
    /* Wrap it in a vbox */
    vbox = gtk_vbox_new(FALSE, 3);
    gtk_widget_set_size_request(vbox, self->priv->album_size+20,self->priv->album_size+40);

    item = (GtkWidget *)gmpc_metaimage_new_size(META_ALBUM_ART,self->priv->album_size);
    gmpc_metaimage_set_scale_up(GMPC_METAIMAGE(item), TRUE);
    gtk_widget_set_has_tooltip(GTK_WIDGET(item), FALSE);
    gmpc_metaimage_set_squared(GMPC_METAIMAGE(item), TRUE);

    gmpc_metaimage_update_cover_from_song_delayed(GMPC_METAIMAGE(item), complete_list_iter->song);

    gtk_box_pack_start(GTK_BOX(vbox), item, TRUE, TRUE, 0);
    /* Set artist name */
    if(complete_list_iter->song->albumartist){
        GtkWidget *label = gtk_label_new(complete_list_iter->song->albumartist);
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
        gtk_box_pack_end(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    }else{
        GtkWidget *label = gtk_label_new(complete_list_iter->song->artist);
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
        gtk_box_pack_end(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    }

    /* Set album name */
    label = gtk_label_new("");
    temp = g_markup_printf_escaped("<b>%s</b>", complete_list_iter->song->album);
    gtk_label_set_markup(GTK_LABEL(label), temp);
    g_free(temp);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_box_pack_end(GTK_BOX(vbox), label, FALSE, FALSE, 0);


    /* Attach it to the song */
    g_object_add_weak_pointer(G_OBJECT(vbox),&(complete_list_iter->userdata));
    complete_list_iter->userdata = vbox;//g_object_ref_sink(vbox);
    complete_list_iter->freefunc = (void *)gtk_widget_destroy;
    g_object_set_data(G_OBJECT(vbox), "item", item);
    g_signal_connect(G_OBJECT(item), "menu_populate_client", G_CALLBACK(album_button_press), complete_list_iter->song);
    /*
    g_signal_connect(item, "button-press-event",
            G_CALLBACK(album_button_press), complete_list_iter->song);
            */
    return vbox;
}

static void filter_list(GtkEntry *entry, gpointer data)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(data);
    GRegex *regex = NULL;
    int items = 0;
    GList *list = NULL;
    const gchar *search_query = gtk_entry_get_text(GTK_ENTRY(self->priv->filter_entry));
    MpdData_real *complete_list_iter = NULL;
    if(search_query[0] != '\0')
    {
        gchar *str = g_strdup(search_query);
        gchar **test = g_strsplit(g_strstrip(str), " ", -1);
        int i=0;
        GString *s = g_string_new("((?:");
        GError *error = NULL;
        g_free(str);
        for(i=0;test && test[i];i++){
            gchar *temp = g_regex_escape_string(test[i], -1);
            s = g_string_append(s, ".*");
            s= g_string_append(s, temp);
            s = g_string_append(s, ".*");
            if(test[i+1] != NULL)
                s = g_string_append(s, "|");
            g_free(temp);
        }
        g_string_append_printf(s,"){%i})",i);
        g_log(AV_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"regex: %s\n", s->str);
        regex = g_regex_new(s->str, G_REGEX_CASELESS|G_REGEX_EXTENDED, 0,&error);;
        if(regex)
        {
            g_string_free(s, TRUE);
            for(complete_list_iter = (MpdData_real *) mpd_data_get_first(self->priv->complete_list);
                    complete_list_iter;
                    complete_list_iter = (MpdData_real *)mpd_data_get_next_real((MpdData *)complete_list_iter, FALSE))
            {
                if(g_regex_match(regex,complete_list_iter->song->album,0,NULL)||
                        g_regex_match(regex,complete_list_iter->song->artist,0,NULL)||
                        (complete_list_iter->song->albumartist && g_regex_match(regex,complete_list_iter->song->albumartist,0,NULL))){
                    items++;
                    list = g_list_append(list, complete_list_iter);
                }
            }
        }
        if(error) {
            g_log(AV_LOG_DOMAIN, G_LOG_LEVEL_WARNING," error creating regex: %s\n", error->message);
            g_error_free(error);
        }
        g_regex_unref(regex);
    }
    if(self->priv->current_item) g_list_free(self->priv->current_item);
    self->priv->current_item = g_list_first(list);
    self->priv->require_scale_update = TRUE;
    gtk_range_set_value(GTK_RANGE(self->priv->slider_scale), 0);

    update_view(self);
}

static void position_changed(GtkRange *range, gpointer data)
{
    AlbumViewPlugin *self = ALBUM_VIEW_PLUGIN(data);
    gint i=0,value = (int)gtk_range_get_value(range)*self->priv->supported_columns;
    self->priv->current_item = g_list_first(self->priv->current_item);
    for(i=0;i<value && self->priv->current_item && self->priv->current_item->next; self->priv->current_item = self->priv->current_item->next){i++;}
    update_view(self);
}
static gboolean update_view_real(AlbumViewPlugin *self)
{
    MpdData *complete_list_iter;
    const char *search_query = gtk_entry_get_text(GTK_ENTRY(self->priv->filter_entry));
    int i=0;
    int j=0;
    gchar *artist= NULL;
    int items =0;
    MpdData *data = NULL;
    GList *entries = NULL;
    GList *list = (self->priv->item_table)?gtk_container_get_children(GTK_CONTAINER(self->priv->item_table)):NULL;
    GList *iter;
    GRegex *regex = NULL;

    g_log(AV_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"search query: %s\n", search_query);

    if(self->priv->item_table)
        gtk_widget_hide(self->priv->item_table);
    for(iter =  g_list_first(list); iter; iter = iter->next){
        GtkWidget *widget = iter->data;
        gtk_container_remove(GTK_CONTAINER(self->priv->item_table), widget);
    }
    g_list_free(list);
    list = NULL;



    gtk_widget_show(self->priv->albumview_box);

    if(self->priv->current_item ==  NULL){
        int items  =0;
        for(complete_list_iter = mpd_data_get_first(self->priv->complete_list);
                complete_list_iter;
                complete_list_iter = mpd_data_get_next_real(complete_list_iter, FALSE))
        {
            items++;
            self->priv->current_item = g_list_prepend(self->priv->current_item, complete_list_iter);
        }
        self->priv->current_item = g_list_reverse(self->priv->current_item);
//        self->priv->current_item = g_list_first(self->priv->current_item);
        gtk_range_set_value(GTK_RANGE(self->priv->slider_scale), 0);
        self->priv->require_scale_update = TRUE;
    }
    /* TODO: Only update this when something changed! */
    if(self->priv->require_scale_update)
    {
        int items = g_list_length(g_list_first(self->priv->current_item));
        if(ceil(items/(double)self->priv->supported_columns)-self->priv->supported_rows > 0)
        {
            gtk_widget_set_sensitive(GTK_WIDGET(self->priv->slider_scale), TRUE);
            gtk_range_set_range(GTK_RANGE(self->priv->slider_scale), 0,
                    ceil(items/(double)self->priv->supported_columns)-self->priv->supported_rows);
        }
        else{
            gtk_widget_set_sensitive(GTK_WIDGET(self->priv->slider_scale), FALSE);
            gtk_range_set_range(GTK_RANGE(self->priv->slider_scale), 0,1);
        }
        self->priv->require_scale_update = FALSE;
    }

    /**
     * Create holding table if it does not exist
     */
    if(!self->priv->item_table){
        self->priv->item_table = (GtkWidget *)gmpc_widgets_qtable_new();
        gmpc_widgets_qtable_set_item_width(GMPC_WIDGETS_QTABLE(self->priv->item_table),
                self->priv->album_size+25);
        gmpc_widgets_qtable_set_item_height(GMPC_WIDGETS_QTABLE(self->priv->item_table),
                self->priv->album_size+40);
        gtk_box_pack_start(GTK_BOX(self->priv->albumview_box), self->priv->item_table, TRUE, TRUE, 0);
    }

    /**
     * Add albums
     */

    if(self->priv->current_item)
    {
        int rows = self->priv->supported_rows;
        GList *iter = self->priv->current_item;
        int v_items = 0;
        do
        {
            complete_list_iter = iter->data;
            if(complete_list_iter->song)
            {
                GtkWidget *vbox = complete_list_iter->userdata;
                GtkWidget *item;
                int a,b;
                if(vbox == NULL){
                   vbox = create_button(self, (MpdData_real *)complete_list_iter);
                }
                else{
                    item = g_object_get_data(G_OBJECT(vbox), "item");
                    /* Resize if needed */
                    if(self->priv->album_size != gmpc_metaimage_get_size(GMPC_METAIMAGE(item))){
                        gtk_widget_set_size_request(vbox, self->priv->album_size+20,self->priv->album_size+40);
                        gmpc_metaimage_set_size(GMPC_METAIMAGE(item), self->priv->album_size);
                        gmpc_metaimage_reload_image(GMPC_METAIMAGE(item));
                    }

                }
                entries = g_list_prepend(entries, vbox);
                j++;
            }
            v_items++;
        }while(v_items < (rows*self->priv->supported_columns)&& (iter = iter->next));
    }
    /* remove list */
    if(list) g_list_free(list);
    list = NULL;

    for(iter = entries = g_list_reverse(entries); iter; iter = g_list_next(iter)){
        gtk_container_add(GTK_CONTAINER(self->priv->item_table), iter->data);
    }
    if(entries) g_list_free(entries);

    gtk_widget_show_all(self->priv->albumview_box);
    /**
     * Remove the timeout
     */
    if(self->priv->update_timeout)
        g_source_remove(self->priv->update_timeout);
    self->priv->update_timeout = 0;
    return FALSE;
}


void update_view(AlbumViewPlugin *self)
{
    if(self->priv->update_timeout != 0) {
        g_source_remove(self->priv->update_timeout);
    }
    self->priv->update_timeout = g_timeout_add(10, (GSourceFunc)update_view_real,self);
}

/**
 * Gobject plugin
 */
static void albumview_plugin_class_init (AlbumViewPluginClass *klass);

static int *albumview_plugin_get_version(GmpcPluginBase *plug, int *length)
{
	static int version[3] = {PLUGIN_MAJOR_VERSION,PLUGIN_MINOR_VERSION,PLUGIN_MICRO_VERSION};
	if(length) *length = 3;
	return (int *)version;
}

static const char *albumview_plugin_get_name(GmpcPluginBase *plug)
{
	return ("Album View");
}
static GObject *albumview_plugin_constructor(GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	AlbumViewPluginClass * klass;
	AlbumViewPlugin *self;
	GObjectClass * parent_class;
	klass = (g_type_class_peek (albumview_plugin_get_type()));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	self = (AlbumViewPlugin *) parent_class->constructor (type, n_construct_properties, construct_properties);

    g_log(AV_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Constructing plugin");

	/* setup private structure */
	self->priv = g_malloc0(sizeof(AlbumViewPluginPrivate));
    /* set defaults */
    self->priv->supported_rows = 1;
    self->priv->supported_columns = 1;
    self->priv->data = NULL;
    self->priv->album_size = 200;
    self->priv->filter_entry = NULL;
    self->priv->slider_scale = NULL;
    self->priv->progress_bar = NULL;
    self->priv->max_entries = 0;
    self->priv->current_entry = 0;
    self->priv->current_item = NULL;
    self->priv->update_timeout = 0;
    self->priv->complete_list = NULL;
    self->priv->item_table = NULL;
    self->priv->albumview_ref = NULL;
    self->priv->albumview_box = NULL;
    self->priv->require_scale_update = FALSE;

    /* Watch status changed signals */
    g_signal_connect_object(G_OBJECT(gmpcconn), "connection-changed", G_CALLBACK(albumview_connection_changed), self, 0);

    /* Setup textdomain */
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    GMPC_PLUGIN_BASE(self)->translation_domain = GETTEXT_PACKAGE;
	GMPC_PLUGIN_BASE(self)->plugin_type = GMPC_PLUGIN_NO_GUI;

    albumview_plugin_init(self);

	return G_OBJECT(self);
}
static void albumview_plugin_finalize(GObject *obj) {
    AlbumViewPlugin *self = (AlbumViewPlugin *)obj;
	AlbumViewPluginClass * klass = (g_type_class_peek (play_queue_plugin_get_type()));
	gpointer parent_class = g_type_class_peek_parent (klass);

    g_log(AV_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Destroying plugin");

    if(self->priv){
        if(self->priv->current_item) g_list_free(self->priv->current_item);
        self->priv->current_item = NULL;
        if(self->priv->complete_list) mpd_data_free(self->priv->complete_list);
        self->priv->complete_list = NULL;

        g_free(self->priv);
		self->priv = NULL;
	}
	if(parent_class)
		G_OBJECT_CLASS(parent_class)->finalize(obj);
}


static void albumview_plugin_class_init (AlbumViewPluginClass *klass)
{
	G_OBJECT_CLASS(klass)->finalize =		albumview_plugin_finalize;
	G_OBJECT_CLASS(klass)->constructor =	albumview_plugin_constructor;
	/* Connect plugin functions */
	GMPC_PLUGIN_BASE_CLASS(klass)->get_version = albumview_plugin_get_version;
	GMPC_PLUGIN_BASE_CLASS(klass)->get_name =	 albumview_plugin_get_name;

	GMPC_PLUGIN_BASE_CLASS(klass)->get_enabled = albumview_get_enabled;
	GMPC_PLUGIN_BASE_CLASS(klass)->set_enabled = albumview_set_enabled;

	GMPC_PLUGIN_BASE_CLASS(klass)->save_yourself = albumview_browser_save_myself;
}

static void albumview_plugin_browser_iface_init(GmpcPluginBrowserIfaceIface * iface) {
	iface->browser_add = albumview_add;
    iface->browser_selected = albumview_selected;
    iface->browser_unselected = albumview_unselected;
}

const GType albumview_plugin_get_type(void) {
	static GType albumview_plugin_type_id = 0;
	if(albumview_plugin_type_id == 0) {
		static const GTypeInfo info = {
			.class_size = sizeof(AlbumViewPluginClass),
			.class_init = (GClassInitFunc)albumview_plugin_class_init,
			.instance_size = sizeof(AlbumViewPlugin),
			.n_preallocs = 0
		};

		albumview_plugin_type_id = g_type_register_static(GMPC_PLUGIN_TYPE_BASE, "AlbumViewPlugin", &info, 0);

        /** Browser interface */
		static const GInterfaceInfo iface_info = { (GInterfaceInitFunc) albumview_plugin_browser_iface_init,
            (GInterfaceFinalizeFunc) NULL, NULL};
		g_type_add_interface_static (albumview_plugin_type_id, GMPC_PLUGIN_TYPE_BROWSER_IFACE, &iface_info);
	}
	return albumview_plugin_type_id;
}

G_MODULE_EXPORT GType plugin_get_type(void)
{
    return albumview_plugin_get_type();
}
