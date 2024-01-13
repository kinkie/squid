/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLDESTINATIONDOMAIN_H
#define SQUID_ACLDESTINATIONDOMAIN_H

#include "acl/Checklist.h"
#include "acl/Data.h"
#include "acl/ParameterizedNode.h"
#include "dns/forward.h"

namespace Acl
{

/// a "dstdomain" or "dstdom_regex" ACL
class DestinationDomainCheck: public ParameterizedNode< ACLData<const char *> >
{
public:
    /* Acl::Node API */
    int match(ACLChecklist *) override;
    bool requiresRequest() const override {return true;}
    const Acl::Options &options() override;

private:
    Acl::BooleanOptionValue lookupBanned; ///< Are DNS lookups allowed?
};

} // namespace Acl

#endif /* SQUID_ACLDESTINATIONDOMAIN_H */

