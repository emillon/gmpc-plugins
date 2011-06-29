/* gmpc-fullscreen_info (GMPC plugin)
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
#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gmpc/plugin.h>
#include <gmpc-metaimage.h>
#include <gmpc-extras.h>
#include <gdk/gdkkeysyms.h>
#include <config.h>

typedef struct _FullscreenInfoPlugin
{
        GmpcPluginBase  parent_instance;
} FullscreenInfoPlugin ;

typedef struct _FullscreenInfoPluginClass
{
        GmpcPluginBaseClass parent_class;
} FullscreenInfoPluginClass;

extern GmpcConnection *gmpcconn;

gulong signal_handler = 0;

GtkWidget *buttons[4] = {NULL, };
GtkWidget *label = NULL;
/**
 * Get/Set enabled functions.
 */
static gboolean fullscreen_info_get_enabled(GmpcPluginBase *plug)
{
	return cfg_get_single_value_as_int_with_default(config, "fullscreen_info-plugin", "enable", TRUE);
}
static void fullscreen_info_set_enabled(GmpcPluginBase *plug, int enabled)
{
	cfg_set_single_value_as_int(config, "fullscreen_info-plugin", "enable", enabled);

    pl3_tool_menu_update();
}


static void gfi_button_press_event(GtkWidget *widevent, GdkEventButton *event, GtkWidget *window)
{
    gtk_widget_destroy(window);
    if(signal_handler)
        g_signal_handler_disconnect(G_OBJECT(gmpcconn), signal_handler);
    signal_handler = 0;
}
static gboolean gfi_window_fullscreen(GtkWidget *window)
{
    gtk_window_fullscreen(GTK_WINDOW(window));
    return FALSE;
}
static void gfi_size_allocate(GtkWidget *window, GtkAllocation *alloc, GtkWidget *image)
{
    g_idle_add((GSourceFunc)gfi_window_fullscreen, window);
}

static void gfi_size_allocate_mi(GtkWidget *window, GtkAllocation *alloc, GtkWidget *image){
    int size = (alloc->width < alloc->height)?alloc->width:alloc->height;
    printf("Size allocate mi: %i\n", size);
    if(size != gmpc_metaimage_get_size(GMPC_METAIMAGE(image)))
    {
        gmpc_metaimage_set_size(GMPC_METAIMAGE(image),size);
        gmpc_metaimage_reload_image(GMPC_METAIMAGE(image));
    }
}
static void play_pause(MpdObj *mi)
{
    if(mpd_player_get_state(mi) == MPD_PLAYER_PLAY){
        mpd_player_pause(mi);
    }else{
        mpd_player_play(mi);
    }
}
static void status_changed(GmpcConnection *gmpcconn, MpdObj *mi, ChangedStatusType what, gpointer data)
{
    if(what&MPD_CST_STATE){
        GtkWidget *child = gtk_bin_get_child(GTK_BIN(buttons[2]));

        if(mpd_player_get_state(connection) == MPD_PLAYER_PLAY){
            gtk_image_set_from_stock(GTK_IMAGE(child),GTK_STOCK_MEDIA_PAUSE,GTK_ICON_SIZE_DIALOG); 
        }else{
            gtk_image_set_from_stock(GTK_IMAGE(child),GTK_STOCK_MEDIA_PLAY,GTK_ICON_SIZE_DIALOG); 
        }
    }
    if(what&(MPD_CST_SONGID|MPD_CST_SONGPOS|MPD_CST_PLAYLIST|MPD_CST_STATE))
    {
        char buffer[512];
        mpd_Song *song = mpd_playlist_get_current_song(mi);
        if(song && mpd_player_get_state(mi) != MPD_PLAYER_STOP)
        {
            mpd_song_markup_escaped(buffer, 512, C_("header title markup","<span size='xx-large'>[[%title% - &[%artist%]]|%shortfile%]</span>"),song);
        }
        else{
            snprintf(buffer, 512, "<span size='xx-large'>%s</span>", _("Gnome Music Player Client"));
        }
        gtk_label_set_markup(GTK_LABEL(label), buffer);
    }
}

