/* gmpc-lyrics (GMPC plugin)
 * Copyright (C) 2006-2009 Qball Cow <qball@sarine.nl>
 * Copyright (C) 2006 Maxime Petazzoni <maxime.petazzoni@bulix.org>
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

#include "gmpc-lyrics.h"

#define LYRICS_FROM "\n\nLyric from "
gmpcPlugin plugin;


static GtkWidget *lyrics_pref_vbox = NULL;
static GtkWidget *lyrics_pref_table = NULL;

static const gchar *lyrics_get_translation_domain(void);
static xmlGenericErrorFunc handler = NULL;

int lyrics_get_enabled();
void lyrics_set_enabled(int enabled);
/* Playlist window row reference */
int fetch_lyric(mpd_Song *song, MetaDataType type, char **path);

void fetch_lyric_uris(mpd_Song *song, MetaDataType type, void (*callback)(GList *list, void *data), void *user_data);

void xml_error_func(void * ctx, const char * msg, ...);
void setup_xml_error();
void destroy_xml_error(); 

/**
 * Used by gmpc to check against what version the plugin is compiled
 */
int plugin_api_version = PLUGIN_API_VERSION;

/**
 * Get plugin priority, the lower the sooner checked 
 */
static int fetch_priority()
{
	return cfg_get_single_value_as_int_with_default(config, "lyric-provider", "priority", 90);
}
static void set_priority(int priority)
{
	cfg_set_single_value_as_int(config, "lyric-provider", "priority", priority);
}

/**
 * Preferences structure
 */
gmpcPrefPlugin lyrics_gpp = {
	.construct  = lyrics_construct,
	.destroy    = lyrics_destroy
};

/**
 * Meta data structure 
 */
gmpcMetaDataPlugin lyric_fetch = {
	.get_priority   = fetch_priority,
    .set_priority   = set_priority,
    .get_metadata = fetch_lyric_uris
};

gmpcPlugin plugin = {
	/** Name */
	.name           = N_("Lyrics fetcher"),
	/** Version */
	.version        = {PLUGIN_MAJOR_VERSION,PLUGIN_MINOR_VERSION,PLUGIN_MICRO_VERSION},
	/** Plugin type */
	.plugin_type    = GMPC_PLUGIN_META_DATA,
	/** initialize function */
	.init           = &lyrics_init, 
	/** Preferences structure */
	.pref           = &lyrics_gpp,
	/** Meta data structure */
	.metadata       = &lyric_fetch,
	/** get_enabled()  */
	.get_enabled    = lyrics_get_enabled,
	/** set enabled() */
	.set_enabled    = lyrics_set_enabled,


    .get_translation_domain = lyrics_get_translation_domain
};

static int num_apis = 2;
static struct lyrics_api apis[] =
  {
    {N_("LeosLyrics"),
     "http://api.leoslyrics.com/",
     "api_search.php?auth=" PLUGIN_AUTH "&artist=%s&songtitle=%s",
     "api_search.php?auth=" PLUGIN_AUTH "&songtitle=%s",
     "api_lyrics.php?auth=" PLUGIN_AUTH "&hid=%s",
     __leoslyrics_get_id,
     __leoslyrics_get_lyrics,
    },
    {N_("Lyrics Tracker"),
     "http://lyrictracker.com/soap.php?cln=" PLUGIN_AUTH "&clv=undef",
     "&act=query&ar=%s&ti=%s",
     "&act=query&ti=%s",
     "&act=detail&id=%s",
     __lyrictracker_get_id,
     __lyrictracker_get_lyrics,
    },
    {NULL,NULL,NULL,NULL,NULL, NULL, NULL}
  };


typedef struct Query {
    mpd_Song *song;
    void (*callback)(GList *, gpointer );
    gpointer user_data;
    int index;
    int pref;
    int exact_match;
    GList *list;
}Query;


void fetch_query_iterate (Query *q);

void xml_error_func(void * ctx, const char * msg, ...) { } // yes.. do this much on error

