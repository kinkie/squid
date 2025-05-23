/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 20    Store Controller */

#include "squid.h"
#include "mem_node.h"
#include "MemStore.h"
#include "SquidConfig.h"
#include "SquidMath.h"
#include "store/Controller.h"
#include "store/Disks.h"
#include "store/forward.h"
#include "store/LocalSearch.h"
#include "tools.h"
#include "Transients.h"

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

/*
 * store_dirs_rebuilding is initialized to _1_ as a hack so that
 * storeDirWriteCleanLogs() doesn't try to do anything unless _all_
 * cache_dirs have been read.  For example, without this hack, Squid
 * will try to write clean log files if -kparse fails (because it
 * calls fatal()).
 */
int Store::Controller::store_dirs_rebuilding = 1;

Store::Controller::Controller() :
    disks(new Disks),
    sharedMemStore(nullptr),
    localMemStore(false),
    transients(nullptr)
{
    assert(!store_table);
}

/// this destructor is never called because Controller singleton is immortal
Store::Controller::~Controller()
{
    // assert at runtime because we cannot `= delete` an overridden destructor
    assert(!"Controller is never destroyed");
}

void
Store::Controller::init()
{
    if (IamWorkerProcess()) {
        if (MemStore::Enabled()) {
            sharedMemStore = new MemStore;
            sharedMemStore->init();
        } else if (Config.memMaxSize > 0) {
            localMemStore = true;
        }
    }

    disks->init();

    if (Transients::Enabled() && IamWorkerProcess()) {
        transients = new Transients;
        transients->init();
    }
}

void
Store::Controller::create()
{
    disks->create();

#if !(_SQUID_WINDOWS_ || _SQUID_MINGW_)
    pid_t pid;
    do {
        PidStatus status;
        pid = WaitForAnyPid(status, WNOHANG);
    } while (pid > 0 || (pid < 0 && errno == EINTR));
#endif
}

void
Store::Controller::maintain()
{
    static time_t last_warn_time = 0;

    disks->maintain();

    /* this should be emitted by the oversize dir, not globally */

    if (Root().currentSize() > Store::Root().maxSize()) {
        if (squid_curtime - last_warn_time > 10) {
            debugs(20, DBG_CRITICAL, "WARNING: Disk space over limit: "
                   << Store::Root().currentSize() / 1024.0 << " KB > "
                   << (Store::Root().maxSize() >> 10) << " KB");
            last_warn_time = squid_curtime;
        }
    }
}

void
Store::Controller::getStats(StoreInfoStats &stats) const
{
    if (sharedMemStore)
        sharedMemStore->getStats(stats);
    else {
        // move this code to a non-shared memory cache class when we have it
        stats.mem.shared = false;
        stats.mem.capacity = Config.memMaxSize;
        stats.mem.size = mem_node::StoreMemSize();
        if (localMemStore) {
            // XXX: also count internal/in-transit objects
            stats.mem.count = hot_obj_count;
        } else {
            // XXX: count internal/in-transit objects instead
            stats.mem.count = hot_obj_count;
        }
    }

    disks->getStats(stats);

    // low-level info not specific to memory or disk cache
    stats.store_entry_count = StoreEntry::inUseCount();
    stats.mem_object_count = MemObject::inUseCount();
}

void
Store::Controller::stat(StoreEntry &output) const
{
    storeAppendPrintf(&output, "Store Directory Statistics:\n");
    storeAppendPrintf(&output, "Store Entries          : %lu\n",
                      (unsigned long int)StoreEntry::inUseCount());
    storeAppendPrintf(&output, "Maximum Swap Size      : %" PRIu64 " KB\n",
                      maxSize() >> 10);
    storeAppendPrintf(&output, "Current Store Swap Size: %.2f KB\n",
                      currentSize() / 1024.0);
    storeAppendPrintf(&output, "Current Capacity       : %.2f%% used, %.2f%% free\n",
                      Math::doublePercent(currentSize(), maxSize()),
                      Math::doublePercent((maxSize() - currentSize()), maxSize()));

    if (sharedMemStore)
        sharedMemStore->stat(output);

    disks->stat(output);
}

