#pragma once

#include <ipmid/types.hpp>
#include <memory>
#include <nlohmann/json.hpp>

namespace ipmi
{
namespace sensor
{

/**
 * @brief Grab a handle to the entity map.
 */
const EntityInfoMap& getIpmiEntityRecords();

/**
 * @brief Open the default entity map json file, and if present and valid json,
 * return a built entity map.
 *
 * @return the map
 */
EntityInfoMap buildEntityMapFromFile();

/**
 * @brief Given json data validate the data matches the expected format for the
 * entity map configuration and parse the data into a map of the entities.
 *
 * If any entry is invalid, the entire contents passed in is disregarded as
 * possibly corrupt.
 *
 * @param[in] data - the json data
 * @return the map
 */
EntityInfoMap buildJsonEntityMap(const nlohmann::json& data);

/**
 * @brief Owner of the EntityInfoMap.
 */
class EntityInfoMapContainer
{
  public:
    /** Get ahold of the owner. */
    static EntityInfoMapContainer* getContainer();
    /** Get ahold of the records. */
    const EntityInfoMap& getIpmiEntityRecords();

  private:
    EntityInfoMapContainer(const EntityInfoMap& entityRecords) :
        entityRecords(entityRecords)
    {
    }
    EntityInfoMap entityRecords;
};

} // namespace sensor
} // namespace ipmi
