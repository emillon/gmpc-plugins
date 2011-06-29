AUTOMAKE_OPTIONS=1.11
plugindir=${libdir}/gmpc/plugins/

plugin_LTLIBRARIES = mmkeysplugin.la
INCLUDES = -DG_LOG_DOMAIN=\"MMKeysPlugin\"

mmkeysplugin_la_VALAFLAGS= --pkg=dbus-glib-1 --pkg=gtk+-2.0 --pkg=glib-2.0 --pkg=libmpd --pkg=gmpc --pkg=gmpc-extras --basedir=$(top_builddir) --vapidir=$(top_srcdir)/vapi/ --target-glib=2.16 

mmkeysplugin_la_SOURCES=	\
    mmkeys.vala

mmkeysplugin_la_LIBADD= @modules_LIBS@ 
mmkeysplugin_la_CFLAGS= @modules_CFLAGS@  -Wall -Wmissing-declarations
mmkeysplugin_la_LDFLAGS= -module -avoid-version 

EXTRA_DIST=\
    vapi/gmpc.vapi\
    vapi/libmpd.vapi\
    vapi/gmpc-extras.vapi

