/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/*
 * AUTHOR: Andrey Shorin <tolsty@tushino.com>
 * AUTHOR: Guido Serassio <serassio@squid-cache.org>
 * AUTHOR: Francesco Chemolli <kinkie@squid-cache.org>
 */

#ifndef SQUID_COMPAT_SYSLOG_H
#define SQUID_COMPAT_SYSLOG_H

#if HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifndef LOG_EMERG
#define LOG_PID 0x01
#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7
#define LOG_DAEMON (3 << 3)
#endif

const unsigned int SYSLOG_MAX_MSG_SIZE = 1024;

#if !HAVE_SYSLOG && HAVE_REPORTEVENTA
void openlog(const char *ident, int logopt, int facility);
void syslog(int priority, const char *fmt, ...);
#endif

#endif /* SQUID_COMPAT_SYSLOG_H */