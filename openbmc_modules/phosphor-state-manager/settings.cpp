#include "settings.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>

namespace settings
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using sdbusplus::exception::SdBusError;

constexpr auto mapperService = "xyz.openbmc_project.ObjectMapper";
constexpr auto mapperPath = "/xyz/openbmc_project/object_mapper";
constexpr auto mapperIntf = "xyz.openbmc_project.ObjectMapper";

Objects::Objects(sdbusplus::bus::bus& bus) : bus(bus)
{
    std::vector<std::string> settingsIntfs = {autoRebootIntf, powerRestoreIntf};
    auto depth = 0;

    auto mapperCall = bus.new_method_call(mapperService, mapperPath, mapperIntf,
                                          "GetSubTree");
    mapperCall.append(root);
    mapperCall.append(depth);
    mapperCall.append(settingsIntfs);

    using Interfaces = std::vector<Interface>;
    using MapperResponse = std::map<Path, std::map<Service, Interfaces>>;
    MapperResponse result;

    try
    {
        auto response = bus.call(mapperCall);

        response.read(result);
        if (result.empty())
        {
            log<level::ERR>("Invalid response from mapper");
            elog<InternalFailure>();
        }
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in mapper GetSubTree",
                        entry("ERROR=%s", e.what()));
        elog<InternalFailure>();
    }

    for (const auto& iter : result)
    {
        const Path& path = iter.first;

        for (const auto& serviceIter : iter.second)
        {
            for (const auto& interface : serviceIter.second)
            {
                if (autoRebootIntf == interface)
                {
                    autoReboot = path;
                }
                else if (powerRestoreIntf == interface)
                {
                    powerRestorePolicy = path;
                }
            }
        }
    }
}

Service Objects::service(const Path& path, const Interface& interface) const
{
    using Interfaces = std::vector<Interface>;
    auto mapperCall =
        bus.new_method_call(mapperService, mapperPath, mapperIntf, "GetObject");
    mapperCall.append(path);
    mapperCall.append(Interfaces({interface}));

    std::map<Service, Interfaces> result;

    try
    {
        auto response = bus.call(mapperCall);
        response.read(result);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in mapper GetObject",
                        entry("ERROR=%s", e.what()));
        elog<InternalFailure>();
    }

    if (result.empty())
    {
        log<level::ERR>("Invalid response from mapper");
        elog<InternalFailure>();
    }

    return result.begin()->first;
}

} // namespace settings
