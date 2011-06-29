/* gmpc-lirc (GMPC plugin)
 * Copyright (C) 2007-2009 Qball Cow <qball@sarine.nl>
 * Copyright (C) 2007 Igor Stirbu <igor.stirby@gmail.com>
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
/*
 * vim:ts=4:sw=4
 */
#include <stdio.h>
#include <string.h>
#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gmpc/plugin.h>
#include <lirc/lirc_client.h>
#include <libmpd/debug_printf.h>

#define SEEK_STEP_SIZE 5
int plugin_api_version = PLUGIN_API_VERSION;

/*
 * GMPC Plugin functions
 * */
static int plugin_get_enabled(void);
static void plugin_set_enabled(int enabled);

static void plugin_init(void);
static void plugin_destroy(void);

gboolean lirc_handler(GIOChannel *source,
		GIOCondition condition, gpointer data);

/*
 * LIRC vars and functions
 * */
int lirc_fd = -1;
struct lirc_config *lirc_cfg = NULL;
GIOChannel *channel = NULL;
guint input_tag = 0;


static const gchar *plugin_get_translation_domain(void)
{
    return GETTEXT_PACKAGE;
}

/*
 * LIRC Plugin
 * */
gmpcPlugin plugin = {
	.name			= "LIRC Plugin",
	.version		= {0,14,0},
	.plugin_type	= GMPC_PLUGIN_NO_GUI,
	.init			= plugin_init,
	.destroy		= plugin_destroy,
	.get_enabled	= plugin_get_enabled,
	.set_enabled	= plugin_set_enabled,
    .get_translation_domain = plugin_get_translation_domain
};

/*
 * Plugin enabled get/set
 * */
static int plugin_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, "lirc-plugin", "enable", TRUE);
}

static void plugin_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "lirc-plugin", "enable", enabled);
}

/*
 * LIRC init and destroy
 * */

static int init_lirc()
{
	int err = 0;

	lirc_fd = lirc_init("gmpc", 1);
	if (lirc_fd == -1) {
		debug_printf(DEBUG_INFO, "lirc_init() failed");
		return 1;
	}

	err = lirc_readconfig(NULL, &lirc_cfg, NULL);
	if (err == -1) {
		lirc_deinit();
		debug_printf(DEBUG_INFO, "lirc_readconfig() failed");
		return 1;
	}

	return 0;
}
static void destroy_lirc()
{
	if (lirc_cfg) {
		lirc_freeconfig(lirc_cfg);
		lirc_cfg = NULL;
	}
	if (lirc_fd != -1) {
		lirc_deinit();
		lirc_fd = -1;
	}
}

static int init_channel()
{
	GIOStatus status = G_IO_STATUS_NORMAL;

	channel = g_io_channel_unix_new(lirc_fd);
	if (channel == NULL) {
		debug_printf(DEBUG_INFO, "g_io_channel_unix_new() failed");
		return 1;
	}

	status = g_io_channel_set_flags(channel,
			G_IO_FLAG_IS_READABLE | G_IO_FLAG_NONBLOCK, NULL);
	if (status != G_IO_STATUS_NORMAL) {
		debug_printf(DEBUG_INFO, "g_io_channel_set_flags() failed");
		return 1;
	}

	input_tag = g_io_add_watch(channel, G_IO_IN,
			lirc_handler, NULL);
	if (input_tag < 0) {
		debug_printf(DEBUG_INFO, "g_io_add_watch() failed");
		return 1;
	}

	return 0;
}
static void destroy_channel()
{
	if (input_tag) {
		g_source_remove(input_tag);
		input_tag = 0;
	}

	if (channel) {
		g_io_channel_shutdown(channel, TRUE, NULL);
		channel = NULL;
	}
}

void plugin_init()
{
	int err = 0;

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");


	err = init_lirc();
	if (err) {
		debug_printf(DEBUG_INFO, "init_lirc() failed");
		return;
	}

	err = init_channel();
	if (err) {
		debug_printf(DEBUG_INFO, "init_channel() failed");
		destroy_lirc();
		return;
	}
}

void plugin_destroy(void)
{
	destroy_channel();
	destroy_lirc();
}

gboolean lirc_handler(GIOChannel *source,
		GIOCondition condition, gpointer data)
{
	int err;
	char *code, *c;
	while((err = lirc_nextcode(&code))==0 && code!=NULL)
	{
		while((err = lirc_code2char(lirc_cfg, code, &c))==0 && c!=NULL)
		{
			if (strcasecmp("PLAYPAUSE",c) == 0) {
				debug_printf(DEBUG_INFO, "PLAYPAUSE");
				switch(mpd_player_get_state(connection))
				{
					case MPD_PLAYER_STOP:
						mpd_player_play(connection);
						break;
					case MPD_PLAYER_PLAY:
					case MPD_PLAYER_PAUSE:
						mpd_player_pause(connection);
					default:
						/* do nothing */
						break;
				}
				
			}
			else if (strcasecmp("NEXT",c) == 0) {
				mpd_player_next(connection);
			}
			else if (strcasecmp("PREV",c) == 0) {
				mpd_player_prev(connection);
			}
			else if (strcasecmp("STOP",c) == 0) {
				mpd_player_stop(connection);
			}
			else if (strcasecmp("FASTFORWARD",c) == 0) {
				mpd_player_seek(connection, mpd_status_get_elapsed_song_time(connection)+SEEK_STEP_SIZE);
			}
			else if (strcasecmp("FASTBACKWARD",c) == 0) {
				mpd_player_seek(connection, mpd_status_get_elapsed_song_time(connection)-SEEK_STEP_SIZE);
			}
			else if (strcasecmp("REPEAT",c) == 0) {
				mpd_player_set_repeat(connection, !mpd_player_get_repeat(connection));
			}
			else if (strcasecmp("RANDOM",c) == 0) {
				mpd_player_set_random(connection, !mpd_player_get_random(connection));
			}
			else if (strcasecmp("RAISE",c) == 0) {
				create_playlist3();
			}
			else if (strcasecmp("HIDE",c) == 0) {
				pl3_hide();
			}
			else if (strcasecmp("TOGGLE_HIDDEN",c) == 0) {
				pl3_toggle_hidden();
			}
			else if (strcasecmp("VOLUME_UP",c) == 0) {
				mpd_status_set_volume(connection, mpd_status_get_volume(connection)+5);
			}
			else if (strcasecmp("VOLUME_DOWN",c) == 0) {
				mpd_status_set_volume(connection, mpd_status_get_volume(connection)-5);
			}
			else if (strcasecmp("SHOW_NOTIFICATION",c) == 0) {
				tray_icon2_create_tooltip();
			}
		}
	}

	return TRUE;
}
