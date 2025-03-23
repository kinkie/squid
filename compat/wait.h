/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */
/*
 * portabilty status for macros defined in wait.h on Posix
 */
#ifndef SQUID_COMPAT_WAIT_H
#define SQUID_COMPAT_WAIT_H

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(status) (WTERMSIG(status) == 0)
#endif
#ifndef WIFSIGNALED
#define WIFSIGNALED(status) (((signed char)(((status) & 0x7f) + 1) >> 1) > 0)
#endif
#ifndef WTERMSIG
#define WTERMSIG(status) ((status) & 0x7f)
#endif

#endif /* SQUID_COMPAT_WAIT_H */
