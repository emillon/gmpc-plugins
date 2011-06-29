plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = alarmplugin.la

alarmplugin_la_SOURCES= plugin.c
alarmplugin_la_LIBADD= @gmpcalarm_LIBS@
alarmplugin_la_CFLAGS= @gmpcalarm_CFLAGS@ -Wmissing-declarations -Wall 
alarmplugin_la_LDFLAGS= -module -avoid-version

EXTRA_DIST= gmpc-alarm.h
