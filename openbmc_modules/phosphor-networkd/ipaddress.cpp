#include "config.h"

#include "ipaddress.hpp"

#include "ethernet_interface.hpp"
#include "util.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
namespace phosphor
{
namespace network
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using NotAllowed = sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

IPAddress::IPAddress(sdbusplus::bus::bus& bus, const char* objPath,
                     EthernetInterface& parent, IP::Protocol type,
                     const std::string& ipaddress, IP::AddressOrigin origin,
                     uint8_t prefixLength, const std::string& gateway) :
    IPIfaces(bus, objPath, true),
    parent(parent)
{

    IP::address(ipaddress);
    IP::prefixLength(prefixLength);
    IP::gateway(gateway);
    IP::type(type);
    IP::origin(origin);

    // Emit deferred signal.
    emit_object_added();
}
std::string IPAddress::address(std::string /*ipAddress*/)
{
    elog<NotAllowed>(Reason("Property update is not allowed"));
}
uint8_t IPAddress::prefixLength(uint8_t /*value*/)
{
    elog<NotAllowed>(Reason("Property update is not allowed"));
}
std::string IPAddress::gateway(std::string /*gateway*/)
{
    elog<NotAllowed>(Reason("Property update is not allowed"));
}
IP::Protocol IPAddress::type(IP::Protocol /*type*/)
{
    elog<NotAllowed>(Reason("Property update is not allowed"));
}
IP::AddressOrigin IPAddress::origin(IP::AddressOrigin /*origin*/)
{
    elog<NotAllowed>(Reason("Property update is not allowed"));
}
void IPAddress::delete_()
{
    if (origin() != IP::AddressOrigin::Static)
    {
        log<level::ERR>("Tried to delete a non-static address"),
            entry("ADDRESS=%s", address().c_str()),
            entry("PREFIX=%" PRIu8, prefixLength()),
            entry("INTERFACE=%s", parent.interfaceName().c_str());
        elog<InternalFailure>();
    }

    parent.deleteObject(address());
}

} // namespace network
} // namespace phosphor
