plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = discogsplugin.la

discogsplugin_la_SOURCES=	\
	plugin.c 


discogsplugin_la_LIBADD= @gmpcdiscogs_LIBS@ 
discogsplugin_la_CFLAGS= @gmpcdiscogs_CFLAGS@  -Wall -Wmissing-declarations
discogsplugin_la_LDFLAGS= -module -avoid-version 
