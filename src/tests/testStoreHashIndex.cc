/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "compat/cppunit.h"
#include "MemObject.h"
#include "SquidConfig.h"
#include "Store.h"
#include "store/Disks.h"
#include "StoreSearch.h"
#include "TestSwapDir.h"

/*
 * test the store framework
 */

class TestStoreHashIndex : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(TestStoreHashIndex);
    CPPUNIT_TEST(testStats);
    CPPUNIT_TEST(testMaxSize);
    CPPUNIT_TEST(testSearch);
    CPPUNIT_TEST_SUITE_END();

public:
protected:
    void testStats();
    void testMaxSize();
    void testSearch();
};
CPPUNIT_TEST_SUITE_REGISTRATION(TestStoreHashIndex);

static void
addSwapDir(TestSwapDirPointer aStore)
{
    allocate_new_swapdir(Config.cacheSwap);
    Config.cacheSwap.swapDirs[Config.cacheSwap.n_configured] = aStore.getRaw();
    ++Config.cacheSwap.n_configured;
}

void
TestStoreHashIndex::testStats()
{
    StoreEntry *logEntry = new StoreEntry;
    logEntry->createMemObject("dummy_storeId", nullptr, HttpRequestMethod());
    logEntry->store_status = STORE_PENDING;
    TestSwapDirPointer aStore (new TestSwapDir);
    TestSwapDirPointer aStore2 (new TestSwapDir);
    addSwapDir(aStore);
    addSwapDir(aStore2);
    CPPUNIT_ASSERT_EQUAL(false, aStore->statsCalled);
    CPPUNIT_ASSERT_EQUAL(false, aStore2->statsCalled);
    Store::Stats(logEntry);
    free_cachedir(&Config.cacheSwap);
    CPPUNIT_ASSERT_EQUAL(true, aStore->statsCalled);
    CPPUNIT_ASSERT_EQUAL(true, aStore2->statsCalled);
}

void
TestStoreHashIndex::testMaxSize()
{
    StoreEntry *logEntry = new StoreEntry;
    logEntry->createMemObject("dummy_storeId", nullptr, HttpRequestMethod());
    logEntry->store_status = STORE_PENDING;
    TestSwapDirPointer aStore (new TestSwapDir);
    TestSwapDirPointer aStore2 (new TestSwapDir);
    addSwapDir(aStore);
    addSwapDir(aStore2);
    CPPUNIT_ASSERT_EQUAL(static_cast<uint64_t>(6), Store::Root().maxSize());
    free_cachedir(&Config.cacheSwap);
}

static StoreEntry *
addedEntry(Store::Disk *aStore,
           String name,
           String,
           String
          )
{
    StoreEntry *e = new StoreEntry();
    e->store_status = STORE_OK;
    e->setMemStatus(NOT_IN_MEMORY);
    e->swap_status = SWAPOUT_DONE; /* bogus haha */
    e->swap_filen = 0; /* garh - lower level*/
    e->swap_dirn = -1;

    for (size_t i = 0; i < Config.cacheSwap.n_configured; ++i) {
        if (INDEXSD(i) == aStore)
            e->swap_dirn = i;
    }

    CPPUNIT_ASSERT (e->swap_dirn != -1);
    e->swap_file_sz = 0; /* garh lower level */
    e->lastref = squid_curtime;
    e->timestamp = squid_curtime;
    e->expires = squid_curtime;
    e->lastModified(squid_curtime);
    e->refcount = 1;
    e->ping_status = PING_NONE;
    EBIT_CLR(e->flags, ENTRY_VALIDATED);
    e->hashInsert((const cache_key *)name.termedBuf()); /* do it after we clear KEY_PRIVATE */
    return e;
}

static void commonInit()
{
    static bool inited = false;

    if (inited)
        return;

    inited = true;

    Mem::Init();

    Config.Store.avgObjectSize = 1024;

    Config.Store.objectsPerBucket = 20;

    Config.Store.maxObjectSize = 2048;

    Config.memShared.defaultTo(false);

    Config.store_dir_select_algorithm = xstrdup("round-robin");
}

/* TODO make this a cbdata class */

static bool cbcalled;

static void
searchCallback(void *)
{
    cbcalled = true;
}

void
TestStoreHashIndex::testSearch()
{
    commonInit();
    TestSwapDirPointer aStore (new TestSwapDir);
    TestSwapDirPointer aStore2 (new TestSwapDir);
    addSwapDir(aStore);
    addSwapDir(aStore2);
    Store::Root().init();
    StoreEntry * entry1 = addedEntry(aStore.getRaw(), "name", nullptr, nullptr);
    StoreEntry * entry2 = addedEntry(aStore2.getRaw(), "name2", nullptr, nullptr);
    StoreSearchPointer search = Store::Root().search(); /* search for everything in the store */

    /* nothing should be immediately available */
    CPPUNIT_ASSERT_EQUAL(false, search->error());
    CPPUNIT_ASSERT_EQUAL(false, search->isDone());
    CPPUNIT_ASSERT_EQUAL(static_cast<StoreEntry *>(nullptr), search->currentItem());

    /* trigger a callback */
    cbcalled = false;
    search->next(searchCallback, nullptr);
    CPPUNIT_ASSERT_EQUAL(true, cbcalled);

    /* we should have access to a entry now, that matches the entry we had before */
    CPPUNIT_ASSERT_EQUAL(false, search->error());
    CPPUNIT_ASSERT_EQUAL(false, search->isDone());
    /* note the hash order is random - the test happens to be in a nice order */
    CPPUNIT_ASSERT_EQUAL(entry1, search->currentItem());
    //CPPUNIT_ASSERT_EQUAL(false, search->next());

    /* trigger another callback */
    cbcalled = false;
    search->next(searchCallback, nullptr);
    CPPUNIT_ASSERT_EQUAL(true, cbcalled);

    /* we should have access to a entry now, that matches the entry we had before */
    CPPUNIT_ASSERT_EQUAL(false, search->error());
    CPPUNIT_ASSERT_EQUAL(false, search->isDone());
    CPPUNIT_ASSERT_EQUAL(entry2, search->currentItem());
    //CPPUNIT_ASSERT_EQUAL(false, search->next());

    /* trigger another callback */
    cbcalled = false;
    search->next(searchCallback, nullptr);
    CPPUNIT_ASSERT_EQUAL(true, cbcalled);

    /* now we should have no error, we should have finished and have no current item */
    CPPUNIT_ASSERT_EQUAL(false, search->error());
    CPPUNIT_ASSERT_EQUAL(true, search->isDone());
    CPPUNIT_ASSERT_EQUAL(static_cast<StoreEntry *>(nullptr), search->currentItem());
    //CPPUNIT_ASSERT_EQUAL(false, search->next());
}

// This test uses main() from ./testStore.cc.

