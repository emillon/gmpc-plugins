plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = lastfmplugin.la
INCLUDES = -DG_LOG_DOMAIN=\"LastFMPlugin\"

lastfmplugin_la_SOURCES=	\
	plugin.c 


lastfmplugin_la_LIBADD= @gmpclastfm_LIBS@ 
lastfmplugin_la_CFLAGS= @gmpclastfm_CFLAGS@  -Wall -Wmissing-declarations
lastfmplugin_la_LDFLAGS= -module -avoid-version 
