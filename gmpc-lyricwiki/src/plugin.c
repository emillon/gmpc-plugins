/* gmpc-lyricwiki (GMPC plugin)
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

#include <stdio.h>
#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libmpd/debug_printf.h>
#include <gmpc/gmpc_easy_download.h>
#include <gmpc/plugin.h>

gmpcPlugin plugin;
/**
 * Get/Set enabled 
 */
static int lyricwiki_get_enabled()
{
	return cfg_get_single_value_as_int_with_default(config, "lyricwiki-plugin", "enable", TRUE);
}
static void lyricwiki_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "lyricwiki-plugin", "enable", enabled);
}

/* Get priority */
static int lyricwiki_fetch_cover_priority(void){
	return cfg_get_single_value_as_int_with_default(config, "lyricwiki-plugin", "priority", 80);
}
static void lyricwiki_fetch_cover_priority_set(int priority){
	cfg_set_single_value_as_int(config, "lyricwiki-plugin", "priority", priority);
}
typedef struct Query {
    mpd_Song *song;
    void (*callback)(GList *, gpointer);
    gpointer user_data;
}Query;
/**
 * Website scraper.. fun
 */
static void lyricwiki_download_callback2(const GEADAsyncHandler *handle, GEADStatus status, gpointer data)
{
    Query *q = (Query *)data;
    GList *list = NULL;
    if(status == GEAD_PROGRESS) return;
    if(status == GEAD_DONE) 
    {
        goffset size= 0;
        const char *data = gmpc_easy_handler_get_data(handle, &size);
        const gchar *url = gmpc_easy_handler_get_uri(handle);
        xmlDocPtr doc = htmlReadMemory(data, (int)size,url, "utf-8", HTML_PARSE_RECOVER|HTML_PARSE_NONET);
        if(doc)
        {        
            xmlXPathContextPtr xpathCtxt = NULL;
            xmlXPathObjectPtr xpathObj = NULL;
            xmlNodePtr node = NULL;
            xpathCtxt = xmlXPathNewContext(doc);
            if(!xpathCtxt) goto error;

            xpathObj = xmlXPathEvalExpression("//*[@id=\"wpTextbox1\"]", xpathCtxt);
            if(!xpathObj){
                printf("failed to evaluate xpath\n");
                goto error;
            }
            if(!xpathObj->nodesetval->nodeMax){
                printf("!xpathObj->nodesetval->nodeMax failed\n");
                goto error;
            }

            node = xpathObj->nodesetval->nodeTab[0];
error:
            if (xpathObj) xmlXPathFreeObject(xpathObj);
            if (xpathCtxt) xmlXPathFreeContext(xpathCtxt);
            if(node) {
                    xmlChar *lyric = xmlNodeGetContent(node);
                    if(lyric)
                    {
                        GMatchInfo *match_info;
                        GRegex *reg = g_regex_new("<(lyrics?)>(.*)</\\1>", G_REGEX_MULTILINE|G_REGEX_DOTALL,0, NULL);
                        g_regex_match(reg, lyric,G_REGEX_MATCH_NEWLINE_ANY, &match_info);
                        while (g_match_info_matches (match_info))
                        {
                            gchar *word = g_match_info_fetch (match_info, 2);
                            if(g_utf8_collate(word, "\n\n<!-- PUT LYRICS HERE (and delete this entire line) -->\n\n")!= 0)
                            {
                                MetaData *mtd = meta_data_new();
                                mtd->type = META_SONG_TXT;
                                mtd->plugin_name = plugin.name;
                                mtd->content_type = META_DATA_CONTENT_TEXT;
                                mtd->content = g_strstrip(word); 
                                list = g_list_append(list, mtd);
                            }
                            g_match_info_next (match_info, NULL);

                        }
                        g_match_info_free (match_info);
                        g_regex_unref (reg);
                        xmlFree(lyric);
                    }
            }

            xmlFreeDoc(doc);
        }
    }

    q->callback(list, q->user_data);
    g_free(q);
}



