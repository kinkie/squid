/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_LOG_FORWARD_H
#define SQUID_SRC_LOG_FORWARD_H

#include "base/RefCount.h"

class AccessLogEntry;
typedef RefCount<AccessLogEntry> AccessLogEntryPointer;

class Logfile;
class LogTags;
class LogTagsErrors;

#endif /* SQUID_SRC_LOG_FORWARD_H */

