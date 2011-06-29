plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = fullscreeninfoplugin.la

fullscreeninfoplugin_la_SOURCES=	\
	plugin.c


fullscreeninfoplugin_la_LIBADD= @gmpcfullscreeninfo_LIBS@ 
fullscreeninfoplugin_la_CFLAGS= @gmpcfullscreeninfo_CFLAGS@   -Wmissing-declarations
fullscreeninfoplugin_la_LDFLAGS= -module -avoid-version 

EXTRA_DIST=
