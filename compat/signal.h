/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_COMPAT_SIGNAL_H
#define SQUID_COMPAT_SIGNAL_H

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifndef SIGHUP
#define SIGHUP 1   /* hangup */
#endif
#ifndef SIGKILL
#define SIGKILL 9  /* kill (cannot be caught or ignored) */
#endif
#ifndef SIGBUS
#define SIGBUS 10  /* bus error */
#endif
#ifndef SIGPIPE
#define SIGPIPE 13 /* write on a pipe with no one to read it */
#endif
#ifndef SIGCHLD
#define SIGCHLD 20 /* to parent on child stop or exit */
#endif
#ifndef SIGUSR1
#define SIGUSR1 30 /* user defined signal 1 */
#endif
#ifndef SIGUSR2
#define SIGUSR2 31 /* user defined signal 2 */
#endif

#endif /* SQUID_COMPAT_SIGNAL_H */
