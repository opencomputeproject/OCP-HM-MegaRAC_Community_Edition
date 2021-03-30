#include "guid.hpp"

#include <ipmid/api.h>
#include <mapper.h>

#include <phosphor-logging/log.hpp>
#include <sstream>
#include <string>

using namespace phosphor::logging;

namespace cache
{

command::Guid guid;

} // namespace cache

namespace command
{

std::unique_ptr<sdbusplus::bus::match_t> matchPtr(nullptr);

static constexpr auto guidObjPath = "/org/openbmc/control/chassis0";
static constexpr auto propInterface = "org.freedesktop.DBus.Properties";

Guid getSystemGUID()
{
    // Canned System GUID for QEMU where the Chassis DBUS object is not
    // populated
    Guid guid = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

    constexpr auto chassisIntf = "org.openbmc.control.Chassis";

    sd_bus_message* reply = nullptr;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus* bus = ipmid_get_sd_bus_connection();
    char* uuid = nullptr;
    char* busname = nullptr;

    do
    {
        int rc = mapper_get_service(bus, guidObjPath, &busname);
        if (rc < 0)
        {
            log<level::ERR>("Failed to get bus name",
                            entry("PATH=%s", guidObjPath),
                            entry("ERROR=%s", strerror(-rc)));
            break;
        }

        rc = sd_bus_call_method(bus, busname, guidObjPath, propInterface, "Get",
                                &error, &reply, "ss", chassisIntf, "uuid");
        if (rc < 0)
        {
            log<level::ERR>("Failed to call Get Method",
                            entry("ERROR=%s", strerror(-rc)));
            break;
        }

        rc = sd_bus_message_read(reply, "v", "s", &uuid);
        if (rc < 0 || uuid == NULL)
        {
            log<level::ERR>("Failed to get a response",
                            entry("ERROR=%s", strerror(-rc)));
            break;
        }

        std::string readUUID(uuid);
        auto len = readUUID.length();

        for (size_t iter = 0, inc = 0; iter < len && inc < BMC_GUID_LEN;
             iter += 2, inc++)
        {
            uint8_t hexVal =
                std::strtoul(readUUID.substr(iter, 2).c_str(), NULL, 16);
            guid[inc] = hexVal;
        }
    } while (0);

    sd_bus_error_free(&error);
    reply = sd_bus_message_unref(reply);
    free(busname);

    return guid;
}

void registerGUIDChangeCallback()
{
    if (matchPtr == nullptr)
    {
        using namespace sdbusplus::bus::match::rules;
        sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};

        matchPtr = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            path_namespace(guidObjPath) + type::signal() +
                member("PropertiesChanged") + interface(propInterface),
            [](sdbusplus::message::message&) {
                cache::guid = getSystemGUID();
            });
    }
}

} // namespace command
