AC_INIT([gmpc-lirc], [0.19.101], [qball@sarine.nl])

m4_define([package_major_version], [0])
m4_define([package_minor_version], [19])
m4_define([package_micro_version], [101])
m4_define([package_version],[package_major_version.package_minor_version.package_micro_version])

AC_CONFIG_AUX_DIR([.])
AC_PREREQ([2.59])
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

GETTEXT_PACKAGE=gmpc-lirc
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


AC_CHECK_HEADER(lirc/lirc_client.h, AC_CHECK_LIB(lirc_client, lirc_init, have_lirc=yes,have_lirc=no),have_lirc=no)
if test "x${have_lirc}" != "xyes"; then
    AC_MSG_RESULT([*** liblircclient header files missing ***]) 
    exit 1;
fi


PKG_CHECK_MODULES([gmpclirc],
[
	glib-2.0 >= 2.4
	gobject-2.0 >= 2.4
	gtk+-2.0 >= 2.4
	gmodule-2.0
	libxml-2.0
	libmpd >= 0.13.0
	gmpc >= 0.14.0
])
AC_SUBST(gmpclirc_LIBS)
AC_SUBST(gmpclirc_CFLAGS)


AC_OUTPUT([Makefile src/Makefile po/Makefile.in])
