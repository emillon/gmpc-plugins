/* gmpc-avahi (GMPC plugin)
 * Copyright (C) 2007-2009 Qball Cow <qball@sarine.nl>
 * Copyright (C) 2007 Jim Ramsay <i.am@jimramsay.com>
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
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <libmpd/debug_printf.h>
#include <gmpc/plugin.h>
#include <gmpc/gmpc-profiles.h>
#include <libmpd/debug_printf.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>

#include <avahi-glib/glib-watch.h>
#include <avahi-glib/glib-malloc.h>
#include <avahi-common/domain.h>
#include <avahi-common/error.h>

#define SERVICE_TYPE "_mpd._tcp"

static void avahi_domain_changed(void);
static const char* avahi_get_default_domain(void);
static void avahi_add_service( const char *name, const char *host, int port );
static void avahi_del_service( const char *name );
static const char* avahi_get_browse_domain(void);

/**** This section is for the gmpc plugin part ****/

static void avahi_init(void);
static void avahi_init_real();
static void avahi_destroy(void); 

static void avahi_pref_construct(GtkWidget *container);
static void avahi_pref_destroy(GtkWidget *container);

static void avahi_set_enabled(int enabled);
static int avahi_get_enabled(void);

static void avahi_update_profiles_menu(void);

static GtkWidget *pref_vbox;
gmpcPrefPlugin avahi_prefs = {
	.construct = avahi_pref_construct,
	.destroy = avahi_pref_destroy
};

int plugin_api_version = PLUGIN_API_VERSION;

static const gchar *avahi_get_translation_domain(void)
{
    return GETTEXT_PACKAGE;
}
gmpcPlugin plugin = {
	.name = "Avahi Zeroconf",
	.version = {PLUGIN_MAJOR_VERSION, PLUGIN_MINOR_VERSION, PLUGIN_MICRO_VERSION},
	.plugin_type = GMPC_PLUGIN_DUMMY,
	.init = avahi_init_real,
    .destroy = avahi_destroy,
	.pref = &avahi_prefs,
	.get_enabled = avahi_get_enabled,
	.set_enabled = avahi_set_enabled,

    .get_translation_domain = avahi_get_translation_domain
};

/**
 * Get/Set enabled
 */
static int avahi_get_enabled()
{
	return cfg_get_single_value_as_int_with_default(config, "avahi-profiles", "enable", TRUE);
}
static void avahi_set_enabled(int enabled)
{
    /* Get old state */
    gboolean old = avahi_get_enabled();
    /* Set new state */
	cfg_set_single_value_as_int(config, "avahi-profiles", "enable", enabled);
    /* if old enabled, new disabled, destroy avahi */
    if(old && !enabled) {
        avahi_destroy();
    }
    /* if old disabled new enabled, start avahi */
    if(!old && enabled) {
        avahi_init();
    }

}

static void avahi_add_service( const char *name, const char *host, int port )
{
	g_debug("Avahi service \"%s\" (%s:%i) added", name,host,port);
	if(gmpc_profiles_has_profile(gmpc_profiles, name))
	{	
		if(g_utf8_collate(gmpc_profiles_get_hostname(gmpc_profiles,name), host))
		{
			g_debug("Avahi service \"%s\" hostname update %s -> %s", name,
				gmpc_profiles_get_hostname(gmpc_profiles,name),host);
			gmpc_profiles_set_hostname(gmpc_profiles, name,host);
		}
		if(gmpc_profiles_get_port(gmpc_profiles,name) != port)
		{
			g_debug("Avahi service \"%s\" port update %i -> %i", name,gmpc_profiles_get_port(gmpc_profiles, name),port);
			gmpc_profiles_set_port(gmpc_profiles, name,port);
		}
	}
	else
	{
		/* create new */
		gchar *id = gmpc_profiles_create_new_item_with_name(gmpc_profiles, name,name);

		gmpc_profiles_set_hostname(gmpc_profiles, id,host);
		gmpc_profiles_set_port(gmpc_profiles, id,port);
		g_debug("Avahi service \"%s\" (%s:%i) created: id %s", name,host,port, id);
	}
}

static void avahi_del_service( const char *name )
{
	g_debug("Avahi service \"%s\" removed", name);
    if(cfg_get_single_value_as_int_with_default(config, "avahi-profiles", "delete-on-disappear", FALSE))
    {
        gmpc_profiles_remove_item(gmpc_profiles, name);
    }
}


