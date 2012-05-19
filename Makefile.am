include Makefile.include
include Makefile.tests
include aws-glib/Makefile.include
include tests/Makefile.include

SUBDIRS = . doc

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
DISTCHECK_CONFIGURE_FLAGS = --enable-maintainer-flags --enable-gtk-doc --enable-introspection=yes --enable-silent-rules

awsdocdir = ${prefix}/doc/aws-glib-1.0
awsdoc_DATA =
awsdoc_DATA += README
awsdoc_DATA += COPYING
awsdoc_DATA += AUTHORS
awsdoc_DATA += ChangeLog
awsdoc_DATA += INSTALL
awsdoc_DATA += NEWS

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =
pkgconfig_DATA += $(top_builddir)/aws-glib-1.0.pc

EXTRA_DIST += $(awsdoc_DATA)

dist-hook:
	@if test -d "$(srcdir)/.git"; then                              \
          (cd "$(srcdir)" &&                                            \
           $(top_srcdir)/missing --run git log --stat ) > ChangeLog.tmp \
           && mv -f ChangeLog.tmp $(top_distdir)/ChangeLog              \
           || (rm -f ChangeLog.tmp;                                     \
               echo Failed to generate ChangeLog >&2);                  \
	else                                                            \
	  echo A git checkout is required to generate a ChangeLog >&2;  \
	fi

uninstall-local:
	-rm -r $(awsdocdir)
