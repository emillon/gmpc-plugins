plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = lircplugin.la

lircplugin_la_SOURCES=\
	plugin.c


lircplugin_la_LIBADD= @gmpclirc_LIBS@ -llirc_client 
lircplugin_la_CFLAGS= @gmpclirc_CFLAGS@ 
lircplugin_la_LDFLAGS= -module -avoid-version 
