#pragma once

#include "config.h"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/Critical/server.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/Warning/server.hpp>
#include <xyz/openbmc_project/Sensor/Value/server.hpp>

namespace phosphor
{
namespace nvme
{

using ValueIface = sdbusplus::xyz::openbmc_project::Sensor::server::Value;

using CriticalInterface =
    sdbusplus::xyz::openbmc_project::Sensor::Threshold::server::Critical;

using WarningInterface =
    sdbusplus::xyz::openbmc_project::Sensor::Threshold::server::Warning;

using NvmeIfaces =
    sdbusplus::server::object::object<ValueIface, CriticalInterface,
                                      WarningInterface>;

class NvmeSSD : public NvmeIfaces
{
  public:
    NvmeSSD() = delete;
    NvmeSSD(const NvmeSSD&) = delete;
    NvmeSSD& operator=(const NvmeSSD&) = delete;
    NvmeSSD(NvmeSSD&&) = delete;
    NvmeSSD& operator=(NvmeSSD&&) = delete;
    virtual ~NvmeSSD() = default;

    /** @brief Constructs NvmeSSD
     *
     * @param[in] bus     - Handle to system dbus
     * @param[in] objPath - The Dbus path of nvme
     */
    NvmeSSD(sdbusplus::bus::bus& bus, const char* objPath) :
        NvmeIfaces(bus, objPath), bus(bus)
    {
    }

    /** @brief Set sensor value temperature to nvme D-bus  */
    void setSensorValueToDbus(const int8_t value);
    /** @brief Check if sensor value higher or lower threshold */
    void checkSensorThreshold();
    /** @brief Set Sensor Threshold to D-bus at beginning */
    void setSensorThreshold(int8_t criticalHigh, int8_t criticalLow,
                            int8_t maxValue, int8_t minValue,
                            int8_t warningHigh, int8_t warningLow);

  private:
    sdbusplus::bus::bus& bus;
};
} // namespace nvme
} // namespace phosphor