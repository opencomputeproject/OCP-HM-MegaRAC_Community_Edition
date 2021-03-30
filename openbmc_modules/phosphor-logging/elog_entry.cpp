#include "elog_entry.hpp"

#include "elog_serialize.hpp"
#include "log_manager.hpp"

namespace phosphor
{
namespace logging
{

// TODO Add interfaces to handle the error log id numbering

void Entry::delete_()
{
    parent.erase(id());
}

bool Entry::resolved(bool value)
{
    auto current =
        sdbusplus::xyz::openbmc_project::Logging::server::Entry::resolved();
    if (value != current)
    {
        value ? associations({}) : associations(assocs);
        current =
            sdbusplus::xyz::openbmc_project::Logging::server::Entry::resolved(
                value);

        uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
        updateTimestamp(ms);

        serialize(*this);
    }

    return current;
}

} // namespace logging
} // namespace phosphor
