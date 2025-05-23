/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/*
 * Copyright (c) 1988, 1989, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)radix.c     8.4 (Berkeley) 11/2/94
 */

/*
 * DEBUG: section 53    Radix Tree data structure implementation
 */

#include "squid.h"
#include "radix.h"
#include "util.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_GRP_H
#include <grp.h>
#endif
#if HAVE_GNUMALLOC_H
#include <gnumalloc.h>
#elif HAVE_MALLOC_H
#include <malloc.h>
#endif
#if HAVE_MEMORY_H
#include <memory.h>
#endif
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

int squid_max_keylen;
struct squid_radix_mask *squid_rn_mkfreelist;
struct squid_radix_node_head *squid_mask_rnhead;
static char *addmask_key;
static unsigned char normal_chars[] = {0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xFF};
static char *rn_zeros, *rn_ones;

/* aliases */
#define rn_masktop (squid_mask_rnhead->rnh_treetop)
#define rn_dupedkey rn_u.rn_leaf.rn_Dupedkey
#define rn_off rn_u.rn_node.rn_Off
#define rn_l rn_u.rn_node.rn_L
#define rn_r rn_u.rn_node.rn_R
#define rm_mask rm_rmu.rmu_mask
#define rm_leaf rm_rmu.rmu_leaf /* extra field would make 32 bytes */

/* Helper macros */
#define squid_R_Malloc(p, t, n) (p = (t) xmalloc((unsigned int)(n)))
#define squid_Free(p) xfree((char *)p)
#define squid_MKGet(m) {\
    if (squid_rn_mkfreelist) {\
        m = squid_rn_mkfreelist; \
        squid_rn_mkfreelist = (m)->rm_mklist; \
    } else \
        squid_R_Malloc(m, struct squid_radix_mask *, sizeof (*(m)));\
    }

#define squid_MKFree(m) { (m)->rm_mklist = squid_rn_mkfreelist; squid_rn_mkfreelist = (m);}

#ifndef min
#define min(x,y) ((x)<(y)? (x) : (y))
#endif
/*
 * The data structure for the keys is a radix tree with one way
 * branching removed.  The index rn_b at an internal node n represents a bit
 * position to be tested.  The tree is arranged so that all descendants
 * of a node n have keys whose bits all agree up to position rn_b - 1.
 * (We say the index of n is rn_b.)
 *
 * There is at least one descendant which has a one bit at position rn_b,
 * and at least one with a zero there.
 *
 * A route is determined by a pair of key and mask.  We require that the
 * bit-wise logical and of the key and mask to be the key.
 * We define the index of a route to associated with the mask to be
 * the first bit number in the mask where 0 occurs (with bit number 0
 * representing the highest order bit).
 *
 * We say a mask is normal if every bit is 0, past the index of the mask.
 * If a node n has a descendant (k, m) with index(m) == index(n) == rn_b,
 * and m is a normal mask, then the route applies to every descendant of n.
 * If the index(m) < rn_b, this implies the trailing last few bits of k
 * before bit b are all 0, (and hence consequently true of every descendant
 * of n), so the route applies to all descendants of the node as well.
 *
 * Similar logic shows that a non-normal mask m such that
 * index(m) <= index(n) could potentially apply to many children of n.
 * Thus, for each non-host route, we attach its mask to a list at an internal
 * node as high in the tree as we can go.
 *
 * The present version of the code makes use of normal routes in short-
 * circuiting an explicit mask and compare operation when testing whether
 * a key satisfies a normal route, and also in remembering the unique leaf
 * that governs a subtree.
 */

struct squid_radix_node *
squid_rn_search(void *v_arg, struct squid_radix_node *head) {
    register struct squid_radix_node *x;
    register char *v;

    for (x = head, v = v_arg; x->rn_b >= 0;) {
        if (x->rn_bmask & v[x->rn_off])
            x = x->rn_r;
        else
            x = x->rn_l;
    }
    return (x);
}

struct squid_radix_node *
squid_rn_search_m(void *v_arg, struct squid_radix_node *head, void *m_arg) {
    register struct squid_radix_node *x;
    register char *v = v_arg, *m = m_arg;

    for (x = head; x->rn_b >= 0;) {
        if ((x->rn_bmask & m[x->rn_off]) &&
                (x->rn_bmask & v[x->rn_off]))
            x = x->rn_r;
        else
            x = x->rn_l;
    }
    return x;
}

