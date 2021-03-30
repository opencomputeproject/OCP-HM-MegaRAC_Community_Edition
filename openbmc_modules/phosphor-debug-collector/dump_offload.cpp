#include "config.h"

#include "dump_offload.hpp"

#include <fstream>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace dump
{
namespace offload
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

void requestOffload(fs::path file, uint32_t dumpId, std::string writePath)
{
    using namespace sdbusplus::xyz::openbmc_project::Common::File::Error;
    using ErrnoOpen = xyz::openbmc_project::Common::File::Open::ERRNO;
    using PathOpen = xyz::openbmc_project::Common::File::Open::PATH;
    using ErrnoWrite = xyz::openbmc_project::Common::File::Write::ERRNO;
    using PathWrite = xyz::openbmc_project::Common::File::Write::PATH;

    // open a dump file for a transfer.
    fs::path dumpPath(BMC_DUMP_PATH);
    dumpPath /= std::to_string(dumpId);
    dumpPath /= file.filename();

    std::ifstream infile{dumpPath, std::ios::in | std::ios::binary};
    if (!infile.good())
    {
        // Unable to open the dump file
        log<level::ERR>("Failed to open the dump from file ",
                        entry("ERR=%d", errno),
                        entry("DUMPFILE=%s", dumpPath.c_str()));
        elog<InternalFailure>();
    }

    std::ofstream outfile{writePath, std::ios::out | std::ios::binary};
    if (!outfile.good())
    {
        // Unable to open the write interface
        auto err = errno;
        log<level::ERR>("Write device path does not exist at ",
                        entry("ERR=%d", errno),
                        entry("WRITEINTERFACE=%s", writePath.c_str()));
        elog<Open>(ErrnoOpen(err), PathOpen(writePath.c_str()));
    }

    infile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                      std::ifstream::eofbit);
    outfile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                       std::ifstream::eofbit);

    try
    {
        log<level::INFO>("Opening File for RW ",
                         entry("FILENAME=%s", file.filename().c_str()));
        outfile << infile.rdbuf() << std::flush;
        infile.close();
        outfile.close();
    }
    catch (std::ofstream::failure& oe)
    {
        auto err = errno;
        log<level::ERR>("Failed to write to write interface ",
                        entry("ERR=%s", oe.what()),
                        entry("WRITEINTERFACE=%s", writePath.c_str()));
        elog<Write>(ErrnoWrite(err), PathWrite(writePath.c_str()));
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Failed get the dump from file ",
                        entry("ERR=%s", e.what()),
                        entry("DUMPFILE=%s", dumpPath.c_str()));
        elog<InternalFailure>();
    }
}

} // namespace offload
} // namespace dump
} // namespace phosphor
