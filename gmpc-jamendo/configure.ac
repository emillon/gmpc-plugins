AC_INIT([gmpc-jamendo], [0.20.95], [qball@sarine.nl])
m4_define([package_major_version], [0])
m4_define([package_minor_version], [20])
m4_define([package_micro_version], [95])
m4_define([package_version],[package_major_version.package_minor_version.package_micro_version])

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

GETTEXT_PACKAGE=gmpc-jamendo
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
# test for zlib
zlib=0
AC_CHECK_LIB(z, inflate,zlib=1,zlib=0)
AM_CONDITIONAL(HAVE_ZLIB, test x$zlib = x1)
if test "x$zlib" != "x1"
then
 AC_MSG_ERROR([Jamendo plugin requires zlib])
fi


PKG_CHECK_MODULES([gmpcjamendo],
[
	glib-2.0 >= 2.4
	gobject-2.0 >= 2.4
	gtk+-2.0 >= 2.4
	gmodule-2.0
	libmpd >= 0.15.98
	gmpc > 0.20.2
    libxml-2.0 >= 2.6
    sqlite3
])
GOB2_CHECK([2.0.10])
AC_SUBST(gmpcjamendo_LIBS)
AC_SUBST(gmpcjamendo_CFLAGS)

AC_SUBST(gmpcjamendo2_LIBS)
AC_SUBST(gmpcjamendo2_CFLAGS)


AC_OUTPUT([Makefile src/Makefile data/Makefile po/Makefile.in ])
