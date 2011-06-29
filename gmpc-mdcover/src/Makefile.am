plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = mdcaplugin.la

mdcaplugin_la_SOURCES=	\
	plugin.c 


mdcaplugin_la_LIBADD= @gmpcmdca_LIBS@ 
mdcaplugin_la_CFLAGS= @gmpcmdca_CFLAGS@  
mdcaplugin_la_LDFLAGS= -module -avoid-version 
