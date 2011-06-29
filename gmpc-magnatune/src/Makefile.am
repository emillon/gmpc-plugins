plugindir=${libdir}/gmpc/plugins/
datadestdir = $(datadir)/gmpc/plugins/magnatune/

plugin_LTLIBRARIES = magnatuneplugin.la

EXTRA_DIST=\
	magnatune.h

magnatuneplugin_la_SOURCES=	\
	magnatune.c\
	plugin.c


magnatuneplugin_la_LIBADD= $(gmpcmagnatune_LIBS)
magnatuneplugin_la_CFLAGS= $(gmpcmagnatune_CFLAGS) -I$(srcdir)/ -I$(srcdir)/axl/  -DDATA_DIR=\"${DESTDIR}${datadestdir}\" -DG_LOG_DOMAIN="\"MagnatunePlugin\""
magnatuneplugin_la_LDFLAGS= -module -avoid-version 

