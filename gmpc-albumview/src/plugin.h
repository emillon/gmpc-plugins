#ifndef __SERVERSTATS_PLUGIN_H__
#define __SERVERSTATS_PLUGIN_H__

typedef struct _AlbumViewPluginPrivate AlbumViewPluginPrivate;

typedef struct _AlbumViewPlugin
{
        GmpcPluginBase  parent_instance;
        AlbumViewPluginPrivate *priv;
} AlbumViewPlugin ;

typedef struct _AlbumViewPluginClass
{
        GmpcPluginBaseClass parent_class;
} AlbumViewPluginClass;

static void albumview_add(GmpcPluginBrowserIface *plug, GtkWidget *category_tree);

void albumview_connection_changed(GmpcConnection *conn, MpdObj *mi, int connect,void *usedata);
#endif
