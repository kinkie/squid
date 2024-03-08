/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_CLIENTACTIVEREQUESTS_H
#define SQUID_SRC_CLIENTACTIVEREQUESTS_H

class dlink_list;
class dlink_node;
class ClientHttpRequest;
class StoreEntry;

class ClientActiveRequests
{
    public:
    static ClientActiveRequests &Instance();
    void Add(ClientHttpRequest *, dlink_node *m);
    void Delete(dlink_node *m);
    void MakeHead(dlink_node *m);
    // only use for iterating
    dlink_list *GetActiveRequests() { return ActiveRequests_; };

private:
    ClientActiveRequests();
    dlink_list *ActiveRequests_;
};
#endif /* SQUID_SRC_CLIENTACTIVEREQUESTS_H */
