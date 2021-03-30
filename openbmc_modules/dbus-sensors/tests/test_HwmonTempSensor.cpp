#include <HwmonTempSensor.hpp>
#include <dbus/connection.hpp>
#include <nlohmann/json.hpp>

#include <fstream>

#include "gtest/gtest.h"

TEST(HwmonTempSensor, TestTMP75)
{
    boost::asio::io_service io;
    auto system_bus =
        std::make_shared<dbus::connection>(io, dbus::bus::session);
    dbus::DbusObjectServer object_server(system_bus);

    std::vector<thresholds::Threshold> sensor_thresholds;
    auto t = thresholds::Threshold(thresholds::Level::CRITICAL,
                                   thresholds::Direction::LOW, 80);
    sensor_thresholds.emplace_back(t);

    std::ofstream test_file("test0.txt");
    test_file << "28\n";
    test_file.close();
    auto filename = std::string("test0.txt");
    auto tempsensname = std::string("test sensor");
    HwmonTempSensor test(filename, object_server, io, tempsensname,
                         std::move(sensor_thresholds));

    std::remove("test0.txt");
}

TEST(HwmonTempSensor, TestTMP421)
{
    boost::asio::io_service io;
    auto system_bus =
        std::make_shared<dbus::connection>(io, dbus::bus::session);
    dbus::DbusObjectServer object_server(system_bus);

    std::vector<thresholds::Threshold> sensor_thresholds;
    auto t = thresholds::Threshold(thresholds::Level::WARNING,
                                   thresholds::Direction::HIGH, 80);
    sensor_thresholds.emplace_back(t);

    std::ofstream test_file("test1.txt");
    test_file << "28\n";
    test_file.close();
    auto filename = std::string("test1.txt");
    auto tempsensname = std::string("test sensor");
    HwmonTempSensor test(filename, object_server, io, tempsensname,
                         std::move(sensor_thresholds));

    std::remove("test1.txt");
}
