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

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gmpc/plugin.h>
#include <gmpc/gmpc_easy_download.h>
#include <gmpc/misc.h>
#include <libmpd/libmpd-internal.h>
#include <sqlite3.h>
#include <playlist3-messages.h>
#include "magnatune.h"

static sqlite3 *magnatune_sqlhandle = NULL;

static gchar *user_name = NULL;
static gchar *user_password = NULL;

void magnatune_set_user_password(const gchar *name, const gchar *password)
{
    if(user_name) g_free(user_name);
    user_name = NULL;

    if(name && strlen(name) > 0)
        user_name = gmpc_easy_download_uri_escape(name);

    if(user_password) g_free(user_password);
    user_password = NULL;

    if(password && strlen(password) > 0)
        user_password = gmpc_easy_download_uri_escape(password);
}

static gchar *magnatune_get_url(const gchar *n)
{
    if(user_name != NULL && user_password != NULL)
    {
        int len = strlen(n);
        if (len > 4)
            return g_strdup_printf("http://%s:%s@stream.magnatune.com/all/%*.*s_nospeech.mp3",user_name, user_password, len - 4, len - 4, n);

    }
    return g_strdup_printf("http://he3.magnatune.com/all/%s",n);
}

void magnatune_db_destroy(void)
{
    if(magnatune_sqlhandle)
    {
        sqlite3_close(magnatune_sqlhandle);
        magnatune_sqlhandle = NULL;
    }
}

/* run this before using the other fucntions */
void magnatune_db_init()
{
}

void magnatune_db_open()
{
    gchar *path = NULL;
    const gchar *final = NULL;


    /**
     * if open close it
     */
    if(magnatune_sqlhandle)
    {
        sqlite3_close(magnatune_sqlhandle);
        magnatune_sqlhandle = NULL;
    }

    path = gmpc_get_cache_directory("magnatune.sqlite3");// g_build_filename(final, "gmpc", "magnatune.sqlite3", NULL);
    sqlite3_open(path, &(magnatune_sqlhandle));
    g_free(path);

}
/* FIXME */
gboolean magnatune_db_has_data()
{
    char *query = sqlite3_mprintf("SELECT * from 'sqlite_master'");
    sqlite3_stmt *stmt = NULL;
    const char *tail;
    int r;
    
    
    r= sqlite3_prepare_v2(magnatune_sqlhandle, query, -1,  &stmt,  &tail);
    sqlite3_free(query);
    if(r == SQLITE_OK) {
        while((r = sqlite3_step(stmt)) == SQLITE_ROW){
            sqlite3_finalize(stmt);
            return TRUE;
        }
    }

    sqlite3_finalize(stmt);
    return FALSE;
}