int
squid_rn_refines(void *m_arg, void *n_arg)
{
    register char *m = m_arg, *n = n_arg;
    register char *lim, *lim2 = lim = n + *(u_char *) n;
    int longer = (*(u_char *) n++) - (int) (*(u_char *) m++);
    int masks_are_equal = 1;

    if (longer > 0)
        lim -= longer;
    while (n < lim) {
        if (*n & ~(*m))
            return 0;
        if (*n++ != *m++)
            masks_are_equal = 0;
    }
    while (n < lim2)
        if (*n++)
            return 0;
    if (masks_are_equal && (longer < 0))
        for (lim2 = m - longer; m < lim2;)
            if (*m++)
                return 1;
    return (!masks_are_equal);
}

struct squid_radix_node *
squid_rn_lookup(void *v_arg, void *m_arg, struct squid_radix_node_head *head) {
    register struct squid_radix_node *x;
    char *netmask = NULL;

    if (m_arg) {
        if ((x = squid_rn_addmask(m_arg, 1, head->rnh_treetop->rn_off)) == 0)
            return (0);
        netmask = x->rn_key;
    }
    x = squid_rn_match(v_arg, head);
    if (x && netmask) {
        while (x && x->rn_mask != netmask)
            x = x->rn_dupedkey;
    }
    return x;
}

static int
rn_satsifies_leaf(char *trial, register struct squid_radix_node *leaf, int skip)
{
    register char *cp = trial, *cp2 = leaf->rn_key, *cp3 = leaf->rn_mask;
    char *cplim;
    int length = min(*(u_char *) cp, *(u_char *) cp2);

    if (cp3 == 0)
        cp3 = rn_ones;
    else
        length = min(length, *(u_char *) cp3);
    cplim = cp + length;
    cp3 += skip;
    cp2 += skip;
    for (cp += skip; cp < cplim; cp++, cp2++, cp3++)
        if ((*cp ^ *cp2) & *cp3)
            return 0;
    return 1;
}

struct squid_radix_node *
squid_rn_match(void *v_arg, struct squid_radix_node_head *head) {
    char *v = v_arg;
    register struct squid_radix_node *t = head->rnh_treetop, *x;
    register char *cp = v, *cp2;
    char *cplim;
    struct squid_radix_node *saved_t, *top = t;
    int off = t->rn_off, vlen = *(u_char *) cp, matched_off;
    register int test, b, rn_b;

    /*
     * Open code squid_rn_search(v, top) to avoid overhead of extra
     * subroutine call.
     */
    for (; t->rn_b >= 0;) {
        if (t->rn_bmask & cp[t->rn_off])
            t = t->rn_r;
        else
            t = t->rn_l;
    }
    /*
     * See if we match exactly as a host destination
     * or at least learn how many bits match, for normal mask finesse.
     *
     * It doesn't hurt us to limit how many bytes to check
     * to the length of the mask, since if it matches we had a genuine
     * match and the leaf we have is the most specific one anyway;
     * if it didn't match with a shorter length it would fail
     * with a long one.  This wins big for class B&C netmasks which
     * are probably the most common case...
     */
    if (t->rn_mask)
        vlen = *(u_char *) t->rn_mask;
    cp += off;
    cp2 = t->rn_key + off;
    cplim = v + vlen;
    for (; cp < cplim; cp++, cp2++)
        if (*cp != *cp2)
            goto on1;
    /*
     * This extra grot is in case we are explicitly asked
     * to look up the default.  Ugh!
     */
    if ((t->rn_flags & RNF_ROOT) && t->rn_dupedkey)
        t = t->rn_dupedkey;
    return t;
on1:
    test = (*cp ^ *cp2) & 0xff; /* find first bit that differs */
    for (b = 7; (test >>= 1) > 0;)
        b--;
    matched_off = cp - v;
    b += matched_off << 3;
    rn_b = -1 - b;
    /*
     * If there is a host route in a duped-key chain, it will be first.
     */
    if ((saved_t = t)->rn_mask == 0)
        t = t->rn_dupedkey;
    for (; t; t = t->rn_dupedkey)
        /*
         * Even if we don't match exactly as a host,
         * we may match if the leaf we wound up at is
         * a route to a net.
         */
        if (t->rn_flags & RNF_NORMAL) {
            if (rn_b <= t->rn_b)
                return t;
        } else if (rn_satsifies_leaf(v, t, matched_off))
            return t;
    t = saved_t;
    /* start searching up the tree */
    do {
        register struct squid_radix_mask *m;
        t = t->rn_p;
        if ((m = t->rn_mklist)) {
            /*
             * If non-contiguous masks ever become important
             * we can restore the masking and open coding of
             * the search and satisfaction test and put the
             * calculation of "off" back before the "do".
             */
            do {
                if (m->rm_flags & RNF_NORMAL) {
                    if (rn_b <= m->rm_b)
                        return (m->rm_leaf);
                } else {
                    off = min(t->rn_off, matched_off);
                    x = squid_rn_search_m(v, t, m->rm_mask);
                    while (x && x->rn_mask != m->rm_mask)
                        x = x->rn_dupedkey;
                    if (x && rn_satsifies_leaf(v, x, off))
                        return x;
                }
            } while ((m = m->rm_mklist));
        }
    } while (t != top);
    return 0;
}

