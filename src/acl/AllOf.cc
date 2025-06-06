/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "acl/AllOf.h"
#include "acl/BoolOps.h"
#include "acl/Checklist.h"
#include "cache_cf.h"
#include "MemBuf.h"
#include "sbuf/SBuf.h"
#include "sbuf/Stream.h"

char const *
Acl::AllOf::typeString() const
{
    return "all-of";
}

SBufList
Acl::AllOf::dump() const
{
    return empty() ? SBufList() : nodes.front()->dump();
}

int
Acl::AllOf::doMatch(ACLChecklist *checklist, Nodes::const_iterator start) const
{
    assert(start == nodes.begin()); // we only have one node

    // avoid dereferencing invalid start
    if (empty())
        return 1; // not 0 because in math empty product equals identity

    if (checklist->matchChild(this, start))
        return 1; // match

    return checklist->keepMatching() ? 0 : -1;
}

// called once per "acl name all-of name1 name2 ...." line
void
Acl::AllOf::parse()
{
    Acl::InnerNode *whole = nullptr;
    const auto oldNode = empty() ? nullptr : nodes.front().getRaw();

    // optimization: this logic reduces subtree hight (number of tree levels)
    if (Acl::OrNode *oldWhole = dynamic_cast<Acl::OrNode*>(oldNode)) {
        // this acl saw multiple lines before; add another one to the old node
        whole = oldWhole;
    } else if (oldNode) {
        // this acl saw a single line before; create a new OR inner node

        Acl::OrNode *newWhole = new Acl::OrNode;
        newWhole->context(ToSBuf('(', name, " lines)"), oldNode->cfgline);
        newWhole->add(oldNode); // old (i.e. first) line
        nodes.front() = whole = newWhole;
    } else {
        // this is the first line for this acl; just use it as is
        whole = this;
    }

    assert(whole);
    const int lineId = whole->childrenCount() + 1;

    Acl::AndNode *line = new AndNode;
    line->context(ToSBuf('(', name, " line #", lineId, ')'), config_input_line);
    line->lineParse();

    whole->add(line);
}

