/* gmpc-wikipedia (GMPC plugin)
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
#include <gmpc/gmpc_easy_download.h>


#ifdef WEBKIT
#include <webkit/webkit.h>
#endif
#ifdef WEBKIT_APT
#include <WebKit/webkit.h>
#endif

#include <locale.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libmpd/debug_printf.h>

#include <libsoup/soup.h>

#define wikipedia_logdomain "Gmpc.Plugin.WikiPedia"
static void wp_add(GtkWidget *cat_tree);
static void wp_selected(GtkWidget *container);
static void wp_unselected(GtkWidget *container);
static void wp_changed(GtkWidget *tree, GtkTreeIter *iter);
static void wp_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata);
static void wp_query_callback(const GEADAsyncHandler *handle, GEADStatus status, gpointer data);
static xmlNodePtr get_first_node_by_name(xmlNodePtr xml, gchar *name);

static GtkWidget *moz = NULL;
static GtkWidget *wp_vbox = NULL;
static GtkWidget *pgb = NULL;

static void wp_start_init(void);
static int wp_get_enabled();
static void wp_set_enabled(int enabled);
static int global_progress;

static gchar *old_artist;
static gchar locale[3];
static GRegex *page_guess_re;
static const char *wikipedia_langs[] = {"ar", "az", "bg", "bn", "ca", "cs", "de", "el", "en", "eo", "es", "fa", "fi", "fr", "gu", "he", "hi", "id", "mr", "it", "ja", "lt", "ms", "mk", "pa", "pl", "pt", "ro", "ru", "sv", "ta", "th", "tr", "uk", "vi", "yo", "zh"};

/* Needed plugin_wp stuff */
gmpcPlBrowserPlugin wp_gbp = {
	.add                    = wp_add,
	.selected               = wp_selected,
	.unselected             = wp_unselected,
};
int plugin_api_version = PLUGIN_API_VERSION;

static const gchar *wp_get_translation_domain(void)
{
	return GETTEXT_PACKAGE;
}

gmpcPlugin plugin = {
	.name = N_("WikiPedia (fixed)"),
	.version = {PLUGIN_MAJOR_VERSION,PLUGIN_MINOR_VERSION,PLUGIN_MICRO_VERSION},
	.plugin_type = GMPC_PLUGIN_PL_BROWSER,
	.init = wp_start_init, /*init */
	.browser = &wp_gbp,
	.mpd_status_changed = wp_status_changed, /* status changed */
	.get_enabled = wp_get_enabled,
	.set_enabled = wp_set_enabled,
	.get_translation_domain = wp_get_translation_domain
};


/* Playlist window row reference */
static GtkTreeRowReference *wiki_ref = NULL;
#define WP_SKINSTR "?useskin=chick"
static gchar *current_url = NULL;

static void wp_set_url(char *wp)
{
	gchar *new_url;

	new_url = malloc(strlen(wp) + strlen(WP_SKINSTR) + 1);
	strcpy(new_url, wp);
	strcat(new_url, WP_SKINSTR);

	if (current_url)
		g_free(current_url);
	current_url = soup_uri_decode(new_url);

	webkit_web_view_open(WEBKIT_WEB_VIEW(moz), new_url);
	gtk_widget_show_all(moz);
	g_free(new_url);
}