struct squid_radix_node *
squid_rn_newpair(void *v, int b, struct squid_radix_node nodes[2]) {
    register struct squid_radix_node *tt = nodes, *t = tt + 1;
    t->rn_b = b;
    t->rn_bmask = 0x80 >> (b & 7);
    t->rn_l = tt;
    t->rn_off = b >> 3;
    tt->rn_b = -1;
    tt->rn_key = (char *) v;
    tt->rn_p = t;
    tt->rn_flags = t->rn_flags = RNF_ACTIVE;
    return t;
}

struct squid_radix_node *
squid_rn_insert(void *v_arg, struct squid_radix_node_head *head, int *dupentry, struct squid_radix_node nodes[2]) {
    char *v = v_arg;
    struct squid_radix_node *top = head->rnh_treetop;
    int head_off = top->rn_off, vlen = (int) *((u_char *) v);
    register struct squid_radix_node *t = squid_rn_search(v_arg, top);
    register char *cp = v + head_off;
    register int b;
    struct squid_radix_node *tt;
    /*
     * Find first bit at which v and t->rn_key differ
     */
    {
        register char *cp2 = t->rn_key + head_off;
        register int cmp_res;
        char *cplim = v + vlen;

        while (cp < cplim)
            if (*cp2++ != *cp++)
                goto on1;
        *dupentry = 1;
        return t;
on1:
        *dupentry = 0;
        cmp_res = (cp[-1] ^ cp2[-1]) & 0xff;
        for (b = (cp - v) << 3; cmp_res; b--)
            cmp_res >>= 1;
    }
    {
        register struct squid_radix_node *p, *x = top;
        cp = v;
        do {
            p = x;
            if (cp[x->rn_off] & x->rn_bmask)
                x = x->rn_r;
            else
                x = x->rn_l;
        } while (b > (unsigned) x->rn_b);   /* x->rn_b < b && x->rn_b >= 0 */
        t = squid_rn_newpair(v_arg, b, nodes);
        tt = t->rn_l;
        if ((cp[p->rn_off] & p->rn_bmask) == 0)
            p->rn_l = t;
        else
            p->rn_r = t;
        x->rn_p = t;
        t->rn_p = p;        /* frees x, p as temp vars below */
        if ((cp[t->rn_off] & t->rn_bmask) == 0) {
            t->rn_r = x;
        } else {
            t->rn_r = tt;
            t->rn_l = x;
        }
    }
    return (tt);
}

