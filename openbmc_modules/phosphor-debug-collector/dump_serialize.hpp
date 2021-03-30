#pragma once

#include "config.h"

#include "elog_watch.hpp"

#include <experimental/filesystem>

namespace phosphor
{
namespace dump
{
namespace elog
{

namespace fs = std::experimental::filesystem;

/** @brief Serialize and persist list of ids.
 *  @param[in] list - elog id list.
 *  @param[in] dir - pathname of file where the serialized elog id's will
 *                   be placed.
 */
void serialize(const ElogList& list,
               const fs::path& dir = fs::path(ELOG_ID_PERSIST_PATH));

/** @brief Deserialze a persisted list of ids into list
 *  @param[in] path - pathname of persisted error file
 *  @param[out] list - elog id list
 *  @return bool - true if the deserialization was successful, false otherwise.
 */
bool deserialize(const fs::path& path, ElogList& list);

} // namespace elog
} // namespace dump
} // namespace phosphor
