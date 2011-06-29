plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = wikiplugin.la

wikiplugin_la_SOURCES=\
	plugin.c


wikiplugin_la_LIBADD= \
			@gmpcwiki_LIBS@ \
			@WebKitGtk_LIBS@

wikiplugin_la_CFLAGS= \
			@gmpcwiki_CFLAGS@ \
			@WebKitGtk_CFLAGS@

wikiplugin_la_LDFLAGS= -module -avoid-version 
