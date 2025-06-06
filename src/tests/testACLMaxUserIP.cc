/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"

#if USE_AUTH

#include "acl/Acl.h"
#include "auth/AclMaxUserIp.h"
#include "auth/UserRequest.h"
#include "compat/cppunit.h"
#include "ConfigParser.h"
#include "SquidConfig.h"
#include "unitTestMain.h"

#include <stdexcept>

/*
 * demonstration test file, as new idioms are made they will
 * be shown in the TestBoilerplate source.
 */

class TestACLMaxUserIP : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(TestACLMaxUserIP);
    /* note the statement here and then the actual prototype below */
    CPPUNIT_TEST(testDefaults);
    CPPUNIT_TEST(testParseLine);
    CPPUNIT_TEST_SUITE_END();

protected:
    void testDefaults();
    void testParseLine();
};
CPPUNIT_TEST_SUITE_REGISTRATION( TestACLMaxUserIP );

/* globals required to resolve link issues */
AnyP::PortCfgPointer HttpPortList;

void
TestACLMaxUserIP::testDefaults()
{
    ACLMaxUserIP anACL("max_user_ip");
    /* 0 is not a valid maximum, so we start at 0 */
    CPPUNIT_ASSERT_EQUAL(0,anACL.getMaximum());
    /* and we have no option to turn strict OFF, so start ON. */
    CPPUNIT_ASSERT_EQUAL(false, static_cast<bool>(anACL.beStrict));
    /* an unparsed acl must not be valid - there is no sane default */
    CPPUNIT_ASSERT_EQUAL(false,anACL.valid());
}

/// customizes our test setup
class MyTestProgram: public TestProgram
{
public:
    /* TestProgram API */
    void startup() override;
};

void
MyTestProgram::startup()
{
    Acl::RegisterMaker("max_user_ip", [](Acl::TypeName name)->Acl::Node* { return new ACLMaxUserIP(name); });
}

void
TestACLMaxUserIP::testParseLine()
{
    /* a config line to pass with a lead-in token to seed the parser. */
    char * line = xstrdup("test max_user_ip -s 1");
    /* seed the parser */
    ConfigParser::SetCfgLine(line);
    ConfigParser LegacyParser;
    Acl::Node::ParseNamedAcl(LegacyParser, Config.namedAcls);
    CPPUNIT_ASSERT(Config.namedAcls);
    const auto anACL = Acl::Node::FindByName(SBuf("test"));
    CPPUNIT_ASSERT(anACL);
    ACLMaxUserIP *maxUserIpACL = dynamic_cast<ACLMaxUserIP *>(anACL);
    CPPUNIT_ASSERT(maxUserIpACL);
    if (maxUserIpACL) {
        /* we want a maximum of one, and strict to be true */
        CPPUNIT_ASSERT_EQUAL(1, maxUserIpACL->getMaximum());
        CPPUNIT_ASSERT_EQUAL(true, static_cast<bool>(maxUserIpACL->beStrict));
        /* the acl must be vaid */
        CPPUNIT_ASSERT_EQUAL(true, maxUserIpACL->valid());
    }
    Acl::FreeNamedAcls(&Config.namedAcls);
    xfree(line);
}

int
main(int argc, char *argv[])
{
    return MyTestProgram().run(argc, argv);
}

#endif /* USE_AUTH */

