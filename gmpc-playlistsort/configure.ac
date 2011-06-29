AC_INIT([gmpc-playlistsort], [0.19.101], [qball@sarine.nl])
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

AC_PROG_CC
AM_DISABLE_STATIC
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

PKG_CHECK_MODULES([gmpcplaylistsort],[	glib-2.0 >= 2.4
	gobject-2.0 >= 2.4
	gtk+-2.0 >= 2.8
	gmodule-2.0
	libmpd >= 0.14.98
	libglade-2.0
	gmpc	>= 0.15.4.99
])

AC_SUBST(gmpcplaylistsort_LIBS)
AC_SUBST(gmpcplaylistsort_CFLAGS)


AC_OUTPUT([Makefile src/Makefile])
