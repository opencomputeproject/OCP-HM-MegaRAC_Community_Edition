#include "snmp_client.hpp"
#include "snmp_conf_manager.hpp"
#include "snmp_serialize.hpp"

namespace phosphor
{
namespace network
{
namespace snmp
{

Client::Client(sdbusplus::bus::bus& bus, const char* objPath,
               ConfManager& parent, const std::string& address, uint16_t port) :
    Ifaces(bus, objPath, true),
    id(std::stol(std::experimental::filesystem::path(objPath).filename())),
    parent(parent)
{
    this->address(address);
    this->port(port);

    // Emit deferred signal.
    emit_object_added();
}

std::string Client::address(std::string value)
{
    if (value == Ifaces::address())
    {
        return value;
    }

    parent.checkClientConfigured(value, port());

    auto addr = Ifaces::address(value);
    serialize(id, *this, parent.dbusPersistentLocation);
    return addr;
}

uint16_t Client::port(uint16_t value)
{
    if (value == Ifaces::port())
    {
        return value;
    }

    parent.checkClientConfigured(address(), value);

    auto port = Ifaces::port(value);
    serialize(id, *this, parent.dbusPersistentLocation);
    return port;
}

void Client::delete_()
{
    parent.deleteSNMPClient(id);
}

} // namespace snmp
} // namespace network
} // namespace phosphor