void setup_xml_error() {
  if (handler==NULL)
  	handler = (xmlGenericErrorFunc)xml_error_func;
  initGenericErrorDefaultFunc(&handler);
}
void destroy_xml_error() {
  initGenericErrorDefaultFunc((xmlGenericErrorFunc *)NULL);
}

void lyrics_init (void)
{
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
}


static const gchar *lyrics_get_translation_domain(void)
{
    return GETTEXT_PACKAGE;
}

xmlNodePtr get_node_by_name (xmlNodePtr node, xmlChar *name)
{
  xmlNodePtr cur;

  for (cur = node; cur; cur = cur->next)
    if (xmlStrEqual(cur->name, name) &&
        (cur->type == XML_ELEMENT_NODE))
        return cur;

  return NULL;
}

static char * __lyrics_process_string(char *name)
{
	return gmpc_easy_download_uri_escape(name);//g_escape_uri_string(name);
}
static void fetch_query_lyrics_result(const GEADAsyncHandler *handle, GEADStatus status, gpointer user_data)
{
    Query *q = (Query *)user_data;
    if(status == GEAD_PROGRESS) return;
    if(status == GEAD_DONE) 
    {
        struct lyrics_api *api = &(apis[q->index]); 
        gchar *result = NULL;
        goffset size = 0;
        const gchar *data = gmpc_easy_handler_get_data(handle, &size);
        //gmpc_easy_download_struct dl = {(char *)data,(int)size,-1, NULL, NULL};
        result = api->get_lyrics(data, size);
        if(result)
        {
            MetaData *mtd = meta_data_new();
            printf("Found result: %s\n", api->name);
            mtd->type = META_SONG_TXT;
            mtd->plugin_name = plugin.name;
            mtd->content_type = META_DATA_CONTENT_TEXT;
            mtd->content = result;
            mtd->size= -1;
            if(q->index == q->pref)
                q->list = g_list_prepend(q->list,mtd); 
            else 
                q->list = g_list_append(q->list, mtd);
        }
    }
    q->index++;
    fetch_query_iterate (q);
}

static void fetch_query_search_result(const GEADAsyncHandler *handle, GEADStatus status, gpointer user_data)
{
    Query *q = (Query *)user_data;
    if(status == GEAD_PROGRESS) return;
    if(status == GEAD_DONE)
    {
        struct lyrics_api *api = &(apis[q->index]); 
        xmlDocPtr results_doc;
        goffset size = 0;
        xmlChar *hid = NULL;
        const gchar *data = gmpc_easy_handler_get_data(handle, &size);
        results_doc = xmlParseMemory(data, (int)size);
        /* Get song id */
        hid = (xmlChar *)api->get_id(results_doc, q->song->artist, q->song->title, q->exact_match);
        xmlFreeDoc(results_doc);
        if(hid && strlen((char *)hid) > 0)
        {
            gchar *temp = NULL, *lyrics_url = NULL;
            char *esc_hid = __lyrics_process_string((char *)hid);
            xmlFree(hid);
            hid = NULL;

            temp = g_strdup_printf("%s%s", api->host, api->lyrics_uri);
            lyrics_url = g_strdup_printf(temp, esc_hid);
            g_free(esc_hid);
            g_free(temp);

            if(gmpc_easy_async_downloader(lyrics_url,fetch_query_lyrics_result , q)!= NULL)
            {
                return;
            }
        }
        if(hid) xmlFree(hid);
    }
    q->index++;
    fetch_query_iterate (q);
}
void fetch_query_iterate (Query *q)
{
    printf("Itteration: %i\n", q->index);
    if(q->index < num_apis)
    {
        gchar *search_url;
        struct lyrics_api *api = &(apis[q->index]); 
        printf("Trying data %s\n", api->name);
        if (q->song->artist)
        {
            char *esc_artist = __lyrics_process_string(q->song->artist);
            char *esc_title = __lyrics_process_string(q->song->title);
            gchar *temp = g_strdup_printf("%s%s", api->host, api->search_full);
            search_url = g_strdup_printf(temp, esc_artist,esc_title);
            g_free(esc_artist);
            g_free(esc_title);
            g_free(temp);
        }
        else
        {
            char *esc_title = __lyrics_process_string(q->song->title);
            gchar *temp = g_strdup_printf("%s%s", api->host, api->search_title);
            search_url = g_strdup_printf(temp, esc_title);
            g_free(temp);
            g_free(esc_title);
        }

        if(gmpc_easy_async_downloader(search_url,fetch_query_search_result , q) != NULL)
        {
            g_free(search_url);
            return;
        }
        q->index++;
        g_free(search_url);
        fetch_query_iterate(q);
        return;
    }
    /* Return results */
    printf("Return lyrics api v2\n");
    q->callback(q->list,q->user_data);
    g_free(q);
}

