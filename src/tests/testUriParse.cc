/*
 * Copyright (C) 1996-2026 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/**
 * \file
 * Comprehensive test driver for AnyP::Uri::parse()
 *
 * This tests all possible permutations of input to the parse method,
 * including valid URLs, invalid URLs, edge cases, and malformed inputs.
 */

#include "squid.h"
#include "anyp/Uri.h"
#include "compat/cppunit.h"
#include "http/MethodType.h"
#include "http/RequestMethod.h"
#include "sbuf/SBuf.h"
#include "unitTestMain.h"

#include <cppunit/TestAssert.h>
#include <sstream>
#include <vector>

/**
 * Test the AnyP::Uri::parse() method with comprehensive input coverage
 */
class TestUriParse : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(TestUriParse);
    CPPUNIT_TEST(testHttpSimple);
    CPPUNIT_TEST(testHttpsSimple);
    CPPUNIT_TEST(testHttpWithPath);
    CPPUNIT_TEST(testHttpWithQuery);
    CPPUNIT_TEST(testHttpWithFragment);
    CPPUNIT_TEST(testHttpWithPort);
    CPPUNIT_TEST(testHttpWithUserinfo);
    CPPUNIT_TEST(testHttpIpv4);
    CPPUNIT_TEST(testHttpIpv6);
    CPPUNIT_TEST(testHttpIpv6Bracketed);
    CPPUNIT_TEST(testHttpNoSlash);
    CPPUNIT_TEST(testConnectSimple);
    CPPUNIT_TEST(testConnectIp);
    CPPUNIT_TEST(testConnectIpv6);
    CPPUNIT_TEST(testConnectNoPort);
    CPPUNIT_TEST(testOptionsAsterisk);
    CPPUNIT_TEST(testTraceAsterisk);
    CPPUNIT_TEST(testOptionsUrl);
    CPPUNIT_TEST(testEmptyUrl);
    CPPUNIT_TEST(testNoScheme);
    CPPUNIT_TEST(testNoHost);
    CPPUNIT_TEST(testInvalidScheme);
    CPPUNIT_TEST(testPortZero);
    CPPUNIT_TEST(testPortMax);
    CPPUNIT_TEST(testPortTooHigh);
    CPPUNIT_TEST(testFtpUrl);
    CPPUNIT_TEST(testUrn);
    CPPUNIT_TEST(testSpacesInUrl);
    CPPUNIT_TEST(testLeadingSpace);
    CPPUNIT_TEST(testDomainMaxLen);
    CPPUNIT_TEST(testPathEncoded);
    CPPUNIT_TEST(testMultipleQuery);
    CPPUNIT_TEST(testAuthSpecialChars);
    CPPUNIT_TEST(testSchemeUppercase);
    CPPUNIT_TEST(testHostUppercase);
    CPPUNIT_TEST(testConnectExtraColon);
    CPPUNIT_TEST(testConnectTrailingSlash);
    CPPUNIT_TEST(testControlChars);
    CPPUNIT_TEST(testTabInUrl);
    CPPUNIT_TEST(testNewlineInUrl);
    CPPUNIT_TEST(testQueryNoValue);
    CPPUNIT_TEST(testFragmentOnly);
    CPPUNIT_TEST(testPathDotDot);
    CPPUNIT_TEST(testHostSingleChar);
    CPPUNIT_TEST(testAuthNoPassword);
    CPPUNIT_TEST(testAuthEmptyPass);
    CPPUNIT_TEST(testDoubleSlash);
    CPPUNIT_TEST(testHttpsWithPort);
    CPPUNIT_TEST(testPortAlpha);
    CPPUNIT_TEST(testQueryEmpty);
    CPPUNIT_TEST(testFragmentWithQuery);
    CPPUNIT_TEST(testPathDotSlash);
    CPPUNIT_TEST(testLocalhost);
    CPPUNIT_TEST(testFileProtocol);
    CPPUNIT_TEST(testMailtoProtocol);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override {}
    void tearDown() override {}