/* if needed, this could be taught to cache the result */
uint64_t
Store::Controller::maxSize() const
{
    /* TODO: include memory cache ? */
    return disks->maxSize();
}

uint64_t
Store::Controller::minSize() const
{
    /* TODO: include memory cache ? */
    return disks->minSize();
}

uint64_t
Store::Controller::currentSize() const
{
    /* TODO: include memory cache ? */
    return disks->currentSize();
}

uint64_t
Store::Controller::currentCount() const
{
    /* TODO: include memory cache ? */
    return disks->currentCount();
}

int64_t
Store::Controller::maxObjectSize() const
{
    /* TODO: include memory cache ? */
    return disks->maxObjectSize();
}

void
Store::Controller::configure()
{
    disks->configure();

    store_swap_high = (long) (((float) maxSize() *
                               (float) Config.Swap.highWaterMark) / (float) 100);
    store_swap_low = (long) (((float) maxSize() *
                              (float) Config.Swap.lowWaterMark) / (float) 100);
    store_pages_max = Config.memMaxSize / sizeof(mem_node);

    // TODO: move this into a memory cache class when we have one
    const int64_t memMax = static_cast<int64_t>(min(Config.Store.maxInMemObjSize, Config.memMaxSize));
    const int64_t disksMax = disks->maxObjectSize();
    store_maxobjsize = std::max(disksMax, memMax);
}

StoreSearch *
Store::Controller::search()
{
    // this is the only kind of search we currently support
    return NewLocalSearch();
}

void
Store::Controller::sync(void)
{
    if (sharedMemStore)
        sharedMemStore->sync();
    disks->sync();
}

/*
 * handle callbacks all available fs'es
 */
int
Store::Controller::callback()
{
    /* mem cache callbacks ? */
    return disks->callback();
}

/// update reference counters of the recently touched entry
void
Store::Controller::referenceBusy(StoreEntry &e)
{
    // special entries do not belong to any specific Store, but are IN_MEMORY
    if (EBIT_TEST(e.flags, ENTRY_SPECIAL))
        return;

    /* Notify the fs that we're referencing this object again */

    if (e.hasDisk())
        disks->reference(e);

    // Notify the memory cache that we're referencing this object again
    if (sharedMemStore && e.mem_status == IN_MEMORY)
        sharedMemStore->reference(e);

    // TODO: move this code to a non-shared memory cache class when we have it
    if (e.mem_obj) {
        if (mem_policy->Referenced)
            mem_policy->Referenced(mem_policy, &e, &e.mem_obj->repl);
    }
}

/// dereference()s an idle entry
/// \returns false if and only if the entry should be deleted
bool
Store::Controller::dereferenceIdle(StoreEntry &e, bool wantsLocalMemory)
{
    // special entries do not belong to any specific Store, but are IN_MEMORY
    if (EBIT_TEST(e.flags, ENTRY_SPECIAL))
        return true;

    // idle private entries cannot be reused
    if (EBIT_TEST(e.flags, KEY_PRIVATE))
        return false;

    bool keepInStoreTable = false; // keep only if somebody needs it there

    // Notify the fs that we are not referencing this object any more. This
    // should be done even if we overwrite keepInStoreTable afterwards.

    if (e.hasDisk())
        keepInStoreTable = disks->dereference(e) || keepInStoreTable;

    // Notify the memory cache that we're not referencing this object any more
    if (sharedMemStore && e.mem_status == IN_MEMORY)
        keepInStoreTable = sharedMemStore->dereference(e) || keepInStoreTable;

    // TODO: move this code to a non-shared memory cache class when we have it
    if (e.mem_obj) {
        if (mem_policy->Dereferenced)
            mem_policy->Dereferenced(mem_policy, &e, &e.mem_obj->repl);
        // non-shared memory cache relies on store_table
        if (localMemStore)
            keepInStoreTable = wantsLocalMemory || keepInStoreTable;
    }

    if (e.hittingRequiresCollapsing()) {
        // If we were writing this now-locally-idle entry, then we did not
        // finish and should now destroy an incomplete entry. Otherwise, do not
        // leave this idle StoreEntry behind because handleIMSReply() lacks
        // freshness checks when hitting a collapsed revalidation entry.
        keepInStoreTable = false; // may overrule fs decisions made above
    }

    return keepInStoreTable;
}

