/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

// need a hack to avoid pulling cppunit in.
// fix coming in a dedicated PR
#define SQUID_COMPAT_CPPUNIT_H 1
#include "squid.h"
#include <gtest/gtest.h>

#include "mem/forward.h"
#include "mem/Pool.h"

namespace {

class SomethingToAlloc
{
public:
    int aValue;
};

class MoreToAlloc
{
    MEMPROXY_CLASS(MoreToAlloc);

public:
    int aValue = 0;
};

TEST(testMem, testMemPool)
{
    MemAllocator *Pool = memPoolCreate("Test Pool", sizeof(SomethingToAlloc));
    ASSERT_NE(Pool, nullptr);

    auto *something = static_cast<SomethingToAlloc *>(Pool->alloc());
    ASSERT_NE(something, nullptr);
    ASSERT_EQ(something->aValue, 0);
    something->aValue = 5;
    Pool->freeOne(something);

    // Pool should use the FreeList to allocate next object
    auto *otherthing = static_cast<SomethingToAlloc *>(Pool->alloc());
    ASSERT_EQ(otherthing, something);
    ASSERT_EQ(otherthing->aValue, 0);
    Pool->freeOne(otherthing);

    delete Pool;
}

TEST(testMem, testMemProxy)
{
    auto *something = new MoreToAlloc;
    ASSERT_NE(something, nullptr);
    ASSERT_EQ(something->aValue, 0);
    something->aValue = 5;
    delete something;

    // The MEMPROXY pool should use its FreeList to allocate next object
    auto *otherthing = new MoreToAlloc;
    ASSERT_EQ(otherthing, something);
    ASSERT_EQ(otherthing->aValue, 0);
}

}; /* namespace */

time_t squid_curtime = 0;
