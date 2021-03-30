#pragma once

#include "hwmonio.hpp"
#include "sensorset.hpp"
#include "types.hpp"

#include <gpioplus/handle.hpp>
#include <memory>
#include <optional>
#include <stdplus/handle/managed.hpp>
#include <unordered_set>

namespace sensor
{

struct valueAdjust
{
    double gain = 1.0;
    int offset = 0;
    std::unordered_set<int> rmRCs;
};

/** @class Sensor
 *  @brief Sensor object based on a SensorSet container's key type
 *  @details Sensor object to create and modify an associated device's sensor
 *  attributes based on the key type of each sensor in the set provided by the
 *  device.
 */
class Sensor
{
  public:
    Sensor() = delete;
    Sensor(const Sensor&) = delete;
    Sensor(Sensor&&) = default;
    Sensor& operator=(const Sensor&) = delete;
    Sensor& operator=(Sensor&&) = default;
    ~Sensor() = default;

    /**
     * @brief Constructs Sensor object
     *
     * @param[in] sensor - A pair of sensor indentifiers
     * @param[in] ioAccess - Hwmon sysfs access
     * @param[in] devPath - Device sysfs path
     */
    explicit Sensor(const SensorSet::key_type& sensor,
                    const hwmonio::HwmonIOInterface* ioAccess,
                    const std::string& devPath);

    /**
     * @brief Adds any sensor removal return codes for the sensor
     * @details Add all return codes defined within a device's config file
     * for the entire device or for the specific sensor.
     *
     * @param[in] rcList - List of return codes found for the sensor
     */
    void addRemoveRCs(const std::string& rcList);

    /**
     * @brief Get the adjustments struct for the sensor
     *
     * @return - Sensor adjustment struct
     */
    inline const valueAdjust& getAdjusts() const
    {
        return _sensorAdjusts;
    }

    /**
     * @brief Adjusts a sensor value
     * @details Adjusts the value given by any gain and/or offset defined
     * for this sensor object and returns that adjusted value.
     *
     * @param[in] value - Value to be adjusted
     *
     * @return - Adjusted sensor value
     */
    SensorValueType adjustValue(SensorValueType value);

    /**
     * @brief Add value interface and value property for sensor
     * @details When a sensor has an associated input file, the Sensor.Value
     * interface is added along with setting the Value property to the
     * corresponding value found in the input file.
     *
     * @param[in] retryIO - Hwmon sysfs file retry constraints
     *                      (number of and delay between)
     * @param[in] info - Sensor object information
     *
     * @return - Shared pointer to the value object
     */
    std::shared_ptr<ValueObject> addValue(const RetryIO& retryIO,
                                          ObjectInfo& info);

    /**
     * @brief Add status interface and functional property for sensor
     * @details OperationalStatus interface is added and the Functional property
     * is set depending on whether a fault file exists and if it does it will
     * also depend on the content of the fault file. _hasFaultFile will also be
     * set to true if fault file exists.
     *
     * @param[in] info - Sensor object information
     *
     * @return - Shared pointer to the status object
     */
    std::shared_ptr<StatusObject> addStatus(ObjectInfo& info);

    /**
     * @brief Get the scale from the sensor.
     *
     * @return - Scale value
     */
    inline int64_t getScale(void) const
    {
        return _scale;
    }

    /**
     * @brief Get the GPIO handle from the sensor.
     *
     * @return - Pointer to the GPIO handle interface, can be nullptr.
     */
    inline const gpioplus::HandleInterface* getGpio(void) const
    {
        return _handle.get();
    }

    /**
     * @brief Get whether the sensor has a fault file or not.
     *
     * @return - Boolean on whether the sensor has a fault file
     */
    inline bool hasFaultFile(void) const
    {
        return _hasFaultFile;
    }

  private:
    /** @brief Sensor object's identifiers */
    SensorSet::key_type _sensor;

    /** @brief Hwmon sysfs access. */
    const hwmonio::HwmonIOInterface* _ioAccess;

    /** @brief Physical device sysfs path. */
    const std::string& _devPath;

    /** @brief Structure for storing sensor adjustments */
    valueAdjust _sensorAdjusts;

    /** @brief Optional pointer to GPIO handle. */
    std::unique_ptr<gpioplus::HandleInterface> _handle;

    /** @brief sensor scale from configuration. */
    int64_t _scale;

    /** @brief Tracks whether the sensor has a fault file or not. */
    bool _hasFaultFile;
};

/**
 * @brief Locks the gpio represented by the handle
 *
 * @param[in] handle - The gpio handle to lock
 */
void gpioLock(const gpioplus::HandleInterface*&& handle);

/** @brief The type which is responsible for managing the lock */
using GpioLocker =
    stdplus::Managed<const gpioplus::HandleInterface*>::Handle<gpioLock>;

/**
 * @brief Unlocks the gpio and creates a lock object to ensure
 *        the gpio is locked again.
 *
 * @param[in] handle - The gpio handle to unlock and wrap
 */
std::optional<GpioLocker> gpioUnlock(const gpioplus::HandleInterface* handle);

} // namespace sensor
