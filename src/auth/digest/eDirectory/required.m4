## Copyright (C) 1996-2023 The Squid Software Foundation and contributors
##
## Squid software is distributed under GPLv2+ license and includes
## contributions from numerous individuals and organizations.
## Please see the COPYING and CONTRIBUTORS files for details.
##

dnl AC_CHECK_HEADERS([ldap.h winldap.h],[BUILD_HELPER="eDirectory_userip"])
SQUID_DOES_LDAP_WORK_REALLY
AS_IF([test "x$ac_cv_lib_ldap_ldap_init" = "xyes"].[ BUILD_HELPER="eDirectory_userip" ])
