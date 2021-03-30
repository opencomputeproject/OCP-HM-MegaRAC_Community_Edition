#include <iostream>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <string>
#include <variant>

/* Fan Control */
static constexpr auto objectPath = "/xyz/openbmc_project/settings/fanctrl/zone";
static constexpr auto busName = "xyz.openbmc_project.State.FanCtrl";
static constexpr auto intf = "xyz.openbmc_project.Control.Mode";
static constexpr auto property = "Manual";
using Value = std::variant<bool>;

/* Host Sensor. */
static constexpr auto sobjectPath =
    "/xyz/openbmc_project/extsensors/margin/sluggish0";
static constexpr auto sbusName = "xyz.openbmc_project.Hwmon.external";
static constexpr auto sintf = "xyz.openbmc_project.Sensor.Value";
static constexpr auto sproperty = "Value";
using sValue = std::variant<int64_t>;

static constexpr auto propertiesintf = "org.freedesktop.DBus.Properties";

static void SetHostSensor(void)
{
    int64_t value = 300;
    sValue v{value};

    std::string busname{sbusName};
    auto PropertyWriteBus = sdbusplus::bus::new_system();
    std::string path{sobjectPath};

    auto pimMsg = PropertyWriteBus.new_method_call(
        busname.c_str(), path.c_str(), propertiesintf, "Set");

    pimMsg.append(sintf);
    pimMsg.append(sproperty);
    pimMsg.append(v);

    try
    {
        auto responseMsg = PropertyWriteBus.call(pimMsg);
        fprintf(stderr, "call to Set the host sensor value succeeded.\n");
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        fprintf(stderr, "call to Set the host sensor value failed.\n");
    }
}

static std::string GetControlPath(int8_t zone)
{
    return std::string(objectPath) + std::to_string(zone);
}

static void SetManualMode(int8_t zone)
{
    bool setValue = (bool)0x01;

    Value v{setValue};

    std::string busname{busName};
    auto PropertyWriteBus = sdbusplus::bus::new_system();
    std::string path = GetControlPath(zone);

    auto pimMsg = PropertyWriteBus.new_method_call(
        busname.c_str(), path.c_str(), propertiesintf, "Set");

    pimMsg.append(intf);
    pimMsg.append(property);
    pimMsg.append(v);

    try
    {
        auto responseMsg = PropertyWriteBus.call(pimMsg);
        fprintf(stderr, "call to Set the manual mode succeeded.\n");
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        fprintf(stderr, "call to Set the manual mode failed.\n");
    }
}

int main(int argc, char* argv[])
{
    int rc = 0;

    int64_t zone = 0x01;

    SetManualMode(zone);
    SetHostSensor();
    return rc;
}
