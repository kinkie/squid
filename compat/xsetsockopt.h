/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/*
 * AUTHOR: Francesco Chemolli <kinkie@squid-cache.org>
 */

#ifndef SQUID_COMPAT_XSETSOCKOPT_H
#define SQUID_COMPAT_XSETSOCKOPT_H

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

/* portable wrapper for setsockopt */
SQUIDCEXTERN int
xsetsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);

#endif /* SQUID_COMPAT_XSETSOCKOPT_H */
