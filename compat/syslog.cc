/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/*
 * syslog compatibility layer derives from git
 *
 * AUTHOR: Andrey Shorin <tolsty@tushino.com>
 * AUTHOR: Guido Serassio <serassio@squid-cache.org>
 * AUTHOR: Francesco Chemolli <kinkie@squid-cache.org>
 */

#include "squid.h"
#include "syslog.h"

/* syslog emulation layer derived from git */
static HANDLE ms_eventlog;

void openlog(const char *ident, int logopt, int facility)
{
    if (ms_eventlog)
        return;

    ms_eventlog = RegisterEventSourceA(nullptr, ident);

    // note: RegisterEventAtSourceA may fail and return nullptr.
    //   in that case we'll just retry at the next message or not log
}

void syslog(int priority, const char *fmt, ...)
{
    WORD logtype;
    char *str = static_cast<char *>(xmalloc(SYSLOG_MAX_MSG_SIZE));
    int str_len;
    va_list ap;

    if (!ms_eventlog)
        return;

    va_start(ap, fmt);
    str_len = vsnprintf(str, SYSLOG_MAX_MSG_SIZE - 1, fmt, ap);
    va_end(ap);

    if (str_len < 0)
    {
        /* vsnprintf failed */
        return;
    }

    switch (priority)
    {
    case LOG_EMERG:
    case LOG_ALERT:
    case LOG_CRIT:
    case LOG_ERR:
        logtype = EVENTLOG_ERROR_TYPE;
        break;

    case LOG_WARNING:
        logtype = EVENTLOG_WARNING_TYPE;
        break;

    case LOG_NOTICE:
    case LOG_INFO:
    case LOG_DEBUG:
    default:
        logtype = EVENTLOG_INFORMATION_TYPE;
        break;
    }

    // Windows API suck. They are overengineered
    ReportEventA(ms_eventlog, logtype, 0, 0, nullptr, 1, 0,
                 const_cast<const char **>(&str), nullptr);
}