bool
Store::Controller::markedForDeletion(const cache_key *key) const
{
    // assuming a public key, checking Transients should cover all cases.
    return transients && transients->markedForDeletion(key);
}

bool
Store::Controller::markedForDeletionAndAbandoned(const StoreEntry &e) const
{
    // The opposite check order could miss a reader that has arrived after the
    // !readers() and before the markedForDeletion() check.
    return markedForDeletion(reinterpret_cast<const cache_key*>(e.key)) &&
           transients && !transients->readers(e);
}

bool
Store::Controller::hasReadableDiskEntry(const StoreEntry &e) const
{
    return disks->hasReadableEntry(e);
}

/// flags problematic entries before find() commits to finalizing/returning them
void
Store::Controller::checkFoundCandidate(const StoreEntry &entry) const
{
    checkTransients(entry);

    // The "hittingRequiresCollapsing() has an active writer" checks below
    // protect callers from getting stuck and/or from using a stale revalidation
    // reply. However, these protections are not reliable because the writer may
    // disappear at any time and/or without a trace. Collapsing adds risks...
    if (entry.hittingRequiresCollapsing()) {
        if (entry.hasTransients()) {
            // Too late to check here because the writer may be gone by now, but
            // Transients do check when they setCollapsingRequirement().
        } else {
            // a local writer must hold a lock on its writable entry
            if (!(entry.locked() && entry.isAccepting()))
                throw TextException("no local writer", Here());
        }
    }
}

StoreEntry *
Store::Controller::find(const cache_key *key)
{
    if (const auto entry = peek(key)) {
        try {
            if (!entry->key)
                allowSharing(*entry, key);
            checkFoundCandidate(*entry);
            entry->touch();
            referenceBusy(*entry);
            return entry;
        } catch (const std::exception &ex) {
            debugs(20, 2, "failed with " << *entry << ": " << ex.what());
            entry->release();
            // fall through
        }
    }
    return nullptr;
}

/// indexes and adds SMP-tracking for an ephemeral peek() result
void
Store::Controller::allowSharing(StoreEntry &entry, const cache_key *key)
{
    // anchorToCache() below and many find() callers expect a registered entry
    addReading(&entry, key);

    if (entry.hasTransients()) {
        // store hadWriter before computing `found`; \see Transients::get()
        const auto hadWriter = transients->hasWriter(entry);
        const auto found = anchorToCache(entry);
        if (!found) {
            // !found should imply hittingRequiresCollapsing() regardless of writer presence
            if (!entry.hittingRequiresCollapsing()) {
                debugs(20, DBG_IMPORTANT, "ERROR: Squid BUG: missing ENTRY_REQUIRES_COLLAPSING for " << entry);
                throw TextException("transients entry missing ENTRY_REQUIRES_COLLAPSING", Here());
            }

            if (!hadWriter) {
                // prevent others from falling into the same trap
                throw TextException("unattached transients entry missing writer", Here());
            }
        }
    }
}

StoreEntry *
Store::Controller::findCallbackXXX(const cache_key *key)
{
    // We could check for mem_obj presence (and more), moving and merging some
    // of the duplicated neighborsUdpAck() and neighborsHtcpReply() code here,
    // but that would mean polluting Store with HTCP/ICP code. Instead, we
    // should encapsulate callback-related data in a protocol-neutral MemObject
    // member or use an HTCP/ICP-specific index rather than store_table.

    // cannot reuse peekAtLocal() because HTCP/ICP callbacks may use private keys
    return static_cast<StoreEntry*>(hash_lookup(store_table, key));
}

/// \returns either an existing local reusable StoreEntry object or nil
/// To treat remotely marked entries specially,
/// callers ought to check markedForDeletion() first!
StoreEntry *
Store::Controller::peekAtLocal(const cache_key *key)
{
    if (StoreEntry *e = static_cast<StoreEntry*>(hash_lookup(store_table, key))) {
        // callers must only search for public entries
        assert(!EBIT_TEST(e->flags, KEY_PRIVATE));
        assert(e->publicKey());
        checkTransients(*e);

        // TODO: ignore and maybe handleIdleEntry() unlocked intransit entries
        // because their backing store slot may be gone already.
        return e;
    }
    return nullptr;
}

