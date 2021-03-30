#include "config.h"

#include "elog_meta.hpp"

namespace phosphor
{
namespace logging
{
namespace metadata
{
namespace associations
{

#if defined PROCESS_META

template <>
void build<xyz::openbmc_project::Common::Callout::Device::CALLOUT_DEVICE_PATH>(
    const std::string& match, const std::vector<std::string>& data,
    AssociationList& list)
{
    std::map<std::string, std::string> metadata;
    parse(data, metadata);
    auto iter = metadata.find(match);
    if (metadata.end() != iter)
    {
        auto comp = [](const auto& first, const auto& second) {
            return (std::strcmp(std::get<0>(first), second) < 0);
        };
        auto callout = std::lower_bound(callouts.begin(), callouts.end(),
                                        (iter->second).c_str(), comp);
        if ((callouts.end() != callout) &&
            !std::strcmp((iter->second).c_str(), std::get<0>(*callout)))
        {
            list.emplace_back(std::make_tuple(
                CALLOUT_FWD_ASSOCIATION, CALLOUT_REV_ASSOCIATION,
                std::string(INVENTORY_ROOT) + std::get<1>(*callout)));
        }
    }
}

template <>
void build<
    xyz::openbmc_project::Common::Callout::Inventory::CALLOUT_INVENTORY_PATH>(
    const std::string& match, const std::vector<std::string>& data,
    AssociationList& list)
{
    std::map<std::string, std::string> metadata;
    parse(data, metadata);
    auto iter = metadata.find(match);
    if (metadata.end() != iter)
    {
        list.emplace_back(std::make_tuple(CALLOUT_FWD_ASSOCIATION,
                                          CALLOUT_REV_ASSOCIATION,
                                          std::string(iter->second.c_str())));
    }
}

#endif

} // namespace associations
} // namespace metadata
} // namespace logging
} // namespace phosphor
