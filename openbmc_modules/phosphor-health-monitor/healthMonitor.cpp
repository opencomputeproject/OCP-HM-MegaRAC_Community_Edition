#include "config.h"

#include "healthMonitor.hpp"

#include <phosphor-logging/log.hpp>
#include <sdeventplus/event.hpp>

#include <fstream>
#include <iostream>

static constexpr bool DEBUG = false;

namespace phosphor
{
namespace health
{

using namespace phosphor::logging;

void HealthSensor::setSensorThreshold(uint8_t criticalHigh, uint8_t warningHigh)
{
    CriticalInterface::criticalHigh(criticalHigh);
    WarningInterface::warningHigh(warningHigh);
}

void HealthSensor::setSensorValueToDbus(const uint8_t value)
{
    ValueIface::value(value);
}

/* Create dbus utilization sensor object for each configured sensors */
void HealthMon::createHealthSensors()
{
    for (auto& cfg : sensorConfigs)
    {
        std::string objPath = std::string(HEALTH_SENSOR_PATH) + cfg.name;
        auto healthSensor =
            std::make_shared<HealthSensor>(bus, objPath.c_str());
        healthSensors.emplace(cfg.name, healthSensor);

        std::string logMsg = cfg.name + " Health Sensor created";
        log<level::INFO>(logMsg.c_str(), entry("NAME = %s", cfg.name.c_str()));

        /* Set configured values of crtical and warning high to dbus */
        healthSensor->setSensorThreshold(cfg.criticalHigh, cfg.warningHigh);
    }
}

/** @brief Parsing Health config JSON file  */
Json HealthMon::parseConfigFile(std::string configFile)
{
    std::ifstream jsonFile(configFile);
    if (!jsonFile.is_open())
    {
        log<level::ERR>("config JSON file not found",
                        entry("FILENAME = %s", configFile.c_str()));
    }

    auto data = Json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        log<level::ERR>("config readings JSON parser failure",
                        entry("FILENAME = %s", configFile.c_str()));
    }

    return data;
}

void printConfig(HealthMon::HealthConfig& cfg)
{
    std::cout << "Name: " << cfg.name << "\n";
    std::cout << "Freq: " << cfg.freq << "\n";
    std::cout << "Window Size: " << cfg.windowSize << "\n";
    std::cout << "Critical value: " << cfg.criticalHigh << "\n";
    std::cout << "warning value: " << cfg.warningHigh << "\n";
    std::cout << "Critical log: " << cfg.criticalLog << "\n";
    std::cout << "Warning log: " << cfg.warningLog << "\n";
    std::cout << "Critical Target: " << cfg.criticalTgt << "\n";
    std::cout << "Warning Target: " << cfg.warningTgt << "\n\n";
}

void HealthMon::getConfigData(Json& data, HealthConfig& cfg)
{

    static const Json empty{};

    cfg.freq = data.value("Frequency", 0);
    cfg.windowSize = data.value("Window_size", 0);
    auto threshold = data.value("Threshold", empty);
    if (!threshold.empty())
    {
        auto criticalData = threshold.value("Critical", empty);
        if (!criticalData.empty())
        {
            cfg.criticalHigh = criticalData.value("Value", 0);
            cfg.criticalLog = criticalData.value("Log", true);
            cfg.criticalTgt = criticalData.value("Target", "");
        }
        auto warningData = threshold.value("Warning", empty);
        if (!warningData.empty())
        {
            cfg.warningHigh = warningData.value("Value", 0);
            cfg.warningLog = warningData.value("Log", true);
            cfg.warningTgt = warningData.value("Target", "");
        }
    }
}

std::vector<HealthMon::HealthConfig> HealthMon::getHealthConfig()
{

    std::vector<HealthConfig> cfgs;
    HealthConfig cfg;
    auto data = parseConfigFile(HEALTH_CONFIG_FILE);

    // print values
    if (DEBUG)
        std::cout << "Config json data:\n" << data << "\n\n";

    /* Get CPU config data */
    for (auto& j : data.items())
    {
        auto key = j.key();
        if (std::find(cfgNames.begin(), cfgNames.end(), key) != cfgNames.end())
        {
            HealthConfig cfg = HealthConfig();
            cfg.name = j.key();
            getConfigData(j.value(), cfg);
            cfgs.push_back(cfg);
            if (DEBUG)
                printConfig(cfg);
        }
        else
        {
            std::string logMsg = key + " Health Sensor not supported";
            log<level::ERR>(logMsg.c_str(), entry("NAME = %s", key.c_str()));
        }
    }
    return cfgs;
}

} // namespace health
} // namespace phosphor

/**
 * @brief Main
 */
int main()
{

    // Get a default event loop
    auto event = sdeventplus::Event::get_default();

    // Get a handle to system dbus
    auto bus = sdbusplus::bus::new_default();

    // Create an health monitor object
    phosphor::health::HealthMon healthMon(bus);

    // Request service bus name
    bus.request_name(HEALTH_BUS_NAME);

    // Attach the bus to sd_event to service user requests
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    event.loop();

    return 0;
}
