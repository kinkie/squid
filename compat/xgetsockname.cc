/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "compat/xgetsockname.h"

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if _SQUID_WINDOWS_ || _SQUID_MINGW_

int
xgetsockname(int s, struct sockaddr *n, socklen_t *l)
{
    int i = *l;
    if (::getsockname(_get_osfhandle(s), n, &i) == SOCKET_ERROR)
    {
        errno = WSAGetLastError();
        return -1;
    }
    else
        return 0;
}
#endif /* _SQUID_WINDOWS_ || _SQUID_MINGW_ */