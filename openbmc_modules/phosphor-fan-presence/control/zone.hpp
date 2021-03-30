#pragma once
#include "fan.hpp"
#include "sdbusplus.hpp"
#include "types.hpp"
#include "xyz/openbmc_project/Control/ThermalMode/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <optional>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace control
{

using ThermalObject = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Control::server::ThermalMode>;

/**
 * The mode fan control will run in:
 *   - init - only do the initialization steps
 *   - control - run normal control algorithms
 */
enum class Mode
{
    init,
    control
};

/**
 * @class Represents a fan control zone, which is a group of fans
 * that behave the same.
 */
class Zone : public ThermalObject
{
  public:
    Zone() = delete;
    Zone(const Zone&) = delete;
    Zone(Zone&&) = delete;
    Zone& operator=(const Zone&) = delete;
    Zone& operator=(Zone&&) = delete;
    ~Zone() = default;

    /**
     * Constructor
     * Creates the appropriate fan objects based on
     * the zone definition data passed in.
     *
     * @param[in] mode - mode of fan control
     * @param[in] bus - the dbus object
     * @param[in] path - object instance path
     * @param[in] event - Event loop reference
     * @param[in] def - the fan zone definition data
     */
    Zone(Mode mode, sdbusplus::bus::bus& bus, const std::string& path,
         const sdeventplus::Event& event, const ZoneDefinition& def);

    /**
     * @brief Get the zone's bus
     *
     * @return The bus used by the zone
     */
    inline auto& getBus()
    {
        return _bus;
    }

    /**
     * @brief Get the zone's path
     *
     * @return The path of this zone
     */
    inline auto& getPath()
    {
        return _path;
    }

    /**
     * @brief Get the zone's hosted interfaces
     *
     * @return The interfaces hosted by this zone
     */
    inline auto& getIfaces()
    {
        return _ifaces;
    }

    /**
     * Sets all fans in the zone to the speed
     * passed in when the zone is active
     *
     * @param[in] speed - the fan speed
     */
    void setSpeed(uint64_t speed);

    /**
     * Sets the zone to full speed regardless of zone's active state
     */
    void setFullSpeed();

    /**
     * @brief Sets the automatic fan control allowed active state
     *
     * @param[in] group - A group that affects the active state
     * @param[in] isActiveAllow - Active state according to group
     */
    void setActiveAllow(const Group* group, bool isActiveAllow);

    /**
     * @brief Sets the floor change allowed state
     *
     * @param[in] group - A group that affects floor changes
     * @param[in] isAllow - Allow state according to group
     */
    inline void setFloorChangeAllow(const Group* group, bool isAllow)
    {
        _floorChange[*(group)] = isAllow;
    }

    /**
     * @brief Sets the decrease allowed state of a group
     *
     * @param[in] group - A group that affects speed decreases
     * @param[in] isAllow - Allow state according to group
     */
    inline void setDecreaseAllow(const Group* group, bool isAllow)
    {
        _decAllowed[*(group)] = isAllow;
    }

    /**
     * @brief Sets a given object's event data for a property on this zone
     *
     * @param[in] object - Name of the object containing the property
     * @param[in] interface - Interface name containing the property
     * @param[in] property - Property name
     * @param[in] data - Property value
     */
    inline void setObjectData(const std::string& object,
                              const std::string& interface,
                              const std::string& property, EventData* data)
    {
        _objects[object][interface][property] = data;
    }

    /**
     * @brief Sets a given object's property value
     *
     * @param[in] object - Name of the object containing the property
     * @param[in] interface - Interface name containing the property
     * @param[in] property - Property name
     * @param[in] value - Property value
     */
    template <typename T>
    void setPropertyValue(const char* object, const char* interface,
                          const char* property, T value)
    {
        _properties[object][interface][property] = value;
    };

    /**
     * @brief Sets a given object's property value
     *
     * @param[in] object - Name of the object containing the property
     * @param[in] interface - Interface name containing the property
     * @param[in] property - Property name
     * @param[in] value - Property value
     */
    template <typename T>
    void setPropertyValue(const std::string& object,
                          const std::string& interface,
                          const std::string& property, T value)
    {
        _properties[object][interface][property] = value;
    };

    /**
     * @brief Get the value of an object's property
     *
     * @param[in] object - Name of the object containing the property
     * @param[in] interface - Interface name containing the property
     * @param[in] property - Property name
     *
     * @return - The property value
     */
    template <typename T>
    inline auto getPropertyValue(const std::string& object,
                                 const std::string& interface,
                                 const std::string& property)
    {
        return std::get<T>(_properties.at(object).at(interface).at(property));
    };

    /**
     * @brief Get the object's property variant
     *
     * @param[in] object - Name of the object containing the property
     * @param[in] interface - Interface name containing the property
     * @param[in] property - Property name
     *
     * @return - The property variant
     */
    inline auto getPropValueVariant(const std::string& object,
                                    const std::string& interface,
                                    const std::string& property)
    {
        return _properties.at(object).at(interface).at(property);
    };

    /**
     * @brief Get a property's value after applying a set of visitors
     * to translate the property value's type change to keep from
     * affecting the configured use of the property.
     *
     * @param[in] intf = Interface name containing the property
     * @param[in] prop = Property name
     * @param[in] variant = Variant containing the property's value from
     *                      the supported property types.
     */
    template <typename T>
    inline auto getPropertyValueVisitor(const char* intf, const char* prop,
                                        PropertyVariantType& variant)
    {
        // Handle the transition of the dbus sensor value type from
        // int64 to double which also removed the scale property.
        // https://gerrit.openbmc-project.xyz/11739
        if (strcmp(intf, "xyz.openbmc_project.Sensor.Value") == 0 &&
            strcmp(prop, "Value") == 0)
        {
            // Use 'optional' variable to determine if the sensor value
            // is set within the visitor based on the supported types.
            // A non-supported type configured will assert.
            std::optional<T> value;
            std::visit(
                [&value](auto&& val) {
                    // If the type configured is int64, but the sensor value
                    // property's type is double, scale it by 1000 and return
                    // the value as an int64 as configured.
                    using V = std::decay_t<decltype(val)>;
                    if constexpr (std::is_same_v<T, int64_t> &&
                                  std::is_same_v<V, double>)
                    {
                        val = val * 1000;
                        value = std::lround(val);
                    }
                    // If the type configured matches the sensor value
                    // property's type, just return the value as its
                    // given type.
                    else if constexpr (std::is_same_v<T, V>)
                    {
                        value = val;
                    }
                },
                variant);

            // Unable to return Sensor Value property
            // as given type configured.
            assert(value);

            return value.value();
        }

        // Default to return the property's value by the data type
        // configured, applying no visitors to the variant.
        return std::get<T>(variant);
    };

    /**
     * @brief Remove an object's interface
     *
     * @param[in] object - Name of the object with the interface
     * @param[in] interface - Interface name to remove
     */
    inline void removeObjectInterface(const char* object, const char* interface)
    {
        auto it = _properties.find(object);
        if (it != std::end(_properties))
        {
            _properties[object].erase(interface);
        }
    }

    /**
     * @brief Remove a service associated to a group
     *
     * @param[in] group - Group associated with service
     * @param[in] name - Service name to remove
     */
    void removeService(const Group* group, const std::string& name);

    /**
     * @brief Set or update a service name owner in use
     *
     * @param[in] group - Group associated with service
     * @param[in] name - Service name
     * @param[in] hasOwner - Whether the service is owned or not
     */
    void setServiceOwner(const Group* group, const std::string& name,
                         const bool hasOwner);

    /**
     * @brief Set or update all services for a group
     *
     * @param[in] group - Group to get service names for
     */
    void setServices(const Group* group);

    /**
     * @brief Get the group's list of service names
     *
     * @param[in] group - Group to get service names for
     *
     * @return - The list of service names
     */
    inline auto getGroupServices(const Group* group)
    {
        return _services.at(*group);
    }

    /**
     * @brief Initialize a set speed event properties and actions
     *
     * @param[in] event - Set speed event
     */
    void initEvent(const SetSpeedEvent& event);

    /**
     * @brief Removes all the set speed event properties and actions
     *
     * @param[in] event - Set speed event
     */
    void removeEvent(const SetSpeedEvent& event);

    /**
     * @brief Get the default floor speed
     *
     * @return - The defined default floor speed
     */
    inline auto getDefFloor()
    {
        return _defFloorSpeed;
    };

    /**
     * @brief Set the default floor
     *
     * @param[in] speed - Speed to set the default floor to
     */
    inline void setDefFloor(uint64_t speed)
    {
        _defFloorSpeed = speed;
    };

    /**
     * @brief Get the ceiling speed
     *
     * @return - The current ceiling speed
     */
    inline auto& getCeiling() const
    {
        return _ceilingSpeed;
    };

    /**
     * @brief Set the ceiling speed to the given speed
     *
     * @param[in] speed - Speed to set the ceiling to
     */
    inline void setCeiling(uint64_t speed)
    {
        _ceilingSpeed = speed;
    };

    /**
     * @brief Swaps the ceiling key value with what's given and
     * returns the value that was swapped.
     *
     * @param[in] keyValue - New ceiling key value
     *
     * @return - Ceiling key value prior to swapping
     */
    inline auto swapCeilingKeyValue(int64_t keyValue)
    {
        std::swap(_ceilingKeyValue, keyValue);
        return keyValue;
    };

    /**
     * @brief Get the increase speed delta
     *
     * @return - The current increase speed delta
     */
    inline auto& getIncSpeedDelta() const
    {
        return _incSpeedDelta;
    };

    /**
     * @brief Get the decrease speed delta
     *
     * @return - The current decrease speed delta
     */
    inline auto& getDecSpeedDelta() const
    {
        return _decSpeedDelta;
    };

    /**
     * @brief Set the floor speed to the given speed and increase target
     * speed to the floor when target is below floor where floor changes
     * are allowed.
     *
     * @param[in] speed - Speed to set the floor to
     */
    void setFloor(uint64_t speed);

    /**
     * @brief Set the requested speed base to be used as the speed to
     * base a new requested speed target from
     *
     * @param[in] speedBase - Base speed value to use
     */
    inline void setRequestSpeedBase(uint64_t speedBase)
    {
        _requestSpeedBase = speedBase;
    };

    /**
     * @brief Calculate the requested target speed from the given delta
     * and increase the fan speeds, not going above the ceiling.
     *
     * @param[in] targetDelta - The delta to increase the target speed by
     */
    void requestSpeedIncrease(uint64_t targetDelta);

    /**
     * @brief Calculate the requested target speed from the given delta
     * and increase the fan speeds, not going above the ceiling.
     *
     * @param[in] targetDelta - The delta to increase the target speed by
     */
    void requestSpeedDecrease(uint64_t targetDelta);

    /**
     * @brief Callback function for the increase timer that delays
     * processing of requested speed increases while fans are increasing
     */
    void incTimerExpired();

    /**
     * @brief Callback function for the decrease timer that processes any
     * requested speed decreases if allowed
     */
    void decTimerExpired();

    /**
     * @brief Get the event loop used with this zone's timers
     *
     * @return - The event loop for timers
     */
    inline auto& getEventLoop()
    {
        return _eventLoop;
    }

    /**
     * @brief Remove the given signal event
     *
     * @param[in] seIter - Iterator pointing to the signal event to remove
     */
    inline void removeSignal(std::vector<SignalEvent>::iterator& seIter)
    {
        std::get<signalEventDataPos>(*seIter).reset();
        if (std::get<signalMatchPos>(*seIter) != nullptr)
        {
            std::get<signalMatchPos>(*seIter).reset();
        }
    }

    /**
     * @brief Get the list of timer events
     *
     * @return - List of timer events
     */
    inline auto& getTimerEvents()
    {
        return _timerEvents;
    }

    /**
     * @brief Find the first instance of a timer event
     *
     * @param[in] eventGroup - Group associated with a timer
     * @param[in] eventActions - List of actions associated with a timer
     * @param[in] eventTimers - List of timers to find the timer in
     *
     * @return - Iterator to the timer event
     */
    std::vector<TimerEvent>::iterator
        findTimer(const Group& eventGroup,
                  const std::vector<Action>& eventActions,
                  std::vector<TimerEvent>& eventTimers);

    /**
     * @brief Add a timer to the list of timer based events
     *
     * @param[in] name - Event name associated with timer
     * @param[in] group - Group associated with a timer
     * @param[in] actions - List of actions associated with a timer
     * @param[in] tConf - Configuration for the new timer
     */
    void addTimer(const std::string& name, const Group& group,
                  const std::vector<Action>& actions, const TimerConf& tConf);

    /**
     * @brief Callback function for event timers that processes the given
     * actions for a group
     *
     * @param[in] eventGroup - Group to process actions on
     * @param[in] eventActions - List of event actions to run
     */
    void timerExpired(const Group& eventGroup,
                      const std::vector<Action>& eventActions);

    /**
     * @brief Get the service for a given path and interface from cached
     * dataset and add a service that's not found
     *
     * @param[in] path - Path to get service for
     * @param[in] intf - Interface to get service for
     *
     * @return - The service name
     */
    const std::string& getService(const std::string& path,
                                  const std::string& intf);

    /**
     * @brief Add a set of services for a path and interface
     * by retrieving all the path subtrees to the given depth
     * from root for the interface
     *
     * @param[in] path - Path to add services for
     * @param[in] intf - Interface to add services for
     * @param[in] depth - Depth of tree traversal from root path
     *
     * @return - The associated service to the given path and interface
     * or empty string for no service found
     */
    const std::string& addServices(const std::string& path,
                                   const std::string& intf, int32_t depth);

    /**
     * @brief Dbus signal change callback handler
     *
     * @param[in] msg - Expanded sdbusplus message data
     * @param[in] eventData - The single event's data
     */
    void handleEvent(sdbusplus::message::message& msg,
                     const EventData* eventData);

    /**
     * @brief Add a signal to the list of signal based events
     *
     * @param[in] name - Event name
     * @param[in] data - Event data for signal
     * @param[in] match - Subscribed signal match
     */
    inline void
        addSignal(const std::string& name, std::unique_ptr<EventData>&& data,
                  std::unique_ptr<sdbusplus::server::match::match>&& match)
    {
        _signalEvents[name].emplace_back(std::move(data), std::move(match));
    }

    /**
     * @brief Set a property to be persisted
     *
     * @param[in] intf - Interface containing property
     * @param[in] prop - Property to be persisted
     */
    inline void setPersisted(const std::string& intf, const std::string& prop)
    {
        _persisted[intf].emplace_back(prop);
    }

    /**
     * @brief Get persisted property
     *
     * @param[in] intf - Interface containing property
     * @param[in] prop - Property persisted
     *
     * @return - True if property is to be persisted, false otherwise
     */
    auto getPersisted(const std::string& intf, const std::string& prop);

    /**
     * @brief Get a property value from the zone object or the bus when
     * the property requested is not on the zone object
     *
     * @param[in] path - Path of object
     * @param[in] intf - Object interface
     * @param[in] prop - Object property
     *
     * @return - Property's value
     */
    template <typename T>
    auto getPropertyByName(const std::string& path, const std::string& intf,
                           const std::string& prop)
    {
        T value;
        auto pathIter = _objects.find(path);
        if (pathIter != _objects.end())
        {
            auto intfIter = pathIter->second.find(intf);
            if (intfIter != pathIter->second.end())
            {
                if (intf == "xyz.openbmc_project.Control.ThermalMode")
                {
                    auto var = ThermalMode::getPropertyByName(prop);
                    // Use visitor to determine if requested property
                    // type(T) is available on this interface and read it
                    std::visit(
                        [&value](auto&& val) {
                            using V = std::decay_t<decltype(val)>;
                            if constexpr (std::is_same_v<T, V>)
                            {
                                value = val;
                            }
                        },
                        var);

                    return value;
                }
            }
        }

        // Retrieve the property's value applying any visitors necessary
        auto service = getService(path, intf);
        auto variant = util::SDBusPlus::getPropertyVariant<PropertyVariantType>(
            _bus, service, path, intf, prop);
        value = getPropertyValueVisitor<T>(intf.c_str(), prop.c_str(), variant);

        return value;
    };

    /**
     * @brief Overridden thermal object's set 'Current' property function
     *
     * @param[in] value - Value to set 'Current' to
     *
     * @return - The updated value of the 'Current' property
     */
    virtual std::string current(std::string value);

  private:
    /**
     * The dbus object
     */
    sdbusplus::bus::bus& _bus;

    /**
     * Zone object path
     */
    const std::string _path;

    /**
     * Zone supported interfaces
     */
    const std::vector<std::string> _ifaces;

    /**
     * Full speed for the zone
     */
    const uint64_t _fullSpeed;

    /**
     * The zone number
     */
    const size_t _zoneNum;

    /**
     * The default floor speed for the zone
     */
    uint64_t _defFloorSpeed;

    /**
     * The default ceiling speed for the zone
     */
    const uint64_t _defCeilingSpeed;

    /**
     * The floor speed to not go below
     */
    uint64_t _floorSpeed = _defFloorSpeed;

    /**
     * The ceiling speed to not go above
     */
    uint64_t _ceilingSpeed = _defCeilingSpeed;

    /**
     * The previous sensor value for calculating the ceiling
     */
    int64_t _ceilingKeyValue = 0;

    /**
     * Automatic fan control active state
     */
    bool _isActive = true;

    /**
     * Target speed for this zone
     */
    uint64_t _targetSpeed = _fullSpeed;

    /**
     * Speed increase delta
     */
    uint64_t _incSpeedDelta = 0;

    /**
     * Speed decrease delta
     */
    uint64_t _decSpeedDelta = 0;

    /**
     * Requested speed base
     */
    uint64_t _requestSpeedBase = 0;

    /**
     * Speed increase delay in seconds
     */
    std::chrono::seconds _incDelay;

    /**
     * Speed decrease interval in seconds
     */
    std::chrono::seconds _decInterval;

    /**
     * The increase timer object
     */
    Timer _incTimer;

    /**
     * The decrease timer object
     */
    Timer _decTimer;

    /**
     * Event loop used on set speed event timers
     */
    sdeventplus::Event _eventLoop;

    /**
     * The vector of fans in this zone
     */
    std::vector<std::unique_ptr<Fan>> _fans;

    /**
     * @brief Map of object property values
     */
    std::map<std::string,
             std::map<std::string, std::map<std::string, PropertyVariantType>>>
        _properties;

    /**
     * @brief Map of zone objects
     */
    std::map<std::string,
             std::map<std::string, std::map<std::string, EventData*>>>
        _objects;

    /**
     * @brief Map of interfaces to persisted properties
     */
    std::map<std::string, std::vector<std::string>> _persisted;

    /**
     * @brief Map of active fan control allowed by groups
     */
    std::map<const Group, bool> _active;

    /**
     * @brief Map of floor change allowed by groups
     */
    std::map<const Group, bool> _floorChange;

    /**
     * @brief Map of groups controlling decreases allowed
     */
    std::map<const Group, bool> _decAllowed;

    /**
     * @brief Map of group service names
     */
    std::map<const Group, std::vector<Service>> _services;

    /**
     * @brief Map tree of paths to services of interfaces
     */
    std::map<std::string, std::map<std::string, std::vector<std::string>>>
        _servTree;

    /**
     * @brief List of signal event arguments and Dbus matches
     * for callbacks per event name
     */
    std::map<std::string, std::vector<SignalEvent>> _signalEvents;

    /**
     * @brief List of timers per event name
     */
    std::map<std::string, std::vector<TimerEvent>> _timerEvents;

    /**
     * @brief Save the thermal control current mode property
     * to persisted storage
     */
    void saveCurrentMode();

    /**
     * @brief Restore persisted thermal control current mode property
     * value, setting the mode to "Default" otherwise
     */
    void restoreCurrentMode();

    /**
     * @brief Get the request speed base if defined, otherwise the
     * the current target speed is returned
     *
     * @return - The request speed base or current target speed
     */
    inline auto getRequestSpeedBase() const
    {
        return (_requestSpeedBase != 0) ? _requestSpeedBase : _targetSpeed;
    };
};

} // namespace control
} // namespace fan
} // namespace phosphor
