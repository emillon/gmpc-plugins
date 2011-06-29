plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = playlistsortplugin.la

playlistsortplugin_la_SOURCES=	\
	plugin.c 


playlistsortplugin_la_LIBADD= @gmpcplaylistsort_LIBS@ 
playlistsortplugin_la_CFLAGS= @gmpcplaylistsort_CFLAGS@  
playlistsortplugin_la_LDFLAGS= -module -avoid-version 
