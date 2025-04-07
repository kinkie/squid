/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "compat/xaccept.h"

#if _SQUID_WINDOWS_ || _SQUID_MINGW_
#if HAVE_WINDOWS_H
#include <windows.h>
#endif
#if HAVE_WKINSOCK2_H
#include <winsock2.h>
#endif

int
xaccept(int s, struct sockaddr *a, socklen_t *l)
{
    SOCKET result;
    if ((result = accept(_get_osfhandle(s), a, l)) == INVALID_SOCKET)
        errno = WSAGetLastError();
        if (errno == WSAECONNRESET) {
            // for Comm::TcpAcceptor::acceptInto() special handling
            // Client gave up, or was accept()'d by other SMP worker.
            // Linux POSIX API uses the Connection Aborted code for this case.
            errno = ECONNABORTED;
        } else if (errno == WSAEMFILE) {
            // too many open sockets, cannot accept another right now
            errno = EMFILE;
        }
        return -1;
    else
        return _open_osfhandle(result, 0);
}

#endif
