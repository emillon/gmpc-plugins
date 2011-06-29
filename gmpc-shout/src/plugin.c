/* gmpc-shout (GMPC plugin)
 * Copyright (C) 2007-2009 Qball Cow <qball@sarine.nl>
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
#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gmpc/plugin.h>
#include <gmpc/playlist3-messages.h>
#ifndef __WIN32__
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define SHOUT_COMMAND "mplayer -ao pulse -nocache http://192.150.0.120:8000/mpd.ogg"
#include <config.h>


gboolean stopped = FALSE;
GPid ogg123_pid = -1;


static GtkWidget *si_shout = NULL;
static GtkWidget *shout_vbox = NULL;
static void start_ogg123(void);
static void stop_ogg123(void);
void shout_mpd_status_changed(MpdObj *mi, ChangedStatusType what, void *data);
void shout_construct(GtkWidget *container);
void shout_destroy(GtkWidget *container);
static int shout_get_enabled(void);
static void shout_set_enabled(int enabled);

gmpcPrefPlugin shout_gpp = {
	shout_construct,
	shout_destroy
};
gmpcPlugin plugin;


int plugin_api_version = PLUGIN_API_VERSION;

static void shout_plugin_destroy(void);

static const char * shout_get_translation_domain(void)
{
    return GETTEXT_PACKAGE;
}

static void shout_si_start(void)
{
    stopped = FALSE;
    start_ogg123();

}
static void shout_si_stop(void)
{
    stop_ogg123();
    stopped = TRUE;
}

static void shout_si_show_pref(void)
{
    preferences_show_pref_window(plugin.id);
}

static gboolean shout_si_button_press_event(GtkWidget *icon, GdkEventButton *event, gpointer data)
{
    if(event->button == 3)
    {
        GtkWidget *item; 
        GtkMenu *menu = gtk_menu_new();
        g_object_ref_sink(G_OBJECT(menu));

        if(ogg123_pid < 0 && mpd_player_get_state(connection) == MPD_STATUS_STATE_PLAY) {
            item = gtk_image_menu_item_new_with_label(_("Start"));
            g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(shout_si_start), NULL);
            gtk_menu_shell_append(GTK_MENU(menu), item);
        }
        else if (ogg123_pid >= 0 && mpd_player_get_state(connection) == MPD_STATUS_STATE_PLAY) {
            item = gtk_image_menu_item_new_with_label(_("Stop"));
            g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(shout_si_stop), NULL);
            gtk_menu_shell_append(GTK_MENU(menu), item);
        }
        item = gtk_image_menu_item_new_with_label(_("Preferences"));
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(shout_si_show_pref), NULL);
        gtk_menu_shell_append(GTK_MENU(menu), item);

        gtk_widget_show_all(GTK_WIDGET(menu));
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
        g_object_unref(menu);
        /* Again, check for active page and set button otherwise */
        return TRUE;
    }
    return FALSE;
}
static void shout_add_si_icon(void)
{
    if(shout_get_enabled() && si_shout == NULL) {
        GtkWidget *image = gtk_image_new_from_icon_name("add-url", GTK_ICON_SIZE_MENU);
        si_shout = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(si_shout), image);
        main_window_add_status_icon(si_shout); 
        gtk_widget_show_all(si_shout);
        gtk_widget_set_sensitive(gtk_bin_get_child(GTK_BIN(si_shout)), FALSE);

        g_signal_connect(G_OBJECT(si_shout), "button-press-event", G_CALLBACK(shout_si_button_press_event),NULL);

        gtk_widget_set_tooltip_text(si_shout, _("Shout plugin"));
    }
}
static void shout_init(void)
{
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    gtk_init_add((GtkFunction)shout_add_si_icon, NULL);
}

/* main plugin_osd info */
gmpcPlugin plugin = {
	.name           = N_("Shout plugin"),
    .version        = {PLUGIN_MAJOR_VERSION,PLUGIN_MINOR_VERSION,PLUGIN_MICRO_VERSION},
	.plugin_type    = GMPC_PLUGIN_NO_GUI,
    .init           = shout_init,
	.destroy        = shout_plugin_destroy, /* Destroy */
	.mpd_status_changed = &shout_mpd_status_changed,
	.pref           = &shout_gpp,
	.get_enabled    = shout_get_enabled,
	.set_enabled    = shout_set_enabled,
    .get_translation_domain = shout_get_translation_domain
};


static int shout_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, "shout-plugin", "enable", FALSE);
}
static void shout_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "shout-plugin", "enable", enabled);
	if(enabled )
	{
        shout_add_si_icon();
		if(mpd_player_get_state(connection) == MPD_STATUS_STATE_PLAY)
		{
			start_ogg123();
		}

	} else {
		/* stop */
		stop_ogg123();

        gtk_widget_destroy(si_shout);
        si_shout = NULL;
	}
}