static void gfi_fullscreen(GmpcPluginBase *plu)
{
    if(fullscreen_info_get_enabled(plu) == FALSE) return;
    gchar *temp;
    GtkWidget *popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *metaimage = gmpc_metaimage_new(META_ALBUM_ART); 
    GtkWidget *vbox = gtk_vbox_new(FALSE, 6);
    GtkWidget *hbox = gtk_hbox_new(TRUE, 6);
    gtk_widget_set_has_tooltip(metaimage, FALSE);

    gtk_window_set_keep_above(GTK_WINDOW(popup), TRUE);
    /* Setup the meta image */
    gmpc_metaimage_set_no_cover_icon(GMPC_METAIMAGE(metaimage),(char *)"gmpc");
    gmpc_metaimage_set_squared(GMPC_METAIMAGE(metaimage), FALSE);
	gmpc_metaimage_set_connection(GMPC_METAIMAGE(metaimage), connection);
    gmpc_metaimage_update_cover(GMPC_METAIMAGE(metaimage), connection,MPD_CST_SONGID, NULL);
    g_signal_connect(G_OBJECT(metaimage), "button-press-event", G_CALLBACK(gfi_button_press_event), popup);


    gtk_container_add(GTK_CONTAINER(popup), vbox);
    gtk_window_set_resizable(GTK_WINDOW(popup), TRUE);

    g_signal_connect(G_OBJECT(popup), "size-allocate", G_CALLBACK(gfi_size_allocate), metaimage);

    g_signal_connect(G_OBJECT(metaimage), "size-allocate", G_CALLBACK(gfi_size_allocate_mi), metaimage);


    /**
     * Main title label 
     */
    label = gtk_label_new("");
    temp = g_markup_printf_escaped("<span size='xx-large'>%s</span>", _("Gnome Music Player Client"));
    gtk_label_set_markup(GTK_LABEL(label), temp);
    g_free(temp);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_ensure_style(label);
    gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &(label->style->white));

    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE,0);

    gtk_box_pack_start(GTK_BOX(vbox), metaimage, TRUE, TRUE,  0);
    /**
     * Control buttons */
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE,0);



    /* Prev */
    buttons[0] = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(buttons[0]),
            gtk_image_new_from_stock(GTK_STOCK_MEDIA_PREVIOUS, GTK_ICON_SIZE_DIALOG));
    gtk_button_set_relief(GTK_BUTTON(buttons[0]), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(hbox), buttons[0], FALSE, TRUE, 0);
    g_signal_connect_swapped(G_OBJECT(buttons[0]), "clicked", G_CALLBACK(mpd_player_prev), connection);
    /* STOP */
    buttons[1] = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(buttons[1]),
            gtk_image_new_from_stock(GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_DIALOG));
    gtk_button_set_relief(GTK_BUTTON(buttons[1]), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(hbox), buttons[1], FALSE, TRUE, 0);

    g_signal_connect_swapped(G_OBJECT(buttons[1]), "clicked", G_CALLBACK(mpd_player_stop), connection);

    /* Play/Pause */
    buttons[2] = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(buttons[2]),
            gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_DIALOG));
    gtk_button_set_relief(GTK_BUTTON(buttons[2]), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(hbox), buttons[2], FALSE, TRUE, 0);

    g_signal_connect_swapped(G_OBJECT(buttons[2]), "clicked", G_CALLBACK(play_pause), connection);

    /* Next */
    buttons[3] = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(buttons[3]),
            gtk_image_new_from_stock(GTK_STOCK_MEDIA_NEXT, GTK_ICON_SIZE_DIALOG));
    gtk_button_set_relief(GTK_BUTTON(buttons[3]), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(hbox), buttons[3], FALSE, TRUE, 0);

    g_signal_connect_swapped(G_OBJECT(buttons[3]), "clicked", G_CALLBACK(mpd_player_next), connection);

    gtk_widget_ensure_style(popup);
    gtk_widget_modify_bg(popup, GTK_STATE_NORMAL, &(popup->style->black));


    status_changed(gmpcconn, connection, MPD_CST_STATE|MPD_CST_SONGPOS, NULL);
    signal_handler = g_signal_connect(G_OBJECT(gmpcconn), "status-changed", G_CALLBACK(status_changed), NULL);
    gtk_widget_show_all(popup);
}

