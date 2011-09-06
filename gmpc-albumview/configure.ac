AC_INIT([gmpc-albumview], [0.20.95], [qball@sarine.nl])
m4_define([package_major_version], [0])
m4_define([package_minor_version], [20])
m4_define([package_micro_version], [95])
m4_define([package_version],[package_major_version.package_minor_version.package_micro_version])

AC_PREREQ(2.59)
AC_CONFIG_AUX_DIR([.])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

AC_DEFINE(PLUGIN_MAJOR_VERSION, package_major_version, ["Major version of plugin"])
AC_DEFINE(PLUGIN_MINOR_VERSION, package_minor_version, ["Minor version of plugin"])
AC_DEFINE(PLUGIN_MICRO_VERSION, package_micro_version, ["Micro version of plugin"])

# i18n support
#intltool, libtool. Check for CC compiler
AC_PROG_INTLTOOL([0.21])
dnl please keep them in alphabetical order
ALL_LINGUAS=

GETTEXT_PACKAGE=gmpc-albumview
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

PKG_CHECK_MODULES([gmpcalbumview],[	glib-2.0 >= 2.10
	gobject-2.0 >= 2.4
	gtk+-2.0 >= 2.8
	gmodule-2.0
	libmpd >= 0.15.98
	gmpc	>= 0.19.2
])

AC_SUBST(gmpcalbumview_LIBS)
AC_SUBST(gmpcalbumview_CFLAGS)


AC_OUTPUT([Makefile src/Makefile po/Makefile.in 
    pixmaps/Makefile
    pixmaps/hicolor/Makefile
    pixmaps/hicolor/16x16/Makefile
    pixmaps/hicolor/16x16/apps/Makefile
    pixmaps/hicolor/22x22/Makefile
    pixmaps/hicolor/22x22/apps/Makefile
    pixmaps/hicolor/24x24/Makefile
    pixmaps/hicolor/24x24/apps/Makefile
    pixmaps/hicolor/32x32/Makefile
    pixmaps/hicolor/32x32/apps/Makefile
    pixmaps/hicolor/48x48/Makefile
    pixmaps/hicolor/48x48/apps/Makefile
    pixmaps/hicolor/64x64/Makefile
    pixmaps/hicolor/64x64/apps/Makefile
    pixmaps/hicolor/72x72/Makefile
    pixmaps/hicolor/72x72/apps/Makefile
    pixmaps/hicolor/96x96/Makefile
    pixmaps/hicolor/96x96/apps/Makefile
    pixmaps/hicolor/128x128/Makefile
    pixmaps/hicolor/128x128/apps/Makefile
    pixmaps/hicolor/scalable/Makefile
    pixmaps/hicolor/scalable/apps/Makefile
    pixmaps/Humanity/Makefile
    pixmaps/Humanity/16x16/Makefile
    pixmaps/Humanity/16x16/apps/Makefile
    pixmaps/Humanity/22x22/Makefile
    pixmaps/Humanity/22x22/apps/Makefile
    pixmaps/Humanity/24x24/Makefile
    pixmaps/Humanity/24x24/apps/Makefile
    pixmaps/Humanity/32x32/Makefile
    pixmaps/Humanity/32x32/apps/Makefile
    pixmaps/Humanity/48x48/Makefile
    pixmaps/Humanity/48x48/apps/Makefile
    pixmaps/Humanity/64x64/Makefile
    pixmaps/Humanity/64x64/apps/Makefile
    pixmaps/Humanity/72x72/Makefile
    pixmaps/Humanity/72x72/apps/Makefile
    pixmaps/Humanity/96x96/Makefile
    pixmaps/Humanity/96x96/apps/Makefile
    pixmaps/Humanity/128x128/Makefile
    pixmaps/Humanity/128x128/apps/Makefile
    pixmaps/Humanity/scalable/Makefile
    pixmaps/Humanity/scalable/apps/Makefile
])
