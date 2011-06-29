/* gmpc-extraplaylist (GMPC plugin)
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

#include <string.h>
#include <glade/glade.h>
#include <gmpc/plugin.h>
#include <gmpc/gmpc-extras.h>
#include <config.h>
/* External pointer + function, there internal from gmpc */
extern GladeXML *pl3_xml;
GtkWidget *extraplaylist = NULL;
static GtkWidget *extraplaylist_paned = NULL;
static GmpcPluginBase *play_queue_plugin = NULL;

static void extra_playlist_add();

/* Hack to be able to create a copy of the play-queue plugin */
GObject * play_queue_plugin_new(const gchar *uid);

static void extra_playlist_destroy() {
	if(extraplaylist) {
		int pos = gtk_paned_get_position(GTK_PANED(extraplaylist_paned));
		printf("pos is: %i\n",pos);
		if(pos>0)
			cfg_set_single_value_as_int(config, "extraplaylist", "paned-pos", pos);
	}
}

static int get_enabled() {
	return cfg_get_single_value_as_int_with_default(config,"extraplaylist", "enabled", 0); 
}
typedef struct _gmpcPluginParent {
    gmpcPlugin *old;
    GmpcPluginBase *new;
}_gmpcPluginParent;
static void ep_view_changed(GtkTreeSelection *selection, gpointer user_data)
{
    GtkTreeModel *model= NULL;
    GtkTreeIter iter;
    GtkTreeView *tree = playlist3_get_category_tree_view();
    if(gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        int type = 0;
        _gmpcPluginParent *parent = NULL;
        gtk_tree_model_get(model, &iter, PL3_CAT_TYPE, &type, -1);
        parent = (_gmpcPluginParent *)plugin_get_from_id(type);
        if(parent)
        {
           if(parent->new){
                printf("%i %i\n", G_OBJECT_TYPE(parent->new), G_OBJECT_TYPE(play_queue_plugin));
                if(G_OBJECT_TYPE(parent->new) == G_OBJECT_TYPE(play_queue_plugin))
                {
                     if(extraplaylist){
                        gtk_widget_hide(extraplaylist);
                        if(gtk_bin_get_child(GTK_BIN(extraplaylist)))
                            gmpc_plugin_browser_iface_browser_unselected(GMPC_PLUGIN_BROWSER_IFACE(play_queue_plugin),extraplaylist); 
                    }
                    return;
                }
           }
           {
               gboolean enable = get_enabled();
               if(enable)
               {
                   if(!extraplaylist) {
                       extra_playlist_add();
                   }else{
                       if(gtk_bin_get_child(GTK_BIN(extraplaylist)) == NULL){
                           gmpc_plugin_browser_iface_browser_selected(GMPC_PLUGIN_BROWSER_IFACE(play_queue_plugin), extraplaylist);
                           gtk_widget_show(extraplaylist);
                       }
                   }

               }else if(extraplaylist){
                   gtk_widget_hide(extraplaylist);
                   if(gtk_bin_get_child(GTK_BIN(extraplaylist)))
                       gmpc_plugin_browser_iface_browser_unselected(GMPC_PLUGIN_BROWSER_IFACE(play_queue_plugin),extraplaylist); 
               }
           }
        }
    }
}


static void extra_playlist_add() {

    GtkWidget *temp = NULL;
	if(pl3_xml == NULL) return;

	/**
	 * Hack it into the main view
	 */
    extraplaylist = gtk_event_box_new();
    /* Set border to fits gmpc's standard  */
    gtk_container_set_border_width(GTK_CONTAINER(extraplaylist), 0);

	temp = glade_xml_get_widget(pl3_xml, "vbox7");
	g_object_ref(temp);
    if(cfg_get_single_value_as_int_with_default(config, "extraplaylist", "vertical-layout", TRUE))
    {
        extraplaylist_paned = gtk_vpaned_new();
    }else{
        extraplaylist_paned = gtk_hpaned_new();
    }
	gtk_container_remove(GTK_CONTAINER(glade_xml_get_widget(pl3_xml,"hpaned1")),temp);



    if(!cfg_get_single_value_as_int_with_default(config, "extraplaylist", "vertical-layout-swapped",FALSE))
    {
        gtk_paned_pack1(GTK_PANED(extraplaylist_paned), temp, TRUE, TRUE); 
        gtk_paned_pack2(GTK_PANED(extraplaylist_paned), extraplaylist, TRUE, TRUE); 
    }else{
        gtk_paned_pack2(GTK_PANED(extraplaylist_paned), temp, TRUE, TRUE); 
        gtk_paned_pack1(GTK_PANED(extraplaylist_paned), extraplaylist, TRUE, TRUE); 
    }

//    gtk_box_pack_start(GTK_BOX(glade_xml_get_widget(pl3_xml, "vbox_control")), extraplaylist_paned, TRUE, TRUE, 0);
    gtk_paned_pack2(GTK_PANED(glade_xml_get_widget(pl3_xml, "hpaned1")), extraplaylist_paned,TRUE, TRUE);//, TRUE, TRUE, 0);

	gtk_paned_set_position(GTK_PANED(extraplaylist_paned),cfg_get_single_value_as_int_with_default(config, "extraplaylist", "paned-pos", 400));

	gtk_widget_show(extraplaylist_paned);
    gtk_widget_hide(extraplaylist);

    if(play_queue_plugin == NULL) {
        play_queue_plugin = (GmpcPluginBase *)play_queue_plugin_new("extra-playlist-plugin");
    }
    gmpc_plugin_browser_iface_browser_selected(GMPC_PLUGIN_BROWSER_IFACE(play_queue_plugin), extraplaylist);
    gtk_widget_show(extraplaylist);

    /* Attach changed signal */
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(playlist3_get_category_tree_view())), "changed",
        G_CALLBACK(ep_view_changed), NULL);
}


