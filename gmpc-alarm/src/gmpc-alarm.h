/* gmpc-alarm (GMPC plugin)
 * Copyright (C) 2008-2009 Qball Cow <qball@sarine.nl>
 * Copyright (C) 2006-2008 Gavin Gilmour <gavin@brokentrain.net>
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

#ifndef __GMPC_ALARM_H__
#define __GMPC_ALARM_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmpc/plugin.h>
#include <libmpd/debug_printf.h>

void alarm_init (void);
void alarm_construct (GtkWidget *container);
void alarm_destroy (GtkWidget *container);

#endif