StoreEntry *
Store::Controller::peek(const cache_key *key)
{
    debugs(20, 3, storeKeyText(key));

    if (markedForDeletion(key)) {
        debugs(20, 3, "ignoring marked in-transit " << storeKeyText(key));
        return nullptr;
    }

    if (StoreEntry *e = peekAtLocal(key)) {
        debugs(20, 3, "got local in-transit entry: " << *e);
        return e;
    }

    // Must search transients before caches because we must sync those we find.
    if (transients) {
        if (StoreEntry *e = transients->get(key)) {
            debugs(20, 3, "got shared in-transit entry: " << *e);
            return e;
        }
    }

    if (sharedMemStore) {
        if (StoreEntry *e = sharedMemStore->get(key)) {
            debugs(20, 3, "got mem-cached entry: " << *e);
            return e;
        }
    }

    if (const auto e = disks->get(key)) {
        debugs(20, 3, "got disk-cached entry: " << *e);
        return e;
    }

    debugs(20, 4, "cannot locate " << storeKeyText(key));
    return nullptr;
}

bool
Store::Controller::transientsReader(const StoreEntry &e) const
{
    return transients && e.hasTransients() && transients->isReader(e);
}

bool
Store::Controller::transientsWriter(const StoreEntry &e) const
{
    return transients && e.hasTransients() && transients->isWriter(e);
}

int64_t
Store::Controller::accumulateMore(StoreEntry &entry) const
{
    return disks->accumulateMore(entry);
    // The memory cache should not influence for-swapout accumulation decision.
}

// Must be called from StoreEntry::release() or releaseRequest() because
// those methods currently manage local indexing of StoreEntry objects.
// TODO: Replace StoreEntry::release*() with Root().evictCached().
void
Store::Controller::evictCached(StoreEntry &e)
{
    debugs(20, 7, e);
    if (transients)
        transients->evictCached(e);
    memoryEvictCached(e);
    disks->evictCached(e);
}

void
Store::Controller::evictIfFound(const cache_key *key)
{
    debugs(20, 7, storeKeyText(key));

    if (StoreEntry *entry = peekAtLocal(key)) {
        debugs(20, 5, "marking local in-transit " << *entry);
        entry->release(true);
        return;
    }

    if (sharedMemStore)
        sharedMemStore->evictIfFound(key);

    disks->evictIfFound(key);

    if (transients)
        transients->evictIfFound(key);
}

/// whether the memory cache is allowed to store that many additional pages
bool
Store::Controller::memoryCacheHasSpaceFor(const int pagesRequired) const
{
    // XXX: We count mem_nodes but may free shared memory pages instead.
    const auto fits = mem_node::InUseCount() + pagesRequired <= store_pages_max;
    debugs(20, 7, fits << ": " << mem_node::InUseCount() << '+' << pagesRequired << '?' << store_pages_max);
    return fits;
}

void
Store::Controller::freeMemorySpace(const int bytesRequired)
{
    const auto pagesRequired = (bytesRequired + SM_PAGE_SIZE-1) / SM_PAGE_SIZE;

    if (memoryCacheHasSpaceFor(pagesRequired))
        return;

    // XXX: When store_pages_max is smaller than pagesRequired, we should not
    // look for more space (but we do because we want to abandon idle entries?).

    // limit our performance impact to one walk per second
    static time_t lastWalk = 0;
    if (lastWalk == squid_curtime)
        return;
    lastWalk = squid_curtime;

    debugs(20, 2, "need " << pagesRequired << " pages");

    // let abandon()/handleIdleEntry() know about the impeding memory shortage
    memoryPagesDebt_ = pagesRequired;

    // XXX: SMP-unaware: Walkers should iterate memory cache, not store_table.
    // XXX: Limit iterations by time, not arbitrary count.
    const auto walker = mem_policy->PurgeInit(mem_policy, 100000);
    int removed = 0;
    while (const auto entry = walker->Next(walker)) {
        // Abandoned memory cache entries are purged during memory shortage.
        entry->abandon(__func__); // may delete entry
        ++removed;

        if (memoryCacheHasSpaceFor(pagesRequired))
            break;
    }
    // TODO: Move to RemovalPolicyWalker::Done() that has more/better details.
    debugs(20, 3, "removed " << removed << " out of " << hot_obj_count  << " memory-cached entries");
    walker->Done(walker);
    memoryPagesDebt_ = 0;
}

