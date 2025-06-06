/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_MESSAGEBUCKET_H
#define SQUID_SRC_MESSAGEBUCKET_H

#if USE_DELAY_POOLS

#include "BandwidthBucket.h"
#include "base/RefCount.h"
#include "comm/forward.h"
#include "MessageDelayPools.h"

/// Limits Squid-to-client bandwidth for each matching response
class MessageBucket : public RefCountable, public BandwidthBucket
{
    MEMPROXY_CLASS(MessageBucket);

public:
    typedef RefCount<MessageBucket> Pointer;

    MessageBucket(const int speed, const int initialLevelPercent, const double sizeLimit, MessageDelayPool::Pointer pool);

    /* BandwidthBucket API */
    int quota() override;
    void scheduleWrite(Comm::IoCallback *state) override;
    void reduceBucket(int len) override;

private:
    MessageDelayPool::Pointer theAggregate;
};

#endif /* USE_DELAY_POOLS */

#endif /* SQUID_SRC_MESSAGEBUCKET_H */