void fetch_lyric_uris(mpd_Song *song, MetaDataType type, void (*callback)(GList *list, void *data), void *user_data)
{

    printf("lyrics api v2\n");
    if(song->title != NULL && type == META_SONG_TXT)
    {
        Query *q = g_malloc0(sizeof(*q));
        q->callback = callback;
        q->user_data = user_data;
        q->index = 0;
        q->song = song;
        q->pref =cfg_get_single_value_as_int_with_default(config, "lyrics-plugin", "api-id", 0);
        q->exact_match = cfg_get_single_value_as_int_with_default(config, "lyrics-plugin", "exact-match", 1);
        q->list = NULL;

        fetch_query_iterate(q);

        return;
    }
    callback(NULL, user_data);
}

/** Called when enabling the plugin from the preferences dialog. Set
 * the configuration so that we'll know that the plugin is enabled
 * afterwards.
 */

static void lyrics_match_toggle (GtkWidget *wid)
{
	int match = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid));
	cfg_set_single_value_as_int(config, "lyrics-plugin", "exact-match", match);
}


/** Called when the user changes the lyrics API. Set a configuration
 * value to the new API id.
 */
void lyrics_api_changed (GtkWidget *wid)
{
	int id = gtk_combo_box_get_active(GTK_COMBO_BOX(wid));
	cfg_set_single_value_as_int(config, "lyrics-plugin", "api-id", id);

	debug_printf(DEBUG_INFO, "Saved API ID: %d\n", cfg_get_single_value_as_int_with_default(config, "lyrics-plugin", "api-id", 0));
}

void lyrics_destroy (GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), lyrics_pref_vbox);
}

/** 
 * Initialize GTK widgets for the preferences window.
 */
void lyrics_construct (GtkWidget *container)
{
	GtkWidget *label, *combo, *match;
	int i;

	label = gtk_label_new(_("Preferred lyric site :"));
	combo = gtk_combo_box_new_text();
	match = gtk_check_button_new_with_mnemonic(_("Exact _match only"));

	lyrics_pref_table = gtk_table_new(2, 2, FALSE);
	lyrics_pref_vbox = gtk_vbox_new(FALSE,6);

	for (i=0; apis[i].name ; i++)
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _(apis[i].name));

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo),
			cfg_get_single_value_as_int_with_default(config, "lyrics-plugin", "api-id", 0));

	gtk_table_attach_defaults(GTK_TABLE(lyrics_pref_table), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(lyrics_pref_table), combo, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(lyrics_pref_table), match, 0, 2, 1, 2);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(match),
				cfg_get_single_value_as_int_with_default(config, "lyrics-plugin", "exact-match", 1));

	gtk_widget_set_sensitive(lyrics_pref_table, cfg_get_single_value_as_int_with_default(config, "lyrics-plugin", "enable", 0));

	/* TODO: check that this stuff actually works */
	//gtk_widget_set_sensitive(match, 0);

	/* Connect signals */
	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(lyrics_api_changed), NULL);
	g_signal_connect(G_OBJECT(match), "toggled", G_CALLBACK(lyrics_match_toggle), NULL);

	gtk_box_pack_start(GTK_BOX(lyrics_pref_vbox), lyrics_pref_table, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(container), lyrics_pref_vbox);
	gtk_widget_show_all(container);
}

int lyrics_get_enabled()
{
	return 	cfg_get_single_value_as_int_with_default(config, "lyrics-plugin", "enable", 0);
}

void lyrics_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "lyrics-plugin", "enable", enabled);
}
