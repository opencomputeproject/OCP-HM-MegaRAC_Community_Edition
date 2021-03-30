#pragma once

#include "pid/ec/pid.hpp"

#include <limits>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <string>

/* This program assumes sensors use the Sensor.Value interface
 * and for sensor->write() I only implemented sysfs as a type,
 * but -- how would it know whether to use Control.FanSpeed or Control.FanPwm?
 *
 * One could get the interface list for the object and search for Control.*
 * but, it needs to know the maximum, minimum.  The only sensors it wants to
 * write in this code base are Fans...
 */
enum class IOInterfaceType
{
    NONE, // There is no interface.
    EXTERNAL,
    DBUSPASSIVE,
    DBUSACTIVE, // This means for write that it needs to look up the interface.
    SYSFS,
    UNKNOWN
};

/* WriteInterfaceType is different because Dbusactive/passive. how to know... */
IOInterfaceType getWriteInterfaceType(const std::string& path);

IOInterfaceType getReadInterfaceType(const std::string& path);

void tryRestartControlLoops(void);

/*
 * Given a configuration structure, fill out the information we use within the
 * PID loop.
 */
void initializePIDStruct(ec::pid_info_t* info, const ec::pidinfo& initial);

void dumpPIDStruct(ec::pid_info_t* info);

struct SensorProperties
{
    int64_t scale;
    double value;
    double min;
    double max;
    std::string unit;
};

struct SensorThresholds
{
    double lowerThreshold = std::numeric_limits<double>::quiet_NaN();
    double upperThreshold = std::numeric_limits<double>::quiet_NaN();
};

const std::string sensorintf = "xyz.openbmc_project.Sensor.Value";
const std::string criticalThreshInf =
    "xyz.openbmc_project.Sensor.Threshold.Critical";
const std::string propertiesintf = "org.freedesktop.DBus.Properties";

class DbusHelperInterface
{
  public:
    virtual ~DbusHelperInterface() = default;

    /** @brief Get the service providing the interface for the path.
     *
     * @warning Throws exception on dbus failure.
     */
    virtual std::string getService(sdbusplus::bus::bus& bus,
                                   const std::string& intf,
                                   const std::string& path) = 0;

    /** @brief Get all Sensor.Value properties for a service and path.
     *
     * @param[in] bus - A bus to use for the call.
     * @param[in] service - The service providing the interface.
     * @param[in] path - The dbus path.
     * @param[out] prop - A pointer to a properties struct to fill out.
     *
     * @warning Throws exception on dbus failure.
     */
    virtual void getProperties(sdbusplus::bus::bus& bus,
                               const std::string& service,
                               const std::string& path,
                               struct SensorProperties* prop) = 0;

    /** @brief Get Critical Threshold current assert status
     *
     * @param[in] bus - A bus to use for the call.
     * @param[in] service - The service providing the interface.
     * @param[in] path - The dbus path.
     */
    virtual bool thresholdsAsserted(sdbusplus::bus::bus& bus,
                                    const std::string& service,
                                    const std::string& path) = 0;
};

class DbusHelper : public DbusHelperInterface
{
  public:
    DbusHelper() = default;
    ~DbusHelper() = default;
    DbusHelper(const DbusHelper&) = default;
    DbusHelper& operator=(const DbusHelper&) = default;
    DbusHelper(DbusHelper&&) = default;
    DbusHelper& operator=(DbusHelper&&) = default;

    std::string getService(sdbusplus::bus::bus& bus, const std::string& intf,
                           const std::string& path) override;

    void getProperties(sdbusplus::bus::bus& bus, const std::string& service,
                       const std::string& path,
                       struct SensorProperties* prop) override;

    bool thresholdsAsserted(sdbusplus::bus::bus& bus,
                            const std::string& service,
                            const std::string& path) override;

    template <typename T>
    void getProperty(sdbusplus::bus::bus& bus, const std::string& service,
                     const std::string& path, const std::string& interface,
                     const std::string& propertyName, T& prop)
    {
        namespace log = phosphor::logging;

        auto msg = bus.new_method_call(service.c_str(), path.c_str(),
                                       propertiesintf.c_str(), "Get");

        msg.append(interface, propertyName);

        std::variant<T> result;
        try
        {
            auto valueResponseMsg = bus.call(msg);
            valueResponseMsg.read(result);
        }
        catch (const sdbusplus::exception::SdBusError& ex)
        {
            log::log<log::level::ERR>("Get Property Failed",
                                      log::entry("WHAT=%s", ex.what()));
            throw;
        }

        prop = std::get<T>(result);
    }
};

std::string getSensorPath(const std::string& type, const std::string& id);
std::string getMatch(const std::string& type, const std::string& id);
void scaleSensorReading(const double min, const double max, double& value);
bool validType(const std::string& type);

struct VariantToDoubleVisitor
{
    template <typename T>
    std::enable_if_t<std::is_arithmetic<T>::value, double>
        operator()(const T& t) const
    {
        return static_cast<double>(t);
    }

    template <typename T>
    std::enable_if_t<!std::is_arithmetic<T>::value, double>
        operator()(const T& t) const
    {
        throw std::invalid_argument("Cannot translate type to double");
    }
};

/*
 * Given a path that optionally has a glob portion, fill it out.
 */
std::string FixupPath(std::string original);
