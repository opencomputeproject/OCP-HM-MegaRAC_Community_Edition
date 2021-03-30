#pragma once

#include <shadow.h>

#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
namespace phosphor
{
namespace user
{
namespace shadow
{

using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using namespace phosphor::logging;

/** @class Lock
 *  @brief Responsible for locking and unlocking /etc/shadow
 */
class Lock
{
  public:
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
    Lock(Lock&&) = delete;
    Lock& operator=(Lock&&) = delete;

    /** @brief Default constructor that just locks the shadow file */
    Lock()
    {
        if (!lckpwdf())
        {
            log<level::ERR>("Locking Shadow failed");
            elog<InternalFailure>();
        }
    }
    ~Lock()
    {
        if (!ulckpwdf())
        {
            log<level::ERR>("Un-Locking Shadow failed");
            elog<InternalFailure>();
        }
    }
};

} // namespace shadow
} // namespace user
} // namespace phosphor
