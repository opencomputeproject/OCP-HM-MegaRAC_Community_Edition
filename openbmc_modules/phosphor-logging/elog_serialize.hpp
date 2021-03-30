#pragma once

#include "config.h"

#include "elog_entry.hpp"

#include <experimental/filesystem>
#include <string>
#include <vector>

namespace phosphor
{
namespace logging
{

namespace fs = std::experimental::filesystem;

/** @brief Serialize and persist error d-bus object
 *  @param[in] a - const reference to error entry.
 *  @param[in] dir - pathname of directory where the serialized error will
 *                   be placed.
 *  @return fs::path - pathname of persisted error file
 */
fs::path serialize(const Entry& e,
                   const fs::path& dir = fs::path(ERRLOG_PERSIST_PATH));

/** @brief Deserialze a persisted error into a d-bus object
 *  @param[in] path - pathname of persisted error file
 *  @param[in] e - reference to error object which is the target of
 *             deserialization.
 *  @return bool - true if the deserialization was successful, false otherwise.
 */
bool deserialize(const fs::path& path, Entry& e);

} // namespace logging
} // namespace phosphor
