plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = albumviewplugin.la

albumviewplugin_la_SOURCES=	\
	plugin.c 


albumviewplugin_la_LIBADD= @gmpcalbumview_LIBS@ 
albumviewplugin_la_CFLAGS= @gmpcalbumview_CFLAGS@ -DPIXMAP_DATA_DIR="\"$(pkgdatadir)/icons\""
albumviewplugin_la_LDFLAGS= -module -avoid-version 

EXTRA_DIST= plugin.h
