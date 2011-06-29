plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = libextraplaylist.la

libextraplaylist_la_SOURCES=\
	main.c


libextraplaylist_la_LIBADD= @gmpcextraplaylist_LIBS@
libextraplaylist_la_CFLAGS= @gmpcextraplaylist_CFLAGS@ -Wall -Wmissing-declarations
libextraplaylist_la_LDFLAGS= -module -avoid-version 

