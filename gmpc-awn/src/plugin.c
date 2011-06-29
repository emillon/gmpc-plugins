/* gmpc-awn (GMPC plugin)
 * Copyright (C) 2007-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkx.h>
#include <gmpc/plugin.h>
#include <gmpc/metadata.h>
#include <gmpc/misc.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

static void awn_update_cover(GmpcMetaWatcher *gmv, mpd_Song *song, MetaDataType type, MetaDataResult ret, MetaData *met); 
static int awn_get_enabled(void)
{
    return cfg_get_single_value_as_int_with_default(config, "awn-plugin", "enable", TRUE);
}
static void awn_set_enabled(int enabled)
{
    cfg_set_single_value_as_int(config, "awn-plugin", "enable", enabled);
}

/* from gseal-transition.h */
#ifndef GSEAL
#define gtk_widget_get_window(x) (x)->window
#endif

static XID get_main_window_xid() {
    GtkWidget* window = NULL;

    window = playlist3_get_window();

    if (!window || playlist3_window_is_hidden()) {
        return None;
    }

    return GDK_WINDOW_XID(gtk_widget_get_window(window));
}

static void setAwnIcon(const char *filename) {
    XID xid;
    DBusGConnection *connection;
    DBusGProxy *proxy;
    GError *error;	

    xid = get_main_window_xid();

    if (xid == None) {
        return;
    }

    error = NULL;
    connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

    if (connection != NULL)
    {
        proxy = dbus_g_proxy_new_for_name(
                connection,
                "com.google.code.Awn",
                "/com/google/code/Awn",
                "com.google.code.Awn");
        error = NULL;
        dbus_g_proxy_call(proxy, "SetTaskIconByXid", &error, G_TYPE_INT64,
                (gint64)xid, G_TYPE_STRING, filename, G_TYPE_INVALID);
    }
}

static void unsetAwnIcon() {
    XID xid;
    DBusGConnection *connection;
    DBusGProxy *proxy;
    GError *error;	

    xid = get_main_window_xid();

    if (xid == None) {
        return;
    }

    error = NULL;
    connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

    if (connection != NULL)
    {
        proxy = dbus_g_proxy_new_for_name(
                connection,
                "com.google.code.Awn",
                "/com/google/code/Awn",
                "com.google.code.Awn");
        error = NULL;
        dbus_g_proxy_call(proxy, "UnsetTaskIconByXid", &error,
                G_TYPE_INT64, (gint64)xid, G_TYPE_INVALID);
    }
}
static void setAwnProgress(int progress) {
    XID xid;
    DBusGConnection *connection;
    DBusGProxy *proxy;
    GError *error;	

    xid = get_main_window_xid();

    if (xid == None) {
        return;
    }

    error = NULL;
    connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

    if (connection != NULL)
    {
        proxy = dbus_g_proxy_new_for_name(
                connection,
                "com.google.code.Awn",
                "/com/google/code/Awn",
                "com.google.code.Awn");
        error = NULL;
        dbus_g_proxy_call(proxy, "SetProgressByXid", &error,G_TYPE_INT64, (gint64)xid,
                G_TYPE_INT, progress, G_TYPE_INVALID);
    }
}
/**
 * Destroy the plugin
 */
static void awn_plugin_destroy(void)
{

}


static void awn_plugin_init(void)
{
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    g_signal_connect(G_OBJECT(gmw), "data-changed", G_CALLBACK(awn_update_cover), NULL);
}

static void awn_update_cover(GmpcMetaWatcher *gmv, mpd_Song *song, MetaDataType type, MetaDataResult ret, MetaData *met) 
{
    mpd_Song  *song2;
    if(!awn_get_enabled())
        return;

    song2 = mpd_playlist_get_current_song(connection);

    unsetAwnIcon();
    if(!song2 || type != META_ALBUM_ART || !gmpc_meta_watcher_match_data(META_ALBUM_ART, song2, song))
        return;
    if(ret == META_DATA_AVAILABLE) {
        if(met->content_type == META_DATA_CONTENT_URI)
        {
            const gchar *path = meta_data_get_uri(met);
            setAwnIcon(path);
        }
    }
}



static void awn_song_changed(MpdObj *mi)
{
    MetaDataResult ret;
    MetaData *met = NULL;
    mpd_Song *song = NULL;

    song = mpd_playlist_get_current_song(mi);
    ret = gmpc_meta_watcher_get_meta_path(gmw,song, META_ALBUM_ART,&met);
    awn_update_cover(gmw, song, META_ALBUM_ART, ret, met);
    if(met)
        meta_data_free(met);
}

/* mpd changed */
static void   awn_mpd_status_changed(MpdObj *mi, ChangedStatusType what, void *data)
{
    if(!awn_get_enabled())
        return;
    if(what&(MPD_CST_SONGID))
        awn_song_changed(mi);
    if(what&(MPD_CST_ELAPSED_TIME|MPD_CST_TOTAL_TIME))
    {

        int totalTime = mpd_status_get_total_song_time(connection);                                 		
        int elapsedTime = mpd_status_get_elapsed_song_time(connection);	
        gdouble progress = elapsedTime/(gdouble)MAX(totalTime,1)*100;
        if((int)progress == 0)
            progress = 100;
        setAwnProgress((int)progress);
    }
}

static const gchar* awn_get_translation_domain(void) {
    return GETTEXT_PACKAGE;
}

int plugin_api_version = PLUGIN_API_VERSION;
/* main plugin_awn info */
gmpcPlugin plugin = {
    .name 					= N_("Avant Window Navigator Plugin"),
    .version 				= { MAJOR_VERSION, MINOR_VERSION, MICRO_VERSION },
    .plugin_type 			= GMPC_PLUGIN_NO_GUI,
    .init 					= awn_plugin_init,
    .destroy 				= awn_plugin_destroy,
    .mpd_status_changed 	= &awn_mpd_status_changed,
    .get_enabled 			= awn_get_enabled,
    .set_enabled 			= awn_set_enabled,
    .get_translation_domain = awn_get_translation_domain
};
