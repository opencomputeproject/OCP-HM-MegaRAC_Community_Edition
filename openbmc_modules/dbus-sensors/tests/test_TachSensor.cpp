#include <TachSensor.hpp>
#include <Thresholds.hpp>
#include <dbus/connection.hpp>
#include <nlohmann/json.hpp>

#include <fstream>

#include "gtest/gtest.h"

TEST(TachSensor, TestTachSensor)
{
    boost::asio::io_service io;
    auto system_bus =
        std::make_shared<dbus::connection>(io, dbus::bus::session);
    dbus::DbusObjectServer object_server(system_bus);

    std::vector<thresholds::Threshold> sensor_thresholds;
    auto t = thresholds::Threshold(thresholds::Level::CRITICAL,
                                   thresholds::Direction::LOW, 1000);
    sensor_thresholds.emplace_back(t);

    std::ofstream test_file("test.txt");
    test_file << "10000\n";
    test_file.close();
    auto filename = std::string("test.txt");
    auto fanname = std::string("test fan");
    TachSensor test(filename, object_server, system_bus, io, fanname,
                    std::move(sensor_thresholds));
    std::remove("test.txt");
}
