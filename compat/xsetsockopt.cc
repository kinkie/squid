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

#include "squid.h"
#include "xsetsockopt.h"

// include this header before winsock2.h
#if HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif

#if HAVE_WINSOCK2_H
#include <winsock2.h>
#endif

// all windows native code requires windows.h
#if HAVE_WINDOWS_H
#include <windows.h>
#endif

// needed for _commmit and _get_osfhandle
#if HAVE_IO_H
#include <io.h>
#endif

SQUIDCEXTERN int
xsetsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len)
{
#if _SQUID_WINDOWS_ || _SQUID_MINGW_
    SOCKET fhandle = _get_osfhandle(socket);
    if (setsockopt(fhandle, level, option_name, (const char *)option_value, option_len) == SOCKET_ERROR)
    {
        errno = WSAGetLastError();
        return -1;
    }
    return 0;
#else /* _SQUID_WINDOWS_ || _SQUID_MINGW_ */
    return setsockopt(socket, level, option_name, option_value, option_len);
#endif
}
