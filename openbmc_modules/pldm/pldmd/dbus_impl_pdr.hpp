#pragma once

#include "libpldm/pdr.h"
#include "libpldm/platform.h"

#include "xyz/openbmc_project/PLDM/PDR/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <vector>

namespace pldm
{
namespace dbus_api
{

using PdrIntf = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::PLDM::server::PDR>;

/** @class Pdr
 *  @brief OpenBMC PLDM.PDR Implementation
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.PLDM.PDR DBus APIs.
 */
class Pdr : public PdrIntf
{
  public:
    Pdr() = delete;
    Pdr(const Pdr&) = delete;
    Pdr& operator=(const Pdr&) = delete;
    Pdr(Pdr&&) = delete;
    Pdr& operator=(Pdr&&) = delete;
    virtual ~Pdr() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] path - Path to attach at.
     *  @param[in] repo - pointer to BMC's primary PDR repo
     */
    Pdr(sdbusplus::bus::bus& bus, const std::string& path,
        const pldm_pdr* repo) :
        PdrIntf(bus, path.c_str(), repo),
        pdrRepo(repo){};

    /** @brief Implementation for PdrIntf.FindStateEffecterPDR
     *  @param[in] tid - PLDM terminus ID.
     *  @param[in] entityID - entity that can be associated with PLDM State set.
     *  @param[in] stateSetId - value that identifies PLDM State set.
     */
    std::vector<std::vector<uint8_t>>
        findStateEffecterPDR(uint8_t tid, uint16_t entityID,
                             uint16_t stateSetId) override;

    /** @brief Implementation for PdrIntf.FindStateSensorPDR
     *  @param[in] tid - PLDM terminus ID.
     *  @param[in] entityID - entity that can be associated with PLDM State set.
     *  @param[in] stateSetId - value that identifies PLDM State set.
     */
    std::vector<std::vector<uint8_t>>
        findStateSensorPDR(uint8_t tid, uint16_t entityID,
                           uint16_t stateSetId) override;

  private:
    /** @brief pointer to BMC's primary PDR repo */
    const pldm_pdr* pdrRepo;
};

} // namespace dbus_api
} // namespace pldm
