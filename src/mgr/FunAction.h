/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 16    Cache Manager API */

#ifndef SQUID_SRC_MGR_FUNACTION_H
#define SQUID_SRC_MGR_FUNACTION_H

#include "mgr/Action.h"
#include "mgr/ActionCreator.h"

namespace Mgr
{

/// function-based cache manager Action; a wrapper for so called legacy actions
/// that do everything using a single OBJH function
class FunAction: public Action
{
protected:
    FunAction(const CommandPointer &cmd, OBJH *aHandler);

public:
    static Pointer Create(const CommandPointer &cmd, OBJH *aHandler);

    /* Action API */
    void respond(const Request& request) override;
    // we cannot aggregate because we do not even know what the handler does
    bool aggregatable() const override { return false; }

protected:
    /* Action API */
    void dump(StoreEntry *entry) override;

private:
    OBJH *handler; ///< legacy function that collects and dumps info
};

} // namespace Mgr

#endif /* SQUID_SRC_MGR_FUNACTION_H */