static void wp_query_callback(const GEADAsyncHandler *handle, GEADStatus status, gpointer ignored)
{
	xmlDocPtr doc;
	goffset size;
	xmlNodePtr root, sect, desc, url, c1, query;
	const gchar *data;
	gchar *targeturl, *txt;



    g_log(wikipedia_logdomain, G_LOG_LEVEL_DEBUG, "query returned %i\n",status);
	if(status != GEAD_DONE)
		return;
    g_log(wikipedia_logdomain, G_LOG_LEVEL_DEBUG, "query returned done\n");
    data = gmpc_easy_handler_get_data(handle, &size);

	doc = xmlParseMemory(data, size);
	if(!doc)
		return;
	root = xmlDocGetRootElement(doc);
	if(!root)
		return;
	sect = get_first_node_by_name(root,"Section");
	if (!sect)
		goto out_doc;

	for(c1 = sect->xmlChildrenNode; c1; c1 = c1->next)
	{
		desc = get_first_node_by_name(c1, "Text");
		url = get_first_node_by_name(c1, "Url");
		if (!desc || !url)
			continue;
		txt = xmlNodeListGetString(doc, desc->xmlChildrenNode, 1);
		if (!txt)
			continue;
		if (g_regex_match_full(page_guess_re, txt, strlen(txt), 0, 0, NULL, NULL) &&
				xmlNodeListGetString(doc, url->xmlChildrenNode, 1))
		{
			wp_set_url(xmlNodeListGetString(doc, url->xmlChildrenNode, 1));
			break;
		}

	}
	/* nothing was found by regex, use first entry */
	if (!c1) {
		c1 = sect->xmlChildrenNode;
		if (c1) {
			url = get_first_node_by_name(c1, "Url");
			if (url && xmlNodeListGetString(doc, url->xmlChildrenNode, 1))
				wp_set_url(xmlNodeListGetString(doc,url->xmlChildrenNode,1));
		} else {
			/* nothing is found, if we are localized, grab our search string back and do some magic */
			query = get_first_node_by_name(root, "Query");
			if (!query)
				goto out_doc;
			txt = xmlNodeListGetString(doc, query->xmlChildrenNode, 1);
			if (!txt)
				goto out_doc;

			/* fist try english wikipedia, it's the biggest after all */
			const gchar *oldurl = gmpc_easy_handler_get_uri(handle);
			if (!g_str_has_prefix(oldurl, "http://en.")) {
				gchar *newurl = g_strdup_printf("http://en.wikipedia.org/w/api.php?action=opensearch&search=%s&format=xml", txt);
				g_log(wikipedia_logdomain, G_LOG_LEVEL_DEBUG, "Trying to fetch: %s\n", newurl);
				gmpc_easy_async_downloader(newurl, wp_query_callback, NULL);
				g_free(newurl);
				goto out_doc;
			}
			/* nothing is found, display localized wikipedia
			 * "unknown article" page. Not loading anything is
			 * confusing */
			targeturl = g_strdup_printf("http://%s.wikipedia.org/wiki/%s", locale, txt);
			wp_set_url(targeturl);
			g_free(targeturl);
		}
	}
out_doc:
	xmlFreeDoc(doc);
}

/* We need to remove &, ? and /, the rest goes to the webkit widget */
static gchar *wp_clean_for_url(const gchar *str)
{
	static GRegex *re;
	gchar *nstr;
    gchar *retv;
	GError *error = NULL;

	if (!re) {
		re = g_regex_new("[&/\\?]", G_REGEX_MULTILINE, 0, &error);
		if (error) {
			g_log(wikipedia_logdomain, G_LOG_LEVEL_DEBUG, "Build regexp %s\n", error->message);
			g_error_free(error);
			return NULL;
		}
	}

	nstr = g_regex_replace(re, str, strlen(str), 0, "", 0, &error);
	if (error) {
		g_log(wikipedia_logdomain, G_LOG_LEVEL_DEBUG, "regexp replace %s\n", error->message);
		g_error_free(error);
		return NULL;
	}

    retv =  gmpc_easy_download_uri_escape(nstr);
    g_free(nstr);
    return retv;
}