static void avahi_del_all_services( void )
{

}

/**
 * Preferences
 */
static const char* avahi_get_browse_domain(void)
{
	static char value[128];
	gchar *def =  (gchar *)(avahi_get_default_domain());
	if( !def )
		def = "local";
	char* tmpval = cfg_get_single_value_as_string_with_default(config, "avahi-profiles", "domain", def);
	strncpy( value, tmpval, 128 );
	value[127] = '\0';
	cfg_free_string( tmpval );
	return value;
}

static void avahi_profiles_domain_applied(GtkWidget *button, GtkWidget *entry)
{
	const char *str = gtk_entry_get_text(GTK_ENTRY(entry));
	if(str && strcmp( str, avahi_get_browse_domain() ) != 0)
	{
		if( avahi_is_valid_domain_name( str ) ) {
			cfg_set_single_value_as_string(config, "avahi-profiles", "domain",(char *)str);
			debug_printf(DEBUG_INFO, "Searching domain '%s'\n", str );
			// Start browsing the new domain
			avahi_domain_changed();
		} else {
			// TODO: Popup an error and set the text back maybe?
			//fprintf( stderr, "Domain '%s' is not valid\n", str );
			gtk_entry_set_text(GTK_ENTRY(entry), avahi_get_browse_domain() );
		}
	}
	gtk_widget_set_sensitive( button, FALSE );
}

static void avahi_profiles_domain_changed(GtkWidget *entry, GtkWidget *button)
{
	const char *str = gtk_entry_get_text(GTK_ENTRY(entry));
	if(str && strcmp( str, avahi_get_browse_domain() ) != 0) {
		// Activate the "apply" button
		gtk_widget_set_sensitive( button, TRUE );
	} else {
		// deactivate the "apply" button
		gtk_widget_set_sensitive( button, FALSE );
	}
}
static void avahi_del_on_remove_changed(GtkToggleButton *button, gpointer user_data)
{
    gint value = gtk_toggle_button_get_active(button);

    cfg_set_single_value_as_int(config, "avahi-profiles", "delete-on-disappear", value);
}

void avahi_pref_destroy(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), pref_vbox);
}

void avahi_pref_construct(GtkWidget *container)
{
	GtkWidget *entry_hbox = gtk_hbox_new(FALSE,3);
	GtkWidget *entry = gtk_entry_new();
	GtkWidget *apply = gtk_button_new_from_stock( GTK_STOCK_APPLY );
    GtkWidget *del_on_remove_ck = gtk_check_button_new_with_label("Remove profile if server dissapears");
	pref_vbox = gtk_vbox_new(FALSE,6);


	// The domain to browse
	gtk_entry_set_text(GTK_ENTRY(entry), avahi_get_browse_domain());

	// On change of the domain box
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(avahi_profiles_domain_changed), apply);

	// The "Apply" button
	gtk_widget_set_sensitive( apply, FALSE );
	g_signal_connect(G_OBJECT(apply), "clicked", G_CALLBACK(avahi_profiles_domain_applied), entry);

	// Put the entry and label in the hbox:
	gtk_box_pack_start(GTK_BOX(entry_hbox), gtk_label_new("Search Domain:"), FALSE, FALSE,0);
	gtk_box_pack_start(GTK_BOX(entry_hbox), entry, FALSE, FALSE,0);
	gtk_box_pack_start(GTK_BOX(entry_hbox), apply, FALSE, FALSE,0);

	// Put it all in the vbox:
	gtk_box_pack_start(GTK_BOX(pref_vbox), entry_hbox, FALSE, FALSE,0);


    // Delete on remove  checkbox
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(del_on_remove_ck), 
            cfg_get_single_value_as_int_with_default(config, "avahi-profiles", "delete-on-disappear", FALSE));
    g_signal_connect(G_OBJECT(del_on_remove_ck), "toggled", G_CALLBACK(avahi_del_on_remove_changed), NULL);
    gtk_box_pack_start(GTK_BOX(pref_vbox), del_on_remove_ck, FALSE, FALSE, 0);


	gtk_container_add(GTK_CONTAINER(container), pref_vbox);
	gtk_widget_show_all(container);
}

/**** Everything below here is all avahi api intergration stuff: ****/
static AvahiGLibPoll *glib_poll = NULL;
static AvahiClient *client = NULL;
static AvahiServiceBrowser *browser = NULL;

