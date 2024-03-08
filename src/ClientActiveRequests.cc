/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"

#include "ClientActiveRequests.h"
#include "dlink.h"

ClientActiveRequests&
ClientActiveRequests::Instance()
{
    static ClientActiveRequests instance;
    return instance;
}

void
ClientActiveRequests::MakeHead(dlink_node *m)
{
    dlinkDelete(m, ActiveRequests_);
    dlinkAdd(m->data, m, ActiveRequests_);
}

void
ClientActiveRequests::Add(ClientHttpRequest * r, dlink_node *m)
{
    dlinkAdd(r, m, ActiveRequests_);
}

void
ClientActiveRequests::Delete(dlink_node *m)
{
    dlinkDelete(m, ActiveRequests_);
}

ClientActiveRequests::ClientActiveRequests()
{
    ActiveRequests_ = new dlink_list;
}
