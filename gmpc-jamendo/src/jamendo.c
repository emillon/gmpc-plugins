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

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gmpc/plugin.h>
#include <gmpc/gmpc_easy_download.h>
#include <gmpc/misc.h>
#include <libmpd/libmpd-internal.h>
#include <zlib.h>
#include <sqlite3.h>
#include <libxml/xmlreader.h>
#include <libxml/tree.h>
#include "jamendo.h"

static void jamendo_cleanup_xml();
static sqlite3 *jamendo_sqlhandle = NULL;

/* hack fix this */
//int jamendo_end_download();
char *GENRE_LIST[] =
{
"", "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
"Grunge", "Hip-Hop", "Jazz", "Metal ", "New Age ", "Oldies ", "Other ",
"Pop ", "R&B ", "Rap ", "Reggae ", "Rock ", "Techno ", "Industrial ",
"Alternative ", "Ska ", "Death Metal ", "Pranks ", "Soundtrack ", "Euro-Techno ", "Ambient ",
"Trip-Hop ", "Vocal ", "Jazz+Funk ", "Fusion ", "Trance ", "Classical ", "Instrumental ",
"Acid ", "House ", "Game ", "Sound Clip ", "Gospel ", "Noise ", "Alternative Rock ",
"Bass ", "Soul ", "Punk ", "Space ", "Meditative ", "Instrumental Pop ", "Instrumental Rock ",
"Ethnic ", "Gothic ", "Darkwave ", "Techno-Industrial ", "Electronic ", "Pop-Folk ", "Eurodance ",
"Dream ", "Southern Rock ", "Comedy ", "Cult ", "Gangsta ", "Top 40 ", "Christian Rap ",
"Pop/Funk ", "Jungle ", "Native US ", "Cabaret ", "New Wave ", "Psychadelic ", "Rave ",
"Showtunes ", "Trailer ", "Lo-Fi ", "Tribal ", "Acid Punk ", "Acid Jazz ", "Polka ",
"Retro ", "Musical ", "Rock & Roll ", "Hard Rock ", "Folk ", "Folk-Rock ", "National Folk ",
"Swing ", "Fast Fusion ", "Bebob ", "Latin ", "Revival ", "Celtic ", "Bluegrass ",
"Avantgarde ", "Gothic Rock ", "Progressive Rock ", "Psychedelic Rock ", "Symphonic Rock ", "Slow Rock ", "Big Band ",
"Chorus ", "Easy Listening ", "Acoustic ", "Humour ", "Speech ", "Chanson ", "Opera ",
"Chamber Music ", "Sonata ", "Symphony ", "Booty Bass ", "Primus ", "Porn Groove ", "Satire",
"Slow Jam", "Club", "Tango", "Samba", "Folklore", "Ballad", "Power Ballad",
"Rhythmic Soul", "Freestyle", "Duet", "Punk Rock", "Drum Solo", "Acapella", "Euro-House",
"Dance Hall", "Goa", "Drum & Bass", "Club\"House", "Hardcore", "Terror", "Indie",
"BritPop", "Negerpunk", "Polsk Punk", "Beat", "Christian Gangsta Rap", "Heavy Metal", "Black Metal",
"Crossover", "Contemporary Christian", "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime",
"JPop", "Synthpop", "Unknown", NULL
};


void jamendo_db_destroy(void)
{
    if(jamendo_sqlhandle)
    {
        sqlite3_close(jamendo_sqlhandle);
        jamendo_sqlhandle = NULL;
    }
}

/* run this before using the other fucntions */
void jamendo_db_init()
{
}

