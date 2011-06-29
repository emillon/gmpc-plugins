m4_define([package_major_version], [0])
m4_define([package_minor_version], [0])
m4_define([package_micro_version], [1])
m4_define([package_version],[package_major_version.package_minor_version.package_micro_version])

AC_INIT(src)
AC_CONFIG_AUX_DIR([.])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([gmpc-fullscreeninfo], [package_version],qball@qballcow.nl)

AC_DEFINE(PLUGIN_MAJOR_VERSION, package_major_version, ["Major version of plugin"])
AC_DEFINE(PLUGIN_MINOR_VERSION, package_minor_version, ["Minor version of plugin"])
AC_DEFINE(PLUGIN_MICRO_VERSION, package_micro_version, ["Micro version of plugin"])

AC_PROG_CC
AM_DISABLE_STATIC
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

#intltool, libtool. Check for CC compiler
AC_PROG_INTLTOOL([0.21])

# i18n support
dnl please keep them in alphabetical order
ALL_LINGUAS="ar bg bs de es fr ga hu nb nl pl ro ru sq sv zh_CN"

GETTEXT_PACKAGE=gmpc-fullscreeninfo
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",[GMPC_FULLSCREENINFO gettext package])
AM_GLIB_GNU_GETTEXT


if test "x${prefix}" = "xNONE"; then
	PACKAGE_PREFIX="${ac_default_prefix}"
else
	PACKAGE_PREFIX="${prefix}"
fi
AC_MSG_RESULT($PACKAGE_PREFIX)
PACKAGE_LOCALE_DIR="${PACKAGE_PREFIX}/${DATADIRNAME}/locale"
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR,"$PACKAGE_LOCALE_DIR",[The package's locale path for gettext])

PKG_CHECK_MODULES([gmpcfullscreeninfo],
[
	glib-2.0 >= 2.10
	gobject-2.0 >= 2.4
	gtk+-2.0 >= 2.8
	gmodule-2.0
	cairo
	libxml-2.0
	libmpd >= 0.17.0
    gmpc >= 0.18.1
    ])
AC_SUBST(gmpcfullscreeninfo_LIBS)
AC_SUBST(gmpcfullscreeninfo_CFLAGS)


AC_OUTPUT([Makefile src/Makefile po/Makefile.in])
