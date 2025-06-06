/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_STORE_SWAPMETAOUT_H
#define SQUID_SRC_STORE_SWAPMETAOUT_H

#include "base/HardFun.h"
#include "base/ToCpp.h"
#include "store/forward.h"

#include <memory>

/// C++ wrapper for the legacy xmalloc()/xcalloc() deallocator
/// \sa xfree_cppwrapper() with a slightly different (FREE-matching) signature.
CtoCpp1(xfree, const void *)

// TODO: Move AllocedBuf and xfree_cpp() to src/base/Memory.h or similar.
/// memory allocated by xmalloc() or xcalloc(), to be freed by xfree()
using AllocedBuf = std::unique_ptr<void, HardFun<void, const void *, &xfree_cpp> >;

namespace Store {

/// swap metadata prefix and all swap metadata fields of the given entry
/// \param size gets filled with the total swap metadata size
AllocedBuf PackSwapMeta(const StoreEntry &, size_t &size);

} // namespace Store

#endif /* SQUID_SRC_STORE_SWAPMETAOUT_H */

