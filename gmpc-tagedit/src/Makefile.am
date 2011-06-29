plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = gmpctageditplugin.la

gmpctageditplugin_la_SOURCES=	\
								plugin.c
nodist_gmpctageditplugin_la_SOURCES=\
								gmpc-mpddata-model-tagedit.c
								
BUILT_SOURCES=\
	gmpc-mpddata-model-tagedit.c\
	gmpc-mpddata-model-tagedit.h

%.c %.h: %.gob
	@GOB2@ --no-touch-headers --always-private-header  $<


gmpctageditplugin_la_LIBADD= @gmpctagedit_LIBS@ 
gmpctageditplugin_la_CFLAGS= @gmpctagedit_CFLAGS@ -Wall -Wmissing-declarations
gmpctageditplugin_la_LDFLAGS= -module -avoid-version 

EXTRA_DIST=\
		   gmpc-mpddata-model-tagedit.gob\
		   tagedit.h
DISTCLEANFILES=\
		   gmpc-mpddata-model-tagedit.c\
		   gmpc-mpddata-model-tagedit.h\
		   gmpc-mpddata-model-tagedit-private.h

