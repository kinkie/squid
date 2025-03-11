/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_COMPAT_OS_XKILL_H
#define SQUID_COMPAT_OS_XKILL_H

#if !HAVE_KILL && HAVE_OPENPROCESS

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

/* portable wrapper for kill(2) */
SQUIDCEXTERN int
kill(pid_t pid, int sig);

#endif /* !HAVE_KILL */

#endif /* SQUID_COMPAT_OS_XKILL_H */