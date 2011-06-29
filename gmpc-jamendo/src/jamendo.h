/* gmpc-jamendo (GMPC plugin)
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

void jamendo_db_destroy(void);
void jamendo_db_init();
void jamendo_db_open();
gboolean jamendo_db_has_data();
MpdData * jamendo_db_get_genre_list();
MpdData * jamendo_db_get_album_list(char *wanted_genre,char *wanted_artist);
MpdData * jamendo_db_get_artist_list(char *genre);

GList * jamendo_db_get_url_list(const char *wanted_genre,const char *wanted_artist, const char *wanted_album);
MpdData* jamendo_db_get_song_list(const char *wanted_genre,const char *wanted_artist, const char *wanted_album,gboolean exact);


gchar *jamendo_get_artist_image(const gchar *wanted_artist);
gchar *jamendo_get_album_image(const gchar *wanted_artist, const gchar *wanted_album);

gchar *jamendo_get_album_url(const gchar *wanted_artist, const gchar *wanted_album);

MpdData* jamendo_db_title_search( const char *wanted_title);
#endif
