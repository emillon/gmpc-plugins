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
__lyrictracker_get_id (xmlDocPtr results_doc, gchar *artist, gchar *songtitle, int exact)
{
  gchar *hid = NULL;
  xmlNodePtr results, node;

  results = xmlDocGetRootElement(results_doc);
  if (!results)
    return NULL;

  /* If no results returned, don't even search in them */
  if (strcmp((gchar *)xmlGetProp(results, (xmlChar *)"count"), "0") == 0)
    return NULL;

  node = get_node_by_name (results->xmlChildrenNode, (xmlChar *)"result");
  while (node)
    {
	// TODO: figure out how to match exact when lyrictracker comes back up again
      if (/*(strcasecmp((gchar *)xmlGetProp(node, (xmlChar *)"artist"),
                      artist) == 0) &&*/
          (strcasecmp((gchar *)xmlGetProp(node, (xmlChar *)"title"),
                      songtitle) == 0))
        {
          hid = (gchar *)xmlGetProp(node, (xmlChar *)"id");
          if (hid)
            return hid;
        }

      node = get_node_by_name (node->next, (xmlChar *)"result");
    }

  return NULL;
}

gchar *
__lyrictracker_get_lyrics (const gchar *srcdata, gssize srcsize)
{
  gsize converted;

  return g_convert(srcdata, srcsize, "UTF-8", "ISO-8859-1",
                   NULL, &converted, NULL);
}

/* the following functions are not used but left here in 
 * case somebody needs them later
 */
 /*
gchar *
__lyrictracker_get_artist (xmlDocPtr results_doc, const gchar *srcdata, gssize srcsize,
                           gchar *hid)
{
  gchar *artist = NULL;
  xmlNodePtr results, node;

  results = xmlDocGetRootElement(results_doc);
  if (!results)
    return NULL;
*/
  /* If no results returned, don't even search in them */
/*  if (strcmp((gchar *)xmlGetProp(results, (xmlChar *)"count"), "0") == 0)
    return NULL;

  node = get_node_by_name (results->xmlChildrenNode, (xmlChar *)"result");
  while (node)
    {
      if (strcmp((gchar *)xmlGetProp(node, (xmlChar *)"id"), hid) == 0)
        {
          artist = (gchar *)xmlGetProp(node, (xmlChar *)"artist");
          if (artist)
            return artist;
        }

      node = get_node_by_name (node->next, (xmlChar *)"result");
    }

  return NULL;
}

gchar *
__lyrictracker_get_songtitle (xmlDocPtr results_doc, const gchar *srcdata, gssize srcsize,
                              gchar *hid)
{
  gchar *songtitle = NULL;
  xmlNodePtr results, node;

  results = xmlDocGetRootElement(results_doc);
  if (!results)
    return NULL;
*/
  /* If no results returned, don't even search in them */
/*  if (strcmp((gchar *)xmlGetProp(results, (xmlChar *)"count"), "0") == 0)
    return NULL;

  node = get_node_by_name (results->xmlChildrenNode, (xmlChar *)"result");
  while (node)
    {
      if (strcmp((gchar *)xmlGetProp(node, (xmlChar *)"id"), hid) == 0)
        {
          songtitle = (gchar *)xmlGetProp(node, (xmlChar *)"title");
          if (songtitle)
            return songtitle;
        }

      node = get_node_by_name (node->next, (xmlChar *)"result");
    }

  return NULL;
}
*/
