#include "elog-errors.hpp"
#include "error-HostEvent.hpp"
#include "sensorhandler.hpp"
#include "storagehandler.hpp"

#include <mapper.h>
#include <systemd/sd-bus.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <ipmid/api.hpp>
#include <ipmid/types.hpp>
#include <memory>
#include <phosphor-logging/elog.hpp>
#include <vector>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>

using namespace std;
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Logging::server;

std::string readESEL(const char* fileName)
{
    std::string content;
    std::ifstream handle(fileName);

    if (handle.fail())
    {
        log<level::ERR>("Failed to open eSEL", entry("FILENAME=%s", fileName));
        return content;
    }

    handle.seekg(0, std::ios::end);
    content.resize(handle.tellg());
    handle.seekg(0, std::ios::beg);
    handle.read(&content[0], content.size());
    handle.close();

    return content;
}

void createProcedureLogEntry(uint8_t procedureNum)
{
    // Read the eSEL data from the file.
    static constexpr auto eSELFile = "/tmp/esel";
    auto eSELData = readESEL(eSELFile);

    // Each byte in eSEL is formatted as %02x with a space between bytes and
    // insert '/0' at the end of the character array.
    static constexpr auto byteSeparator = 3;
    std::unique_ptr<char[]> data(
        new char[(eSELData.size() * byteSeparator) + 1]());

    for (size_t i = 0; i < eSELData.size(); i++)
    {
        sprintf(&data[i * byteSeparator], "%02x ", eSELData[i]);
    }
    data[eSELData.size() * byteSeparator] = '\0';

    using error = sdbusplus::org::open_power::Host::Error::MaintenanceProcedure;
    using metadata = org::open_power::Host::MaintenanceProcedure;

    report<error>(metadata::ESEL(data.get()),
                  metadata::PROCEDURE(static_cast<uint32_t>(procedureNum)));
}
