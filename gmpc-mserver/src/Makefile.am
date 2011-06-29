plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = gmpcmserverplugin.la

gmpcmserverplugin_la_SOURCES=	\
								plugin.c

gmpcmserverplugin_la_LIBADD= @gmpcmserver_LIBS@ 
gmpcmserverplugin_la_CFLAGS= @gmpcmserver_CFLAGS@ -Wall -lm -lmicrohttpd
gmpcmserverplugin_la_LDFLAGS= -module -avoid-version 

EXTRA_DIST=