static int gfi_add_tool_menu(GmpcPluginToolMenuIface *plug, GtkMenu *menu)
{
    GtkWidget *item = NULL;
    if(fullscreen_info_get_enabled(GMPC_PLUGIN_BASE(plug)) == FALSE) return 0;

    item = gtk_image_menu_item_new_with_label(_("Fullscreen Info"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
            gtk_image_new_from_icon_name("playlist-browser", GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_add_accelerator(GTK_WIDGET(item), "activate", gtk_menu_get_accel_group(GTK_MENU(menu)), GDK_F11, 0, GTK_ACCEL_VISIBLE);
    g_signal_connect_swapped(G_OBJECT(item), "activate", 
            G_CALLBACK(gfi_fullscreen), plug);
    return 1;
}

/**
 * Gobject plugin
 */
static void fullscreen_info_plugin_class_init (FullscreenInfoPluginClass *klass);
GType fullscreen_info_plugin_get_type(void);

static int *fullscreen_info_plugin_get_version(GmpcPluginBase *plug, int *length)
{
	static int version[3] = {PLUGIN_MAJOR_VERSION,PLUGIN_MINOR_VERSION,PLUGIN_MICRO_VERSION};
	if(length) *length = 3;
	return (int *)version;
}

static const char *fullscreen_info_plugin_get_name(GmpcPluginBase *plug)
{
	return N_("Fullscreen Info");
}
static GObject *fullscreen_info_plugin_constructor(GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	FullscreenInfoPluginClass * klass;
	FullscreenInfoPlugin *self;
	GObjectClass * parent_class;
	klass = (g_type_class_peek (fullscreen_info_plugin_get_type()));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	self = (FullscreenInfoPlugin *) parent_class->constructor (type, n_construct_properties, construct_properties);

    /* Setup textdomain */
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	/* Make it an internal plugin */
    GMPC_PLUGIN_BASE(self)->translation_domain = GETTEXT_PACKAGE;
	GMPC_PLUGIN_BASE(self)->plugin_type = GMPC_PLUGIN_NO_GUI;

	return G_OBJECT(self);
}


static void fullscreen_info_plugin_class_init (FullscreenInfoPluginClass *klass)
{
	G_OBJECT_CLASS(klass)->constructor =	fullscreen_info_plugin_constructor;
	/* Connect plugin functions */
	GMPC_PLUGIN_BASE_CLASS(klass)->get_version = fullscreen_info_plugin_get_version;
	GMPC_PLUGIN_BASE_CLASS(klass)->get_name =	 fullscreen_info_plugin_get_name;

	GMPC_PLUGIN_BASE_CLASS(klass)->get_enabled = fullscreen_info_get_enabled;
	GMPC_PLUGIN_BASE_CLASS(klass)->set_enabled = fullscreen_info_set_enabled;
}

static void fullscreen_info_plugin_gmpc_plugin_tool_menu_iface_interface_init (GmpcPluginToolMenuIfaceIface * iface) {
	iface->tool_menu_integration = gfi_add_tool_menu;
}

GType fullscreen_info_plugin_get_type(void) {
	static GType fullscreen_info_plugin_type_id = 0;
	if(fullscreen_info_plugin_type_id == 0) {
		static const GTypeInfo info = {
			.class_size = sizeof(FullscreenInfoPluginClass),
			.class_init = (GClassInitFunc)fullscreen_info_plugin_class_init,
			.instance_size = sizeof(FullscreenInfoPlugin),
			.n_preallocs = 0
		};

		static const GInterfaceInfo iface_info = { (GInterfaceInitFunc) fullscreen_info_plugin_gmpc_plugin_tool_menu_iface_interface_init, 
            (GInterfaceFinalizeFunc) NULL, NULL};

		fullscreen_info_plugin_type_id = g_type_register_static(GMPC_PLUGIN_TYPE_BASE, "FullscreenInfosPlugin", &info, 0);

		g_type_add_interface_static (fullscreen_info_plugin_type_id, GMPC_PLUGIN_TYPE_TOOL_MENU_IFACE, &iface_info);
	}
	return fullscreen_info_plugin_type_id;
}

G_MODULE_EXPORT GType plugin_get_type(void)
{
    return fullscreen_info_plugin_get_type();
}
