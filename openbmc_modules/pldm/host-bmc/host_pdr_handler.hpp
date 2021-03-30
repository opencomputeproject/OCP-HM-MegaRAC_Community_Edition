#pragma once

#include "libpldm/base.h"
#include "libpldm/platform.h"

#include "common/types.hpp"
#include "common/utils.hpp"
#include "libpldmresponder/pdr_utils.hpp"
#include "pldmd/dbus_impl_requester.hpp"

#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>

#include <deque>
#include <map>
#include <memory>
#include <vector>

using namespace pldm::dbus_api;

namespace pldm
{

using EntityType = uint16_t;
// vector which would hold the PDR record handle data returned by
// pldmPDRRepositoryChgEvent event data
using ChangeEntry = uint32_t;
using PDRRecordHandles = std::deque<ChangeEntry>;

/** @struct SensorEntry
 *
 *  SensorEntry is a unique key which maps a sensorEventType request in the
 *  PlatformEventMessage command to a host sensor PDR. This struct is a key
 *  in a std::map, so implemented operator==and operator<.
 */
struct SensorEntry
{
    pdr::TerminusID terminusID;
    pdr::SensorID sensorID;

    bool operator==(const SensorEntry& e) const
    {
        return ((terminusID == e.terminusID) && (sensorID == e.sensorID));
    }

    bool operator<(const SensorEntry& e) const
    {
        return ((terminusID < e.terminusID) ||
                ((terminusID == e.terminusID) && (sensorID < e.sensorID)));
    }
};

using HostStateSensorMap = std::map<SensorEntry, pdr::SensorInfo>;

/** @class HostPDRHandler
 *  @brief This class can fetch and process PDRs from host firmware
 *  @details Provides an API to fetch PDRs from the host firmware. Upon
 *  receiving the PDRs, they are stored into the BMC's primary PDR repo.
 *  Adjustments are made to entity association PDRs received from the host,
 *  because they need to be assimilated into the BMC's entity association
 *  tree. A PLDM event containing the record handles of the updated entity
 *  association PDRs is sent to the host.
 */
class HostPDRHandler
{
  public:
    HostPDRHandler() = delete;
    HostPDRHandler(const HostPDRHandler&) = delete;
    HostPDRHandler(HostPDRHandler&&) = delete;
    HostPDRHandler& operator=(const HostPDRHandler&) = delete;
    HostPDRHandler& operator=(HostPDRHandler&&) = delete;
    ~HostPDRHandler() = default;

    /** @brief Constructor
     *  @param[in] mctp_fd - fd of MCTP communications socket
     *  @param[in] mctp_eid - MCTP EID of host firmware
     *  @param[in] event - reference of main event loop of pldmd
     *  @param[in] repo - pointer to BMC's primary PDR repo
     *  @param[in] tree - pointer to BMC's entity association tree
     *  @param[in] requester - reference to Requester object
     */
    explicit HostPDRHandler(int mctp_fd, uint8_t mctp_eid,
                            sdeventplus::Event& event, pldm_pdr* repo,
                            pldm_entity_association_tree* entityTree,
                            Requester& requester);

    /** @brief fetch PDRs from host firmware. See @class.
     *  @param[in] recordHandles - list of record handles pointing to host's
     *             PDRs that need to be fetched.
     */

    void fetchPDR(PDRRecordHandles&& recordHandles);

    /** @brief Send a PLDM event to host firmware containing a list of record
     *  handles of PDRs that the host firmware has to fetch.
     *  @param[in] pdrTypes - list of PDR types that need to be looked up in the
     *                        BMC repo
     *  @param[in] eventDataFormat - format for PDRRepositoryChgEvent in DSP0248
     */
    void sendPDRRepositoryChgEvent(std::vector<uint8_t>&& pdrTypes,
                                   uint8_t eventDataFormat);

    /** @brief Lookup host sensor info corresponding to requested SensorEntry
     *
     *  @param[in] entry - TerminusID and SensorID
     *
     *  @return SensorInfo corresponding to the input paramter SensorEntry
     *          throw std::out_of_range exception if not found
     */
    const pdr::SensorInfo& lookupSensorInfo(const SensorEntry& entry) const
    {
        return sensorMap.at(entry);
    }

  private:
    /** @brief fetchPDR schedules work on the event loop, this method does the
     *  actual work. This is so that the PDR exchg with the host is async.
     *  @param[in] source - sdeventplus event source
     */
    void _fetchPDR(sdeventplus::source::EventBase& source);

    /** @brief Merge host firmware's entity association PDRs into BMC's
     *  @details A merge operation involves adding a pldm_entity under the
     *  appropriate parent, and updating container ids.
     *  @param[in] pdr - entity association pdr
     */
    void mergeEntityAssociations(const std::vector<uint8_t>& pdr);

    /** @brief Find parent of input entity type, from the entity association
     *  tree
     *  @param[in] type - PLDM entity type
     *  @param[out] parent - PLDM entity information of parent
     *  @return bool - true if parent found, false otherwise
     */
    bool getParent(EntityType type, pldm_entity& parent);

    /** @brief fd of MCTP communications socket */
    int mctp_fd;
    /** @brief MCTP EID of host firmware */
    uint8_t mctp_eid;
    /** @brief reference of main event loop of pldmd, primarily used to schedule
     *  work.
     */
    sdeventplus::Event& event;
    /** @brief pointer to BMC's primary PDR repo, host PDRs are added here */
    pldm_pdr* repo;
    /** @brief Pointer to BMC's entity association tree */
    pldm_entity_association_tree* entityTree;
    /** @brief reference to Requester object, primarily used to access API to
     *  obtain PLDM instance id.
     */
    Requester& requester;
    /** @brief sdeventplus event source */
    std::unique_ptr<sdeventplus::source::Defer> pdrFetchEvent;
    /** @brief list of PDR record handles pointing to host's PDRs */
    PDRRecordHandles pdrRecordHandles;
    /** @brief maps an entity type to parent pldm_entity from the BMC's entity
     *  association tree
     */
    std::map<EntityType, pldm_entity> parents;
    /** @brief D-Bus property changed signal match */
    std::unique_ptr<sdbusplus::bus::match::match> hostOffMatch;

    /** @brief sensorMap is a lookup data structure that is build from the
     *         hostPDR that speeds up the lookup of <TerminusID, SensorID> in
     *         PlatformEventMessage command request.
     */
    HostStateSensorMap sensorMap;
};

} // namespace pldm
