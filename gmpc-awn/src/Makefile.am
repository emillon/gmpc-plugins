libdir=${prefix}/lib/gmpc/plugins/

lib_LTLIBRARIES = awnplugin.la

awnplugin_la_SOURCES= plugin.c 


awnplugin_la_LIBADD= @gmpcawn_LIBS@ 
awnplugin_la_CFLAGS= @gmpcawn_CFLAGS@ -Wmissing-declarations -Wall
awnplugin_la_LDFLAGS= -module -avoid-version

