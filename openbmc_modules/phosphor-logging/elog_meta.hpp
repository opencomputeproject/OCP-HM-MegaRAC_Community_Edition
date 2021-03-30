#pragma once

#include "callouts-gen.hpp"
#include "elog_entry.hpp"

#include <algorithm>
#include <cstring>
#include <phosphor-logging/elog-errors.hpp>
#include <string>
#include <tuple>
#include <vector>

namespace phosphor
{
namespace logging
{
namespace metadata
{

using Metadata = std::string;

namespace associations
{

using Type = void(const std::string&, const std::vector<std::string>&,
                  AssociationList& list);

/** @brief Pull out metadata name and value from the string
 *         <metadata name>=<metadata value>
 *  @param [in] data - metadata key=value entries
 *  @param [out] metadata - map of metadata name:value
 */
inline void parse(const std::vector<std::string>& data,
                  std::map<std::string, std::string>& metadata)
{
    constexpr auto separator = '=';
    for (const auto& entryItem : data)
    {
        auto pos = entryItem.find(separator);
        if (std::string::npos != pos)
        {
            auto key = entryItem.substr(0, entryItem.find(separator));
            auto value = entryItem.substr(entryItem.find(separator) + 1);
            metadata.emplace(std::move(key), std::move(value));
        }
    }
};

/** @brief Combine the metadata keys and values from the map
 *         into a vector of strings that look like:
 *            "<metadata name>=<metadata value>"
 *  @param [in] data - metadata key:value map
 *  @param [out] metadata - vector of "key=value" strings
 */
inline void combine(const std::map<std::string, std::string>& data,
                    std::vector<std::string>& metadata)
{
    for (const auto& [key, value] : data)
    {
        std::string line{key};
        line += "=" + value;
        metadata.push_back(std::move(line));
    }
}

/** @brief Build error associations specific to metadata. Specialize this
 *         template for handling a specific type of metadata.
 *  @tparam M - type of metadata
 *  @param [in] match - metadata to be handled
 *  @param [in] data - metadata key=value entries
 *  @param [out] list - list of error association objects
 */
template <typename M>
void build(const std::string& match, const std::vector<std::string>& data,
           AssociationList& list) = delete;

// Example template specialization - we don't want to do anything
// for this metadata.
using namespace example::xyz::openbmc_project::Example::Elog;
template <>
inline void build<TestErrorTwo::DEV_ID>(const std::string& match,
                                        const std::vector<std::string>& data,
                                        AssociationList& list)
{
}

template <>
inline void
    build<example::xyz::openbmc_project::Example::Device::Callout::
              CALLOUT_DEVICE_PATH_TEST>(const std::string& match,
                                        const std::vector<std::string>& data,
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
            constexpr auto ROOT = "/xyz/openbmc_project/inventory";

            list.push_back(std::make_tuple(
                "callout", "fault", std::string(ROOT) + std::get<1>(*callout)));
        }
    }
}

// The PROCESS_META flag is needed to get out of tree builds working. Such
// builds will have access only to internal error interfaces, hence handlers
// for out dbus error interfaces won't compile. This flag is not set by default,
// the phosphor-logging recipe enabled it.
#if defined PROCESS_META

template <>
void build<xyz::openbmc_project::Common::Callout::Device::CALLOUT_DEVICE_PATH>(
    const std::string& match, const std::vector<std::string>& data,
    AssociationList& list);

template <>
void build<
    xyz::openbmc_project::Common::Callout::Inventory::CALLOUT_INVENTORY_PATH>(
    const std::string& match, const std::vector<std::string>& data,
    AssociationList& list);

#endif // PROCESS_META

} // namespace associations
} // namespace metadata
} // namespace logging
} // namespace phosphor
