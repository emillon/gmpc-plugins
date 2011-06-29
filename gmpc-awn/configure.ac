AC_PREREQ(2.58)

m4_define([package_name], [gmpc-awn])
#Define the version
m4_define([package_major_version], [0])
m4_define([package_minor_version], [19])
m4_define([package_micro_version], [101])
m4_define([package_version], [package_major_version.package_minor_version.package_micro_version])

AC_INIT([gmpc-awn], [0.19.101], [qball@sarine.nl])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

AC_DEFINE(MAJOR_VERSION, package_major_version, [Major version number.])
AC_DEFINE(MINOR_VERSION, package_minor_version, [Minor version number.])
AC_DEFINE(MICRO_VERSION, package_micro_version, [Micro version number.])

AC_PROG_INTLTOOL([0.21])

# i18n support
dnl please keep them in alphabetical order
ALL_LINGUAS=""

GETTEXT_PACKAGE=package_name
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",[gettext package name.])
AM_GLIB_GNU_GETTEXT

if test "x${prefix}" = "xNONE"; then
    PACKAGE_PREFIX="${ac_default_prefix}"
else
    PACKAGE_PREFIX="${prefix}"
fi
PACKAGE_LOCALE_DIR="${PACKAGE_PREFIX}/${DATADIRNAME}/locale"
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR,"$PACKAGE_LOCALE_DIR",[The package's locale path for gettext])

AC_PROG_CC
AM_DISABLE_STATIC
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

PKG_CHECK_MODULES([gmpcawn],
[
    gmpc >= 0.15.4.96
    dbus-glib-1
])

AC_SUBST(gmpcawn_LIBS)
AC_SUBST(gmpcawn_CFLAGS)

AC_OUTPUT([Makefile src/Makefile po/Makefile.in])
