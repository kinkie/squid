/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "cbdata.h"
#include "comm/Connection.h"
#include "comm/IoCallback.h"
#include "comm/Loops.h"
#include "comm/Write.h"
#include "fd.h"
#include "fde.h"
#include "globals.h"
#include "MemBuf.h"
#include "StatCounters.h"
#if USE_DELAY_POOLS
#include "ClientInfo.h"
#endif

#include <cerrno>

void
Comm::Write(const Comm::ConnectionPointer &conn, MemBuf *mb, AsyncCall::Pointer &callback)
{
    Comm::Write(conn, mb->buf, mb->size, callback, mb->freeFunc());
}

void
Comm::Write(const Comm::ConnectionPointer &conn, const char *buf, int size, AsyncCall::Pointer &callback, FREE * free_func)
{
    debugs(5, 5, conn << ": sz " << size << ": asynCall " << callback);

    /* Make sure we are open, not closing, and not writing */
    assert(fd_table[conn->fd].flags.open);
    assert(!fd_table[conn->fd].closing());
    Comm::IoCallback *ccb = COMMIO_FD_WRITECB(conn->fd);
    assert(!ccb->active());

    fd_table[conn->fd].writeStart = squid_curtime;
    ccb->conn = conn;
    /* Queue the write */
    ccb->setCallback(IOCB_WRITE, callback, (char *)buf, free_func, size);
    ccb->selectOrQueueWrite();
}

/** Write to FD.
 * This function is used by the lowest level of IO loop which only has access to FD numbers.
 * We have to use the Comm::ioCallbacks() to map FD numbers to waiting data and Comm::Connections.
 * Once the write has been concluded we schedule the waiting call with success/fail results.
 */
void
Comm::HandleWrite(int fd, void *data)
{
    Comm::IoCallback *state = static_cast<Comm::IoCallback *>(data);
    int len = 0;
    int nleft;

    assert(state->conn != nullptr);
    assert(state->conn->fd == fd);

    debugs(5, 5, state->conn << ": off " <<
           (long int) state->offset << ", sz " << (long int) state->size << ".");

    nleft = state->size - state->offset;

#if USE_DELAY_POOLS
    BandwidthBucket *bucket = BandwidthBucket::SelectBucket(&fd_table[fd]);
    if (bucket) {
        assert(bucket->selectWaiting);
        bucket->selectWaiting = false;
        if (nleft > 0 && !bucket->applyQuota(nleft, state)) {
            return;
        }
    }
#endif /* USE_DELAY_POOLS */

    /* actually WRITE data */
    int xerrno = errno = 0;
    len = FD_WRITE_METHOD(fd, state->buf + state->offset, nleft);
    xerrno = errno;
    debugs(5, 5, "write() returns " << len);

#if USE_DELAY_POOLS
    if (bucket) {
        /* we wrote data - drain them from bucket */
        bucket->reduceBucket(len);
    }
#endif /* USE_DELAY_POOLS */

    fd_bytes(fd, len, IoDirection::Write);
    ++statCounter.syscalls.sock.writes;
    // After each successful partial write,
    // reset fde::writeStart to the current time.
    fd_table[fd].writeStart = squid_curtime;

    if (len == 0) {
        /* Note we even call write if nleft == 0 */
        /* We're done */
        if (nleft != 0)
            debugs(5, DBG_IMPORTANT, "FD " << fd << " write failure: connection closed with " << nleft << " bytes remaining.");

        state->finish(nleft ? Comm::COMM_ERROR : Comm::OK, 0);
    } else if (len < 0) {
        /* An error */
        if (fd_table[fd].flags.socket_eof) {
            debugs(50, 2, "FD " << fd << " write failure: " << xstrerr(xerrno) << ".");
            state->finish(nleft ? Comm::COMM_ERROR : Comm::OK, xerrno);
        } else if (ignoreErrno(xerrno)) {
            debugs(50, 9, "FD " << fd << " write failure: " << xstrerr(xerrno) << ".");
            state->selectOrQueueWrite();
        } else {
            debugs(50, 2, "FD " << fd << " write failure: " << xstrerr(xerrno) << ".");
            state->finish(nleft ? Comm::COMM_ERROR : Comm::OK, xerrno);
        }
    } else {
        /* A successful write, continue */
        state->offset += len;

        if (state->offset < state->size) {
            /* Not done, reinstall the write handler and write some more */
            state->selectOrQueueWrite();
        } else {
            state->finish(nleft ? Comm::OK : Comm::COMM_ERROR, 0);
        }
    }
}

