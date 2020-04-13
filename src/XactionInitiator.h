/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_XACTION_INITIATOR_H
#define SQUID_SRC_XACTION_INITIATOR_H

/// identifies a protocol agent or Squid feature initiating transactions
class XactionInitiator
{
public:
    /// transaction triggers
    enum Initiator {
        initUnknown = 0,
        initClient = 1 << 0,       ///< HTTP or FTP client
        initPeerPool = 1 << 1,     ///< PeerPool manager
        initCertFetcher = 1 << 2,  ///< Missing intermediate certificates fetching code
        initEsi = 1 << 3,          ///< ESI processing code
        initCacheDigest = 1 << 4,  ///< Cache Digest fetching code
        initHtcp = 1 << 5,         ///< HTCP client
        initIcp = 1 << 6,          ///< the ICP/neighbors subsystem
        initIcmp = 1 << 7,         ///< the ICMP RTT database (NetDB) neighbors exchange subsystem
        initAsn = 1 << 8,          ///< the ASN db subsystem
        initIpc = 1 << 9,          ///< the IPC subsystem
        initAdaptation = 1 << 10,  ///< ICAP/ECAP requests generated by Squid
        initIcon = 1 << 11,        ///< internal icons
        initPeerMcast = 1 << 12,   ///< neighbor multicast
        initServer = 1 << 13,      ///< HTTP/2 push request (not yet supported by Squid)

        initAdaptationOrphan_ = 1 << 31  ///< eCAP-created HTTP message w/o an associated HTTP transaction (not ACL-detectable)
    };

    typedef uint32_t Initiators;  ///< Initiator set

    // this class is a just a trivial wrapper so we allow explicit conversions
    XactionInitiator(Initiator i) :
        initiator(i) {}

    /// whether this initiator belongs to the given set
    bool in(Initiators setOfInitiators) const { return (initiator & setOfInitiators) != 0; }

    /// whether the transaction was initiated by an internal subsystem
    bool internalClient() const
    {
        return (initiator & InternalInitiators()) != 0;
    }

    /// internally generated requests
    static Initiators InternalInitiators()
    {
        return initPeerPool | initCertFetcher | initEsi | initCacheDigest | initIcp | initIcmp | initIpc | initAdaptation | initIcon | initPeerMcast;
    }

    /// all initiators
    static Initiators AllInitiators()
    {
        return 0xFFFFFFFF;
    }

    static Initiators ParseInitiators(const char *name);

private:
    XactionInitiator() {}

    Initiator initiator;
};

#endif  // SQUID_SRC_XACTION_INITIATOR_H
