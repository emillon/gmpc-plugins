AC_INIT([gmpc-mmkeys], [0.19.101], [qball@sarine.nl])

#Define the version
m4_define([package_major_version], [0])
m4_define([package_minor_version], [19])
m4_define([package_micro_version], [101])
m4_define([package_version], [package_major_version.package_minor_version.package_micro_version])

AC_PREREQ(2.58)
# so in metapackage it builds standalone
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_LIBTOOL

#intltool, libtool. Check for CC compiler
#AC_PROG_INTLTOOL([0.21])
#ALL_LINGUAS=""
#AM_GNU_GETTEXT_VERSION(0.11.5)
#AM_GLIB_GNU_GETTEXT
#GETTEXT_PACKAGE=gmpc-mmkeys
#AC_SUBST(GETTEXT_PACKAGE)
#AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [GETTEXT Package name])
#AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/${DATADIRNAME}/locale", [Name of gettext locale directory])



AM_PROG_VALAC([0.7.0])

PKG_PROG_PKG_CONFIG


PKG_CHECK_MODULES([modules],[
        gtk+-2.0  >= 2.12
        glib-2.0  >= 2.16
        libmpd
        gmpc >= 0.18.1
        dbus-glib-1
        ])

AC_OUTPUT([
	Makefile
])
