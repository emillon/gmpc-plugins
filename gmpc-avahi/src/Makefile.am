plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = avahiplugin.la

avahiplugin_la_SOURCES=	\
	plugin.c

avahiplugin_la_LIBADD= @gmpcavahi_LIBS@
avahiplugin_la_CFLAGS= @gmpcavahi_CFLAGS@ -Wall -DG_LOG_DOMAIN=\"AvahiPlugin\"
avahiplugin_la_LDFLAGS= -module -avoid-version 