static void avahi_resolve_callback(
	AvahiServiceResolver *r,
	AVAHI_GCC_UNUSED AvahiIfIndex interface,
	AVAHI_GCC_UNUSED AvahiProtocol protocol,
	AvahiResolverEvent event,
	const char *name,
	const char *type,
	const char *domain,
	const char *host_name,
	const AvahiAddress *address,
	uint16_t port,
	AvahiStringList *txt,
	AvahiLookupResultFlags flags,
	AVAHI_GCC_UNUSED void* userdata) {

	assert(r);

	/* Called whenever a service has been resolved successfully or timed out */
	debug_printf(DEBUG_INFO,"resolved: name:%s type:%s domain:%s host_name:%s\n", name,type,domain,host_name);
	switch (event) {
		case AVAHI_RESOLVER_FAILURE:
			debug_printf(DEBUG_ERROR, "(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
			break;

		case AVAHI_RESOLVER_FOUND: {
			char a[AVAHI_ADDRESS_STR_MAX];
			avahi_address_snprint(a, sizeof(a), address);
			debug_printf(DEBUG_INFO,"a: %s:%s:%i\n", name,a, port);	
			avahi_add_service( name, a, port );
		}
        default:
            break;
	}

	avahi_service_resolver_free(r);
}

static void avahi_browse_callback(
	AvahiServiceBrowser *b,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	AvahiBrowserEvent event,
	const char *name,
	const char *type,
	const char *domain,
	AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
	void* userdata) {
	
	AvahiClient *c = userdata;
	assert(b);
	debug_printf(DEBUG_INFO,"browser callback: name:%s type:%s domain:%s\n",name,type,domain);
	/* Called whenever a new services becomes available on the LAN or is removed from the LAN */

	switch (event) {
		case AVAHI_BROWSER_FAILURE:
			
				debug_printf(DEBUG_ERROR, "(Browser) %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
			//avahi_del_all_services();
			return;

		case AVAHI_BROWSER_NEW:
			/* We ignore the returned resolver object. In the callback
			   function we free it. If the server is terminated before
			   the callback function is called the server will free
			   the resolver for us. */

			if (!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, avahi_resolve_callback, c)))
				debug_printf(DEBUG_WARNING, "Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(c)));
			
			break;

		case AVAHI_BROWSER_REMOVE:
			avahi_del_service( name );
			break;

		case AVAHI_BROWSER_ALL_FOR_NOW:
		case AVAHI_BROWSER_CACHE_EXHAUSTED:
			break;
	}
}

static void avahi_client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
	assert(c);
	debug_printf(DEBUG_INFO,"client callback\n");
	/* Called whenever the client or server state changes */

	if (state == AVAHI_CLIENT_FAILURE) {
		debug_printf(DEBUG_ERROR, "Server connection failure: %s\n", avahi_strerror(avahi_client_errno(c)));
		// TODO: Maybe try to reconnect here?
	}
}

static void avahi_domain_changed(void)
{
	if( browser ) {
		avahi_service_browser_free(browser);
		avahi_del_all_services();
	}

	browser = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
			SERVICE_TYPE, avahi_get_browse_domain(), 0, avahi_browse_callback, client);
	if( !browser ) {
		debug_printf(DEBUG_ERROR, "Failed to create service browser for domain %s: %s\n", avahi_get_browse_domain(),
				avahi_strerror(avahi_client_errno(client)));
	}
}

static const char* avahi_get_default_domain()
{
	if( !client )
		return NULL;
	return avahi_client_get_domain_name(client);
}

static void avahi_init_real() {
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	avahi_init();
}

void avahi_init() {
	int error;
    
    if(!avahi_get_enabled()) return;

	glib_poll = avahi_glib_poll_new (NULL, G_PRIORITY_DEFAULT);

	/* Allocate a new client */
	client = avahi_client_new(avahi_glib_poll_get(glib_poll), 0, avahi_client_callback, NULL, &error);

	/* Check wether creating the client object succeeded */
	if (!client) {
		debug_printf(DEBUG_ERROR, "Failed to create client: %s\n", avahi_strerror(error));
		return;
	}
	
	/* Create the service browser */
	avahi_domain_changed();
}

static void avahi_destroy(void) {
	if( browser ) {
		avahi_service_browser_free(browser);
		avahi_del_all_services();
        browser = NULL;
	}
    if(client)
    {
        avahi_client_free (client);
        client = NULL;
    }
    if(glib_poll)
    {
        avahi_glib_poll_free (glib_poll);
        glib_poll = NULL;
    }
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