MpdData * magnatune_db_get_genre_list()
{
    MpdData *list = NULL;
    int i,r;

    char *query = sqlite3_mprintf("SELECT genre from 'genres' group by genre");
    sqlite3_stmt *stmt = NULL;
    const char *tail;
    GTimer *timer = g_timer_new();
    r = sqlite3_prepare_v2(magnatune_sqlhandle, query, -1,  &stmt,  &tail);
    if(r ==SQLITE_OK) {
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            list = mpd_new_data_struct_append(list);
            list->type = MPD_DATA_TYPE_TAG;
            list->tag_type = MPD_TAG_ITEM_GENRE;
            list->tag = g_strdup(sqlite3_column_text(stmt,0));
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_free(query);
    g_debug("%f s elapsed getting genres\n", g_timer_elapsed(timer,NULL));
    g_timer_reset(timer);
    list = misc_mpddata_remove_duplicate_songs(list); 
    g_debug("%f s elapsed unique genres list\n", g_timer_elapsed(timer,NULL));
    g_timer_destroy(timer);
    return list;
}

void magnatune_db_load_data(const char *data, const goffset length)
{
    gchar *error = NULL;
    gchar *path;


//    const gchar *final = g_get_user_cache_dir();
    path = gmpc_get_cache_directory("magnatune.sqlite3");//g_build_filename(final, "gmpc", "magnatune.sqlite3", NULL);
    if(magnatune_sqlhandle)
    {
        int status = sqlite3_close(magnatune_sqlhandle);
        if(status != 0)
        {
            gchar *temp =g_strdup_printf("Failed to close magnatune db: %i\n", status); 
            playlist3_show_error_message(temp, ERROR_WARNING);
            g_free(temp);
        }
        magnatune_sqlhandle = NULL;
    }
    if(data){
        GError *error = NULL;
        gssize size= (gssize)length;
        g_file_set_contents(path, data, size, &error);    
        if(error) {
            gchar *temp =g_strdup_printf("Failed to store magnatune db: %s\n", error->message);
            playlist3_show_error_message(temp, ERROR_WARNING);
            g_free(temp);
            g_error_free(error);
        }
    }
    /* open the db if it is closed */
    if(!magnatune_sqlhandle) {


        int retv = sqlite3_open(path, &(magnatune_sqlhandle));
        if(retv != SQLITE_OK)
        {
            g_free(path);

            playlist3_show_error_message("Failed to open the new magnatune database", ERROR_WARNING);
            /* Cleanup */
            return;
        }
    }

    sqlite3_exec(magnatune_sqlhandle, "CREATE INDEX songsAlbumname ON songs(albumname);", NULL, NULL, &error);
    if(error)printf("%i: %s",__LINE__, error);
    sqlite3_exec(magnatune_sqlhandle, "CREATE INDEX genresAlbumname ON genres(albumname);", NULL, NULL, &error);
    if(error)printf("%i: %s",__LINE__, error);
    sqlite3_exec(magnatune_sqlhandle, "CREATE INDEX genresGenrename ON genres(genre);", NULL, NULL, &error);
    if(error)printf("%i: %s",__LINE__, error);
    sqlite3_exec(magnatune_sqlhandle, "CREATE INDEX albumsAlbumname ON albums(albumname);", NULL, NULL, &error);
    if(error)printf("%i: %s",__LINE__, error);

    g_free(path);
}

MpdData * magnatune_db_get_artist_list(char *wanted_genre)
{
    MpdData *list = NULL;
    int r;
    /** check if there is data */
    char *query = sqlite3_mprintf("SELECT albumname from 'genres' WHERE genre=%Q", wanted_genre);
    sqlite3_stmt *stmt = NULL;
    const char *tail;
    GTimer *timer = g_timer_new();
    r = sqlite3_prepare_v2(magnatune_sqlhandle, query, -1,  &stmt,  &tail);
    if(r ==SQLITE_OK) {
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            sqlite3_stmt *stmt2 = NULL;
            const char *tail2;
            char *query2 = sqlite3_mprintf("SELECT artist from 'albums' WHERE albumname=%Q", sqlite3_column_text(stmt,0));
            int r2 = sqlite3_prepare_v2(magnatune_sqlhandle, query2, -1,  &stmt2,  &tail2);
            if(r2 ==SQLITE_OK) {
                while((r2 = sqlite3_step(stmt2)) == SQLITE_ROW)
                {
                    list = mpd_new_data_struct_append(list);
                    list->type = MPD_DATA_TYPE_TAG;
                    list->tag_type = MPD_TAG_ITEM_ARTIST;
                    list->tag = g_strdup(sqlite3_column_text(stmt2,0));
                }
            }
            sqlite3_finalize(stmt2);
            sqlite3_free(query2);
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_free(query);
    g_debug("%f s elapsed getting genres\n", g_timer_elapsed(timer,NULL));
    g_timer_reset(timer);
    list = misc_mpddata_remove_duplicate_songs(list);

    g_debug("%f s elapsed unique artist list\n", g_timer_elapsed(timer,NULL));
    g_timer_destroy(timer);
    return list;
}

MpdData *magnatune_db_get_album_list(char *wanted_genre,char *wanted_artist)
{
    int r;
    MpdData *list = NULL;
    /** check if there is data */

    char *query = sqlite3_mprintf("SELECT albumname from 'albums' WHERE artist=%Q",wanted_artist);
    sqlite3_stmt *stmt = NULL;
    const char *tail;
    GTimer *timer = g_timer_new();
    r = sqlite3_prepare_v2(magnatune_sqlhandle, query, -1,  &stmt,  &tail);
    if(r ==SQLITE_OK) {
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            sqlite3_stmt *stmt2 = NULL;
            const char *tail2;
            char *query2 = sqlite3_mprintf("SELECT albumname from 'genres' WHERE albumname=%Q AND genre=%Q", sqlite3_column_text(stmt,0),wanted_genre);
            int r2 = sqlite3_prepare_v2(magnatune_sqlhandle, query2, -1,  &stmt2,  &tail2);
            if(r2 ==SQLITE_OK) {
                while((r2 = sqlite3_step(stmt2)) == SQLITE_ROW)
                {
                    list = mpd_new_data_struct_append(list);
                    list->type = MPD_DATA_TYPE_TAG;
                    list->tag_type = MPD_TAG_ITEM_ALBUM;
                    list->tag = g_strdup(sqlite3_column_text(stmt2,0));
                }
            }
            sqlite3_finalize(stmt2);
            sqlite3_free(query2);
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_free(query);
    g_debug("%f s elapsed listing albums songs\n", g_timer_elapsed(timer,NULL));
    g_timer_destroy(timer);
    return mpd_data_get_first(list); 
}

static char *__magnatune_get_artist_name(const char *album)
{
    char *retv = NULL;
    sqlite3_stmt *stmt = NULL;
    const char *tail;
    int r = 0;
    char *query =  NULL;
    if(!album) return NULL;
    query = sqlite3_mprintf("SELECT artist from 'albums' WHERE albumname=%Q limit 1",album);
    r=sqlite3_prepare_v2(magnatune_sqlhandle, query, -1,  &stmt,  &tail);
    if(r ==SQLITE_OK) {
        if((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            retv = g_strdup(sqlite3_column_text(stmt, 0));
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_free(query);
    return retv;
}
static char *__magnatune_get_genre_name(const char *album)
{
    char *retv = NULL;
    sqlite3_stmt *stmt = NULL;
    const char *tail;
    int r = 0;
    char *query =  NULL;
    if(!album) return NULL;
    query = sqlite3_mprintf("SELECT genre from 'genres' WHERE albumname=%Q",album);
    r=sqlite3_prepare_v2(magnatune_sqlhandle, query, -1,  &stmt,  &tail);
    if(r ==SQLITE_OK) {
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            const char *temp= sqlite3_column_text(stmt, 0);
            if(retv)
            {
                gchar *t = retv;
                retv = g_strconcat(t, ", ",temp, NULL);
                g_free(t);
            }
            else retv = g_strdup(temp);
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_free(query);
    return retv;
}
static MpdData *__magnatune_get_data_album_from_genre(const char *genre, gboolean exact)
{
    MpdData *list = NULL;
    char *query = NULL;
    sqlite3_stmt *stmt = NULL;
    const char *tail;
    int r = 0;
    GTimer *timer = g_timer_new();
    if(exact)
    {
        query =  sqlite3_mprintf("SELECT songs.albumname,duration,number,desc,mp3 from 'songs' JOIN 'genres' ON songs.albumname = genres.albumname " 
        "WHERE genres.genre=%Q",genre);
    }else{
        query =  sqlite3_mprintf("SELECT songs.albumname,duration,number,desc,mp3 from 'songs' JOIN 'genres' ON songs.albumname = genres.albumname "
        "WHERE genres.genre LIKE '%%%%%q%%%%'",genre);
    }
    r=sqlite3_prepare_v2(magnatune_sqlhandle, query, -1,  &stmt,  &tail);
    if(r ==SQLITE_OK) {
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            gchar *temp = gmpc_easy_download_uri_escape(sqlite3_column_text(stmt,4));
            list = mpd_new_data_struct_append(list);
            list->type = MPD_DATA_TYPE_SONG;
            list->song = mpd_newSong();
            list->song->album = g_strdup(sqlite3_column_text(stmt,0));
            list->song->artist = __magnatune_get_artist_name(list->song->album);
            list->song->genre = __magnatune_get_genre_name(list->song->album);
            list->song->title= g_strdup(sqlite3_column_text(stmt,3));
            list->song->track = g_strdup(sqlite3_column_text(stmt,2));
            list->song->time = sqlite3_column_int(stmt,1);
            list->song->file = magnatune_get_url(temp);
            g_free(temp);
        }
    }else{
        g_warning("Sqlite error: %s\n", tail);
    }

    sqlite3_finalize(stmt);
    sqlite3_free(query);
    g_debug("%f s elapsed getting album songs from genre\n", g_timer_elapsed(timer,NULL));
    g_timer_destroy(timer);
    return list;
}

static MpdData *__magnatune_get_data_album(const char *album, gboolean exact)
{
    MpdData *list = NULL;
    char *query = NULL;
    sqlite3_stmt *stmt = NULL;
    const char *tail;
    int r = 0;
    GTimer *timer = g_timer_new();
    if(exact)
    {
        query =  sqlite3_mprintf("SELECT songs.albumname,duration,number,desc,mp3 from 'songs' " 
        "WHERE songs.albumname=%Q",album);
    }else{
        query =  sqlite3_mprintf("SELECT songs.albumname,duration,number,desc,mp3 from 'songs' "
        "WHERE songs.albumname LIKE '%%%%%q%%%%'",album);
    }
    r=sqlite3_prepare_v2(magnatune_sqlhandle, query, -1,  &stmt,  &tail);
    if(r ==SQLITE_OK) {
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            gchar *temp = gmpc_easy_download_uri_escape(sqlite3_column_text(stmt,4));
            list = mpd_new_data_struct_append(list);
            list->type = MPD_DATA_TYPE_SONG;
            list->song = mpd_newSong();
            list->song->album = g_strdup(sqlite3_column_text(stmt,0));
            list->song->artist = __magnatune_get_artist_name(list->song->album);
            list->song->genre = __magnatune_get_genre_name(list->song->album);
            list->song->title= g_strdup(sqlite3_column_text(stmt,3));
            list->song->track = g_strdup(sqlite3_column_text(stmt,2));
            list->song->time = sqlite3_column_int(stmt,1);
            list->song->file = magnatune_get_url(temp);
            g_free(temp);
        }
    }
    else{
        g_warning("Sqlite error: %s\n", tail);
    }
    sqlite3_finalize(stmt);
    sqlite3_free(query);
    g_debug("%f s elapsed getting album songs\n", g_timer_elapsed(timer,NULL));
    g_timer_destroy(timer);
    return list;
}
static gchar ** __magnatune_get_albums(const char *genre, const char *artist, gboolean exact)
{
    char **retv = NULL;
    sqlite3_stmt *stmt = NULL;
    const char *tail;
    int items = 0;
    int r = 0;
    char *query =  NULL;

    if(genre && !artist) {
        if(exact)
            query = sqlite3_mprintf("SELECT albumname FROM 'genres' WHERE genre=%Q", genre);
        else
            query = sqlite3_mprintf("SELECT albumname FROM 'genres' WHERE genre LIKE '%%%%%q%%%%'", genre);
    }
    else if (artist && !genre) {
        if(exact)
            query = sqlite3_mprintf("SELECT albumname FROM 'albums' WHERE artist=%Q", artist);
        else 
            query = sqlite3_mprintf("SELECT albumname FROM 'albums' WHERE artist LIKE '%%%%%q%%%%'", artist);
    }
    else if (artist && genre) {
        if(exact)
            query = sqlite3_mprintf("SELECT genres.albumname FROM 'albums' JOIN 'genres' on albums.albumname = genres.albumname WHERE albums.artist=%Q AND genres.genre=%Q", artist,genre);
        else
            query = sqlite3_mprintf("SELECT genres.albumname FROM 'albums' JOIN 'genres' on albums.albumname = genres.albumname WHERE albums.artist LIKE '%%%%%q%%%%' AND genres.genre LIKE '%%%%%q%%%%'", artist,genre);
    }


    r=sqlite3_prepare_v2(magnatune_sqlhandle, query, -1,  &stmt,  &tail);
    if(r ==SQLITE_OK) {
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            items++;
            retv = g_realloc(retv, (items+1)*sizeof(*retv));
            retv[items] = NULL;
            retv[items-1] = g_strdup(sqlite3_column_text(stmt, 0));
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_free(query);
    return retv;
}

MpdData* magnatune_db_get_song_list(const char *wanted_genre,const char *wanted_artist, const char *wanted_album, gboolean exact)
{
    MpdData *data = NULL;
    char *query;

    if(!wanted_genre && !wanted_artist && !wanted_album) return NULL;

    GTimer *timer = g_timer_new();
    if(wanted_album) /* album seems to be unique */
    {
        data = __magnatune_get_data_album(wanted_album, exact);
    }else if (wanted_genre && !wanted_artist)
    {
        data = __magnatune_get_data_album_from_genre(wanted_genre, exact);
    }
    else 
    {
        char **albums = __magnatune_get_albums(wanted_genre, wanted_artist, exact);
        if(albums)
        {
            int i;
            for(i=0; albums[i];i++)
            {
                MpdData *data2 =  __magnatune_get_data_album(albums[i], exact);
                data = mpd_data_concatenate(data, data2);
            }
            g_strfreev(albums);
        }
    }

    g_debug("%f s elapsed song list\n", g_timer_elapsed(timer,NULL));
    g_timer_destroy(timer);
    return mpd_data_get_first(data);
}

MpdData * magnatune_db_search_title(const gchar *title)
{

    MpdData *list = NULL;
    char *query = NULL;
    sqlite3_stmt *stmt = NULL;
    const char *tail;
    int r = 0;
        query =  sqlite3_mprintf("SELECT songs.albumname,duration,number,desc,mp3 from 'songs' "
        "WHERE songs.desc LIKE '%%%%%q%%%%'",title);
    r=sqlite3_prepare_v2(magnatune_sqlhandle, query, -1,  &stmt,  &tail);
    if(r ==SQLITE_OK) {
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            gchar *temp = gmpc_easy_download_uri_escape(sqlite3_column_text(stmt,4));
            list = mpd_new_data_struct_append(list);
            list->type = MPD_DATA_TYPE_SONG;
            list->song = mpd_newSong();
            list->song->album = g_strdup(sqlite3_column_text(stmt,0));
            list->song->artist = __magnatune_get_artist_name(list->song->album);
            list->song->genre = __magnatune_get_genre_name(list->song->album);
            list->song->title= g_strdup(sqlite3_column_text(stmt,3));
            list->song->track = g_strdup(sqlite3_column_text(stmt,2));
            list->song->time = sqlite3_column_int(stmt,1);
            list->song->file = magnatune_get_url(temp);
            g_free(temp);
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_free(query);
    return list;
}



gchar *magnatune_get_artist_image(const gchar *wanted_artist)
{
    char *retv = NULL;
    sqlite3_stmt *stmt = NULL;
    char *artist =  __magnatune_process_string(wanted_artist);
    const char *tail;
    int r = 0;
    char *query =  NULL;
    query = sqlite3_mprintf("SELECT homepage from 'artists' WHERE artist LIKE '%%%%%q%%%%' limit 1",artist);
    r=sqlite3_prepare_v2(magnatune_sqlhandle, query, -1,  &stmt,  &tail);
    if(r ==SQLITE_OK) {
        if((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            gchar *temp = gmpc_easy_download_uri_escape( sqlite3_column_text(stmt, 0));
            retv = g_strdup_printf("http://he3.magnatune.com/artists/img/%s_1.jpg", temp);
            g_free(temp);
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_free(query);
    g_free(artist);
    return retv;
}

gchar *magnatune_get_album_url(const gchar *wanted_artist, const gchar *wanted_album)
{

}


gchar *magnatune_get_album_image(const gchar *wanted_artist, const gchar *wanted_album)
{
    char *retv = NULL;
    char *artist =  __magnatune_process_string(wanted_artist);
    char *album =  __magnatune_process_string(wanted_album);
    gchar *artist_enc = gmpc_easy_download_uri_escape(artist);
    gchar *album_enc = gmpc_easy_download_uri_escape(album);
    retv = g_strdup_printf("http://he3.magnatune.com/music/%s/%s/cover_600.jpg",artist_enc, album_enc);
    g_free(artist);
    g_free(album);
    g_free(artist_enc);
    g_free(album_enc);
    return retv;
}
