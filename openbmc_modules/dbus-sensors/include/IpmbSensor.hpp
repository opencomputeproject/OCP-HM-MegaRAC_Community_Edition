#pragma once
#include "sensor.hpp"

#include <boost/asio/deadline_timer.hpp>
#include <boost/container/flat_map.hpp>

#include <chrono>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

enum class IpmbType
{
    meSensor,
    PXE1410CVR,
    IR38363VR,
    ADM1278HSC,
    mpsVR
};

enum class IpmbSubType
{
    temp,
    curr,
    power,
    volt,
    util
};

enum class ReadingFormat
{
    byte0,
    byte3,
    elevenBit,
    elevenBitShift,
};

struct IpmbSensor : public Sensor
{
    IpmbSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
               boost::asio::io_service& io, const std::string& name,
               const std::string& sensorConfiguration,
               sdbusplus::asio::object_server& objectServer,
               std::vector<thresholds::Threshold>&& thresholds,
               uint8_t deviceAddress, std::string& sensorTypeName);
    ~IpmbSensor();

    void checkThresholds(void) override;
    void read(void);
    void init(void);
    void loadDefaults(void);
    void runInitCmd(void);
    bool processReading(const std::vector<uint8_t>& data, double& resp);

    IpmbType type;
    IpmbSubType subType;
    double scaleVal;
    double offsetVal;
    uint8_t commandAddress;
    uint8_t netfn;
    uint8_t command;
    uint8_t deviceAddress;
    uint8_t errorCount;
    std::vector<uint8_t> commandData;
    std::optional<uint8_t> initCommand;
    std::vector<uint8_t> initData;

    ReadingFormat readingFormat;

  private:
    sdbusplus::asio::object_server& objectServer;
    std::shared_ptr<sdbusplus::asio::connection> dbusConnection;
    boost::asio::deadline_timer waitTimer;
};
