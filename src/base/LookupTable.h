/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_BASE_LOOKUPTABLE_H
#define SQUID_SRC_BASE_LOOKUPTABLE_H

#include "sbuf/Algorithms.h"
#include "sbuf/SBuf.h"

#include <unordered_map>

/**
 * a record in the initializer list for a LookupTable
 *
 * In case it is wished to extend the structure of a LookupTable's initializer
 * list, it can be done by using a custom struct which must match
 * LookupTableRecord's signature plus any extra custom fields the user may
 * wish to add; the extended record type must then be passed as RecordType
 * template parameter to LookupTable.
 */
template <typename EnumType>
struct LookupTableRecord
{
    const char *name;
    EnumType id;
};

/**
 * SBuf -> case-insensitive enum lookup table
 *
 * How to use:
 * enum enum_type { ... };
 * static const LookupTable<enum_type>::Record initializerTable[] = {
 *   {"key1", ENUM_1}, {"key2", ENUM_2}, ... {nullptr, ENUM_INVALID_VALUE}
 * };
 * LookupTable<enum_type> lookupTableInstance(ENUM_INVALID_VALUE, initializerTable);
 *
 * then in the code:
 * SBuf s(string_to_lookup);
 * enum_type item = lookupTableInstance.lookup(s);
 * if (item != ENUM_INVALID_VALUE) { // do stuff }
 *
 */

template<typename EnumType, typename RecordType = LookupTableRecord<EnumType>, typename Hasher = CaseInsensitiveSBufHash >
class LookupTable
{
public:
    /// element of the lookup table initialization list
    using Record=RecordType;
    using lookupTable_t = std::unordered_map<const SBuf, EnumType, Hasher, CaseInsensitiveSBufEqual>;
    using iterator = typename lookupTable_t::iterator;
    using const_iterator = typename lookupTable_t::const_iterator;

    LookupTable(const EnumType theInvalid, const Record data[]) :
        invalidValue(theInvalid)
    {
        for (auto i = 0; data[i].name != nullptr; ++i) {
            lookupTable[SBuf(data[i].name)] = data[i].id;
        }
    }

    EnumType lookup(const SBuf &key) const {
        auto r = lookupTable.find(key);
        if (r == lookupTable.end())
            return invalidValue;
        return r->second;
    }

    const SBuf reverse_lookup(EnumType value) const {
        for (auto &i : lookupTable) {
            if (i.second == value)
                return i.first;
        }
        return reverse_lookup(invalidValue);
    }

    // iterate via std::pair<SBuf, EnumType>
    const_iterator begin() const { return lookupTable.begin(); }
    const_iterator end() const { return lookupTable.end(); }

private:
    lookupTable_t lookupTable;
    EnumType invalidValue;
};

#endif /* SQUID_SRC_BASE_LOOKUPTABLE_H */

