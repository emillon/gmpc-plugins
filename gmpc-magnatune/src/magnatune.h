/* gmpc-magnatune (GMPC plugin)
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

#ifndef __MAGNATUNE_H__
#define __MAGNATUNE_H__

void magnatune_db_destroy(void);
void magnatune_db_init();
void magnatune_db_open();
gboolean magnatune_db_has_data();
MpdData * magnatune_db_get_genre_list();
MpdData * magnatune_db_get_album_list(char *wanted_genre,char *wanted_artist);
MpdData * magnatune_db_get_artist_list(char *genre);

GList * magnatune_db_get_url_list(const char *wanted_genre,const char *wanted_artist, const char *wanted_album);
MpdData* magnatune_db_get_song_list(const char *wanted_genre,const char *wanted_artist, const char *wanted_album,gboolean exact);


gchar *magnatune_get_artist_image(const gchar *wanted_artist);
gchar *magnatune_get_album_image(const gchar *wanted_artist, const gchar *wanted_album);

gchar *magnatune_get_album_url(const gchar *wanted_artist, const gchar *wanted_album);

char * __magnatune_process_string(const char *name);

MpdData * magnatune_db_search_title(const gchar *title);


void magnatune_db_load_data(const char *data, const goffset length);

char * __magnatune_process_string(const char *name);
#endif
