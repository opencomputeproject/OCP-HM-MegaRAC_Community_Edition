#include "config.h"

#include "vlan_interface.hpp"

#include "ethernet_interface.hpp"
#include "network_manager.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <string>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace network
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

VlanInterface::VlanInterface(sdbusplus::bus::bus& bus,
                             const std::string& objPath, DHCPConf dhcpEnabled,
                             bool nICEnabled, uint32_t vlanID,
                             EthernetInterface& intf, Manager& parent) :
    VlanIface(bus, objPath.c_str()),
    DeleteIface(bus, objPath.c_str()),
    EthernetInterface(bus, objPath, dhcpEnabled, parent, false),
    parentInterface(intf)
{
    id(vlanID);
    EthernetInterfaceIntf::nICEnabled(nICEnabled);
    VlanIface::interfaceName(EthernetInterface::interfaceName());
    MacAddressIntf::mACAddress(parentInterface.mACAddress());

    emit_object_added();
}

std::string VlanInterface::mACAddress(std::string)
{
    log<level::ERR>("Tried to set MAC address on VLAN");
    elog<InternalFailure>();
}

void VlanInterface::writeDeviceFile()
{
    using namespace std::string_literals;
    fs::path confPath = manager.getConfDir();
    std::string fileName = EthernetInterface::interfaceName() + ".netdev"s;
    confPath /= fileName;
    std::fstream stream;
    try
    {
        stream.open(confPath.c_str(), std::fstream::out);
    }
    catch (std::ios_base::failure& e)
    {
        log<level::ERR>("Unable to open the VLAN device file",
                        entry("FILE=%s", confPath.c_str()),
                        entry("ERROR=%s", e.what()));
        elog<InternalFailure>();
    }

    stream << "[NetDev]\n";
    stream << "Name=" << EthernetInterface::interfaceName() << "\n";
    stream << "Kind=vlan\n";
    stream << "[VLAN]\n";
    stream << "Id=" << id() << "\n";
    stream.close();
}

void VlanInterface::delete_()
{
    parentInterface.deleteVLANObject(EthernetInterface::interfaceName());
}

} // namespace network
} // namespace phosphor