// move this into [non-shared] memory cache class when we have one
/// whether e should be kept in local RAM for possible future caching
bool
Store::Controller::keepForLocalMemoryCache(StoreEntry &e) const
{
    if (!e.memoryCachable())
        return false;

    // does the current and expected size obey memory caching limits?
    assert(e.mem_obj);
    const int64_t loadedSize = e.mem_obj->endOffset();
    const int64_t expectedSize = e.mem_obj->expectedReplySize(); // may be < 0
    const int64_t ramSize = max(loadedSize, expectedSize);
    const int64_t ramLimit = min(
                                 static_cast<int64_t>(Config.memMaxSize),
                                 static_cast<int64_t>(Config.Store.maxInMemObjSize));
    return ramSize <= ramLimit;
}

void
Store::Controller::memoryOut(StoreEntry &e, const bool preserveSwappable)
{
    bool keepInLocalMemory = false;
    if (sharedMemStore)
        sharedMemStore->write(e); // leave keepInLocalMemory false
    else if (localMemStore)
        keepInLocalMemory = keepForLocalMemoryCache(e);

    debugs(20, 7, "keepInLocalMemory: " << keepInLocalMemory);

    if (!keepInLocalMemory)
        e.trimMemory(preserveSwappable);
}

/// removes the entry from the memory cache
/// XXX: Dangerous side effect: Unlocked entries lose their mem_obj.
void
Store::Controller::memoryEvictCached(StoreEntry &e)
{
    // TODO: Untangle memory caching from mem_obj.
    if (sharedMemStore)
        sharedMemStore->evictCached(e);
    else // TODO: move into [non-shared] memory cache class when we have one
        if (!e.locked())
            e.destroyMemObject();
}

void
Store::Controller::memoryDisconnect(StoreEntry &e)
{
    if (sharedMemStore)
        sharedMemStore->disconnect(e);
    // else nothing to do for non-shared memory cache
}

void
Store::Controller::noteStoppedSharedWriting(StoreEntry &e)
{
    if (transients && e.hasTransients()) // paranoid: the caller should check
        transients->completeWriting(e);
}

int
Store::Controller::transientReaders(const StoreEntry &e) const
{
    return (transients && e.hasTransients()) ?
           transients->readers(e) : 0;
}

void
Store::Controller::transientsDisconnect(StoreEntry &e)
{
    if (transients)
        transients->disconnect(e);
}

void
Store::Controller::handleIdleEntry(StoreEntry &e)
{
    bool keepInLocalMemory = false;

    if (EBIT_TEST(e.flags, ENTRY_SPECIAL)) {
        // Icons (and cache digests?) should stay in store_table until we
        // have a dedicated storage for them (that would not purge them).
        // They are not managed [well] by any specific Store handled below.
        keepInLocalMemory = true;
    } else if (sharedMemStore) {
        // leave keepInLocalMemory false; sharedMemStore maintains its own cache
    } else if (localMemStore) {
        keepInLocalMemory = keepForLocalMemoryCache(e) && // in good shape and
                            // the local memory cache is not overflowing
                            memoryCacheHasSpaceFor(memoryPagesDebt_);
    }

    // An idle, unlocked entry that only belongs to a SwapDir which controls
    // its own index, should not stay in the global store_table.
    if (!dereferenceIdle(e, keepInLocalMemory)) {
        debugs(20, 5, "destroying unlocked entry: " << &e << ' ' << e);
        destroyStoreEntry(static_cast<hash_link*>(&e));
        return;
    }

    debugs(20, 5, "keepInLocalMemory: " << keepInLocalMemory);

    // formerly known as "WARNING: found KEY_PRIVATE"
    assert(!EBIT_TEST(e.flags, KEY_PRIVATE));

    // TODO: move this into [non-shared] memory cache class when we have one
    if (keepInLocalMemory) {
        e.setMemStatus(IN_MEMORY);
        e.mem_obj->unlinkRequest();
        return;
    }

    // We know the in-memory data will be gone. Get rid of the entire entry if
    // it has nothing worth preserving on disk either.
    if (!e.swappedOut()) {
        e.release(); // deletes e
        return;
    }

    memoryEvictCached(e); // may already be gone
    // and keep the entry in store_table for its on-disk data
}

