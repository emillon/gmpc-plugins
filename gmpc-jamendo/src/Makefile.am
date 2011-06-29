plugindir=${libdir}/gmpc/plugins/
datadestdir = $(datadir)/gmpc/plugins/jamendo/

plugin_LTLIBRARIES = jamendoplugin.la

EXTRA_DIST=\
	jamendo.h

jamendoplugin_la_SOURCES=	\
	jamendo.c\
	plugin.c


jamendoplugin_la_LIBADD= $(gmpcjamendo_LIBS) $(gmpcjamendo2_LIBS) -lz
jamendoplugin_la_CFLAGS= $(gmpcjamendo_CFLAGS) $(gmpcjamendo2_CFLAGS) -I$(srcdir)/ -DDATA_DIR=\"${DESTDIR}${datadestdir}\"
jamendoplugin_la_LDFLAGS= -module -avoid-version 