void jamendo_db_open()
{
    gchar *path = NULL;
    gchar *final = NULL;


    /**
     * if open close it
     */
    if(jamendo_sqlhandle)
    {
        sqlite3_close(jamendo_sqlhandle);
        jamendo_sqlhandle = NULL;
    }

//    final = g_get_user_cache_dir();
    path = gmpc_get_cache_directory("jamendo.sqlite3");//g_build_filename(final, "gmpc", "jamendo.sqlite3", NULL);
    sqlite3_open(path, &(jamendo_sqlhandle));
    g_free(path);
}
/* FIXME */
gboolean jamendo_db_has_data()
{
    char *query = sqlite3_mprintf("SELECT * from 'sqlite_master'");
    sqlite3_stmt *stmt = NULL;
    const char *tail;
    int r;
    
    
    r= sqlite3_prepare_v2(jamendo_sqlhandle, query, -1,  &stmt,  &tail);
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

MpdData * jamendo_db_get_genre_list()
{
    MpdData *list = NULL;
    xmlNodePtr **root;
    xmlNodePtr **cur;
    int i;

    char *query = sqlite3_mprintf("SELECT genre from 'tracks' group by genre");
    sqlite3_stmt *stmt;
    const char *tail;
    int r = sqlite3_prepare_v2(jamendo_sqlhandle, query, -1,  &stmt,  &tail);
    if(r ==SQLITE_OK) {
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            list = mpd_new_data_struct_append(list);
            list->type = MPD_DATA_TYPE_TAG;
            list->tag_type = MPD_TAG_ITEM_GENRE;
            list->tag = g_strdup(sqlite3_column_text(stmt,0));
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_free(query);
    return misc_mpddata_remove_duplicate_songs(list); 
}


#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
static char gz_magic[2] = {0x1f, 0x8b};
static
int skip_gzip_header(const unsigned char * src, int size) {
    int idx;
    if (size < 10 || memcmp(src, gz_magic, 2))
        return -1;
    if (src[2] != Z_DEFLATED) {
        fprintf(stderr, "unsupported compression method (%d).\n", (int)src[3]);
        return -1;
    }
    idx = 10;          
    /* skip past header, mtime and xos */

    if (src[3] & EXTRA_FIELD)
        idx += src[idx] + (src[idx+1]<<8) + 2;
    if (src[3] & ORIG_NAME) {
        while (src[idx]) idx++; idx++;
    }
    if (src[3] & COMMENT) {
        while (src[idx]) idx++; idx++; 
    }
    if (src[3] & HEAD_CRC)
        idx += 2;
    return idx;
}

int read_cb(void *z, char *buffer, int size)
{
    z_stream *zs = z;
    if(zs){
        zs->next_out = (void *)buffer;
        zs->avail_out = size;
        int r = inflate(zs, Z_SYNC_FLUSH);
        if(r == Z_OK || r == Z_STREAM_END){
            return size-zs->avail_out;
        }
    }
    printf("failed unzipping stream\n");
    return -1;
}
int close_cb (void *z)
{
    printf("Close unzip stream\n");
    z_stream *zs = z;
    inflateEnd(zs);
    g_free(zs);
    return 0;
}

void jamendo_db_load_data(const char *data, const goffset length)
{
    gchar *error = NULL;
    xmlTextReaderPtr reader = NULL;
    gchar *path;

    if(data){
        gssize size = (gssize)length;
        long length2;
        gchar *path = NULL;
        z_stream *zs = g_malloc0(sizeof*zs);

        long data_start = skip_gzip_header(data,size);
        if (data_start == -1) return ;
        zs->next_in  = (void *)((data)+ data_start);
        zs->avail_in = size- data_start;
        if (inflateInit2(zs, -MAX_WBITS) != Z_OK)
            return;
        reader = xmlReaderForIO(read_cb, close_cb,zs, NULL, NULL, 0);
        if(reader == NULL)
        {   
            /* Cleanup */
            close_cb(zs);
            return;
        }

    }
    else
    {
        /* Nothing downloaded, nothing to update */
        return;
    }
    /* open the db if it is closed */
    if(!jamendo_sqlhandle) {
        path = gmpc_get_user_path("jamendo.sqlite3"); 

        int retv = sqlite3_open(path, &(jamendo_sqlhandle));
        g_free(path);
        if(retv != SQLITE_OK)
        {
            /* Cleanup */
            xmlFreeTextReader(reader);
//            gmpc_easy_download_clean(dld);
//            g_free(dld);
            return;
        }
    }
    sqlite3_exec(jamendo_sqlhandle, "DROP TABLE Tracks",NULL, NULL, NULL);
    sqlite3_exec(jamendo_sqlhandle, "DROP TABLE Artist",NULL, NULL, NULL);
    sqlite3_exec(jamendo_sqlhandle, "DROP TABLE Album",NULL, NULL, NULL);
    sqlite3_exec(jamendo_sqlhandle, "VACUUM;",NULL, NULL, NULL);



    sqlite3_exec(jamendo_sqlhandle, "CREATE TABLE 'Tracks' ("
            "'id' INTEGER PRIMARY KEY AUTOINCREMENT,"
            "'artist'   TEXT, "
            "'album'    TEXT, "
            "'genre'    TEXT, "
            "'title'    TEXT, "
            "'duration' INTEGER, "
            "'track'    TEXT, "
            "'trackid'       INTEGER "
            ")"
            ,NULL, NULL, &error);
    if(error)
    {
        printf("Error: %s\n", error);
    }

    sqlite3_exec(jamendo_sqlhandle, "CREATE TABLE 'Artist' ("
            "'id' INTEGER PRIMARY KEY AUTOINCREMENT,"
            "'artist'   TEXT, "
            "'artistid' TEXT, "
            "'image'    TEXT "
            ")"
            ,NULL, NULL, &error);
    if(error)
    {
        printf("Error: %s\n", error);
    }

    sqlite3_exec(jamendo_sqlhandle, "CREATE TABLE 'Album' ("
            "'id' INTEGER PRIMARY KEY AUTOINCREMENT,"
            "'artist'   TEXT, "
            "'album'   TEXT, "
            "'albumid'  TEXT, "
            "'genre'    TEXT, "
            "'image'    TEXT "
            ")"
            ,NULL, NULL, &error);
    if(error)
    {
        printf("Error: %s\n", error);
    }

    sqlite3_exec(jamendo_sqlhandle, "BEGIN;", NULL, NULL, NULL);
    int retv = xmlTextReaderRead(reader);
    while(retv==1)
    {
        const xmlChar *name = xmlTextReaderConstName(reader);
        if(name && xmlStrcmp(name, "artist") == 0)
        {
            xmlNodePtr root = xmlTextReaderExpand(reader);
            xmlNodePtr cur_artist = root->xmlChildrenNode;
            xmlChar *image = NULL;
            xmlChar *artist = NULL;
            xmlNodePtr cur_albums = NULL;
            while(cur_artist)
            {
                if(!xmlStrcmp(cur_artist->name, "name")) {
                    artist = xmlNodeGetContent(cur_artist);
                }
                if(!xmlStrcmp(cur_artist->name, "image")) {
                    image = xmlNodeGetContent(cur_artist);
                }
                if(!xmlStrcmp(cur_artist->name, "Albums")) {
                    cur_albums = cur_artist->xmlChildrenNode; 
                }
                cur_artist = cur_artist->next;
            }
            if(artist && cur_albums)
            {
                char *query = sqlite3_mprintf("INSERT INTO 'Artist' ('artist', 'image') VALUES('%q','%q')",
                        artist,
                        (image != NULL)?image:""); 
                sqlite3_exec(jamendo_sqlhandle, query, NULL, NULL, NULL);
                sqlite3_free(query);
                while(cur_albums)
                {
                    char *albumid = NULL;
                    char *album = NULL;
                    gint genre = 0;
                    xmlNodePtr cur_tracks = NULL;
                    xmlNodePtr t = cur_albums->xmlChildrenNode;
                    while(t)
                    {
                        if(xmlStrcmp(t->name, "name") == 0){
                            album = xmlNodeGetContent(t);
                        }
                        if(xmlStrcmp(t->name, "Tracks") == 0){
                            cur_tracks =  t->xmlChildrenNode;
                        }
                        if(xmlStrcmp(t->name, "id3genre") == 0){
                            xmlChar *temp = xmlNodeGetContent(t);
                            genre = atoi(temp);
                            xmlFree(temp);
                        }
                        if(xmlStrcmp(t->name, "id") == 0){
                            albumid = xmlNodeGetContent(t);
                        }
                        t = t->next ;
                    }
                    if(album && cur_tracks) {
                        const char *genre_name= GENRE_LIST[genre];
                        gchar *error = NULL; 
                        char *albumimage = g_strdup_printf("http://api.jamendo.com/get2/image/album/redirect/?id=%s&imagesize=600",albumid);
                        char *query = sqlite3_mprintf("INSERT INTO 'Album' ('artist','album','genre','albumid', 'image') VALUES('%q','%q','%q','%q','%q')",
                                artist,
                                album,
                                genre_name,
                                albumid, 
                                albumimage
                                );
                        sqlite3_exec(jamendo_sqlhandle, query, NULL, NULL,&error);
                        sqlite3_free(query);
                        g_free(albumimage);
                        if(error){
                            printf("Error: %s\n", error);
                        }

                        while(cur_tracks)
                        {
                            if(xmlStrcmp(cur_tracks->name, "track")==0)
                            {
                                gchar *title, *duration, *id;
                                xmlNodePtr track = cur_tracks->xmlChildrenNode;
                                while(track)
                                {
                                    if(xmlStrcmp(track->name, "name")==0) {
                                        title  =xmlNodeGetContent(track);
                                    }

                                    if(xmlStrcmp(track->name, "duration")==0) {
                                        duration =xmlNodeGetContent(track);
                                    }

                                    if(xmlStrcmp(track->name, "id")==0) {
                                        id =xmlNodeGetContent(track);
                                    }
                                    track = track->next;
                                }
                                char *query = sqlite3_mprintf("INSERT INTO 'tracks' ('artist', 'album', 'genre','title','duration', 'trackid') VALUES('%q','%q', '%q','%q','%q',%q)",
                                        artist,
                                        album,
                                        genre_name, 
                                        title,
                                        duration,
                                        id);
                                //printf("query: %s\n",query);
                                sqlite3_exec(jamendo_sqlhandle, query, NULL, NULL, NULL);
                                sqlite3_free(query);
                                if(id) xmlFree(id);
                                if(duration) xmlFree(duration);
                                if(title) xmlFree(title);
                            }
                            cur_tracks =cur_tracks->next;
                        }
                    }

                    cur_albums = cur_albums->next;
                    if(album) xmlFree(album);
                    if(albumid) xmlFree(albumid);
                }
            }
            if(artist) xmlFree(artist);
            if(image) xmlFree(image);
            retv =xmlTextReaderNext(reader);
        }
        else
            retv = xmlTextReaderRead(reader);
    }
    printf("indexes\n");
    sqlite3_exec(jamendo_sqlhandle, "CREATE INDEX AlbumAlbum ON Album(album);", NULL, NULL, &error);
    if(error)printf("Error %i; %s\n",__LINE__, error);
    sqlite3_exec(jamendo_sqlhandle, "CREATE INDEX AlbumGenre ON Album(genre);", NULL, NULL, &error);
    if(error)printf("Error %i; %s\n",__LINE__, error);
    sqlite3_exec(jamendo_sqlhandle, "CREATE INDEX ArtistArtist ON Artist(artist);", NULL, NULL, &error);
    if(error)printf("Error %i; %s\n",__LINE__, error);
    sqlite3_exec(jamendo_sqlhandle, "CREATE INDEX TracksArtist ON Tracks(artist);", NULL, NULL, &error);
    if(error)printf("Error %i; %s\n",__LINE__, error);
    sqlite3_exec(jamendo_sqlhandle, "CREATE INDEX TracksAlbum ON Tracks(album);", NULL, NULL, &error);
    if(error)printf("Error %i; %s\n",__LINE__, error);
    sqlite3_exec(jamendo_sqlhandle, "CREATE INDEX TracksGenre ON Tracks(genre);", NULL, NULL, &error);
    if(error)printf("Error %i; %s\n",__LINE__, error);

    printf("flushing\n");
    sqlite3_exec(jamendo_sqlhandle, "END;", NULL, NULL, NULL);

    printf("done\n");

    xmlFreeTextReader(reader);
//    gmpc_easy_download_clean(dld);
//    g_free(dld);

  //  g_idle_add(jamendo_end_download, NULL);
}
/*
void jamendo_db_download_xml(gpointer data )
{
    gmpc_easy_download_struct *dld = g_malloc0(sizeof(*dld));
    dld->callback_data = data;
    dld->callback = cb;
    dld->max_size = -1;
    dld->data = NULL;
    dld->size = 0;
    g_thread_create((GThreadFunc)jamendo_db_download_xml_thread, dld,FALSE, NULL);

}
*/
MpdData * jamendo_db_get_artist_list(char *wanted_genre)
{
    MpdData *list = NULL;
    xmlNodePtr root;
    xmlNodePtr cur;
    /** check if there is data */
    char *query = sqlite3_mprintf("SELECT artist from 'tracks' WHERE genre=%Q  group by artist", wanted_genre);
    sqlite3_stmt *stmt;
    const char *tail;
    int r = sqlite3_prepare_v2(jamendo_sqlhandle, query, -1,  &stmt,  &tail);
    sqlite3_free(query);
    if(r ==SQLITE_OK) {
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            list = mpd_new_data_struct_append(list);
            list->type = MPD_DATA_TYPE_TAG;
            list->tag_type = MPD_TAG_ITEM_ARTIST;
            list->tag = g_strdup(sqlite3_column_text(stmt,0));
        }
        sqlite3_finalize(stmt);
    }
    return list;// misc_mpddata_remove_duplicate_songs(list);
}

MpdData *jamendo_db_get_album_list(char *wanted_genre,char *wanted_artist)
{
    MpdData *list = NULL;
    xmlNodePtr **root;
    xmlNodePtr **cur;
    /** check if there is data */

    char *query = sqlite3_mprintf("SELECT album from 'tracks' WHERE artist=%Q AND genre=%Q group by album",wanted_artist,wanted_genre);
    sqlite3_stmt *stmt;
    const char *tail;
    int r = sqlite3_prepare_v2(jamendo_sqlhandle, query, -1,  &stmt,  &tail);

    sqlite3_free(query);
    if(r ==SQLITE_OK) {
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            list = mpd_new_data_struct_append(list);
            list->type = MPD_DATA_TYPE_TAG;
            list->tag_type = MPD_TAG_ITEM_ALBUM;
            list->tag = g_strdup(sqlite3_column_text(stmt,0));
        }
        sqlite3_finalize(stmt);
    }
    return mpd_data_get_first(list); 
}

