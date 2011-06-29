AUTOMAKE_OPTIONS = foreign 1.6

SUBDIRS = src po

po/$(GETTEXT_PACKAGE).pot:
	cd $(top_srcdir)/po && intltool-update -p -g gmpc-awn
