#pragma once
#include "Thresholds.hpp"
#include "sensor.hpp"

#include <systemd/sd-journal.h>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <gpiod.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class PresenceSensor
{
  public:
    PresenceSensor(const std::string& pinName, bool inverted,
                   boost::asio::io_service& io, const std::string& name);
    ~PresenceSensor();

    void monitorPresence(void);
    void read(void);
    bool getValue(void);

  private:
    bool status = true;
    bool inverted;
    gpiod::line gpioLine;
    boost::asio::posix::stream_descriptor gpioFd;
    std::string name;
};

namespace redundancy
{
constexpr const char* full = "Full";
constexpr const char* degraded = "Degraded";
constexpr const char* failed = "Failed";
} // namespace redundancy

class RedundancySensor
{
  public:
    RedundancySensor(size_t count, const std::vector<std::string>& children,
                     sdbusplus::asio::object_server& objectServer,
                     const std::string& sensorConfiguration);
    ~RedundancySensor();

    void update(const std::string& name, bool failed);

  private:
    size_t count;
    std::string state = redundancy::full;
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> association;
    sdbusplus::asio::object_server& objectServer;
    boost::container::flat_map<std::string, bool> statuses;
};

class TachSensor : public Sensor
{
  public:
    TachSensor(const std::string& path, const std::string& objectType,
               sdbusplus::asio::object_server& objectServer,
               std::shared_ptr<sdbusplus::asio::connection>& conn,
               std::unique_ptr<PresenceSensor>&& presence,
               std::optional<RedundancySensor>* redundancy,
               boost::asio::io_service& io, const std::string& fanName,
               std::vector<thresholds::Threshold>&& thresholds,
               const std::string& sensorConfiguration,
               const std::pair<size_t, size_t>& limits);
    ~TachSensor();

  private:
    sdbusplus::asio::object_server& objServer;
    std::optional<RedundancySensor>* redundancy;
    std::unique_ptr<PresenceSensor> presence;
    std::shared_ptr<sdbusplus::asio::dbus_interface> itemIface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> itemAssoc;
    boost::asio::posix::stream_descriptor inputDev;
    boost::asio::deadline_timer waitTimer;
    boost::asio::streambuf readBuf;
    std::string path;
    size_t errCount;
    void setupRead(void);
    void handleResponse(const boost::system::error_code& err);
    void checkThresholds(void) override;
};

inline void logFanInserted(const std::string& device)
{
    sd_journal_send("MESSAGE=%s", "Fan Inserted", "PRIORITY=%i", LOG_ERR,
                    "REDFISH_MESSAGE_ID=%s", "OpenBMC.0.1.FanInserted",
                    "REDFISH_MESSAGE_ARGS=%s", device.c_str(), NULL);
}

inline void logFanRemoved(const std::string& device)
{
    sd_journal_send("MESSAGE=%s", "Fan Removed", "PRIORITY=%i", LOG_ERR,
                    "REDFISH_MESSAGE_ID=%s", "OpenBMC.0.1.FanRemoved",
                    "REDFISH_MESSAGE_ARGS=%s", device.c_str(), NULL);
}

inline void logFanRedundancyLost(void)
{
    sd_journal_send("MESSAGE=%s", "Fan Inserted", "PRIORITY=%i", LOG_ERR,
                    "REDFISH_MESSAGE_ID=%s", "OpenBMC.0.1.FanRedundancyLost",
                    NULL);
}

inline void logFanRedundancyRestored(void)
{
    sd_journal_send("MESSAGE=%s", "Fan Removed", "PRIORITY=%i", LOG_ERR,
                    "REDFISH_MESSAGE_ID=%s",
                    "OpenBMC.0.1.FanRedundancyRegained", NULL);
}
