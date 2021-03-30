#pragma once
#include "sensor.hpp"

#include <boost/asio/deadline_timer.hpp>
#include <boost/container/flat_map.hpp>

#include <chrono>
#include <limits>
#include <memory>
#include <string>
#include <vector>

struct MCUTempSensor : public Sensor
{
    MCUTempSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                  boost::asio::io_service& io, const std::string& name,
                  const std::string& sensorConfiguration,
                  sdbusplus::asio::object_server& objectServer,
                  std::vector<thresholds::Threshold>&& thresholds,
                  uint8_t busId, uint8_t mcuAddress, uint8_t tempReg);
    ~MCUTempSensor();

    void checkThresholds(void) override;
    void read(void);
    void init(void);

    uint8_t busId;
    uint8_t mcuAddress;
    uint8_t tempReg;

  private:
    int getMCURegsInfoWord(uint8_t regs, int16_t* pu16data);
    sdbusplus::asio::object_server& objectServer;
    std::shared_ptr<sdbusplus::asio::connection> dbusConnection;
    boost::asio::deadline_timer waitTimer;
};