struct squid_radix_node *
squid_rn_addmask(void *n_arg, int search, int skip) {
    char *netmask = (char *) n_arg;
    register struct squid_radix_node *x;
    register char *cp, *cplim;
    register int b = 0, mlen, j;
    int maskduplicated, m0, isnormal;
    struct squid_radix_node *saved_x;
    static int last_zeroed = 0;

    if ((mlen = *(u_char *) netmask) > squid_max_keylen)
        mlen = squid_max_keylen;
    if (skip == 0)
        skip = 1;
    if (mlen <= skip)
        return (squid_mask_rnhead->rnh_nodes);
    if (skip > 1)
        memcpy(addmask_key + 1, rn_ones + 1, skip - 1);
    if ((m0 = mlen) > skip)
        memcpy(addmask_key + skip, netmask + skip, mlen - skip);
    /*
     * Trim trailing zeroes.
     */
    for (cp = addmask_key + mlen; (cp > addmask_key) && cp[-1] == 0;)
        cp--;
    mlen = cp - addmask_key;
    if (mlen <= skip) {
        if (m0 >= last_zeroed)
            last_zeroed = mlen;
        return (squid_mask_rnhead->rnh_nodes);
    }
    if (m0 < last_zeroed)
        memset(addmask_key + m0, '\0', last_zeroed - m0);
    *addmask_key = last_zeroed = mlen;
    x = squid_rn_search(addmask_key, rn_masktop);
    if (memcmp(addmask_key, x->rn_key, mlen) != 0)
        x = 0;
    if (x || search)
        return (x);
    squid_R_Malloc(x, struct squid_radix_node *, squid_max_keylen + 2 * sizeof(*x));
    if ((saved_x = x) == 0)
        return (0);
    memset(x, '\0', squid_max_keylen + 2 * sizeof(*x));
    netmask = cp = (char *) (x + 2);
    memcpy(cp, addmask_key, mlen);
    x = squid_rn_insert(cp, squid_mask_rnhead, &maskduplicated, x);
    if (maskduplicated) {
        fprintf(stderr, "squid_rn_addmask: mask impossibly already in tree");
        squid_Free(saved_x);
        return (x);
    }
    /*
     * Calculate index of mask, and check for normalcy.
     */
    cplim = netmask + mlen;
    isnormal = 1;
    for (cp = netmask + skip; (cp < cplim) && *(u_char *) cp == 0xff;)
        cp++;
    if (cp != cplim) {
        for (j = 0x80; (j & *cp) != 0; j >>= 1)
            b++;
        if (*cp != normal_chars[b] || cp != (cplim - 1))
            isnormal = 0;
    }
    b += (cp - netmask) << 3;
    x->rn_b = -1 - b;
    if (isnormal)
        x->rn_flags |= RNF_NORMAL;
    return (x);
}

static int          /* XXX: arbitrary ordering for non-contiguous masks */
rn_lexobetter(void *m_arg, void *n_arg)
{
    register u_char *mp = m_arg, *np = n_arg, *lim;

    if (*mp > *np)
        return 1;       /* not really, but need to check longer one first */
    if (*mp == *np)
        for (lim = mp + *mp; mp < lim;)
            if (*mp++ > *np++)
                return 1;
    return 0;
}

static struct squid_radix_mask *
rn_new_radix_mask(struct squid_radix_node *tt, struct squid_radix_mask *next) {
    register struct squid_radix_mask *m;

    squid_MKGet(m);
    if (m == 0) {
        fprintf(stderr, "Mask for route not entered\n");
        return (0);
    }
    memset(m, '\0', sizeof *m);
    m->rm_b = tt->rn_b;
    m->rm_flags = tt->rn_flags;
    if (tt->rn_flags & RNF_NORMAL)
        m->rm_leaf = tt;
    else
        m->rm_mask = tt->rn_mask;
    m->rm_mklist = next;
    tt->rn_mklist = m;
    return m;
}

