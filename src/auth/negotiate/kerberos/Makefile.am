## Copyright (C) 1996-2025 The Squid Software Foundation and contributors
##
## Squid software is distributed under GPLv2+ license and includes
## contributions from numerous individuals and organizations.
## Please see the COPYING and CONTRIBUTORS files for details.
##

include $(top_srcdir)/src/Common.am

EXTRA_DIST= \
	README \
	required.m4 \
	negotiate_kerberos_auth.8

libexec_PROGRAMS = \
	negotiate_kerberos_auth \
	negotiate_kerberos_auth_test

AM_CPPFLAGS += -I$(srcdir)

negotiate_kerberos_auth_SOURCES = \
	negotiate_kerberos.h \
	negotiate_kerberos_auth.cc \
	negotiate_kerberos_pac.cc
negotiate_kerberos_auth_LDADD= \
	$(top_builddir)/lib/libmiscencoding.la \
	$(COMPAT_LIB) \
	$(LIBGSS_LIBS) \
	$(LIBHEIMDAL_KRB5_LIBS) \
	$(LIBMIT_KRB5_LIBS) \
	$(LIBNETTLE_LIBS) \
	$(XTRA_LIBS)

negotiate_kerberos_auth_test_SOURCES = \
	negotiate_kerberos_auth_test.cc
negotiate_kerberos_auth_test_LDADD= \
	$(top_builddir)/lib/libmiscencoding.la \
	$(COMPAT_LIB) \
	$(LIBGSS_LIBS) \
	$(LIBHEIMDAL_KRB5_LIBS) \
	$(LIBMIT_KRB5_LIBS) \
	$(LIBNETTLE_LIBS) \
	$(XTRA_LIBS)

man_MANS= negotiate_kerberos_auth.8