static void extra_playlist_init(void ) {
	if( cfg_get_single_value_as_int_with_default(config,"extraplaylist", "enabled", 1)) {
		gtk_init_add((GtkFunction )extra_playlist_add, NULL);
	}
}
static void set_enabled(int enable) {
    cfg_set_single_value_as_int(config,"extraplaylist", "enabled", enable);
    if(enable)
	{
		if(!extraplaylist) {
			extra_playlist_add();
		}else{
            if(gtk_bin_get_child(GTK_BIN(extraplaylist)) == NULL){
                gmpc_plugin_browser_iface_browser_selected(GMPC_PLUGIN_BROWSER_IFACE(play_queue_plugin), extraplaylist);
                gtk_widget_show(extraplaylist);
            }
        }

    } else if(extraplaylist){
		gtk_widget_hide(extraplaylist);
        if(gtk_bin_get_child(GTK_BIN(extraplaylist)))
            gmpc_plugin_browser_iface_browser_unselected(GMPC_PLUGIN_BROWSER_IFACE(play_queue_plugin),extraplaylist); 
	}
    pl3_tool_menu_update();
}

static void extra_playlist_enable_tool_menu(GtkCheckMenuItem *item)
{
    gboolean state = gtk_check_menu_item_get_active(item);
    set_enabled(state);
}
static int extraplaylist_add_tool_menu(GmpcPluginToolMenuIface *plug, GtkMenu *menu)
{
    GtkWidget *item = NULL;

    item = gtk_check_menu_item_new_with_label("Extra Playlist");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), get_enabled());

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_add_accelerator(GTK_WIDGET(item), "activate", gtk_menu_get_accel_group(GTK_MENU(menu)), GDK_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    g_signal_connect(G_OBJECT(item), "activate", 
            G_CALLBACK(extra_playlist_enable_tool_menu), NULL);
    return 1;
}

/**
 * Preferences 
 */
static void preferences_layout_changed(GtkToggleButton *but, gpointer user_data)
{
    gint active = gtk_toggle_button_get_active(but);
    cfg_set_single_value_as_int(config, "extraplaylist", "vertical-layout", active);
}

static void preferences_layout_swapped_changed(GtkToggleButton *but, gpointer user_data)
{
    gint active = gtk_toggle_button_get_active(but);
    cfg_set_single_value_as_int(config, "extraplaylist", "vertical-layout-swapped", active);
}
static  void preferences_construct(GtkWidget *container)
{
    GtkWidget *vbox = gtk_vbox_new(FALSE, 6);
    GtkWidget *label = NULL;

    /* Warning */
    label = gtk_label_new("These settings require a restart to apply.");
    gtk_misc_set_alignment(GTK_MISC(label),0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    /* The checkbox */
    label = gtk_check_button_new_with_label("Use horizontal layout");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label), cfg_get_single_value_as_int_with_default(config, "extraplaylist", "vertical-layout", TRUE));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(label), "toggled", G_CALLBACK(preferences_layout_changed), NULL);

    /* The checkbox */
    label = gtk_check_button_new_with_label("Swap position of the extra playlist");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label), cfg_get_single_value_as_int_with_default(config, "extraplaylist", "vertical-layout-swapped", FALSE));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(label), "toggled", G_CALLBACK(preferences_layout_swapped_changed), NULL);
   
    /* show and add */
    gtk_widget_show_all(vbox);
    gtk_container_add(GTK_CONTAINER(container), vbox);

}
static void preferences_destroy(GtkWidget *container)
{
    gtk_container_remove(GTK_CONTAINER(container), gtk_bin_get_child(GTK_BIN(container)));

}
 gmpcPrefPlugin extra_playlist_preferences = {
    .construct = preferences_construct,
    .destroy = preferences_destroy

 };

gmpcPlugin plugin = {
	.name = "Extra Playlist View",
	.version = {PLUGIN_MAJOR_VERSION, PLUGIN_MINOR_VERSION, PLUGIN_MICRO_VERSION},
	.plugin_type = GMPC_PLUGIN_NO_GUI,
	.init = extra_playlist_init,            /* initialization */
	.destroy = extra_playlist_destroy,         /* Destroy */
	.get_enabled = get_enabled,
	.set_enabled = set_enabled,

    .pref = &extra_playlist_preferences,

    .tool_menu_integration = extraplaylist_add_tool_menu
};
int plugin_api_version = PLUGIN_API_VERSION;
