/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef STATHIST_H_
#define STATHIST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>


class StatHist
{
    public:
    MOCK_METHOD(void, logInit, (unsigned int capacity, double min, double max));
    MOCK_METHOD(void, enumInit, (unsigned int last_enum));
    MOCK_METHOD(void, count, (double val));
    MOCK_METHOD(void, dump, (void *, void *), (const));
}



#endif /* STATHIST_H */