AC_INIT([gmpc-mserver], [0.19.101], [qball@sarine.nl])
m4_define([package_major_version], [0])
m4_define([package_minor_version], [19])
m4_define([package_micro_version], [101])
m4_define([package_version],[package_major_version.package_minor_version.package_micro_version])

# so in metapackage it builds standalone
AC_CONFIG_AUX_DIR([.])
AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE

AC_DEFINE(PLUGIN_MAJOR_VERSION, package_major_version, ["Major version of plugin"])
AC_DEFINE(PLUGIN_MINOR_VERSION, package_minor_version, ["Minor version of plugin"])
AC_DEFINE(PLUGIN_MICRO_VERSION, package_micro_version, ["Micro version of plugin"])

#intltool, libtool. Check for CC compiler
AC_PROG_INTLTOOL([0.21])

# i18n support
dnl please keep them in alphabetical order
ALL_LINGUAS=""

GETTEXT_PACKAGE=gmpc-mserver
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",[GMPC_MSERVER gettext package])
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

PKG_CHECK_MODULES([gmpcmserver],[	glib-2.0 >= 2.4
	gobject-2.0 >= 2.4
	gtk+-2.0 >= 2.4
	gmodule-2.0
	libxml-2.0
	gthread-2.0
	libmpd >= 0.14.99
	gmpc	>= 0.15.4.102
    taglib_c
])

AC_SUBST(gmpcmserver_LIBS)
AC_SUBST(gmpcmserver_CFLAGS)
AC_CHECK_HEADERS([microhttpd.h],
        [],
        AC_MSG_ERROR([Unable to find microhttpd header files]),
        [
        AC_INCLUDES_DEFAULT
        #include <stdarg.h>
        ]
        )
AC_CHECK_LIB([microhttpd], [MHD_start_daemon],,AC_MSG_ERROR([Unable to link with libmicrohttpd]))

AC_OUTPUT([Makefile src/Makefile data/Makefile po/Makefile.in])