bool
Store::Controller::updateOnNotModified(StoreEntry *old, StoreEntry &e304)
{
    Must(old);
    Must(old->mem_obj);
    Must(e304.mem_obj);

    // updateOnNotModified() may be called many times for the same old entry.
    // e304.mem_obj->appliedUpdates value distinguishes two cases:
    //   false: Independent store clients revalidating the same old StoreEntry.
    //          Each such update uses its own e304. The old StoreEntry
    //          accumulates such independent updates.
    //   true: Store clients feeding off the same 304 response. Each such update
    //         uses the same e304. For timestamps correctness and performance
    //         sake, it is best to detect and skip such repeated update calls.
    if (e304.mem_obj->appliedUpdates) {
        debugs(20, 5, "ignored repeated update of " << *old << " with " << e304);
        return true;
    }
    e304.mem_obj->appliedUpdates = true;

    try {
        if (!old->updateOnNotModified(e304)) {
            debugs(20, 5, "updated nothing in " << *old << " with " << e304);
            return true;
        }
    } catch (...) {
        debugs(20, DBG_IMPORTANT, "ERROR: Failed to update a cached response: " << CurrentException);
        return false;
    }

    if (sharedMemStore && old->mem_status == IN_MEMORY && !EBIT_TEST(old->flags, ENTRY_SPECIAL))
        sharedMemStore->updateHeaders(old);

    if (old->swap_dirn > -1)
        disks->updateHeaders(old);

    return true;
}

bool
Store::Controller::allowCollapsing(StoreEntry *e, const RequestFlags &reqFlags,
                                   const HttpRequestMethod &)
{
    const KeyScope keyScope = reqFlags.refresh ? ksRevalidation : ksDefault;
    // set the flag now so that it gets copied into the Transients entry
    e->setCollapsingRequirement(true);
    if (e->makePublic(keyScope)) { // this is needed for both local and SMP collapsing
        debugs(20, 3, "may " << (transients && e->hasTransients() ?
                                 "SMP-" : "locally-") << "collapse " << *e);
        assert(e->hittingRequiresCollapsing());
        return true;
    }
    // paranoid cleanup; the flag is meaningless for private entries
    e->setCollapsingRequirement(false);
    return false;
}

void
Store::Controller::addReading(StoreEntry *e, const cache_key *key)
{
    if (transients)
        transients->monitorIo(e, key, Store::ioReading);
    e->hashInsert(key);
}

void
Store::Controller::addWriting(StoreEntry *e, const cache_key *key)
{
    assert(e);
    if (EBIT_TEST(e->flags, ENTRY_SPECIAL))
        return; // constant memory-resident entries do not need transients

    if (transients)
        transients->monitorIo(e, key, Store::ioWriting);
    // else: non-SMP configurations do not need transients
}

