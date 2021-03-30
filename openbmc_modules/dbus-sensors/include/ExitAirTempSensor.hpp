#pragma once
#include "sensor.hpp"

#include <boost/container/flat_map.hpp>
#include <sdbusplus/bus/match.hpp>

#include <chrono>
#include <limits>
#include <memory>
#include <string>
#include <vector>

struct ExitAirTempSensor;
struct CFMSensor : public Sensor, std::enable_shared_from_this<CFMSensor>
{
    std::vector<std::string> tachs;
    double c1;
    double c2;
    double maxCFM;
    double tachMinPercent;
    double tachMaxPercent;

    std::shared_ptr<ExitAirTempSensor> parent;

    CFMSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
              const std::string& name, const std::string& sensorConfiguration,
              sdbusplus::asio::object_server& objectServer,
              std::vector<thresholds::Threshold>&& thresholds,
              std::shared_ptr<ExitAirTempSensor>& parent);
    ~CFMSensor();

    bool calculate(double&);
    void updateReading(void);
    void setupMatches(void);
    void createMaxCFMIface(void);
    void addTachRanges(const std::string& serviceName, const std::string& path);
    void checkThresholds(void) override;
    uint64_t getMaxRpm(uint64_t cfmMax);

  private:
    std::vector<sdbusplus::bus::match::match> matches;
    boost::container::flat_map<std::string, double> tachReadings;
    boost::container::flat_map<std::string, std::pair<double, double>>
        tachRanges;
    std::shared_ptr<sdbusplus::asio::connection> dbusConnection;
    std::shared_ptr<sdbusplus::asio::dbus_interface> pwmLimitIface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> cfmLimitIface;
    sdbusplus::asio::object_server& objServer;
};

struct ExitAirTempSensor :
    public Sensor,
    std::enable_shared_from_this<ExitAirTempSensor>
{

    double powerFactorMin;
    double powerFactorMax;
    double qMin;
    double qMax;
    double alphaS;
    double alphaF;
    double pOffset = 0;

    // todo: make this private once we don't have to hack in a reading
    boost::container::flat_map<std::string, double> powerReadings;

    ExitAirTempSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                      const std::string& name,
                      const std::string& sensorConfiguration,
                      sdbusplus::asio::object_server& objectServer,
                      std::vector<thresholds::Threshold>&& thresholds);
    ~ExitAirTempSensor();

    void checkThresholds(void) override;
    void updateReading(void);
    void setupMatches(void);

  private:
    double lastReading;

    std::vector<sdbusplus::bus::match::match> matches;
    double inletTemp = std::numeric_limits<double>::quiet_NaN();

    std::shared_ptr<sdbusplus::asio::connection> dbusConnection;
    sdbusplus::asio::object_server& objServer;
    std::chrono::time_point<std::chrono::system_clock> lastTime;
    double getTotalCFM(void);
    bool calculate(double& val);
};
