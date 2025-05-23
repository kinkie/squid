/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 49    SNMP Interface */

#ifndef SQUID_SRC_SNMP_INQUIRER_H
#define SQUID_SRC_SNMP_INQUIRER_H

#include "comm/forward.h"
#include "ipc/Inquirer.h"
#include "snmp/forward.h"
#include "snmp/Pdu.h"

class CommCloseCbParams;

namespace Snmp
{

/// Coordinator's job that sends a PDU request to each strand,
/// aggregates strand responses and send back the result to client
class Inquirer: public Ipc::Inquirer
{
    CBDATA_CHILD(Inquirer);

public:
    Inquirer(const Request& aRequest, const Ipc::StrandCoords& coords);

protected:
    /* AsyncJob API */
    void start() override;
    bool doneAll() const override;

    /* Ipc::Inquirer API */
    void cleanup() override;
    void handleException(const std::exception& e) override;
    void sendResponse() override;
    bool aggregate(Ipc::Response::Pointer aResponse) override;

private:
    void noteCommClosed(const CommCloseCbParams& params);

private:
    Pdu aggrPdu; ///< aggregated pdu
    Comm::ConnectionPointer conn; ///< client connection descriptor

    AsyncCall::Pointer writer; ///< comm_write callback
    AsyncCall::Pointer closer; ///< comm_close handler
};

} // namespace Snmp

#endif /* SQUID_SRC_SNMP_INQUIRER_H */

