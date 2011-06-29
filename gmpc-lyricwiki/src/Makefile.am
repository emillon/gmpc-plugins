plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = lyricwikiplugin.la

lyricwikiplugin_la_SOURCES=	\
	plugin.c 


lyricwikiplugin_la_LIBADD= @gmpclyricwiki_LIBS@ 
lyricwikiplugin_la_CFLAGS= @gmpclyricwiki_CFLAGS@  -Wmissing-declarations -Wmissing-prototypes
lyricwikiplugin_la_LDFLAGS= -module -avoid-version 
