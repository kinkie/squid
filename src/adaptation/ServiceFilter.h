/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ADAPTATION_SERVICEFILTER_H
#define SQUID_SRC_ADAPTATION_SERVICEFILTER_H

#include "AccessLogEntry.h"
#include "adaptation/Elements.h"

class HttpRequest;
class HttpReply;

namespace Adaptation
{

/// information used to search for adaptation services
class ServiceFilter
{
public:
    ServiceFilter(Method, VectPoint, HttpRequest *, HttpReply *, AccessLogEntry::Pointer const &al); // locks
    ServiceFilter(const ServiceFilter &f);
    ~ServiceFilter(); // unlocks

    ServiceFilter &operator =(const ServiceFilter &f);

public:
    Method method; ///< adaptation direction
    VectPoint point; ///< adaptation location
    HttpRequest *request; ///< HTTP request being adapted or cause; may be nil
    HttpReply *reply; ///< HTTP response being adapted; may be nil
    AccessLogEntry::Pointer al; ///< info for the future access.log entry
};

std::ostream &operator <<(std::ostream &, const ServiceFilter &);

} // namespace Adaptation

#endif /* SQUID_SRC_ADAPTATION_SERVICEFILTER_H */