struct squid_radix_node *
squid_rn_addroute(void *v_arg, void *n_arg, struct squid_radix_node_head *head, struct squid_radix_node treenodes[2]) {
    char *v = (char *) v_arg, *netmask = (char *) n_arg;
    register struct squid_radix_node *t, *x = NULL, *tt;
    struct squid_radix_node *saved_tt, *top = head->rnh_treetop;
    short b = 0, b_leaf = 0;
    int keyduplicated;
    char *mmask;
    struct squid_radix_mask *m, **mp;

    /*
     * In dealing with non-contiguous masks, there may be
     * many different routes which have the same mask.
     * We will find it useful to have a unique pointer to
     * the mask to speed avoiding duplicate references at
     * nodes and possibly save time in calculating indices.
     */
    if (netmask) {
        if ((x = squid_rn_addmask(netmask, 0, top->rn_off)) == 0)
            return (0);
        b_leaf = x->rn_b;
        b = -1 - x->rn_b;
        netmask = x->rn_key;
    }
    /*
     * Deal with duplicated keys: attach node to previous instance
     */
    saved_tt = tt = squid_rn_insert(v, head, &keyduplicated, treenodes);
    if (keyduplicated) {
        for (t = tt; tt; t = tt, tt = tt->rn_dupedkey) {
            if (tt->rn_mask == netmask)
                return (0);
            if (netmask == 0 ||
                    (tt->rn_mask &&
                     ((b_leaf < tt->rn_b) ||    /* index(netmask) > node */
                      squid_rn_refines(netmask, tt->rn_mask) ||
                      rn_lexobetter(netmask, tt->rn_mask))))
                break;
        }
        /*
         * If the mask is not duplicated, we wouldn't
         * find it among possible duplicate key entries
         * anyway, so the above test doesn't hurt.
         *
         * We sort the masks for a duplicated key the same way as
         * in a masklist -- most specific to least specific.
         * This may require the unfortunate nuisance of relocating
         * the head of the list.
         */
        if (tt == saved_tt) {
            struct squid_radix_node *xx = x;
            /* link in at head of list */
            tt = treenodes;
            tt->rn_dupedkey = t;
            tt->rn_flags = t->rn_flags;
            tt->rn_p = x = t->rn_p;
            if (x->rn_l == t)
                x->rn_l = tt;
            else
                x->rn_r = tt;
            saved_tt = tt;
            x = xx;
        } else {
            tt = treenodes;
            tt->rn_dupedkey = t->rn_dupedkey;
            t->rn_dupedkey = tt;
        }
        tt->rn_key = (char *) v;
        tt->rn_b = -1;
        tt->rn_flags = RNF_ACTIVE;
    }
    /*
     * Put mask in tree.
     */
    if (netmask) {
        tt->rn_mask = netmask;
        tt->rn_b = x->rn_b;
        tt->rn_flags |= x->rn_flags & RNF_NORMAL;
    }
    t = saved_tt->rn_p;
    if (keyduplicated)
        goto on2;
    b_leaf = -1 - t->rn_b;
    if (t->rn_r == saved_tt)
        x = t->rn_l;
    else
        x = t->rn_r;
    /* Promote general routes from below */
    if (x->rn_b < 0) {
        for (mp = &t->rn_mklist; x; x = x->rn_dupedkey)
            if (x->rn_mask && (x->rn_b >= b_leaf) && x->rn_mklist == 0) {
                if ((*mp = m = rn_new_radix_mask(x, 0)))
                    mp = &m->rm_mklist;
            }
    } else if (x->rn_mklist) {
        /*
         * Skip over masks whose index is > that of new node
         */
        for (mp = &x->rn_mklist; (m = *mp); mp = &m->rm_mklist)
            if (m->rm_b >= b_leaf)
                break;
        t->rn_mklist = m;
        *mp = 0;
    }
on2:
    /* Add new route to highest possible ancestor's list */
    if ((netmask == 0) || (b > t->rn_b))
        return tt;      /* can't lift at all */
    b_leaf = tt->rn_b;
    do {
        x = t;
        t = t->rn_p;
    } while (b <= t->rn_b && x != top);
    /*
     * Search through routes associated with node to
     * insert new route according to index.
     * Need same criteria as when sorting dupedkeys to avoid
     * double loop on deletion.
     */
    for (mp = &x->rn_mklist; (m = *mp); mp = &m->rm_mklist) {
        if (m->rm_b < b_leaf)
            continue;
        if (m->rm_b > b_leaf)
            break;
        if (m->rm_flags & RNF_NORMAL) {
            mmask = m->rm_leaf->rn_mask;
            if (tt->rn_flags & RNF_NORMAL) {
                fprintf(stderr,
                        "Non-unique normal route, mask not entered");
                return tt;
            }
        } else
            mmask = m->rm_mask;
        if (mmask == netmask) {
            m->rm_refs++;
            tt->rn_mklist = m;
            return tt;
        }
        if (squid_rn_refines(netmask, mmask) || rn_lexobetter(netmask, mmask))
            break;
    }
    *mp = rn_new_radix_mask(tt, *mp);
    return tt;
}

