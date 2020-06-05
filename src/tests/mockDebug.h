/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 00    Debug Routines */

#ifndef SQUID_DEBUG_H
#define SQUID_DEBUG_H

#include <sstream>

#define debugs(S, L, C)        \
    if (false)                 \
    {                          \
        std::ostringstream os; \
        os << C;               \
    }

#endif /* SQUID_DEBUG_H */