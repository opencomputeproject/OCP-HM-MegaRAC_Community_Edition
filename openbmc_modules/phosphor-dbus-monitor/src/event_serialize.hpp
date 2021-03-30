#pragma once

#include "config.h"

#include "event_entry.hpp"

#include <experimental/filesystem>
#include <string>

namespace phosphor
{
namespace events
{

namespace fs = std::experimental::filesystem;

/** @brief Serialize and persist event d-bus object
 *  @param[in] event - const reference to event entry.
 *  @param[in] eventName - Name of the event.
 *  @return fs::path - pathname of persisted events file
 */
fs::path serialize(const Entry& event, const std::string& eventName);

/** @brief Deserialze a persisted event into a d-bus object
 *  @param[in] path - pathname of persisted event file
 *  @param[in] event - reference to event object which is the target of
 *             deserialization.
 *  @return bool - true if the deserialization was successful, false otherwise.
 */
bool deserialize(const fs::path& path, Entry& event);

} // namespace events
} // namespace phosphor
