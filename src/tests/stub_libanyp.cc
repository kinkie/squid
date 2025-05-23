/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"

#define STUB_API "anyp/libanyp.la"
#include "tests/STUB.h"

#include "anyp/Uri.h"
AnyP::Uri::Uri(AnyP::UriScheme const &) {STUB}
void AnyP::Uri::touch() STUB
bool AnyP::Uri::parse(const HttpRequestMethod&, const SBuf &) STUB_RETVAL(true)
void AnyP::Uri::host(const char *) STUB
static SBuf nil;
const SBuf &AnyP::Uri::path() const STUB_RETVAL(nil)
void AnyP::Uri::addRelativePath(const char *) STUB
const SBuf &AnyP::Uri::SlashPath()
{
    static SBuf slash("/");
    return slash;
}
const SBuf &AnyP::Uri::Asterisk()
{
    static SBuf asterisk("*");
    return asterisk;
}
SBuf &AnyP::Uri::authority(bool) const STUB_RETVAL(nil)
SBuf &AnyP::Uri::absolute() const STUB_RETVAL(nil)
void urlInitialize() STUB
const char *urlCanonicalFakeHttps(const HttpRequest *) STUB_RETVAL(nullptr)
bool urlIsRelative(const char *) STUB_RETVAL(false)
char *urlRInternal(const char *, unsigned short, const char *, const char *) STUB_RETVAL(nullptr)
char *urlInternal(const char *, const char *) STUB_RETVAL(nullptr)
int matchDomainName(const char *, const char *, enum MatchDomainNameFlags) STUB_RETVAL(0)
bool urlCheckRequest(const HttpRequest *) STUB_RETVAL(false)
void urlExtMethodConfigure() STUB

