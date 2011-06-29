/* gmpc-mdcover (GMPC plugin)
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
#include <gtk/gtk.h>
#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <regex.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <gmpc/plugin.h>
#include <gmpc/metadata.h>
#include <libmpd/libmpd.h>
#include <libmpd/debug_printf.h>
#include <config.h>

int fetch_get_image(mpd_Song *song,MetaDataType type,void (*callback)(GList *uris, gpointer data), gpointer data) ;
void music_dir_cover_art_pref_construct(GtkWidget *container);
void music_dir_cover_art_pref_destroy(GtkWidget *container);
GList * fetch_cover_art_path_list(mpd_Song *song);
GList * fetch_cover_art_path(mpd_Song *song);
static GtkWidget *wp_pref_vbox = NULL;

gmpcPlugin plugin;

/**
 * Required functions
 */
/*
 * Enable/Disable state of the plugins
 */
static int mdca_get_enabled()
{
	return cfg_get_single_value_as_int_with_default(config, "music-dir-cover", "enable", TRUE);
}
static void mdca_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "music-dir-cover", "enable", enabled);
}
/* priority */
static int fetch_cover_priority()
{
    return cfg_get_single_value_as_int_with_default(config, "music-dir-cover", "priority", 10);
}

static void fetch_cover_priority_set(int priority)
{
    cfg_set_single_value_as_int(config, "music-dir-cover", "priority", priority);
}



int fetch_get_image(mpd_Song *song,MetaDataType type,void (*callback)(GList *uris, gpointer data), gpointer data) 
{
    gchar *path = NULL;
	if(song  == NULL || song->file == NULL)
	{
        debug_printf(DEBUG_INFO, "MDCOVER:  No file path to look at.");
        callback(NULL, data);
		return META_DATA_UNAVAILABLE;
	}
	if(type == META_ALBUM_ART)
	{
		GList *retv = fetch_cover_art_path(song);
        callback(retv, data);
		return META_DATA_UNAVAILABLE;
	}
	else if(type == META_SONG_TXT)
	{
		const gchar *musicroot= connection_get_music_directory();
		if(musicroot)
		{
            GList *list;
			gchar *file = g_malloc0((strlen(musicroot)+strlen(song->file)+strlen("lyrics")+2)*sizeof(*file));
			int length = strlen(song->file);
			strcat(file, musicroot);
			strcat(file, G_DIR_SEPARATOR_S);
			for(;length > 0 && song->file[length] != '.';length--);
			strncat(file, song->file, length+1);
			strcat(file, "lyric");	
			if(g_file_test(	file, G_FILE_TEST_EXISTS))
			{
                MetaData *md = meta_data_new();
                md->type = type;
                md->plugin_name = plugin.name; 
                md->content_type = META_DATA_CONTENT_URI;
				md->content = file;
                md->size = 0;
                list = g_list_append(list, md);
                callback(list, data);

				return META_DATA_AVAILABLE;
			}
			g_free(file);
		}
        callback(NULL, data);
		return META_DATA_UNAVAILABLE;
	}
	else if(type == META_ARTIST_ART || type == META_ARTIST_TXT|| type == META_ALBUM_TXT)
	{
		char *extention = NULL;
		char *name = NULL;
		switch(type)
		{
			case META_ARTIST_TXT:
				name = "BIOGRAPHY";
				extention = "";
				break;
			case META_ALBUM_TXT:
				name = song->album;
				extention = ".txt";
				break;
			default:
				name = song->artist;
				extention = ".jpg";
				break;
		}
		if(song->artist)
		{
            GList *list = NULL;
			const gchar *musicroot= connection_get_music_directory();
			gchar **dirs = NULL;
			gchar *fpath = NULL;
			gchar *song_path = NULL;
			int i = 0;
			if(!musicroot){
                callback(NULL, data);
                return META_DATA_UNAVAILABLE;
            }
			song_path = g_path_get_dirname(song->file);
			for(i=strlen(song_path); i >= 0 && path == NULL;i--)
			{
				if(song_path[i] == G_DIR_SEPARATOR)
				{	
					song_path[i] = '\0';
					fpath = g_strdup_printf("%s%c%s%c%s%s",
							musicroot, G_DIR_SEPARATOR,
							song_path, G_DIR_SEPARATOR,
							name,
							extention);
					if(g_file_test(	fpath, G_FILE_TEST_EXISTS))
					{
						path = fpath;
					}
					else
					{
						g_free(fpath);
					}
				}
			}
			g_free(song_path);
			if(path)
			{
                MetaData *md = meta_data_new();
                md->type = type;
                md->plugin_name = plugin.name; 
                md->content_type = META_DATA_CONTENT_URI;
				md->content = path;
                md->size = 0;
                list = g_list_append(list, md);
                callback(list, data);
				return META_DATA_AVAILABLE;
			}
		}
	}

    callback(NULL, data);
    return META_DATA_UNAVAILABLE;
}

GList * fetch_cover_art_path(mpd_Song *song)
{
	GList *list = NULL, *node = NULL;	
	node = list = fetch_cover_art_path_list(song);
	if(list == NULL)
	{
        debug_printf(DEBUG_INFO, "No images available\n");
		return NULL;
	}
    return list;
	/* check for image with name "cover/voorkant/front/large/folder/booklet" */
    /*
	regex_t regt;
	if(!regcomp(&regt,"(voorkant|front|cover|large|folder|booklet)", REG_EXTENDED|REG_ICASE))
	{                                                                	
		do{
			char *data = node->data;
			if(!regexec(&regt, data,0,NULL,0))
			{
				*path = g_strdup(data);
				regfree(&regt);
				g_list_foreach(list, (GFunc)g_free,NULL);
				g_list_free(list);	
				return META_DATA_AVAILABLE;                                     				
			}
		}while((node = g_list_next(node)));
	}
	regfree(&regt);
	*path = g_strdup(list->data);
	
	g_list_foreach(list, (GFunc)g_free,NULL);
	g_list_free(list);	
	return META_DATA_AVAILABLE;
    */
}