void
Store::Controller::syncCollapsed(const sfileno xitIndex)
{
    assert(transients);

    StoreEntry *collapsed = transients->findCollapsed(xitIndex);
    if (!collapsed) { // the entry is no longer active, ignore update
        debugs(20, 7, "not SMP-syncing not-transient " << xitIndex);
        return;
    }

    if (!collapsed->locked()) {
        debugs(20, 3, "skipping (and may destroy) unlocked " << *collapsed);
        handleIdleEntry(*collapsed);
        return;
    }

    assert(collapsed->mem_obj);

    if (EBIT_TEST(collapsed->flags, ENTRY_ABORTED)) {
        debugs(20, 3, "skipping already aborted " << *collapsed);
        return;
    }

    debugs(20, 7, "syncing " << *collapsed);

    Transients::EntryStatus entryStatus;
    transients->status(*collapsed, entryStatus);

    if (entryStatus.waitingToBeFreed) {
        debugs(20, 3, "will release " << *collapsed << " due to waitingToBeFreed");
        collapsed->release(true); // may already be marked
    }

    if (transients->isWriter(*collapsed))
        return; // readers can only change our waitingToBeFreed flag

    assert(transients->isReader(*collapsed));

    bool found = false;
    bool inSync = false;
    if (sharedMemStore && collapsed->mem_obj->memCache.io == Store::ioDone) {
        found = true;
        inSync = true;
        debugs(20, 7, "already handled by memory store: " << *collapsed);
    } else if (sharedMemStore && collapsed->hasMemStore()) {
        found = true;
        inSync = sharedMemStore->updateAnchored(*collapsed);
        // TODO: handle entries attached to both memory and disk
    } else if (collapsed->hasDisk()) {
        found = true;
        inSync = disks->updateAnchored(*collapsed);
    } else {
        try {
            found = anchorToCache(*collapsed);
            inSync = found;
        } catch (...) {
            // TODO: Write an exception handler for the entire method.
            debugs(20, 3, "anchorToCache() failed for " << *collapsed << ": " << CurrentException);
            collapsed->abort();
            return;
        }
    }

    if (entryStatus.waitingToBeFreed && !found) {
        debugs(20, 3, "aborting unattached " << *collapsed <<
               " because it was marked for deletion before we could attach it");
        collapsed->abort();
        return;
    }

    if (inSync) {
        debugs(20, 5, "synced " << *collapsed);
        assert(found);
        collapsed->setCollapsingRequirement(false);
        collapsed->invokeHandlers();
        return;
    }

    if (found) { // unrecoverable problem syncing this entry
        debugs(20, 3, "aborting unsyncable " << *collapsed);
        collapsed->abort();
        return;
    }

    if (!entryStatus.hasWriter) {
        debugs(20, 3, "aborting abandoned-by-writer " << *collapsed);
        collapsed->abort();
        return;
    }

    // the entry is still not in one of the caches
    debugs(20, 7, "waiting " << *collapsed);
    collapsed->setCollapsingRequirement(true);
}

/// If possible and has not been done, associates the entry with its store(s).
/// \returns false for not-yet-cached entries that we may attach later
/// \returns true for other entries after synchronizing them with their store
bool
Store::Controller::anchorToCache(StoreEntry &entry)
{
    assert(entry.hasTransients());
    assert(transientsReader(entry));

    // TODO: Attach entries to both memory and disk

    // TODO: Reduce code duplication with syncCollapsed()
    if (sharedMemStore && entry.mem().memCache.io == Store::ioDone) {
        debugs(20, 5, "already handled by memory store: " << entry);
        return true;
    } else if (sharedMemStore && entry.hasMemStore()) {
        debugs(20, 5, "already anchored to memory store: " << entry);
        return true;
    } else if (entry.hasDisk()) {
        debugs(20, 5, "already anchored to disk: " << entry);
        return true;
    }

    debugs(20, 7, "anchoring " << entry);

    Transients::EntryStatus entryStatus;
    transients->status(entry, entryStatus);

    bool found = false;
    if (sharedMemStore)
        found = sharedMemStore->anchorToCache(entry);
    if (!found)
        found = disks->anchorToCache(entry);

    if (found) {
        debugs(20, 7, "anchored " << entry);
        entry.setCollapsingRequirement(false);
        return true;
    }

    if (entryStatus.waitingToBeFreed)
        throw TextException("will never be able to anchor to an already marked entry", Here());

    if (!entryStatus.hasWriter)
        throw TextException("will never be able to anchor to an abandoned-by-writer entry", Here());

    debugs(20, 7, "skipping not yet cached " << entry);
    entry.setCollapsingRequirement(true);
    return false;
}

bool
Store::Controller::SmpAware()
{
    return MemStore::Enabled() || Disks::SmpAware();
}

void
Store::Controller::checkTransients(const StoreEntry &e) const
{
    if (EBIT_TEST(e.flags, ENTRY_SPECIAL))
        return;
    assert(!transients || e.hasTransients());
}

Store::Controller&
Store::Root()
{
    static const auto root = new Controller();
    return *root;
}