void wp_changed(GtkWidget *tree, GtkTreeIter *iter)
{
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	gchar *artist = NULL;

	if(!song)
		return;

	/* copied from metadata.h */
	if(song->artist) {
		if (cfg_get_single_value_as_int_with_default(config, "metadata", "rename", FALSE))
		{
			gchar **str = g_strsplit(song->artist, ",", 2);
			if(str[1])
				artist = g_strdup_printf("%s %s", g_strstrip(str[1]), g_strstrip(str[0]));
			else
				artist = g_strdup(song->artist);
			g_strfreev(str);
			g_log(wikipedia_logdomain, G_LOG_LEVEL_DEBUG, "string converted to: '%s'", artist);
		} else {
			artist = g_strdup(song->artist);
		}
	}

	if(artist == NULL)
	{
		if (strcmp(old_artist, "NONE") != 0)
		{
			if (current_url)
				g_free(current_url);
			current_url = g_strdup("http://www.musicpd.org/");
			webkit_web_view_open(WEBKIT_WEB_VIEW(moz), current_url);
			old_artist = g_strdup("NONE");
		}
		return;
	}

	gchar *esc_artist = wp_clean_for_url(artist);

	if (strcmp(old_artist, esc_artist) != 0)
	{
		gchar *url = g_strdup_printf("http://%s.wikipedia.org/w/api.php?action=opensearch&search=%s&format=xml", locale, esc_artist);
		g_log(wikipedia_logdomain, G_LOG_LEVEL_DEBUG, "Trying to fetch: %s\n", url);
		gmpc_easy_async_downloader(url, wp_query_callback, NULL);
		g_free(url);

	}
	old_artist = g_strdup(esc_artist);
	g_free(esc_artist);
	g_free(artist);
}

void wp_progress(GtkWidget *mozem, int progress, gpointer data)
{
	float fprog = (float)progress/100.0;
	gchar *text = g_strdup_printf("%d %%",progress);
	global_progress=progress;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pgb),fprog);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pgb),text);
	g_free(text);
}

void wp_progress_started(WebKitWebView  *webkitwebview,WebKitWebFrame *arg1,gpointer user_data)
{
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pgb),0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pgb),"0 %");
	gtk_widget_show(pgb);
}

void wp_progress_finished(WebKitWebView  *webkitwebview,WebKitWebFrame
		*arg1,gpointer user_data)
{
	gtk_widget_hide(pgb);
}

/* open urls in external window. You don't want to navigate without a back
 * button nor url bar.
 * No clean way to handle this, so use global cur_url
 */
static WebKitNavigationResponse wp_navigation_requested(WebKitWebView *web_view,
		WebKitWebFrame *frame, WebKitNetworkRequest *request,
		gpointer user_data)
{
    const gchar *uri;
    gchar *decoded_uri;
	GError *error = NULL;
	GdkScreen *screen;

	uri = webkit_network_request_get_uri(request);
	decoded_uri = soup_uri_decode(uri);
	if (g_str_has_prefix(decoded_uri, current_url)) {
		g_log(wikipedia_logdomain, G_LOG_LEVEL_DEBUG, "Accepting %s\n", uri);
		g_free(decoded_uri);
		return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;
	}
	g_free(decoded_uri);

	g_log(wikipedia_logdomain, G_LOG_LEVEL_DEBUG, "%s != %s\n", uri, current_url);

	screen = gtk_widget_get_screen(GTK_WIDGET(web_view));
	if (!screen)
		screen = gdk_screen_get_default ();

	gtk_show_uri(screen, uri, gtk_get_current_event_time(), &error);
	if (error) {
		g_log(wikipedia_logdomain, G_LOG_LEVEL_DEBUG, "gtk_show_uri %s\n", error->message);
		g_error_free(error);
	}
	return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}

