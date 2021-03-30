#pragma once

#include "average.hpp"
#include "hwmonio.hpp"
#include "interface.hpp"
#include "sensor.hpp"
#include "sensorset.hpp"
#include "sysfs.hpp"
#include "types.hpp"

#include <any>
#include <memory>
#include <optional>
#include <sdbusplus/server.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <string>
#include <vector>

static constexpr auto default_interval = 1000000;

static constexpr auto sensorID = 0;
static constexpr auto sensorLabel = 1;
using SensorIdentifiers = std::tuple<std::string, std::string>;

/** @class MainLoop
 *  @brief hwmon-readd main application loop.
 */
class MainLoop
{
  public:
    MainLoop() = delete;
    MainLoop(const MainLoop&) = delete;
    MainLoop& operator=(const MainLoop&) = delete;
    MainLoop(MainLoop&&) = delete;
    MainLoop& operator=(MainLoop&&) = delete;
    ~MainLoop() = default;

    /** @brief Constructor
     *
     *  @param[in] bus - sdbusplus bus client connection.
     *  @param[in] param - the path parameter provided
     *  @param[in] path - hwmon sysfs instance to manage
     *  @param[in] devPath - physical device sysfs path.
     *  @param[in] prefix - DBus busname prefix.
     *  @param[in] root - DBus sensors namespace root.
     *
     *  Any DBus objects are created relative to the DBus
     *  sensors namespace root.
     *
     *  At startup, the application will own a busname with
     *  the format <prefix>.hwmon<n>.
     */
    MainLoop(sdbusplus::bus::bus&& bus, const std::string& param,
             const std::string& path, const std::string& devPath,
             const char* prefix, const char* root,
             const hwmonio::HwmonIOInterface* ioIntf);

    /** @brief Setup polling timer in a sd event loop and attach to D-Bus
     *         event loop.
     */
    void run();

    /** @brief Stop polling timer event loop from another thread.
     *
     *  Typically only used by testcases.
     */
    void shutdown() noexcept;

    /** @brief Remove sensors slated for removal.
     */
    void removeSensors();

    /** @brief Attempt to add sensors back that had been removed.
     */
    void addDroppedSensors();

  private:
    using mapped_type =
        std::tuple<SensorSet::mapped_type, std::string, ObjectInfo>;
    using SensorState = std::map<SensorSet::key_type, mapped_type>;

    /** @brief Read hwmon sysfs entries */
    void read();

    /** @brief Set up D-Bus object state */
    void init();

    /** @brief sdbusplus bus client connection. */
    sdbusplus::bus::bus _bus;
    /** @brief sdbusplus freedesktop.ObjectManager storage. */
    sdbusplus::server::manager::manager _manager;
    /** @brief the parameter path used. */
    std::string _pathParam;
    /** @brief hwmon sysfs class path. */
    std::string _hwmonRoot;
    /** @brief hwmon sysfs instance. */
    std::string _instance;
    /** @brief physical device sysfs path. */
    std::string _devPath;
    /** @brief DBus busname prefix. */
    const char* _prefix;
    /** @brief DBus sensors namespace root. */
    const char* _root;
    /** @brief DBus object state. */
    SensorState _state;
    /** @brief Sleep interval in microseconds. */
    uint64_t _interval = default_interval;
    /** @brief Hwmon sysfs access. */
    const hwmonio::HwmonIOInterface* _ioAccess;
    /** @brief the Event Loop structure */
    sdeventplus::Event _event;
    /** @brief Read Timer */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> _timer;
    /** @brief Store the specifications of sensor objects */
    std::map<SensorSet::key_type, std::unique_ptr<sensor::Sensor>>
        _sensorObjects;

    /**
     * @brief Map of removed sensors
     */
    std::map<SensorSet::key_type, SensorSet::mapped_type> _rmSensors;

    /** @brief Object of class Average, to handle with average related process
     */
    Average _average;

    /**
     * @brief Get the ID of the sensor
     *
     * @param[in] sensor - Sensor to get the ID of
     */
    std::string getID(SensorSet::container_t::const_reference sensor);

    /**
     * @brief Get the sensor identifiers
     *
     * @param[in] sensor - Sensor to get the identifiers of
     */
    SensorIdentifiers
        getIdentifiers(SensorSet::container_t::const_reference sensor);

    /**
     * @brief Used to create and add sensor objects
     *
     * @param[in] sensor - Sensor to create/add object for
     *
     * @return - Optional
     *     Object state data on success, nothing on failure
     */
    std::optional<ObjectStateData>
        getObject(SensorSet::container_t::const_reference sensor);
};

/** @brief Given a value and map of interfaces, update values and check
 * thresholds.
 */
void updateSensorInterfaces(InterfaceMap& ifaces, SensorValueType value);
