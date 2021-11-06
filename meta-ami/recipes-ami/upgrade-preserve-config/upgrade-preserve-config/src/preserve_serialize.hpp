#pragma once

#include "preserve.hpp"

#include <experimental/filesystem>

namespace phosphor
{
namespace software
{
namespace preserve
{

constexpr auto SEPARATOR = "_";

namespace fs = std::experimental::filesystem;

/** @brief Serialize and persist preservation D-Bus object.
 *  @param[in] manager - const reference to preservation object.
 *  @param[in] path -  path of persistent location where D-Bus object would be
 * saved.
 *  @return fs::path - pathname of persisted preservation file.
 */
fs::path serialize(const PreserveConf& manager, const fs::path& path);

/** @brief Deserialze preservation info into a D-Bus object
 *  @param[in] path - pathname of persisted file.
 *  @param[in] manager - reference to preservation object
 *                       which is the target of deserialization.
 *  @return bool - true if the deserialization was successful, false otherwise.
 */
bool deserialize(const fs::path& path, PreserveConf& manager);

} // namespace preserve
} // namespace software
} // namespace phosphor
