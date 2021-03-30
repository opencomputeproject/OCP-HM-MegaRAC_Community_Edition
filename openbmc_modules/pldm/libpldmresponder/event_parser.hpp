#pragma once

#include "common/types.hpp"
#include "common/utils.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace pldm::responder::events
{

/** @struct StateSensorEntry
 *
 *  StateSensorEntry is a key to uniquely identify a state sensor, so that a
 *  D-Bus action can be defined for PlatformEventMessage command with
 *  sensorEvent type. This struct is used as a key in a std::map so implemented
 *  operator== and operator<.
 */
struct StateSensorEntry
{
    pdr::ContainerID containerId;
    pdr::EntityType entityType;
    pdr::EntityInstance entityInstance;
    pdr::SensorOffset sensorOffset;

    bool operator==(const StateSensorEntry& e) const
    {
        return ((containerId == e.containerId) &&
                (entityType == e.entityType) &&
                (entityInstance == e.entityInstance) &&
                (sensorOffset == e.sensorOffset));
    }

    bool operator<(const StateSensorEntry& e) const
    {
        return (
            (containerId < e.containerId) ||
            ((containerId == e.containerId) && (entityType < e.entityType)) ||
            ((containerId == e.containerId) && (entityType == e.entityType) &&
             (entityInstance < e.entityInstance)) ||
            ((containerId == e.containerId) && (entityType == e.entityType) &&
             (entityInstance == e.entityInstance) &&
             (sensorOffset < e.sensorOffset)));
    }
};

using StateToDBusValue = std::map<pdr::EventState, pldm::utils::PropertyValue>;
using EventDBusInfo = std::tuple<pldm::utils::DBusMapping, StateToDBusValue>;
using EventMap = std::map<StateSensorEntry, EventDBusInfo>;
using Json = nlohmann::json;

/** @class StateSensorHandler
 *
 *  @brief Parses the event state sensor configuration JSON file and build
 *         the lookup data structure, which can map the event state for a
 *         sensor in the PlatformEventMessage command to a D-Bus property and
 *         the property value.
 */
class StateSensorHandler
{
  public:
    StateSensorHandler() = delete;

    /** @brief Parse the event state sensor configuration JSON file and build
     *         the lookup data stucture.
     *
     *  @param[in] dirPath - directory path which has the config JSONs
     */
    explicit StateSensorHandler(const std::string& dirPath);
    virtual ~StateSensorHandler() = default;
    StateSensorHandler(const StateSensorHandler&) = default;
    StateSensorHandler& operator=(const StateSensorHandler&) = default;
    StateSensorHandler(StateSensorHandler&&) = default;
    StateSensorHandler& operator=(StateSensorHandler&&) = default;

    /** @brief If the StateSensorEntry and EventState is valid, the D-Bus
     *         property corresponding to the StateSensorEntry is set based on
     *         the EventState
     *
     *  @param[in] entry - state sensor entry
     *  @param[in] state - event state
     *
     *  @return PLDM completion code
     */
    int eventAction(const StateSensorEntry& entry, pdr::EventState state);

    /** @brief Helper API to get D-Bus information for a StateSensorEntry
     *
     *  @param[in] entry - state sensor entry
     *
     *  @return D-Bus information corresponding to the SensorEntry
     */
    const EventDBusInfo& getEventInfo(const StateSensorEntry& entry) const
    {
        return eventMap.at(entry);
    }

  private:
    EventMap eventMap; //!< a map of StateSensorEntry to D-Bus information

    /** @brief Create a map of EventState to D-Bus property values from
     *         the information provided in the event state configuration
     *         JSON
     *
     *  @param[in] eventStates - a JSON array of event states
     *  @param[in] propertyValues - a JSON array of D-Bus property values
     *  @param[in] type - the type of D-Bus property
     *
     *  @return a map of EventState to D-Bus property values
     */
    StateToDBusValue mapStateToDBusVal(const Json& eventStates,
                                       const Json& propertyValues,
                                       std::string_view type);
};

} // namespace pldm::responder::events