void shout_destroy(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), shout_vbox);
}
static void shout_entry_edited(GtkWidget *entry)
{
	const char *str = gtk_entry_get_text(GTK_ENTRY(entry));
	if(str)
	{
		cfg_set_single_value_as_string(config, "shout-plugin", "command",(char *)str);
	}
}





void shout_construct(GtkWidget *container)
{
	GtkWidget *entry = NULL, *label;
	char *entry_str = cfg_get_single_value_as_string_with_default(config, "shout-plugin", "command", SHOUT_COMMAND);
	shout_vbox = gtk_vbox_new(FALSE,6);

	gtk_container_add(GTK_CONTAINER(container), shout_vbox);

	entry = gtk_entry_new();
	if(entry_str)
	{
		gtk_entry_set_text(GTK_ENTRY(entry), entry_str);
		cfg_free_string(entry_str);
	}
    label = gtk_label_new(_("Playback Command:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(shout_vbox), label, FALSE, FALSE,0);
	gtk_box_pack_start(GTK_BOX(shout_vbox), entry, FALSE, FALSE,0);

	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(shout_entry_edited), NULL);

	gtk_widget_show_all(container);
}

static guint reconnect_timeout = 0;

static gboolean restart_ogg123(void)
{

    reconnect_timeout = 0;
	if(!shout_get_enabled()) return FALSE;

    if(mpd_player_get_state(connection) == MPD_STATUS_STATE_PLAY && !stopped)
    {
        start_ogg123();
    }
    return FALSE;
}
static void                shout_pid_callback                (GPid pid,
		gint status,
		gpointer data)
{
	g_spawn_close_pid(ogg123_pid);
	printf("client died: %i\n", ogg123_pid);
	ogg123_pid = -1;	


    if(si_shout){ 
        gtk_widget_set_sensitive(gtk_bin_get_child(GTK_BIN(si_shout)),FALSE);
        gtk_widget_set_tooltip_text(si_shout, _("Not Playing"));
    }

    /* Reconnect */
    if(mpd_player_get_state(connection) == MPD_STATUS_STATE_PLAY && !stopped)
    {

        if(reconnect_timeout) {
            g_source_remove(reconnect_timeout);
            reconnect_timeout = 0;
        }
        reconnect_timeout = g_timeout_add_seconds(1, restart_ogg123, NULL);
    }
}
static void start_ogg123(void)
{
    if(stopped) return;
    if(reconnect_timeout) {
        g_source_remove(reconnect_timeout);
        reconnect_timeout = 0;
    }
	if(ogg123_pid == -1)
	{
		gchar *uri = cfg_get_single_value_as_string_with_default(config, "shout-plugin", "command", SHOUT_COMMAND);
		gchar **argv = g_strsplit(uri, " ", 0);
		GError *error = NULL;

		if(!g_spawn_async(NULL,argv, NULL, G_SPAWN_SEARCH_PATH|G_SPAWN_DO_NOT_REAP_CHILD,NULL, NULL, &ogg123_pid, &error))
		{
            if(error)
            {
                gchar *message = g_strdup_printf("%s: %s", _("Shout plugin: Failed to spawn client. Error"), error->message);
                playlist3_show_error_message(message, ERROR_WARNING);
                g_free(message);
                g_error_free(error);
                error = NULL;
            }
		}
		else
		{
			g_child_watch_add(ogg123_pid, shout_pid_callback, NULL);

            if(si_shout){
                gtk_widget_set_sensitive(gtk_bin_get_child(GTK_BIN(si_shout)), TRUE);
                gtk_widget_set_tooltip_text(si_shout, _("Playing"));
            }
        }
		printf("spawned pid: %i\n", ogg123_pid);
        g_strfreev(argv);
		g_free(uri);
	}
}

static void stop_ogg123(void)
{
	if(ogg123_pid >= 0)
	{
#ifndef __WIN32__
		printf("killing: %i\n", ogg123_pid);
		kill (ogg123_pid, SIGHUP);
#else
		TerminateProcess (ogg123_pid, 1);
#endif

        if(si_shout){
            gtk_widget_set_sensitive(gtk_bin_get_child(GTK_BIN(si_shout)), FALSE);
            gtk_widget_set_tooltip_text(si_shout, _("Playing"));
        }
    }
	/*	ogg123_pid = -1;*/
}

/* mpd changed */

void   shout_mpd_status_changed(MpdObj *mi, ChangedStatusType what, void *data)
{
	if(!shout_get_enabled())
		return;
	if(what&(MPD_CST_SONGID|MPD_CST_STATE))
	{
		if(mpd_player_get_state(mi) != MPD_STATUS_STATE_PLAY)
		{
			stop_ogg123();
		}
		else
		{
			start_ogg123();
		}
		return;
	}
}
static void shout_plugin_destroy(void)
{
	stop_ogg123();
}

