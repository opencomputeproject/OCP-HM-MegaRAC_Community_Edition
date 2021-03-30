#include "bmc_dump_entry.hpp"

#include "dump_manager.hpp"
#include "dump_offload.hpp"

#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{
namespace bmc
{
using namespace phosphor::logging;

void Entry::delete_()
{
    // Delete Dump file from Permanent location
    try
    {
        fs::remove_all(file.parent_path());
    }
    catch (fs::filesystem_error& e)
    {
        // Log Error message and continue
        log<level::ERR>(e.what());
    }

    // Remove Dump entry D-bus object
    phosphor::dump::Entry::delete_();
}

void Entry::initiateOffload(std::string uri)
{
    phosphor::dump::offload::requestOffload(file, id, uri);
    offloaded(true);
}

} // namespace bmc
} // namespace dump
} // namespace phosphor
