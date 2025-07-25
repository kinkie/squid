## Copyright (C) 1996-2025 The Squid Software Foundation and contributors
##
## Squid software is distributed under GPLv2+ license and includes
## contributions from numerous individuals and organizations.
## Please see the COPYING and CONTRIBUTORS files for details.
##

include $(top_srcdir)/src/Common.am

SUBDIRS=
EXTRA_DIST=

if ENABLE_SNMP
SUBDIRS += snmplib
endif
if ENABLE_SSPI
SUBDIRS += sspi
endif

install: all
install-strip: all

noinst_LTLIBRARIES = \
	libmiscencoding.la \
	libmisccontainers.la \
	libmiscutil.la

if ENABLE_SMBLIB
# smblib is the only user of the rfcnb library
SUBDIRS += rfcnb smblib
endif
if ENABLE_AUTH_NTLM
SUBDIRS += ntlmauth
endif

#
# dirent.c, encrypt.c and getopt.c are needed for native Windows support.
#
EXTRA_libmiscutil_la_SOURCES = \
	dirent.c \
	encrypt.c \
	getopt.c

libmiscencoding_la_SOURCES = \
	base64.c \
	md5.c \
	rfc1738.c \
	rfc2617.c

libmisccontainers_la_SOURCES = \
	hash.cc

libmiscutil_la_SOURCES = \
	Splay.cc \
	heap.c \
	util.cc

TESTS += tests/testRFC1738

check_PROGRAMS += tests/testRFC1738

tests_testRFC1738_SOURCES = \
	tests/testRFC1738.cc

tests_testRFC1738_LDADD= \
	$(top_builddir)/lib/libmiscencoding.la \
	$(top_builddir)/lib/libmiscutil.la \
	$(LIBCPPUNIT_LIBS) \
	$(COMPAT_LIB)

tests_testRFC1738_LDFLAGS = $(LIBADD_DL)

testHeaders: $(top_srcdir)/include/*.h
