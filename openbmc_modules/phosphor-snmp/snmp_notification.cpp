#include "snmp_notification.hpp"
#include "snmp_util.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

#include "xyz/openbmc_project/Common/error.hpp"

namespace phosphor
{
namespace network
{
namespace snmp
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

using snmpSessionPtr =
    std::unique_ptr<netsnmp_session, decltype(&::snmp_close)>;

bool Notification::addPDUVar(netsnmp_pdu& pdu, const OID& objID,
                             size_t objIDLen, u_char type, Value val)
{
    netsnmp_variable_list* varList = nullptr;
    switch (type)
    {
        case ASN_INTEGER:
        {
            auto ltmp = std::get<int32_t>(val);
            varList = snmp_pdu_add_variable(&pdu, objID.data(), objIDLen, type,
                                            &ltmp, sizeof(ltmp));
        }
        break;
        case ASN_UNSIGNED:
        {
            auto ltmp = std::get<uint32_t>(val);
            varList = snmp_pdu_add_variable(&pdu, objID.data(), objIDLen, type,
                                            &ltmp, sizeof(ltmp));
        }
        break;
        case ASN_OPAQUE_U64:
        {
            auto ltmp = std::get<uint64_t>(val);
            varList = snmp_pdu_add_variable(&pdu, objID.data(), objIDLen, type,
                                            &ltmp, sizeof(ltmp));
        }
        break;
        case ASN_OCTET_STR:
        {
            const auto& value = std::get<std::string>(val);
            varList = snmp_pdu_add_variable(&pdu, objID.data(), objIDLen, type,
                                            value.c_str(), value.length());
        }
        break;
    }
    return (varList == nullptr ? false : true);
}

void Notification::sendTrap()
{
    constexpr auto comm = "public";
    netsnmp_session session{0};

    snmp_sess_init(&session);

    init_snmp("snmpapp");

    // TODO: https://github.com/openbmc/openbmc/issues/3145
    session.version = SNMP_VERSION_2c;
    session.community = (u_char*)comm;
    session.community_len = strlen(comm);
    session.callback = nullptr;
    session.callback_magic = nullptr;

    auto mgrs = getManagers();

    for (auto& mgr : mgrs)
    {
        session.peername = const_cast<char*>(mgr.c_str());
        // create the session
        auto ss = snmp_add(
            &session,
            netsnmp_transport_open_client("snmptrap", session.peername),
            nullptr, nullptr);
        if (!ss)
        {
            log<level::ERR>("Unable to get the snmp session.",
                            entry("SNMPMANAGER=%s", mgr.c_str()));
            elog<InternalFailure>();
        }

        // Wrap the raw pointer in RAII
        snmpSessionPtr sessionPtr(ss, &::snmp_close);

        ss = nullptr;

        auto pdu = snmp_pdu_create(SNMP_MSG_TRAP2);
        if (!pdu)
        {
            log<level::ERR>("Failed to create notification PDU");
            elog<InternalFailure>();
        }

        pdu->trap_type = SNMP_TRAP_ENTERPRISESPECIFIC;

        auto trapInfo = getTrapOID();

        if (!snmp_pdu_add_variable(pdu, SNMPTrapOID,
                                   sizeof(SNMPTrapOID) / sizeof(oid),
                                   ASN_OBJECT_ID, trapInfo.first.data(),
                                   trapInfo.second * sizeof(oid)))
        {
            log<level::ERR>("Failed to add the SNMP var(trapID)");
            snmp_free_pdu(pdu);
            elog<InternalFailure>();
        }

        auto objectList = getFieldOIDList();

        for (const auto& object : objectList)
        {
            if (!addPDUVar(*pdu, std::get<0>(object), std::get<1>(object),
                           std::get<2>(object), std::get<3>(object)))
            {
                log<level::ERR>("Failed to add the SNMP var");
                snmp_free_pdu(pdu);
                elog<InternalFailure>();
            }
        }
        // pdu is freed by snmp_send
        if (!snmp_send(sessionPtr.get(), pdu))
        {
            log<level::ERR>("Failed to send the snmp trap.");
            elog<InternalFailure>();
        }

        log<level::DEBUG>("Sent SNMP Trap", entry("MGR=%s", mgr.c_str()));
    }
}

} // namespace snmp
} // namespace network
} // namespace phosphor
