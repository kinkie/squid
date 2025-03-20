/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_COMPAT_GETSOCKNAME_H
#define SQUID_COMPAT_GETSOCKNAME_H

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if _SQUID_WINDOWS_ || _SQUID_MINGW_

SQUIDCEXTERN
int xgetsockname(int s, struct sockaddr *n, socklen_t *l);

#else /* _SQUID_WINDOWS_ || _SQUID_MINGW_ */

inline int
xgetsockname(int s, struct sockaddr *n, socklen_t *l)
{
    return ::getsockname(s, n, l);
}
#endif /* _SQUID_WINDOWS_ || _SQUID_MINGW_ */

#endif /* SQUID_COMPAT_GETSOCKNAME_H */