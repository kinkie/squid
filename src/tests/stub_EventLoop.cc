/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "EventLoop.h"

#define STUB_API "EventLoop.cc"
#include "tests/STUB.h"

EventLoop *EventLoop::Running = nullptr;

EventLoop::EventLoop(): errcount(0), last_loop(false), timeService(nullptr),
    primaryEngine(nullptr), loop_delay(0), error(false), runOnceResult(false)
    STUB_NOP

    void EventLoop::registerEngine(AsyncEngine *) STUB

