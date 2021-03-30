#include "config.h"
#include "snmp_conf_manager.hpp"
#include "snmp_serialize.hpp"
#include "snmp_util.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

#include <experimental/filesystem>

#include <arpa/inet.h>

namespace phosphor
{
namespace network
{
namespace snmp
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using Argument = xyz::openbmc_project::Common::InvalidArgument;

ConfManager::ConfManager(sdbusplus::bus::bus& bus, const char* objPath) :
    details::CreateIface(bus, objPath, true),
    dbusPersistentLocation(SNMP_CONF_PERSIST_PATH), bus(bus),
    objectPath(objPath)
{
}

std::string ConfManager::client(std::string address, uint16_t port)
{
    // will throw exception if it is already configured.
    checkClientConfigured(address, port);

    lastClientId++;
    try
    {
        // just to check whether given address is valid or not.
        resolveAddress(address);
    }
    catch (InternalFailure& e)
    {
        log<level::ERR>("Not a valid address"),
            entry("ADDRESS=%s", address.c_str());
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("Address"),
                              Argument::ARGUMENT_VALUE(address.c_str()));
    }

    // create the D-Bus object
    std::experimental::filesystem::path objPath;
    objPath /= objectPath;
    objPath /= std::to_string(lastClientId);

    auto client = std::make_unique<phosphor::network::snmp::Client>(
        bus, objPath.string().c_str(), *this, address, port);

    // save the D-Bus object
    serialize(lastClientId, *client, dbusPersistentLocation);

    this->clients.emplace(lastClientId, std::move(client));
    return objPath.string();
}

void ConfManager::checkClientConfigured(const std::string& address,
                                        uint16_t port)
{
    if (address.empty())
    {
        log<level::ERR>("Invalid address");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("ADDRESS"),
                              Argument::ARGUMENT_VALUE(address.c_str()));
    }

    for (const auto& val : clients)
    {
        if (val.second.get()->address() == address &&
            val.second.get()->port() == port)
        {
            log<level::ERR>("Client already exist");
            // TODO Add the error(Object already exist) in the D-Bus interface
            // then make the change here,meanwhile send the Internal Failure.
            elog<InvalidArgument>(
                Argument::ARGUMENT_NAME("ADDRESS"),
                Argument::ARGUMENT_VALUE("Client already exist."));
        }
    }
}

void ConfManager::deleteSNMPClient(Id id)
{
    auto it = clients.find(id);
    if (it == clients.end())
    {
        log<level::ERR>("Unable to delete the snmp client.",
                        entry("ID=%d", id));
        return;
    }

    std::error_code ec;
    // remove the persistent file
    fs::path fileName = dbusPersistentLocation;
    fileName /= std::to_string(id);

    if (fs::exists(fileName))
    {
        if (!fs::remove(fileName, ec))
        {
            log<level::ERR>("Unable to delete the file",
                            entry("FILE=%s", fileName.c_str()),
                            entry("ERROR=%d", ec.value()));
        }
    }
    else
    {
        log<level::ERR>("File doesn't exist",
                        entry("FILE=%s", fileName.c_str()));
    }
    // remove the D-Bus Object.
    this->clients.erase(it);
}

void ConfManager::restoreClients()
{
    if (!fs::exists(dbusPersistentLocation) ||
        fs::is_empty(dbusPersistentLocation))
    {
        return;
    }

    for (auto& confFile :
         fs::recursive_directory_iterator(dbusPersistentLocation))
    {
        if (!fs::is_regular_file(confFile))
        {
            continue;
        }

        auto managerID = confFile.path().filename().string();
        Id idNum = std::stol(managerID);

        fs::path objPath = objectPath;
        objPath /= managerID;
        auto manager =
            std::make_unique<Client>(bus, objPath.string().c_str(), *this);
        if (deserialize(confFile.path(), *manager))
        {
            manager->emit_object_added();
            this->clients.emplace(idNum, std::move(manager));
            if (idNum > lastClientId)
            {
                lastClientId = idNum;
            }
        }
    }
}

} // namespace snmp
} // namespace network
} // namespace phosphor