MpdData* jamendo_db_get_song_list(const char *wanted_genre,const char *wanted_artist, const char *wanted_album, gboolean exact)
{
    MpdData *data = NULL;
    xmlNodePtr * root;
    xmlNodePtr * cur;
    char *query;

    if(!wanted_genre && !wanted_artist && !wanted_album) return NULL;

    if(exact)
    {
        char *a=NULL,*b=NULL,*c=NULL;
        if(wanted_genre) {
               a = sqlite3_mprintf("genre=%Q", wanted_genre); 
        }else a = sqlite3_mprintf(""); 

        if(wanted_album) {
               b = sqlite3_mprintf("album=%Q", wanted_album); 
        }else b = sqlite3_mprintf("");

        if(wanted_artist) {
               c = sqlite3_mprintf("artist=%Q", wanted_artist); 
        }else c = sqlite3_mprintf("");
        query = sqlite3_mprintf("SELECT artist,album,genre,title,duration,track,trackid from 'tracks' WHERE %s %s %s %s %s",
        a, (a[0] && (b[0] ||c[0]))?"AND":"",b,(b[0] && c[0])?"AND":"",c) ;
        sqlite3_free(c);
        sqlite3_free(b);
        sqlite3_free(a);
    }else{
        char *a=NULL,*b=NULL,*c=NULL;
        if(wanted_genre) {
               a = sqlite3_mprintf("genre LIKE '%%%%%q%%%%'", wanted_genre); 
        }else a = sqlite3_mprintf(""); 

        if(wanted_album) {
               b = sqlite3_mprintf("album LIKE '%%%%%q%%%%'", wanted_album); 
        }else b = sqlite3_mprintf("");

        if(wanted_artist) {
               c = sqlite3_mprintf("artist LIKE '%%%%%q%%%%'", wanted_artist); 
        }else c = sqlite3_mprintf("");
        query = sqlite3_mprintf("SELECT artist,album,genre,title,duration,track,trackid from 'tracks' WHERE %s %s %s %s %s",
        a, (a[0] && (b[0] ||c[0]))?"AND":"",b,(b[0] && c[0])?"AND":"",c) ;
        sqlite3_free(c);
        sqlite3_free(b);
        sqlite3_free(a);
    }
    sqlite3_stmt *stmt;
    const char *tail;
    int r = sqlite3_prepare_v2(jamendo_sqlhandle, query, -1,  &stmt,  &tail);
    sqlite3_free(query);
    if(r ==SQLITE_OK) {
        printf("creating list\n");
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            data = (MpdData *)mpd_new_data_struct_append((MpdData *)data);
            data->type = MPD_DATA_TYPE_SONG;
            data->song = mpd_newSong();
            data->song->file = g_strdup_printf("http://api.jamendo.com/get2/stream/track/redirect/?id=%i&streamencoding=ogg2", 
                    sqlite3_column_int(stmt, 6)); 
            data->song->title = g_strdup(sqlite3_column_text(stmt, 3));
            data->song->album = g_strdup(sqlite3_column_text(stmt, 1));
            data->song->artist = g_strdup(sqlite3_column_text(stmt, 0));
            data->song->genre = g_strdup(sqlite3_column_text(stmt, 2));
            data->song->time =  sqlite3_column_int(stmt, 4);
            data->song->track =  g_strdup(sqlite3_column_text(stmt, 5));
        }
        sqlite3_finalize(stmt);

        printf("creating list done\n");
    }
    return mpd_data_get_first(data);
}
MpdData* jamendo_db_title_search( const char *wanted_title)
{
    MpdData *data = NULL;
    xmlNodePtr * root;
    xmlNodePtr * cur;
    char *query;

    if(!wanted_title ) return NULL;

    query = sqlite3_mprintf("SELECT artist,album,genre,title,duration,track,trackid from 'Tracks' WHERE title LIKE '%%%%%q%%%%'",
                        wanted_title);

    sqlite3_stmt *stmt;
    const char *tail;
    int r = sqlite3_prepare_v2(jamendo_sqlhandle, query, -1,  &stmt,  &tail);
    sqlite3_free(query);
    if(r ==SQLITE_OK) {
        printf("creating list\n");
        while((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            data = (MpdData *)mpd_new_data_struct_append((MpdData *)data);
            data->type = MPD_DATA_TYPE_SONG;
            data->song = mpd_newSong();
            data->song->file = g_strdup_printf("http://api.jamendo.com/get2/stream/track/redirect/?id=%i&streamencoding=ogg2", 
                    sqlite3_column_int(stmt, 6)); 
            data->song->title = g_strdup(sqlite3_column_text(stmt, 3));
            data->song->album = g_strdup(sqlite3_column_text(stmt, 1));
            data->song->artist = g_strdup(sqlite3_column_text(stmt, 0));
            data->song->genre = g_strdup(sqlite3_column_text(stmt, 2));
            data->song->time =  sqlite3_column_int(stmt, 4);
            data->song->track =  g_strdup(sqlite3_column_text(stmt, 5));
        }
        sqlite3_finalize(stmt);

        printf("creating list done\n");
    }
    return mpd_data_get_first(data);
}




gchar *jamendo_get_artist_image(const gchar *wanted_artist)
{
    sqlite3_stmt *stmt;
    gchar *retv = NULL;
    const char *tail;
    char *query = sqlite3_mprintf("SELECT image FROM 'Artist' WHERE artist LIKE '%%%%%q%%%%'", wanted_artist);
    int r = sqlite3_prepare_v2(jamendo_sqlhandle, query, -1,  &stmt,  &tail);
    sqlite3_free(query);
    if(r ==SQLITE_OK) {
        if((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            retv = g_strdup(sqlite3_column_text(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }

    return retv;
}

gchar *jamendo_get_album_image(const gchar *wanted_artist, const gchar *wanted_album)
{
    sqlite3_stmt *stmt;
    gchar *retv = NULL;
    const char *tail;
    char *query = sqlite3_mprintf("SELECT image FROM 'Album' WHERE artist LIKE '%%%%%q%%%%' AND album LIKE '%%%%%q%%%%'", wanted_artist,wanted_album);
    int r = sqlite3_prepare_v2(jamendo_sqlhandle, query, -1,  &stmt,  &tail);
    sqlite3_free(query);
    if(r ==SQLITE_OK) {
        if((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            retv = g_strdup(sqlite3_column_text(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }

    return retv;
}


gchar *jamendo_get_album_url(const gchar *wanted_artist, const gchar *wanted_album)
{
    sqlite3_stmt *stmt;
    gchar *retv = NULL;
    const char *tail;
    char *query = sqlite3_mprintf("SELECT albumid FROM 'Album' WHERE artist LIKE '%%%%%q%%%%' AND album LIKE '%%%%%q%%%%'", wanted_artist,wanted_album);
    int r = sqlite3_prepare_v2(jamendo_sqlhandle, query, -1,  &stmt,  &tail);
    sqlite3_free(query);
    if(r ==SQLITE_OK) {
        if((r = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            retv = g_strdup_printf("http://www.jamendo.com/album/%s", sqlite3_column_text(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }

    return retv;
}
