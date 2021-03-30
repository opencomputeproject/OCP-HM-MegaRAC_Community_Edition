#include "sensorhandler.hpp"

#include <errno.h>
#include <limits.h>
#include <mapper.h>
#include <stdint.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

extern void send_esel(uint16_t recordid);

sd_bus* bus = NULL;

// Use a lookup table to find the interface name of a specific sensor
// This will be used until an alternative is found.  this is the first
// step for mapping IPMI
int find_openbmc_path(const uint8_t num, dbus_interface_t* interface)
{

    const char* objname = "/org/openbmc/managers/System";

    char *str1, *str2, *str3;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL, *m = NULL;
    int r;
    char* busname = NULL;

    r = mapper_get_service(bus, objname, &busname);
    if (r < 0)
    {
        log<level::ERR>("Failed to get busname", entry("BUS=%s", objname),
                        entry("ERRNO=0x%X", -r));
        goto final;
    }

    r = sd_bus_message_new_method_call(bus, &m, busname, objname, busname,
                                       "getObjectFromByteId");
    if (r < 0)
    {
        log<level::ERR>("Failed to create a method call",
                        entry("ERRNO=0x%X", -r));
    }

    r = sd_bus_message_append(m, "sy", type, num);
    if (r < 0)
    {
        log<level::ERR>("Failed to create a input parameter",
                        entry("ERRNO=0x%X", -r));
    }

    // Call the IPMI responder on the bus so the message can be sent to the CEC
    r = sd_bus_call(bus, m, 0, &error, &reply);
    if (r < 0)
    {
        log<level::ERR>("Failed to call the method", entry("ERRNO=0x%X", -r));
        goto final;
    }

    r = sd_bus_message_read(reply, "(sss)", &str1, &str2, &str3);
    if (r < 0)
    {
        log<level::ERR>("Failed to get a response", entry("ERRNO=0x%X", -r));
        goto final;
    }

    strncpy(interface->bus, str1, MAX_DBUS_PATH);
    strncpy(interface->path, str2, MAX_DBUS_PATH);
    strncpy(interface->interface, str3, MAX_DBUS_PATH);

    interface->sensornumber = num;

final:

    sd_bus_error_free(&error);
    sd_bus_message_unref(m);
    free(busname);

    return r;
}

int main(int argc, char* argv[])
{
    int base;
    char *endptr, *str;
    long val;
    uint16_t num;
    int r;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s sensornumber\n", argv[0]);
        return -1;
    }

    str = argv[1];
    base = (argc > 2) ? atoi(argv[2]) : 10;

    val = strtol(str, &endptr, base);

    num = (uint16_t)val;

    /* Connect to system bus */
    r = sd_bus_open_system(&bus);
    if (r < 0)
    {
        log<level::ERR>("Failed to connect to system bus",
                        entry("ERRNO=0x%X", -r));
        goto finish;
    }

    send_esel(num);

finish:
    sd_bus_unref(bus);

    return 0;
}
