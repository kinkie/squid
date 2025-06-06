/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ACL_HTTPSTATUS_H
#define SQUID_SRC_ACL_HTTPSTATUS_H

#include "acl/Acl.h"
#include "acl/Checklist.h"
#include "splay.h"

/// \ingroup ACLAPI
struct acl_httpstatus_data {
    int status1, status2;
    acl_httpstatus_data(int);
    acl_httpstatus_data(int, int);
    SBuf toStr() const; // was toStr
};

/// \ingroup ACLAPI
class ACLHTTPStatus : public Acl::Node
{
    MEMPROXY_CLASS(ACLHTTPStatus);

public:
    ACLHTTPStatus(char const *);
    ~ACLHTTPStatus() override;

    char const *typeString() const override;
    void parse() override;
    int match(ACLChecklist *checklist) override;
    SBufList dump() const override;
    bool empty () const override;
    bool requiresReply() const override { return true; }

protected:
    Splay<acl_httpstatus_data*> *data;
    char const *class_;
};

#endif /* SQUID_SRC_ACL_HTTPSTATUS_H */

