/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_TESTS_CAPTURINGSTOREENTRY_H
#define SQUID_SRC_TESTS_CAPTURINGSTOREENTRY_H

#include "Store.h"

/* class that captures various call data for test analysis */

class CapturingStoreEntry : public StoreEntry
{
    MEMPROXY_CLASS(CapturingStoreEntry);

public:
    CapturingStoreEntry() : _buffer_calls(0), _flush_calls(0) {}

    String _appended_text;
    int _buffer_calls;
    int _flush_calls;

    void buffer() override {
        _buffer_calls += 1;
    }

    void flush() override {
        _flush_calls += 1;
    }

    void append(char const * buf, int len) override {
        if (!buf || len < 0) // old 'String' can't handle these cases
            return;
        _appended_text.append(buf, len);
    }
};

#endif /* SQUID_SRC_TESTS_CAPTURINGSTOREENTRY_H */

