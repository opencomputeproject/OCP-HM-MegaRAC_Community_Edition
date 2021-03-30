#include "argument.hpp"
#include "gpio_presence.hpp"

#include <systemd/sd-event.h>

#include <iostream>
#include <phosphor-logging/log.hpp>

using namespace phosphor::logging;
using namespace phosphor::gpio;
using namespace phosphor::gpio::presence;

/**
 * Pulls out the path,device pairs from the string
 * passed in
 *
 * @param[in] driverString - space separated path,device pairs
 * @param[out] drivers - vector of device,path tuples filled in
 *                       from driverString
 *
 * @return int - 0 if successful, < 0 else
 */
static int getDrivers(const std::string& driverString,
                      std::vector<Driver>& drivers)
{
    std::istringstream stream{driverString};

    while (true)
    {
        std::string entry;

        // Extract each path,device pair
        stream >> entry;

        if (entry.empty())
        {
            break;
        }

        // Extract the path and device and save them
        auto pos = entry.rfind(',');
        if (pos != std::string::npos)
        {
            auto path = entry.substr(0, pos);
            auto device = entry.substr(pos + 1);

            drivers.emplace_back(device, path);
        }
        else
        {
            std::cerr << "Invalid path,device combination: " << entry << "\n";
            return -1;
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    auto options = ArgumentParser(argc, argv);

    auto inventory = options["inventory"];
    auto key = options["key"];
    auto path = options["path"];
    auto drivers = options["drivers"];
    auto ifaces = options["extra-ifaces"];
    if (argc < 4)
    {
        std::cerr << "Too few arguments\n";
        options.usage(argv);
    }

    if (inventory == ArgumentParser::emptyString)
    {
        std::cerr << "Inventory argument required\n";
        options.usage(argv);
    }

    if (key == ArgumentParser::emptyString)
    {
        std::cerr << "GPIO key argument required\n";
        options.usage(argv);
    }

    if (path == ArgumentParser::emptyString)
    {
        std::cerr << "Device path argument required\n";
        options.usage(argv);
    }

    std::vector<Driver> driverList;

    // Driver list is optional
    if (drivers != ArgumentParser::emptyString)
    {
        if (getDrivers(drivers, driverList) < 0)
        {
            options.usage(argv);
        }
    }

    std::vector<Interface> ifaceList;

    // Extra interfaces list is optional
    if (ifaces != ArgumentParser::emptyString)
    {
        std::stringstream ss(ifaces);
        Interface iface;
        while (std::getline(ss, iface, ','))
        {
            ifaceList.push_back(iface);
        }
    }

    auto bus = sdbusplus::bus::new_default();
    auto rc = 0;
    sd_event* event = nullptr;
    rc = sd_event_default(&event);
    if (rc < 0)
    {
        log<level::ERR>("Error creating a default sd_event handler");
        return rc;
    }
    EventPtr eventP{event};
    event = nullptr;

    auto name = options["name"];
    Presence presence(bus, inventory, path, std::stoul(key), name, eventP,
                      driverList, ifaceList);

    while (true)
    {
        // -1 denotes wait forever
        rc = sd_event_run(eventP.get(), (uint64_t)-1);
        if (rc < 0)
        {
            log<level::ERR>("Failure in processing request",
                            entry("ERROR=%s", strerror(-rc)));
            break;
        }
    }
    return rc;
}