void wp_init()
{
	GError *error = NULL;
	GtkWidget *sw =gtk_scrolled_window_new(NULL,NULL);
	char *l;

	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	wp_vbox = gtk_vbox_new(FALSE, 6);
	moz = webkit_web_view_new();//moz_new();
	if(moz == NULL)
	{
	}
	webkit_web_view_can_go_back_or_forward(WEBKIT_WEB_VIEW(moz), 0);

	gtk_container_add(GTK_CONTAINER(sw), moz);
	gtk_box_pack_start_defaults(GTK_BOX(wp_vbox), sw);
	pgb = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(wp_vbox), pgb, FALSE, TRUE, 0);
	gtk_widget_grab_focus (GTK_WIDGET (moz));
	g_signal_connect(moz, "load_progress_changed", G_CALLBACK(wp_progress), NULL);
	g_signal_connect(moz, "load_finished", G_CALLBACK(wp_progress_finished), NULL);
	g_signal_connect(moz, "load_started", G_CALLBACK(wp_progress_started), NULL);
	g_signal_connect(moz, "navigation-requested", G_CALLBACK(wp_navigation_requested), NULL);
	gtk_widget_show_all(wp_vbox);
	gtk_widget_hide(pgb);
	g_object_ref(G_OBJECT(wp_vbox));
	old_artist = g_strdup("NONE");
	/* we should add inernationalized artist|band|musician|singer|rapper|group in the regex */
	page_guess_re = g_regex_new("\\(.*(artist|band|musician|singer|rapper|group).*\\)", G_REGEX_CASELESS, 0, &error);

	strcpy(locale,"en");
	/*find locale */
	l = setlocale(LC_ALL,"");
	if(l!=NULL)
	{
		int i;
		for (i = 0; i < sizeof(wikipedia_langs) / sizeof(char *); i++) {
			if (strncmp(l, wikipedia_langs[i], 2) == 0) {
				strncpy(locale,l,2);
				locale[2] = 0;
				break;
			}
		}
	}
}

void wp_add(GtkWidget *cat_tree)
{
	GtkTreePath *path = NULL;
	GtkListStore *pl3_tree = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));
	GtkTreeIter iter;
	if(!cfg_get_single_value_as_int_with_default(config, "wp-plugin", "enable", 0)) return;
	gtk_list_store_append(pl3_tree, &iter);
	gtk_list_store_set(pl3_tree, &iter,
			PL3_CAT_TYPE, plugin.id,
			PL3_CAT_TITLE, _("Wikipedia Lookup"),
			PL3_CAT_ICON_ID, "wikipedia",
			-1);
	if (wiki_ref)
	{
		gtk_tree_row_reference_free(wiki_ref);
		wiki_ref = NULL;
	}

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter);
	if (path)
	{
		wiki_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist3_get_category_tree_store()), path);
		gtk_tree_path_free(path);
	}

}

void wp_selected(GtkWidget *container)
{

	if(wp_vbox== NULL)
	{
		wp_init();
	}

	gtk_container_add(GTK_CONTAINER(container), wp_vbox);
	gtk_widget_show_all(wp_vbox);
	wp_changed(NULL, NULL);
	if(global_progress==100)
	{
		gtk_widget_hide(pgb);
	}
}

void wp_unselected(GtkWidget *container)
{
	gtk_widget_hide(moz);
	gtk_container_remove(GTK_CONTAINER(container),wp_vbox);
}


void wp_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "wp-plugin", "enable", enabled);
	if (enabled)
	{
		if(wiki_ref == NULL)
		{
			wp_add(GTK_WIDGET(playlist3_get_category_tree_view()));
		}
	}
	else if (wiki_ref)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(wiki_ref);
		if (path){
			GtkTreeIter iter;
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter, path)){
				gtk_list_store_remove(playlist3_get_category_tree_store(), &iter);
			}
			gtk_tree_path_free(path);
			gtk_tree_row_reference_free(wiki_ref);
			wiki_ref = NULL;
		}
	}
}

int wp_get_enabled()
{
	return cfg_get_single_value_as_int_with_default(config, "wp-plugin", "enable", 0);
}
static void wp_start_init(void)
{
	gchar *path = gmpc_plugin_get_data_path(&plugin);
	gchar *url = g_build_path(G_DIR_SEPARATOR_S,path, "wikipedia", NULL);
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default (),url);
	g_free(path);
	g_free(url);

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
}


static void wp_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	if(pl3_cat_get_selected_browser() == plugin.id)
	{
        /* Only update when songid changed */
        if((what&MPD_CST_SONGID) == MPD_CST_SONGID)
        {
            wp_changed(NULL, NULL);
        }
    }
}

static xmlNodePtr get_first_node_by_name(xmlNodePtr xml, gchar *name) {
	if(name == NULL) return NULL;
	if(xml) {
		xmlNodePtr c = xml->xmlChildrenNode;
		for(;c;c=c->next) {
			if(c->name && xmlStrEqual(c->name, (xmlChar *) name))
				return c;
		}
	}
	return NULL;
}