struct squid_radix_node *
squid_rn_delete(void *v_arg, void *netmask_arg, struct squid_radix_node_head *head) {
    register struct squid_radix_node *t, *p, *x, *tt;
    struct squid_radix_mask *m, *saved_m, **mp;
    struct squid_radix_node *dupedkey, *saved_tt, *top;
    char *v, *netmask;
    int b, head_off, vlen;

    v = v_arg;
    netmask = netmask_arg;
    x = head->rnh_treetop;
    tt = squid_rn_search(v, x);
    head_off = x->rn_off;
    vlen = *(u_char *) v;
    saved_tt = tt;
    top = x;
    if (tt == 0 ||
            memcmp(v + head_off, tt->rn_key + head_off, vlen - head_off))
        return (0);
    /*
     * Delete our route from mask lists.
     */
    if (netmask) {
        if ((x = squid_rn_addmask(netmask, 1, head_off)) == 0)
            return (0);
        netmask = x->rn_key;
        while (tt->rn_mask != netmask)
            if ((tt = tt->rn_dupedkey) == 0)
                return (0);
    }
    if (tt->rn_mask == 0 || (saved_m = m = tt->rn_mklist) == 0)
        goto on1;
    if (tt->rn_flags & RNF_NORMAL) {
        if (m->rm_leaf != tt || m->rm_refs > 0) {
            fprintf(stderr, "squid_rn_delete: inconsistent annotation\n");
            return 0;       /* dangling ref could cause disaster */
        }
    } else {
        if (m->rm_mask != tt->rn_mask) {
            fprintf(stderr, "squid_rn_delete: inconsistent annotation\n");
            goto on1;
        }
        if (--m->rm_refs >= 0)
            goto on1;
    }
    b = -1 - tt->rn_b;
    t = saved_tt->rn_p;
    if (b > t->rn_b)
        goto on1;       /* Wasn't lifted at all */
    do {
        x = t;
        t = t->rn_p;
    } while (b <= t->rn_b && x != top);
    for (mp = &x->rn_mklist; (m = *mp); mp = &m->rm_mklist)
        if (m == saved_m) {
            *mp = m->rm_mklist;
            squid_MKFree(m);
            break;
        }
    if (m == 0) {
        fprintf(stderr, "squid_rn_delete: couldn't find our annotation\n");
        if (tt->rn_flags & RNF_NORMAL)
            return (0);     /* Dangling ref to us */
    }
on1:
    /*
     * Eliminate us from tree
     */
    if (tt->rn_flags & RNF_ROOT)
        return (0);
    t = tt->rn_p;
    if ((dupedkey = saved_tt->rn_dupedkey)) {
        if (tt == saved_tt) {
            x = dupedkey;
            x->rn_p = t;
            if (t->rn_l == tt)
                t->rn_l = x;
            else
                t->rn_r = x;
        } else {
            for (x = p = saved_tt; p && p->rn_dupedkey != tt;)
                p = p->rn_dupedkey;
            if (p)
                p->rn_dupedkey = tt->rn_dupedkey;
            else
                fprintf(stderr, "squid_rn_delete: couldn't find us\n");
        }
        t = tt + 1;
        if (t->rn_flags & RNF_ACTIVE) {
            *++x = *t;
            p = t->rn_p;
            if (p->rn_l == t)
                p->rn_l = x;
            else
                p->rn_r = x;
            x->rn_l->rn_p = x;
            x->rn_r->rn_p = x;
        }
        goto out;
    }
    if (t->rn_l == tt)
        x = t->rn_r;
    else
        x = t->rn_l;
    p = t->rn_p;
    if (p->rn_r == t)
        p->rn_r = x;
    else
        p->rn_l = x;
    x->rn_p = p;
    /*
     * Demote routes attached to us.
     */
    if (t->rn_mklist) {
        if (x->rn_b >= 0) {
            for (mp = &x->rn_mklist; (m = *mp);)
                mp = &m->rm_mklist;
            *mp = t->rn_mklist;
        } else {
            /* If there are any key,mask pairs in a sibling
             * duped-key chain, some subset will appear sorted
             * in the same order attached to our mklist */
            for (m = t->rn_mklist; m && x; x = x->rn_dupedkey)
                if (m == x->rn_mklist) {
                    struct squid_radix_mask *mm = m->rm_mklist;
                    x->rn_mklist = 0;
                    if (--(m->rm_refs) < 0)
                        squid_MKFree(m);
                    m = mm;
                }
            assert(m == NULL);
        }
    }
    /*
     * We may be holding an active internal node in the tree.
     */
    x = tt + 1;
    if (t != x) {
        *t = *x;
        t->rn_l->rn_p = t;
        t->rn_r->rn_p = t;
        p = x->rn_p;
        if (p->rn_l == x)
            p->rn_l = t;
        else
            p->rn_r = t;
    }
out:
    tt->rn_flags &= ~RNF_ACTIVE;
    tt[1].rn_flags &= ~RNF_ACTIVE;
    return (tt);
}

