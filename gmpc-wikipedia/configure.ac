m4_define([plugin_major_version], [0])
m4_define([plugin_minor_version], [15])
m4_define([plugin_micro_version], [5])
m4_define([plugin_nano_version], [0])
m4_define([plugin_version],[plugin_major_version.plugin_minor_version.plugin_micro_version.plugin_nano_version])

AC_INIT([gmpc-wikipedia], [0.19.101], [qball@sarine.nl])

AC_CONFIG_AUX_DIR([.])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

AC_DEFINE(PLUGIN_MAJOR_VERSION, plugin_major_version, ["Major version of plugin"])
AC_DEFINE(PLUGIN_MINOR_VERSION, plugin_minor_version, ["Minor version of plugin"])
AC_DEFINE(PLUGIN_MICRO_VERSION, plugin_micro_version, ["Micro version of plugin"])
AC_DEFINE(PLUGIN_NANO_VERSION, plugin_nano_version, ["Nano version of plugin"])

#intltool, libtool. Check for CC compiler
AC_PROG_INTLTOOL([0.21])

# i18n support
dnl please keep them in alphabetical order
ALL_LINGUAS=""

GETTEXT_PACKAGE=gmpc-wikipedia
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",[GMPC_LAST.FM gettext package])
AM_GLIB_GNU_GETTEXT


if test "x${prefix}" = "xNONE"; then
	PACKAGE_PREFIX="${ac_default_prefix}"
else
	PACKAGE_PREFIX="${prefix}"
fi
AC_MSG_RESULT($PACKAGE_PREFIX)
PACKAGE_LOCALE_DIR="${PACKAGE_PREFIX}/${DATADIRNAME}/locale"
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR,"$PACKAGE_LOCALE_DIR",[The package's locale path for gettext])


AC_PROG_CC
AM_DISABLE_STATIC
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL


############################################################################

PKG_CHECK_MODULES([gmpcwiki],
[
	glib-2.0 >= 2.4
	gobject-2.0 >= 2.4
	gtk+-2.0 >= 2.4
	gmodule-2.0
	libmpd >= 0.14.99
	gmpc >= 0.15.4.102
	libxml-2.0 >= 2.0
    libsoup-2.4
       
])

AC_SUBST(gmpcwiki_LIBS)
AC_SUBST(gmpcwiki_CFLAGS)
	

PKG_CHECK_MODULES([WebKitGtk],[webkit-1.0] ,[webkit=true],[webkit=false])
if test "$webkit" = "true" ; then
	AC_DEFINE(WEBKIT,1,[Webkit compiled from code])

else

	PKG_CHECK_MODULES([WebKitGtk],[WebKitGtk] )
	AC_DEFINE(WEBKIT_APT,1,[WebKit from apt])

fi
AC_SUBST(WebKitGtk_LIBS)
AC_SUBST(WebKitGtk_CFLAGS)
	
	


AC_OUTPUT([Makefile src/Makefile data/Makefile po/Makefile.in])
