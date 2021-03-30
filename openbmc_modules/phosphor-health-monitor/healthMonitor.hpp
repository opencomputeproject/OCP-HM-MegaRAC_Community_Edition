#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/Critical/server.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/Warning/server.hpp>
#include <xyz/openbmc_project/Sensor/Value/server.hpp>

#include <map>
#include <string>

namespace phosphor
{
namespace health
{

using Json = nlohmann::json;
using ValueIface = sdbusplus::xyz::openbmc_project::Sensor::server::Value;

using CriticalInterface =
    sdbusplus::xyz::openbmc_project::Sensor::Threshold::server::Critical;

using WarningInterface =
    sdbusplus::xyz::openbmc_project::Sensor::Threshold::server::Warning;

using healthIfaces =
    sdbusplus::server::object::object<ValueIface, CriticalInterface,
                                      WarningInterface>;

class HealthSensor : public healthIfaces
{
  public:
    HealthSensor() = delete;
    HealthSensor(const HealthSensor&) = delete;
    HealthSensor& operator=(const HealthSensor&) = delete;
    HealthSensor(HealthSensor&&) = delete;
    HealthSensor& operator=(HealthSensor&&) = delete;
    virtual ~HealthSensor() = default;

    /** @brief Constructs HealthSensor
     *
     * @param[in] bus     - Handle to system dbus
     * @param[in] objPath - The Dbus path of health sensor
     */
    HealthSensor(sdbusplus::bus::bus& bus, const char* objPath) :
        healthIfaces(bus, objPath), bus(bus)
    {}

    /** @brief Set sensor value utilization to health sensor D-bus  */
    void setSensorValueToDbus(const uint8_t value);
    /** @brief Set Sensor Threshold to D-bus at beginning */
    void setSensorThreshold(uint8_t criticalHigh, uint8_t warningHigh);

  private:
    sdbusplus::bus::bus& bus;
};

class HealthMon
{
  public:
    HealthMon() = delete;
    HealthMon(const HealthMon&) = delete;
    HealthMon& operator=(const HealthMon&) = delete;
    HealthMon(HealthMon&&) = delete;
    HealthMon& operator=(HealthMon&&) = delete;
    virtual ~HealthMon() = default;

    /** @brief Constructs HealthMon
     *
     * @param[in] bus     - Handle to system dbus
     */
    HealthMon(sdbusplus::bus::bus& bus) : bus(bus)
    {
        // read json file
        sensorConfigs = getHealthConfig();
        createHealthSensors();
    }

    struct HealthConfig
    {
        std::string name;
        uint16_t freq;
        uint16_t windowSize;
        uint8_t criticalHigh;
        uint8_t warningHigh;
        bool criticalLog;
        bool warningLog;
        std::string criticalTgt;
        std::string warningTgt;
    };

    /** @brief List of health sensors supported */
    std::vector<std::string> cfgNames = {"CPU", "Memory"};

    /** @brief Parsing Health config JSON file  */
    Json parseConfigFile(std::string configFile);

    /** @brief reading config for each health sensor component */
    void getConfigData(Json& data, HealthConfig& cfg);

    /** @brief Map of the object HealthSensor */
    std::unordered_map<std::string, std::shared_ptr<HealthSensor>>
        healthSensors;

    /** @brief Create sensors for health monitoring */
    void createHealthSensors();

  private:
    sdbusplus::bus::bus& bus;
    std::vector<HealthConfig> sensorConfigs;
    std::vector<HealthConfig> getHealthConfig();
};

} // namespace health
} // namespace phosphor