void fetch_cover_art_path_list_from_dir(gchar *url, GList **list)
{

	GDir *dir = g_dir_open(url,0,NULL);
	if(dir)
	{
		const char *filename;
		regex_t regt;
		if(!regcomp(&regt,"(png|jpg|jpeg|gif)$", REG_EXTENDED|REG_ICASE))
		{
			filename = g_dir_read_name(dir);
			do{
				/* ignore . files */
				if(filename[0] != '.' || !strncmp(filename, ".folder.jpg",strlen(".folder.jpg")))
				{
					if(!regexec(&regt, filename, 0,NULL,0))	
                    {
                        char *path = g_strdup_printf("%s%c%s", url,G_DIR_SEPARATOR,filename);
                        MetaData *md = meta_data_new();
                        md->type = META_ALBUM_ART;
                        md->plugin_name = plugin.name; 
                        md->content_type = META_DATA_CONTENT_URI;
                        md->content = path;
                        md->size = 0;
                        debug_printf(DEBUG_INFO, "MDCOVER found image %s\n", path);
                        *list = g_list_append(*list, md);
                    }
				}
				filename = g_dir_read_name(dir);
			}while(filename);
		}
		regfree(&regt);
		g_dir_close(dir);
	}
}
GList * fetch_cover_art_path_list(mpd_Song *song) 
{
	char *url =NULL;
	char *dirname = NULL;
	GList *list = NULL;
	GDir *dir = NULL;
	const gchar *musicroot= connection_get_music_directory();
	regex_t regt;


	if(song == NULL || !cfg_get_single_value_as_int_with_default(config, "music-dir-cover", "enable", TRUE))
	{
        debug_printf(DEBUG_INFO, "Plugin is disabled\n");
		return NULL;
	}
	if(song->file == NULL )
	{
        debug_printf(DEBUG_INFO, "The song does not contain path info\n");
		return NULL;
	}

	if(musicroot == NULL)
	{
		debug_printf(DEBUG_WARNING, "No Music Root");
		return NULL;
	}
	dirname = g_path_get_dirname(song->file);
	if(dirname == NULL)
	{
		debug_printf(DEBUG_WARNING, "Cannot get file's directory name");
		return NULL;
	}	
	if(song->album)
	{
		int i = 0;
		char *album = g_strdup(song->album);
		for(i=0;i<strlen(album);i++) if(album[i] == G_DIR_SEPARATOR) album[i] = ' ';
		/* Test the default "Remco" standard */	
		url = g_strdup_printf("%s%c%s%c%s.jpg",
				musicroot,
				G_DIR_SEPARATOR,
				dirname,
				G_DIR_SEPARATOR,
				album
				);
		g_free(album);
		if(g_file_test(url, G_FILE_TEST_EXISTS))
		{
            MetaData *md = meta_data_new();
            md->type = META_ALBUM_ART;
            md->plugin_name = plugin.name; 
            md->content_type = META_DATA_CONTENT_URI;
            md->content = url;
            md->size = 0;

			list = g_list_append(list, md);	
		}
		else g_free(url);
	}
	/* Ok, not found, now let's do it brute-force */

	url = g_strdup_printf("%s/%s/",
			musicroot,
			dirname
			);
    debug_printf(DEBUG_INFO, "Looking into: '%s'\n", url);
	fetch_cover_art_path_list_from_dir(url, &list);
	g_free(url);

	if(!regcomp(&regt,"(CD|DISC) [0-9]$", REG_EXTENDED|REG_ICASE))
	{
		if(!regexec(&regt, dirname,0,NULL,0))
		{
			char *new_dir = NULL;
			int i;
			for(i= strlen(dirname);i> 0 && dirname[i] != G_DIR_SEPARATOR; i--);
			new_dir = g_strndup(dirname, i);
			url = g_strdup_printf("%s%c%s%c", musicroot,G_DIR_SEPARATOR, new_dir,G_DIR_SEPARATOR);
			debug_printf(DEBUG_INFO,"Trying: %s\n", url);
			fetch_cover_art_path_list_from_dir(url, &list);
			g_free(url);
			g_free(new_dir);
		}
	}
	regfree(&regt);
	

	g_free(dirname);
	return g_list_first(list);
}


gmpcMetaDataPlugin mdca_cover = {
	.get_priority = fetch_cover_priority,
    .set_priority = fetch_cover_priority_set,
	.get_metadata = fetch_get_image
};
/* a workaround so gmpc has some sort of api checking */
int plugin_api_version = PLUGIN_API_VERSION;

void mdcover_init(void)
{
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
}
static const gchar *mdcover_get_translation_domain(void)
{
    return GETTEXT_PACKAGE;
}
/* the plugin */
gmpcPlugin plugin = {
	.name           = N_("Music Dir Fetcher"),
	.version        = {PLUGIN_MAJOR_VERSION,PLUGIN_MINOR_VERSION,PLUGIN_MICRO_VERSION},
	.plugin_type    = GMPC_PLUGIN_META_DATA,
    .init           = mdcover_init,
	.metadata       = &mdca_cover, /* meta data */
	.get_enabled    = mdca_get_enabled,
	.set_enabled    = mdca_set_enabled,
    .get_translation_domain = mdcover_get_translation_domain
};



