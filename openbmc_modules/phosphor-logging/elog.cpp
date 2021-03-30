#include "config.h"

#include <phosphor-logging/elog.hpp>
#include <stdexcept>

namespace phosphor
{
namespace logging
{
namespace details
{
using namespace sdbusplus::xyz::openbmc_project::Logging::server;

auto _prepareMsg(const char* funcName)
{
    using phosphor::logging::log;
    constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
    constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
    constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

    constexpr auto IFACE_INTERNAL(
        "xyz.openbmc_project.Logging.Internal.Manager");

    // Transaction id is located at the end of the string separated by a period.

    auto b = sdbusplus::bus::new_default();
    auto mapper = b.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                    MAPPER_INTERFACE, "GetObject");
    mapper.append(OBJ_INTERNAL, std::vector<std::string>({IFACE_INTERNAL}));

    auto mapperResponseMsg = b.call(mapper);
    if (mapperResponseMsg.is_method_error())
    {
        throw std::runtime_error("Error in mapper call");
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);
    if (mapperResponse.empty())
    {
        throw std::runtime_error("Error reading mapper response");
    }

    const auto& host = mapperResponse.cbegin()->first;
    auto m =
        b.new_method_call(host.c_str(), OBJ_INTERNAL, IFACE_INTERNAL, funcName);
    return m;
}

void commit(const char* name)
{
    auto msg = _prepareMsg("Commit");
    uint64_t id = sdbusplus::server::transaction::get_id();
    msg.append(id, name);
    auto bus = sdbusplus::bus::new_default();
    bus.call_noreply(msg);
}

void commit(const char* name, Entry::Level level)
{
    auto msg = _prepareMsg("CommitWithLvl");
    uint64_t id = sdbusplus::server::transaction::get_id();
    msg.append(id, name, static_cast<uint32_t>(level));
    auto bus = sdbusplus::bus::new_default();
    bus.call_noreply(msg);
}
} // namespace details

void commit(std::string&& name)
{
    log<level::ERR>("method is deprecated, use commit() with exception type");
    phosphor::logging::details::commit(name.c_str());
}

} // namespace logging
} // namespace phosphor
