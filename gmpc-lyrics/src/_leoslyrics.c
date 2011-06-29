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

gchar *
__leoslyrics_get_id (xmlDocPtr results_doc, gchar *artist, gchar *songtitle, int exact)
{
  gchar *hid = NULL;
  gchar *exact_match = NULL;
  xmlNodePtr root, results, node;

  root = xmlDocGetRootElement(results_doc);
  if (!root)
    return NULL;

  results = get_node_by_name (root->xmlChildrenNode, (xmlChar *)"searchResults");
  if (!results)
    return NULL;

  node = get_node_by_name (results->xmlChildrenNode, (xmlChar *)"title");
  
  node = get_node_by_name (results->xmlChildrenNode, (xmlChar *)"result");
  if (exact) {
    exact_match = (gchar *) xmlGetProp(node, (xmlChar*)"exactMatch");
    if(g_ascii_strcasecmp(exact_match, "true")) {
      node = NULL; // not exact match
    }
  }
  if (node) {
    hid = (gchar *)xmlGetProp(node, (xmlChar *)"hid");
  }
  if(exact_match)
    xmlFree(exact_match);

  return hid;
}

gchar *
__leoslyrics_get_lyrics (const gchar *srcdata, gssize srcsize)
{
  gchar *data = NULL;
  gchar *converted = NULL;
  xmlDocPtr lyrics_doc;
  xmlNodePtr root, lyric, node;



  lyrics_doc = xmlParseMemory(srcdata, srcsize);
  if (!lyrics_doc)
    return NULL;

  root = xmlDocGetRootElement(lyrics_doc);
  if (!root)
    return NULL;

  lyric = get_node_by_name (root->xmlChildrenNode, (xmlChar *)"lyric");
  if (!lyric)
    {
      xmlFreeDoc(lyrics_doc);
      return NULL;
    }




  node = get_node_by_name (lyric->xmlChildrenNode, (xmlChar *)"text");
  if (node)
    data = (gchar *)xmlNodeGetContent(node);

  converted  = g_strdup(data);
	 /* g_convert_with_fallback(data,strlen(data),
		  "utf-8", "ISO-8859-1",                   
		  " ",NULL,NULL,NULL);
		  */
  xmlFree(data);
  xmlFreeDoc(lyrics_doc);
  return converted;
}

/* the following functions are not used but left here in 
 * case somebody needs them later
 */
 /*
static gchar *
__leoslyrics_get_artist (xmlDocPtr results_doc, const gchar *srcdata, gssize srcsize,
                         gchar *hid)
{
  gchar *artist = NULL;
  xmlDocPtr lyrics_doc;
  xmlNodePtr root, lyric, node;

  lyrics_doc = xmlParseMemory(srcdata, srcsize);
  if (!lyrics_doc)
    return NULL;

  root = xmlDocGetRootElement(lyrics_doc);
  if (!root)
    return NULL;

  lyric = get_node_by_name (root->xmlChildrenNode, (xmlChar *)"lyric");
  if (!lyric)
    {
      xmlFreeDoc(lyrics_doc);
      return NULL;
    }

  node = get_node_by_name (lyric->xmlChildrenNode, (xmlChar *)"artist");
  if (node)
    {
      xmlNodePtr temp = get_node_by_name (node->xmlChildrenNode,
                                          (xmlChar *)"name");
      artist = (gchar *)xmlNodeGetContent(temp);
    }

  xmlFreeDoc(lyrics_doc);
  return artist;
}

static gchar *
__leoslyrics_get_songtitle (xmlDocPtr results_doc, const gchar *srcdata, gssize srcsize,
                            gchar *hid)
{
  gchar *songtitle = NULL;
  xmlDocPtr lyrics_doc;
  xmlNodePtr root, lyric, node;

  lyrics_doc = xmlParseMemory(srcdata, srcsize);
  if (!lyrics_doc)
    return NULL;

  root = xmlDocGetRootElement(lyrics_doc);
  if (!root)
    return NULL;

  lyric = get_node_by_name (root->xmlChildrenNode, (xmlChar *)"lyric");
  if (!lyric)
    {
      xmlFreeDoc(lyrics_doc);
      return NULL;
    }

  node = get_node_by_name (lyric->xmlChildrenNode, (xmlChar *)"title");
  if (node)
    songtitle = (gchar *)xmlNodeGetContent(node);

  xmlFreeDoc(lyrics_doc);
  return songtitle;
}
*/
