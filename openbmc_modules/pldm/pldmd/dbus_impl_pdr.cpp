#include "dbus_impl_pdr.hpp"

#include "libpldm/pdr.h"
#include "libpldm/pldm_types.h"

#include "common/utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <iostream>

using namespace sdbusplus::xyz::openbmc_project::Common::Error;

namespace pldm
{
namespace dbus_api
{

std::vector<std::vector<uint8_t>> Pdr::findStateEffecterPDR(uint8_t tid,
                                                            uint16_t entityID,
                                                            uint16_t stateSetId)
{
    auto pdrs =
        pldm::utils::findStateEffecterPDR(tid, entityID, stateSetId, pdrRepo);

    if (pdrs.empty())
    {
        throw ResourceNotFound();
    }

    return pdrs;
}

std::vector<std::vector<uint8_t>>
    Pdr::findStateSensorPDR(uint8_t tid, uint16_t entityID, uint16_t stateSetId)
{
    auto pdrs =
        pldm::utils::findStateSensorPDR(tid, entityID, stateSetId, pdrRepo);
    if (pdrs.empty())
    {
        throw ResourceNotFound();
    }
    return pdrs;
}
} // namespace dbus_api
} // namespace pldm