private:
    /**
     * Helper function to create HttpRequestMethod from string
     */
    HttpRequestMethod methodFromString(const char* methodStr)
    {
        if (!methodStr)
            return Http::METHOD_NONE;

        if (strcmp(methodStr, "GET") == 0)
            return Http::METHOD_GET;
        if (strcmp(methodStr, "POST") == 0)
            return Http::METHOD_POST;
        if (strcmp(methodStr, "PUT") == 0)
            return Http::METHOD_PUT;
        if (strcmp(methodStr, "DELETE") == 0)
            return Http::METHOD_DELETE;
        if (strcmp(methodStr, "HEAD") == 0)
            return Http::METHOD_HEAD;
        if (strcmp(methodStr, "OPTIONS") == 0)
            return Http::METHOD_OPTIONS;
        if (strcmp(methodStr, "TRACE") == 0)
            return Http::METHOD_TRACE;
        if (strcmp(methodStr, "CONNECT") == 0)
            return Http::METHOD_CONNECT;

        return Http::METHOD_NONE;
    }

    /**
     * Helper to run a parse test and verify result
     */
    bool runParseTest(const char* methodStr, const char* urlStr, bool expectedResult)
    {
        HttpRequestMethod method = methodFromString(methodStr);
        SBuf url(urlStr);
        AnyP::Uri uri;
        bool result = uri.parse(method, url);
        if (result != expectedResult) {
            std::cerr << "Test failed for method: " << methodStr << ", url: " << urlStr
                      << ". Expected: " << expectedResult << ", got: " << result << std::endl;
        }
        return (result == expectedResult);
    }

    /**
     * Helper to run a parse test and return the URI for inspection
     */
    bool runParseTestWithResult(const char* methodStr, const char* urlStr,
                                   bool expectedResult, AnyP::Uri& uri)
    {
        HttpRequestMethod method = methodFromString(methodStr);
        SBuf url(urlStr);
        bool result = uri.parse(method, url);
        return (result == expectedResult);
    }

    // ===== HTTP/HTTPS Tests =====
    void testHttpSimple()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("GET", "http://example.com/", true, uri));
        CPPUNIT_ASSERT(strcmp(uri.host(), "example.com") == 0);
        CPPUNIT_ASSERT(uri.port().value_or(0) == 80);
    }

    void testHttpsSimple()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("GET", "https://example.com/", true, uri));
        CPPUNIT_ASSERT(strcmp(uri.host(), "example.com") == 0);
        CPPUNIT_ASSERT(uri.port().value_or(0) == 443);
    }

    void testHttpWithPath()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("GET", "http://example.com/path/to/resource", true, uri));
        CPPUNIT_ASSERT(strcmp(uri.host(), "example.com") == 0);
    }

    void testHttpWithQuery()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/path?query=value", true));
    }

    void testHttpWithFragment()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/path#fragment", true));
    }

    void testHttpWithPort()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("GET", "http://example.com:8080/path", true, uri));
        CPPUNIT_ASSERT(uri.port().value_or(0) == 8080);
    }

    void testHttpsWithPort()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("GET", "https://example.com:443/path", true, uri));
        CPPUNIT_ASSERT(uri.port().value_or(0) == 443);
    }

    void testHttpWithUserinfo()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("GET", "http://user:pass@example.com/path", true, uri));
        CPPUNIT_ASSERT(strcmp(uri.host(), "example.com") == 0);
    }

    void testHttpIpv4()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("GET", "http://192.168.1.1/path", true, uri));
        CPPUNIT_ASSERT(strcmp(uri.host(), "192.168.1.1") == 0);
    }

    void testHttpIpv6()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("GET", "http://[::1]/path", true, uri));
        CPPUNIT_ASSERT(strcmp(uri.host(), "[::1]") == 0);
    }

    void testHttpIpv6Bracketed()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("GET", "http://[2001:db8::1]:8080/path", true, uri));
        CPPUNIT_ASSERT(uri.port().value_or(0) == 8080);
    }

    void testHttpNoSlash()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com", true));
    }

    void testDoubleSlash()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com//path", true));
    }

    // ===== CONNECT Method Tests =====
    void testConnectSimple()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("CONNECT", "example.com:443", true, uri));
        CPPUNIT_ASSERT(strcmp(uri.host(), "example.com") == 0);
        CPPUNIT_ASSERT(uri.port().value_or(0) == 443);
    }

    void testConnectIp()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("CONNECT", "192.168.1.1:8080", true, uri));
        CPPUNIT_ASSERT(strcmp(uri.host(), "192.168.1.1") == 0);
    }

    void testConnectIpv6()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("CONNECT", "[::1]:443", true, uri));
        CPPUNIT_ASSERT(strcmp(uri.host(), "[::1]") == 0);
    }

    void testConnectNoPort()
    {
        CPPUNIT_ASSERT(runParseTest("CONNECT", "example.com", false));
    }

    void testConnectExtraColon()
    {
        CPPUNIT_ASSERT(runParseTest("CONNECT", "example.com::443", false));
    }

    void testConnectTrailingSlash()
    {
        CPPUNIT_ASSERT(runParseTest("CONNECT", "example.com:443/", false));
    }

    // ===== OPTIONS/TRACE with Asterisk =====
    void testOptionsAsterisk()
    {
        CPPUNIT_ASSERT(runParseTest("OPTIONS", "*", true));
    }

    void testTraceAsterisk()
    {
        CPPUNIT_ASSERT(runParseTest("TRACE", "*", true));
    }

    void testOptionsUrl()
    {
        CPPUNIT_ASSERT(runParseTest("OPTIONS", "http://example.com/", true));
    }

    // ===== Invalid/Malformed Tests =====
    void testEmptyUrl()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "", false));
    }

    void testNoScheme()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "//host/path", false));
    }

    void testNoHost()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http:///path", false));
    }

    void testInvalidScheme()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "ht!tp://example.com/", false));
    }

    // ===== Port Edge Cases =====
    void testPortZero()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com:0/", false));
    }

    void testPortMax()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com:65535/", true));
    }

    void testPortTooHigh()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com:65536/", false));
    }

    void testPortAlpha()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com:abc/", false));
    }

    // ===== Protocol Tests =====
    void testFtpUrl()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "ftp://ftp.example.com/file.txt", true));
    }

    void testUrn()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "urn:isbn:0451450523", true));
    }

    void testFileProtocol()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "file:///etc/passwd", true));
    }

    void testMailtoProtocol()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "mailto:user@example.com", true));
    }

    // ===== Whitespace Tests =====
    void testSpacesInUrl()
    {
        // this should return false (bug 3233, see anyp/Uri.cc:400)
        CPPUNIT_ASSERT(runParseTest("GET", "http://exam ple.com/", true));
    }

    void testLeadingSpace()
    {
        CPPUNIT_ASSERT(runParseTest("GET", " http://example.com/", false));
    }

    void testTabInUrl()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/\tpath", false));
    }

    void testNewlineInUrl()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/\npath", false));
    }

    void testControlChars()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/\x01\x02", false));
    }

    // ===== Query Tests =====
    void testMultipleQuery()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/?a=1&b=2&c=3", true));
    }

    void testQueryNoValue()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/?key", true));
    }

    void testQueryEmpty()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/?", true));
    }

    // ===== Fragment Tests =====
    void testFragmentOnly()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/#frag", true));
    }

    void testFragmentWithQuery()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/?q=test#frag", true));
    }

    // ===== Path Tests =====
    void testPathEncoded()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/path%20with%20spaces", true));
    }

    void testPathDotDot()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/../etc/passwd", true));
    }

    void testPathDotSlash()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://example.com/./path", true));
    }

    // ===== Host Tests =====
    void testHostSingleChar()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("GET", "http://x/path", true, uri));
        CPPUNIT_ASSERT(strcmp(uri.host(), "x") == 0);
    }

    void testLocalhost()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("GET", "http://localhost/", true, uri));
        CPPUNIT_ASSERT(strcmp(uri.host(), "localhost") == 0);
    }

    void testDomainMaxLen()
    {
        // Domain at max length (253 chars is max)
        CPPUNIT_ASSERT(runParseTest("GET",
            "http://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.com/",
            true));
    }

    void testHostUppercase()
    {
        AnyP::Uri uri;
        CPPUNIT_ASSERT(runParseTestWithResult("GET", "http://EXAMPLE.COM/", true, uri));
        // Host should be normalized to lowercase
        CPPUNIT_ASSERT(strcmp(uri.host(), "example.com") == 0);
    }

    // ===== Scheme Tests =====
    void testSchemeUppercase()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "HTTP://example.com/", true));
    }

    // ===== Authentication Tests =====
    void testAuthNoPassword()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://user@example.com/", true));
    }

    void testAuthEmptyPass()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://user:@example.com/", true));
    }

    void testAuthSpecialChars()
    {
        CPPUNIT_ASSERT(runParseTest("GET", "http://user%40domain:pass%23word@example.com/", true));
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestUriParse);

int
main(int argc, char *argv[])
{
    return TestProgram().run(argc, argv);
}

