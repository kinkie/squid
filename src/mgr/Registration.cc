/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 16    Cache Manager API */

#include "squid.h"
#include "mgr/Registration.h"
#include "CacheManager.h"

void
Mgr::RegisterAction(char const *action, char const *desc,
                    OBJH *handler,
                    int pw_req_flag, int atomic)
{
    CacheManager::GetInstance()->registerProfile(action, desc, handler,
                                                 pw_req_flag, atomic);
}

void
Mgr::RegisterAction(char const *action, char const *desc,
                    ClassActionCreationHandler *handler,
                    int pw_req_flag, int atomic)
{
    CacheManager::GetInstance()->registerProfile(action, desc, handler,
                                                 pw_req_flag, atomic);
}
