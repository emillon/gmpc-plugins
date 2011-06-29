plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = shoutplugin.la

shoutplugin_la_SOURCES=	\
	plugin.c	


shoutplugin_la_LIBADD= @gmpcshout_LIBS@ 
shoutplugin_la_CFLAGS= @gmpcshout_CFLAGS@   -Wmissing-declarations
shoutplugin_la_LDFLAGS= -module -avoid-version 

