#include "config.h"

#include "host_pdr_handler.hpp"

#include "libpldm/requester/pldm.h"

#include <assert.h>

#include <nlohmann/json.hpp>

#include <fstream>

namespace pldm
{

using namespace pldm::utils;
using namespace sdbusplus::bus::match::rules;
using Json = nlohmann::json;
namespace fs = std::filesystem;
constexpr auto fruJson = "host_frus.json";
const Json emptyJson{};
const std::vector<Json> emptyJsonList{};

HostPDRHandler::HostPDRHandler(int mctp_fd, uint8_t mctp_eid,
                               sdeventplus::Event& event, pldm_pdr* repo,
                               pldm_entity_association_tree* entityTree,
                               Requester& requester) :
    mctp_fd(mctp_fd),
    mctp_eid(mctp_eid), event(event), repo(repo), entityTree(entityTree),
    requester(requester)
{
    fs::path hostFruJson(fs::path(HOST_JSONS_DIR) / fruJson);
    if (fs::exists(hostFruJson))
    {
        // Note parent entities for entities sent down by the host firmware.
        // This will enable a merge of entity associations.
        try
        {
            std::ifstream jsonFile(hostFruJson);
            auto data = Json::parse(jsonFile, nullptr, false);
            if (data.is_discarded())
            {
                std::cerr << "Parsing Host FRU json file failed" << std::endl;
            }
            else
            {
                auto entities = data.value("entities", emptyJsonList);
                for (auto& entity : entities)
                {
                    EntityType entityType = entity.value("entity_type", 0);
                    auto parent = entity.value("parent", emptyJson);
                    pldm_entity p{};
                    p.entity_type = parent.value("entity_type", 0);
                    p.entity_instance_num = parent.value("entity_instance", 0);
                    parents.emplace(entityType, std::move(p));
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Parsing Host FRU json file failed, exception = "
                      << e.what() << std::endl;
        }
    }

    hostOffMatch = std::make_unique<sdbusplus::bus::match::match>(
        pldm::utils::DBusHandler::getBus(),
        propertiesChanged("/xyz/openbmc_project/state/host0",
                          "xyz.openbmc_project.State.Host"),
        [this, repo](sdbusplus::message::message& msg) {
            DbusChangedProps props{};
            std::string intf;
            msg.read(intf, props);
            const auto itr = props.find("CurrentHostState");
            if (itr != props.end())
            {
                PropertyValue value = itr->second;
                auto propVal = std::get<std::string>(value);
                if (propVal == "xyz.openbmc_project.State.Host.HostState.Off")
                {
                    pldm_pdr_remove_remote_pdrs(repo);
                    this->sensorMap.clear();
                }
            }
        });
}

void HostPDRHandler::fetchPDR(PDRRecordHandles&& recordHandles)
{
    pdrRecordHandles.clear();
    pdrRecordHandles = std::move(recordHandles);

    // Defer the actual fetch of PDRs from the host (by queuing the call on the
    // main event loop). That way, we can respond to the platform event msg from
    // the host firmware.
    pdrFetchEvent = std::make_unique<sdeventplus::source::Defer>(
        event, std::bind(std::mem_fn(&HostPDRHandler::_fetchPDR), this,
                         std::placeholders::_1));
}

void HostPDRHandler::_fetchPDR(sdeventplus::source::EventBase& /*source*/)
{
    pdrFetchEvent.reset();

    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) +
                                    PLDM_GET_PDR_REQ_BYTES);
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    bool merged = false;

    uint32_t nextRecordHandle{};
    uint32_t recordHandle{};
    bool isFormatRecHandles = false;
    if (!pdrRecordHandles.empty())
    {
        recordHandle = pdrRecordHandles.front();
        pdrRecordHandles.pop_front();
        isFormatRecHandles = true;
    }

    do
    {
        auto instanceId = requester.getInstanceId(mctp_eid);

        auto rc =
            encode_get_pdr_req(instanceId, recordHandle, 0, PLDM_GET_FIRSTPART,
                               UINT16_MAX, 0, request, PLDM_GET_PDR_REQ_BYTES);
        if (rc != PLDM_SUCCESS)
        {
            requester.markFree(mctp_eid, instanceId);
            std::cerr << "Failed to encode_get_pdr_req, rc = " << rc
                      << std::endl;
            return;
        }

        uint8_t* responseMsg = nullptr;
        size_t responseMsgSize{};
        auto requesterRc =
            pldm_send_recv(mctp_eid, mctp_fd, requestMsg.data(),
                           requestMsg.size(), &responseMsg, &responseMsgSize);
        std::unique_ptr<uint8_t, decltype(std::free)*> responseMsgPtr{
            responseMsg, std::free};
        requester.markFree(mctp_eid, instanceId);
        if (requesterRc != PLDM_REQUESTER_SUCCESS)
        {
            std::cerr << "Failed to send msg to fetch pdrs, rc = "
                      << requesterRc << std::endl;
            return;
        }

        uint8_t completionCode{};
        uint32_t nextDataTransferHandle{};
        uint8_t transferFlag{};
        uint16_t respCount{};
        uint8_t transferCRC{};
        auto responsePtr =
            reinterpret_cast<struct pldm_msg*>(responseMsgPtr.get());
        rc = decode_get_pdr_resp(
            responsePtr, responseMsgSize - sizeof(pldm_msg_hdr),
            &completionCode, &nextRecordHandle, &nextDataTransferHandle,
            &transferFlag, &respCount, nullptr, 0, &transferCRC);
        if (rc != PLDM_SUCCESS)
        {
            std::cerr << "Failed to decode_get_pdr_resp, rc = " << rc
                      << std::endl;
        }
        else
        {
            std::vector<uint8_t> pdr(respCount, 0);
            rc = decode_get_pdr_resp(
                responsePtr, responseMsgSize - sizeof(pldm_msg_hdr),
                &completionCode, &nextRecordHandle, &nextDataTransferHandle,
                &transferFlag, &respCount, pdr.data(), respCount, &transferCRC);
            if (rc != PLDM_SUCCESS || completionCode != PLDM_SUCCESS)
            {
                std::cerr << "Failed to decode_get_pdr_resp: "
                          << "rc=" << rc
                          << ", cc=" << static_cast<unsigned>(completionCode)
                          << std::endl;
            }
            else
            {
                // Process the PDR host firmware sent us. The most common action
                // is to add the PDR to the the BMC's PDR repo.
                auto pdrHdr = reinterpret_cast<pldm_pdr_hdr*>(pdr.data());
                if (pdrHdr->type == PLDM_PDR_ENTITY_ASSOCIATION)
                {
                    mergeEntityAssociations(pdr);
                    merged = true;
                }
                else
                {
                    if (pdrHdr->type == PLDM_STATE_SENSOR_PDR)
                    {
                        const auto& [terminusHandle, sensorID, sensorInfo] =
                            responder::pdr_utils::parseStateSensorPDR(pdr);
                        SensorEntry sensorEntry{};
                        // TODO: Lookup Terminus ID from the Terminus Locator
                        // PDR with terminusHandle
                        sensorEntry.terminusID = 0;
                        sensorEntry.sensorID = sensorID;
                        sensorMap.emplace(sensorEntry, std::move(sensorInfo));
                    }
                    pldm_pdr_add(repo, pdr.data(), respCount, 0, true);
                }
            }

            recordHandle = nextRecordHandle;
            if (!pdrRecordHandles.empty())
            {
                recordHandle = pdrRecordHandles.front();
                pdrRecordHandles.pop_front();
            }
            else if (isFormatRecHandles)
            {
                break;
            }
        }
    } while (recordHandle);

    if (merged)
    {
        // We have merged host's entity association PDRs with our own. Send an
        // event to the host firmware to indicate the same.
        sendPDRRepositoryChgEvent(
            std::move(std::vector<uint8_t>(1, PLDM_PDR_ENTITY_ASSOCIATION)),
            FORMAT_IS_PDR_HANDLES);
    }
}

bool HostPDRHandler::getParent(EntityType type, pldm_entity& parent)
{
    auto found = parents.find(type);
    if (found != parents.end())
    {
        parent.entity_type = found->second.entity_type;
        parent.entity_instance_num = found->second.entity_instance_num;
        return true;
    }

    return false;
}

void HostPDRHandler::mergeEntityAssociations(const std::vector<uint8_t>& pdr)
{
    size_t numEntities{};
    pldm_entity* entities = nullptr;
    bool merged = false;
    auto entityPdr = reinterpret_cast<pldm_pdr_entity_association*>(
        const_cast<uint8_t*>(pdr.data()) + sizeof(pldm_pdr_hdr));

    pldm_entity_association_pdr_extract(pdr.data(), pdr.size(), &numEntities,
                                        &entities);
    for (size_t i = 0; i < numEntities; ++i)
    {
        pldm_entity parent{};
        if (getParent(entities[i].entity_type, parent))
        {
            auto node = pldm_entity_association_tree_find(entityTree, &parent);
            if (node)
            {
                pldm_entity_association_tree_add(entityTree, &entities[i], node,
                                                 entityPdr->association_type);
                merged = true;
            }
        }
    }
    free(entities);

    if (merged)
    {
        // Update our PDR repo with the merged entity association PDRs
        pldm_entity_association_pdr_add(entityTree, repo, true);
    }
}

void HostPDRHandler::sendPDRRepositoryChgEvent(std::vector<uint8_t>&& pdrTypes,
                                               uint8_t eventDataFormat)
{
    assert(eventDataFormat == FORMAT_IS_PDR_HANDLES);

    // Extract from the PDR repo record handles of PDRs we want the host
    // to pull up.
    std::vector<uint8_t> eventDataOps{PLDM_RECORDS_ADDED};
    std::vector<uint8_t> numsOfChangeEntries(1);
    std::vector<std::vector<ChangeEntry>> changeEntries(
        numsOfChangeEntries.size());
    for (auto pdrType : pdrTypes)
    {
        const pldm_pdr_record* record{};
        do
        {
            record = pldm_pdr_find_record_by_type(repo, pdrType, record,
                                                  nullptr, nullptr);
            if (record && pldm_pdr_record_is_remote(record))
            {
                changeEntries[0].push_back(
                    pldm_pdr_get_record_handle(repo, record));
            }
        } while (record);
    }
    if (changeEntries.empty())
    {
        return;
    }
    numsOfChangeEntries[0] = changeEntries[0].size();

    // Encode PLDM platform event msg to indicate a PDR repo change.
    size_t maxSize = PLDM_PDR_REPOSITORY_CHG_EVENT_MIN_LENGTH +
                     PLDM_PDR_REPOSITORY_CHANGE_RECORD_MIN_LENGTH +
                     changeEntries[0].size() * sizeof(uint32_t);
    std::vector<uint8_t> eventDataVec{};
    eventDataVec.resize(maxSize);
    auto eventData =
        reinterpret_cast<struct pldm_pdr_repository_chg_event_data*>(
            eventDataVec.data());
    size_t actualSize{};
    auto firstEntry = changeEntries[0].data();
    auto rc = encode_pldm_pdr_repository_chg_event_data(
        eventDataFormat, 1, eventDataOps.data(), numsOfChangeEntries.data(),
        &firstEntry, eventData, &actualSize, maxSize);
    if (rc != PLDM_SUCCESS)
    {
        std::cerr
            << "Failed to encode_pldm_pdr_repository_chg_event_data, rc = "
            << rc << std::endl;
        return;
    }
    auto instanceId = requester.getInstanceId(mctp_eid);
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) +
                                    PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES +
                                    actualSize);
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    rc = encode_platform_event_message_req(
        instanceId, 1, 0, PLDM_PDR_REPOSITORY_CHG_EVENT, eventDataVec.data(),
        actualSize, request,
        actualSize + PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES);
    if (rc != PLDM_SUCCESS)
    {
        requester.markFree(mctp_eid, instanceId);
        std::cerr << "Failed to encode_platform_event_message_req, rc = " << rc
                  << std::endl;
        return;
    }

    // Send up the event to host.
    uint8_t* responseMsg = nullptr;
    size_t responseMsgSize{};
    auto requesterRc =
        pldm_send_recv(mctp_eid, mctp_fd, requestMsg.data(), requestMsg.size(),
                       &responseMsg, &responseMsgSize);
    requester.markFree(mctp_eid, instanceId);
    if (requesterRc != PLDM_REQUESTER_SUCCESS)
    {
        std::cerr << "Failed to send msg to report pdrs, rc = " << requesterRc
                  << std::endl;
        return;
    }
    uint8_t completionCode{};
    uint8_t status{};
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(responseMsg);
    rc = decode_platform_event_message_resp(
        responsePtr, responseMsgSize - sizeof(pldm_msg_hdr), &completionCode,
        &status);
    free(responseMsg);
    if (rc != PLDM_SUCCESS || completionCode != PLDM_SUCCESS)
    {
        std::cerr << "Failed to decode_platform_event_message_resp: "
                  << "rc=" << rc
                  << ", cc=" << static_cast<unsigned>(completionCode)
                  << std::endl;
    }
}

} // namespace pldm
