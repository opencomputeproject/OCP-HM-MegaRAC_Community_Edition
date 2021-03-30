#include "conditions.hpp"

#include "sdbusplus.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>

namespace phosphor
{
namespace fan
{
namespace monitor
{
namespace condition
{

using json = nlohmann::json;
using namespace phosphor::logging;

Condition propertiesMatch(std::vector<PropertyState>&& propStates)
{
    return [pStates = std::move(propStates)](sdbusplus::bus::bus& bus) {
        return std::all_of(
            pStates.begin(), pStates.end(), [&bus](const auto& p) {
                return util::SDBusPlus::getPropertyVariant<PropertyValue>(
                           bus, std::get<propObj>(p.first),
                           std::get<propIface>(p.first),
                           std::get<propName>(p.first)) == p.second;
            });
    };
}

Condition getPropertiesMatch(const json& condParams)
{
    if (!condParams.contains("properties"))
    {
        // Log error on missing required parameter
        log<level::ERR>(
            "Missing fan monitor condition properties",
            entry("NAME=%s", condParams["name"].get<std::string>().c_str()));
        throw std::runtime_error("Missing fan monitor condition properties");
    }
    std::vector<PropertyState> propStates;
    for (auto& param : condParams["properties"])
    {
        if (!param.contains("object") || !param.contains("interface") ||
            !param.contains("property"))
        {
            // Log error on missing required parameters
            log<level::ERR>("Missing propertiesMatch condition parameters",
                            entry("REQUIRED_PARAMETERS=%s",
                                  "{object, interface, property}"));
            throw std::runtime_error(
                "Missing propertiesMatch condition parameters");
        }

        auto propAttrs = param["property"];
        if (!propAttrs.contains("name") || !propAttrs.contains("value"))
        {
            // Log error on missing required parameters
            log<level::ERR>(
                "Missing propertiesMatch condition property attributes",
                entry("REQUIRED_ATTRIBUTES=%s", "{name, value}"));
            throw std::runtime_error(
                "Missing propertiesMatch condition property attributes");
        }

        std::string type = "";
        if (propAttrs.contains("type"))
        {
            type = propAttrs["type"].get<std::string>();
        }

        // Add property for propertiesMatch condition
        propStates.emplace_back(PropertyState(
            {param["object"].get<std::string>(),
             param["interface"].get<std::string>(),
             propAttrs["name"].get<std::string>()},
            JsonTypeHandler::getPropValue(propAttrs["value"], type)));
    }

    return make_condition(condition::propertiesMatch(std::move(propStates)));
}

} // namespace condition
} // namespace monitor
} // namespace fan
} // namespace phosphor