static void lyricwiki_download_callback(const GEADAsyncHandler *handle, GEADStatus status, gpointer data)
{
    Query *q = (Query *)data;
    GList *list = NULL;
    if(status == GEAD_PROGRESS) return;
    if(status == GEAD_DONE) 
    {
        goffset size= 0;
        const char *data = gmpc_easy_handler_get_data(handle, &size);
        xmlDocPtr doc = xmlParseMemory(data,(int)size);
        if(doc)
        {
            xmlNodePtr root = xmlDocGetRootElement(doc);
            xmlNodePtr cur = root->xmlChildrenNode;
            for(;cur;cur = cur->next){
                if(xmlStrEqual(cur->name, (xmlChar *)"url")){
                    xmlChar *lyric = xmlNodeGetContent(cur);
/*
                    if(lyric && strcmp(lyric,"Not found")!= 0)
                    {
                        MetaData *mtd = meta_data_new();
                        mtd->type = META_SONG_TXT;
                        mtd->plugin_name = plugin.name;
                        mtd->content_type = META_DATA_CONTENT_TEXT;
                        mtd->content = g_strdup((gchar *)lyric);
                        list = g_list_append(list, mtd);
                    }
                    xmlFree(lyric);
*/
                    char *basename = g_path_get_basename((gchar *) lyric);
                    gchar *full_uri = g_strdup_printf("http://lyrics.wikia.com/index.php?action=edit&title=%s", basename);
                    g_free(basename);
                    xmlFree(lyric);

                    if(gmpc_easy_async_downloader(full_uri, lyricwiki_download_callback2, q)!= NULL)
                    {
                        xmlFreeDoc(doc);
                        g_free(full_uri);
                        return;
                    }

                    g_free(full_uri);
                }

            }
            xmlFreeDoc(doc);
        }
    }

    q->callback(list, q->user_data);
    g_free(q);
}

static void lyricwiki_get_uri(mpd_Song *song, MetaDataType type, void (*callback)(GList *list, gpointer data), gpointer user_data)
{
    if(lyricwiki_get_enabled() && type == META_SONG_TXT && song && song->artist &&song->title)
    {
        Query *q = g_malloc0(sizeof(*q));
        gchar *artist = gmpc_easy_download_uri_escape(song->artist);
        gchar *title =  gmpc_easy_download_uri_escape(song->title);

        gchar *uri_path = g_strdup_printf("http://lyrics.wikia.com/api.php?action=lyrics&artist=%s&song=%s&fmt=xml", artist, title);
        q->callback = callback;
        q->song = song;
        q->user_data = user_data;

        g_free(artist); g_free(title);
        if(gmpc_easy_async_downloader(uri_path, lyricwiki_download_callback, q)!= NULL)
        {
            g_free(uri_path);
            return;
        }
        g_free(q);
        g_free(uri_path);
    }
    /* Return nothing found */
    callback(NULL, user_data);
}

gmpcMetaDataPlugin lw_cover = {
	.get_priority   = lyricwiki_fetch_cover_priority,
    .set_priority   =  lyricwiki_fetch_cover_priority_set,
    .get_metadata   = lyricwiki_get_uri 
};

int plugin_api_version = PLUGIN_API_VERSION;

static void lw_init(void)
{
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
}
static const gchar *lw_get_translation_domain(void)
{
    return GETTEXT_PACKAGE;
}
gmpcPlugin plugin = {
	.name           = N_("LyricWiki.org lyric source"),
	.version        = {PLUGIN_MAJOR_VERSION,PLUGIN_MINOR_VERSION,PLUGIN_MICRO_VERSION},
	.plugin_type    = GMPC_PLUGIN_META_DATA,
    .init           = lw_init,
	.metadata       = &lw_cover,
	.get_enabled    = lyricwiki_get_enabled,
	.set_enabled    = lyricwiki_set_enabled,
    .get_translation_domain = lw_get_translation_domain
};
