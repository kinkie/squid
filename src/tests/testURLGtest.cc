/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "mockDebug.h"
#include "mockStatHist.h"
#include "anyp/Uri.h"
#include "tests/testURL.h"

#include <gtest/gtest.h>
#include <sstream>

namespace {

/* init memory pools */


TEST(testURL, ConstructScheme)
{
    AnyP::UriScheme empty_scheme;
    AnyP::Uri protoless_url(AnyP::PROTO_NONE);
    ASSERT_EQ(empty_scheme, protoless_url.getScheme());

    AnyP::UriScheme ftp_scheme(AnyP::PROTO_FTP);
    AnyP::Uri ftp_url(AnyP::PROTO_FTP);
    ASSERT_EQ(ftp_scheme, ftp_url.getScheme());
}


TEST(testURL, DefaultConstructor)
{
    AnyP::UriScheme aScheme;
    AnyP::Uri aUrl;
    ASSERT_EQ(aScheme, aUrl.getScheme());

    auto *urlPointer = new AnyP::Uri;
    ASSERT_NE(urlPointer, nullptr);
    delete urlPointer;
}

} // namespace

int
main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    Mem::Init();
    AnyP::UriScheme::Init();
    return RUN_ALL_TESTS();
}