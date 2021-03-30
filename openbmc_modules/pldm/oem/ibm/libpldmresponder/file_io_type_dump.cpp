#include "file_io_type_dump.hpp"

#include "libpldm/base.h"
#include "oem/ibm/libpldm/file_io.h"

#include "common/utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <stdint.h>
#include <systemd/sd-bus.h>
#include <unistd.h>

#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Dump/NewDump/server.hpp>

#include <exception>
#include <filesystem>
#include <iostream>

using namespace pldm::utils;

namespace pldm
{
namespace responder
{

static constexpr auto nbdInterfaceDefault = "/dev/nbd1";
static constexpr auto dumpEntry = "xyz.openbmc_project.Dump.Entry";
static constexpr auto dumpObjPath = "/xyz/openbmc_project/dump";

int DumpHandler::fd = -1;

static std::string findDumpObjPath(uint32_t fileHandle)
{
    static constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
    static constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
    static constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
    static constexpr auto systemDumpEntry =
        "xyz.openbmc_project.Dump.Entry.System";
    auto& bus = pldm::utils::DBusHandler::getBus();

    try
    {
        std::vector<std::string> paths;
        auto method = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                          MAPPER_INTERFACE, "GetSubTreePaths");
        method.append(dumpObjPath);
        method.append(0);
        method.append(std::vector<std::string>({systemDumpEntry}));
        auto reply = bus.call(method);
        reply.read(paths);
        for (const auto& path : paths)
        {
            auto dumpId = pldm::utils::DBusHandler().getDbusProperty<uint32_t>(
                path.c_str(), "SourceDumpId", systemDumpEntry);
            if (dumpId == fileHandle)
            {
                return path;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "failed to make a d-bus call to DUMP manager, ERROR="
                  << e.what() << "\n";
    }

    std::cerr << "failed to find dump object for dump id " << fileHandle
              << "\n";
    return {};
}

int DumpHandler::newFileAvailable(uint64_t length)
{
    static constexpr auto dumpInterface = "xyz.openbmc_project.Dump.NewDump";
    auto& bus = pldm::utils::DBusHandler::getBus();

    try
    {
        auto service =
            pldm::utils::DBusHandler().getService(dumpObjPath, dumpInterface);
        using namespace sdbusplus::xyz::openbmc_project::Dump::server;
        auto method = bus.new_method_call(service.c_str(), dumpObjPath,
                                          dumpInterface, "Notify");
        method.append(
            sdbusplus::xyz::openbmc_project::Dump::server::convertForMessage(
                NewDump::DumpType::System),
            fileHandle, length);
        bus.call_noreply(method);
    }
    catch (const std::exception& e)
    {
        std::cerr << "failed to make a d-bus call to DUMP manager, ERROR="
                  << e.what() << "\n";
        return PLDM_ERROR;
    }

    return PLDM_SUCCESS;
}

static std::string getOffloadUri(uint32_t fileHandle)
{
    auto path = findDumpObjPath(fileHandle);
    if (path.empty())
    {
        return {};
    }

    std::string nbdInterface{};
    try
    {
        nbdInterface = pldm::utils::DBusHandler().getDbusProperty<std::string>(
            path.c_str(), "OffloadUri", dumpEntry);
    }
    catch (const std::exception& e)
    {
        std::cerr << "failed to make a d-bus call to DUMP manager, ERROR="
                  << e.what() << "\n";
    }

    if (nbdInterface == "")
    {
        nbdInterface = nbdInterfaceDefault;
    }

    return nbdInterface;
}

int DumpHandler::writeFromMemory(uint32_t offset, uint32_t length,
                                 uint64_t address)
{
    auto nbdInterface = getOffloadUri(fileHandle);
    if (nbdInterface.empty())
    {
        return PLDM_ERROR;
    }

    int flags = O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE;
    if (DumpHandler::fd == -1)
    {
        DumpHandler::fd = open(nbdInterface.c_str(), flags);
        if (DumpHandler::fd == -1)
        {
            std::cerr << "NBD file does not exist at " << nbdInterface
                      << " ERROR=" << errno << "\n";
            return PLDM_ERROR;
        }
    }

    return transferFileData(DumpHandler::fd, false, offset, length, address);
}

int DumpHandler::write(const char* buffer, uint32_t offset, uint32_t& length)
{
    auto nbdInterface = getOffloadUri(fileHandle);
    if (nbdInterface.empty())
    {
        return PLDM_ERROR;
    }

    int flags = O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE;
    if (DumpHandler::fd == -1)
    {
        DumpHandler::fd = open(nbdInterface.c_str(), flags);
        if (DumpHandler::fd == -1)
        {
            std::cerr << "NBD file does not exist at " << nbdInterface
                      << " ERROR=" << errno << "\n";
            return PLDM_ERROR;
        }
    }

    int rc = lseek(DumpHandler::fd, offset, SEEK_SET);
    if (rc == -1)
    {
        std::cerr << "lseek failed, ERROR=" << errno << ", OFFSET=" << offset
                  << "\n";
        return PLDM_ERROR;
    }
    rc = ::write(DumpHandler::fd, buffer, length);
    if (rc == -1)
    {
        std::cerr << "file write failed, ERROR=" << errno
                  << ", LENGTH=" << length << ", OFFSET=" << offset << "\n";
        return PLDM_ERROR;
    }
    length = rc;

    return PLDM_SUCCESS;
}

int DumpHandler::fileAck(uint8_t /*fileStatus*/)
{
    if (DumpHandler::fd >= 0)
    {
        auto path = findDumpObjPath(fileHandle);
        if (!path.empty())
        {
            PropertyValue value{true};
            DBusMapping dbusMapping{path, dumpEntry, "Offloaded", "bool"};
            try
            {
                pldm::utils::DBusHandler().setDbusProperty(dbusMapping, value);
            }
            catch (const std::exception& e)
            {
                std::cerr
                    << "failed to make a d-bus call to DUMP manager, ERROR="
                    << e.what() << "\n";
            }
            close(DumpHandler::fd);
            DumpHandler::fd = -1;
            return PLDM_SUCCESS;
        }
    }

    return PLDM_ERROR;
}

} // namespace responder
} // namespace pldm
