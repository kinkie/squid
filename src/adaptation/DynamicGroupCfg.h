/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ADAPTATION_DYNAMICGROUPCFG_H
#define SQUID_SRC_ADAPTATION_DYNAMICGROUPCFG_H

#include "SquidString.h"

#include <vector>

namespace Adaptation
{

/// DynamicServiceGroup configuration to remember future dynamic chains
class DynamicGroupCfg
{
public:
    typedef std::vector<String> Store;
    typedef String Id;

    Id id; ///< group id
    Store services; ///< services in the group

    bool empty() const { return services.empty(); } ///< no services added

    /// configured service IDs in X-Next-Services value (comma-separated) format
    const String &serviceIds() const { return id; }

    void add(const String &item); ///< updates group id and services
    void clear(); ///< makes the config empty
};

inline
std::ostream &operator <<(std::ostream &os, const DynamicGroupCfg &cfg)
{
    return os << cfg.id;
}

} // namespace Adaptation

#endif /* SQUID_SRC_ADAPTATION_DYNAMICGROUPCFG_H */

