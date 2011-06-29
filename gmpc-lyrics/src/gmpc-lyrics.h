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

#ifndef __GMPC_LYRICS_H__
#define __GMPC_LYRICS_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <config.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlstring.h>

#include <libmpd/debug_printf.h>

#include <gmpc/plugin.h>
#include <gmpc/gmpc_easy_download.h>

/** 
 * We need gmpc's threading abstraction.
 */

#define PLUGIN_AUTH "GMPC+Lyrics+Plugin"
#define USER_AGENT "User-Agent: GmpcSoapLyrics/0.1"

/* Lyrics plugin */
struct lyrics_api
{
  gchar *name;
  gchar *host;

  gchar *search_full;
  gchar *search_title;

  gchar *lyrics_uri;

  gchar *(*get_id)(xmlDocPtr results_doc, gchar *artist, gchar *songtitle, int exact);
  gchar *(*get_lyrics)(const gchar *data, gssize length); /* should return the lyrics if availble or NULL else */

  
};

void lyrics_init (void);
void lyrics_enable_toggle (GtkWidget *wid);
void lyrics_api_changed (GtkWidget *wid);
void lyrics_construct (GtkWidget *container);
void lyrics_destroy (GtkWidget *container);


/* XML Parsing */
xmlNodePtr get_node_by_name (xmlNodePtr node, xmlChar *name);

/* LeosLyrics API */
gchar *__leoslyrics_get_id (xmlDocPtr results_doc, gchar *artist, gchar *songtitle, int exact);
gchar *__leoslyrics_get_lyrics (const gchar *srcdata, gssize srcsize);

/* LyricTracker API */
gchar *__lyrictracker_get_id (xmlDocPtr results_doc, gchar *artist, gchar *songtitle, int exact);
gchar *__lyrictracker_get_lyrics (const gchar *srcdata, gssize srcsize);

#endif
