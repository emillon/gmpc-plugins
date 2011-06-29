plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = lyricsplugin.la

lyricsplugin_la_SOURCES= plugin.c _leoslyrics.c _lyrictracker.c


lyricsplugin_la_LIBADD= @gmpclyrics_LIBS@ 
lyricsplugin_la_CFLAGS= @gmpclyrics_CFLAGS@ -Wmissing-declarations -Wall
lyricsplugin_la_LDFLAGS= -module -avoid-version

EXTRA_DIST= gmpc-lyrics.h