int
squid_rn_walktree(struct squid_radix_node_head *h, int (*f) (struct squid_radix_node *, void *), void *w)
{
    int error;
    struct squid_radix_node *base, *next;
    register struct squid_radix_node *rn = h->rnh_treetop;
    /*
     * This gets complicated because we may delete the node
     * while applying the function f to it, so we need to calculate
     * the successor node in advance.
     */
    /* First time through node, go left */
    while (rn->rn_b >= 0)
        rn = rn->rn_l;
    for (;;) {
        base = rn;
        /* If at right child go back up, otherwise, go right */
        while (rn->rn_p->rn_r == rn && (rn->rn_flags & RNF_ROOT) == 0)
            rn = rn->rn_p;
        /* Find the next *leaf* since next node might vanish, too */
        for (rn = rn->rn_p->rn_r; rn->rn_b >= 0;)
            rn = rn->rn_l;
        next = rn;
        /* Process leaves */
        while ((rn = base)) {
            base = rn->rn_dupedkey;
            if (!(rn->rn_flags & RNF_ROOT) && (error = (*f) (rn, w)))
                return (error);
        }
        rn = next;
        if (rn->rn_flags & RNF_ROOT)
            return (0);
    }
    /* NOTREACHED */
}

int
squid_rn_inithead(struct squid_radix_node_head **head, int off)
{
    register struct squid_radix_node_head *rnh;
    register struct squid_radix_node *t, *tt, *ttt;
    if (*head)
        return (1);
    squid_R_Malloc(rnh, struct squid_radix_node_head *, sizeof(*rnh));
    if (rnh == 0)
        return (0);
    memset(rnh, '\0', sizeof(*rnh));
    *head = rnh;
    t = squid_rn_newpair(rn_zeros, off, rnh->rnh_nodes);
    ttt = rnh->rnh_nodes + 2;
    t->rn_r = ttt;
    t->rn_p = t;
    tt = t->rn_l;
    tt->rn_flags = t->rn_flags = RNF_ROOT | RNF_ACTIVE;
    tt->rn_b = -1 - off;
    *ttt = *tt;
    ttt->rn_key = rn_ones;
    rnh->rnh_addaddr = squid_rn_addroute;
    rnh->rnh_deladdr = squid_rn_delete;
    rnh->rnh_matchaddr = squid_rn_match;
    rnh->rnh_lookup = squid_rn_lookup;
    rnh->rnh_walktree = squid_rn_walktree;
    rnh->rnh_treetop = t;
    return (1);
}

void
squid_rn_init(void)
{
    char *cp, *cplim;
#ifdef KERNEL
    struct domain *dom;

    for (dom = domains; dom; dom = dom->dom_next)
        if (dom->dom_maxrtkey > squid_max_keylen)
            squid_max_keylen = dom->dom_maxrtkey;
#endif
    if (squid_max_keylen == 0) {
        fprintf(stderr,
                "squid_rn_init: radix functions require squid_max_keylen be set\n");
        return;
    }
    squid_R_Malloc(rn_zeros, char *, 3 * squid_max_keylen);
    if (rn_zeros == NULL) {
        fprintf(stderr, "squid_rn_init failed.\n");
        exit(-1);
    }
    memset(rn_zeros, '\0', 3 * squid_max_keylen);
    rn_ones = cp = rn_zeros + squid_max_keylen;
    addmask_key = cplim = rn_ones + squid_max_keylen;
    while (cp < cplim)
        *cp++ = -1;
    if (squid_rn_inithead(&squid_mask_rnhead, 0) == 0) {
        fprintf(stderr, "rn_init2 failed.\n");
        exit(-1);
    }
}

