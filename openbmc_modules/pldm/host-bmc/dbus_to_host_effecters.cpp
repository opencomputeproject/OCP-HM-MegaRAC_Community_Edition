#include "dbus_to_host_effecters.hpp"

#include "libpldm/pdr.h"
#include "libpldm/platform.h"
#include "libpldm/requester/pldm.h"

#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/State/OperatingSystem/Status/server.hpp>

#include <fstream>
#include <iostream>

namespace pldm
{
namespace host_effecters
{

using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

constexpr auto hostEffecterJson = "dbus_to_host_effecter.json";

void HostEffecterParser::populatePropVals(
    const Json& dBusValues, std::vector<PropertyValue>& propertyValues,
    const std::string& propertyType)

{
    for (const auto& elem : dBusValues)
    {
        auto value = jsonEntryToDbusVal(propertyType, elem);
        propertyValues.emplace_back(value);
    }
}

void HostEffecterParser::parseEffecterJson(const std::string& jsonPath)
{
    fs::path jsonDir(jsonPath);
    if (!fs::exists(jsonDir) || fs::is_empty(jsonDir))
    {
        std::cerr << "Host Effecter json path does not exist, DIR=" << jsonPath
                  << "\n";
        return;
    }

    fs::path jsonFilePath = jsonDir / hostEffecterJson;
    if (!fs::exists(jsonFilePath))
    {
        std::cerr << "json does not exist, PATH=" << jsonFilePath << "\n";
        throw InternalFailure();
    }

    std::ifstream jsonFile(jsonFilePath);
    auto data = Json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        std::cerr << "Parsing json file failed, FILE=" << jsonFilePath << "\n";
        throw InternalFailure();
    }
    const Json empty{};
    const std::vector<Json> emptyList{};

    auto entries = data.value("entries", emptyList);
    for (const auto& entry : entries)
    {
        EffecterInfo effecterInfo;
        effecterInfo.mctpEid = entry.value("mctp_eid", 0xFF);
        auto jsonEffecterInfo = entry.value("effecter_info", empty);
        auto effecterId =
            jsonEffecterInfo.value("effecterID", PLDM_INVALID_EFFECTER_ID);
        effecterInfo.containerId = jsonEffecterInfo.value("containerID", 0);
        effecterInfo.entityType = jsonEffecterInfo.value("entityType", 0);
        effecterInfo.entityInstance =
            jsonEffecterInfo.value("entityInstance", 0);
        effecterInfo.compEffecterCnt =
            jsonEffecterInfo.value("compositeEffecterCount", 0);
        auto effecters = entry.value("effecters", emptyList);
        for (const auto& effecter : effecters)
        {
            DBusEffecterMapping dbusInfo{};
            auto jsonDbusInfo = effecter.value("dbus_info", empty);
            dbusInfo.dbusMap.objectPath = jsonDbusInfo.value("object_path", "");
            dbusInfo.dbusMap.interface = jsonDbusInfo.value("interface", "");
            dbusInfo.dbusMap.propertyName =
                jsonDbusInfo.value("property_name", "");
            dbusInfo.dbusMap.propertyType =
                jsonDbusInfo.value("property_type", "");
            Json propertyValues = jsonDbusInfo["property_values"];

            populatePropVals(propertyValues, dbusInfo.propertyValues,
                             dbusInfo.dbusMap.propertyType);

            const std::vector<uint8_t> emptyStates{};
            auto state = effecter.value("state", empty);
            dbusInfo.state.stateSetId = state.value("id", 0);
            auto states = state.value("state_values", emptyStates);
            if (dbusInfo.propertyValues.size() != states.size())
            {
                std::cerr << "Number of states do not match with"
                          << " number of D-Bus property values in the json. "
                          << "Object path " << dbusInfo.dbusMap.objectPath
                          << " and property " << dbusInfo.dbusMap.propertyName
                          << " will not be monitored \n";
                continue;
            }
            for (const auto& s : states)
            {
                dbusInfo.state.states.emplace_back(s);
            }

            auto effecterInfoIndex = hostEffecterInfo.size();
            auto dbusInfoIndex = effecterInfo.dbusInfo.size();
            createHostEffecterMatch(
                dbusInfo.dbusMap.objectPath, dbusInfo.dbusMap.interface,
                effecterInfoIndex, dbusInfoIndex, effecterId);
            effecterInfo.dbusInfo.emplace_back(std::move(dbusInfo));
        }
        hostEffecterInfo.emplace_back(std::move(effecterInfo));
    }
}

void HostEffecterParser::processHostEffecterChangeNotification(
    const DbusChgHostEffecterProps& chProperties, size_t effecterInfoIndex,
    size_t dbusInfoIndex, uint16_t effecterId)
{
    const auto& propertyName = hostEffecterInfo[effecterInfoIndex]
                                   .dbusInfo[dbusInfoIndex]
                                   .dbusMap.propertyName;

    const auto& it = chProperties.find(propertyName);

    if (it == chProperties.end())
    {
        return;
    }

    if (effecterId == PLDM_INVALID_EFFECTER_ID)
    {
        constexpr auto localOrRemote = false;
        effecterId = findStateEffecterId(
            pdrRepo, hostEffecterInfo[effecterInfoIndex].entityType,
            hostEffecterInfo[effecterInfoIndex].entityInstance,
            hostEffecterInfo[effecterInfoIndex].containerId,
            hostEffecterInfo[effecterInfoIndex]
                .dbusInfo[dbusInfoIndex]
                .state.stateSetId,
            localOrRemote);
        if (effecterId == PLDM_INVALID_EFFECTER_ID)
        {
            std::cerr << "Effecter id not found in pdr repo \n";
            return;
        }
    }
    constexpr auto hostStateInterface =
        "xyz.openbmc_project.State.OperatingSystem.Status";
    constexpr auto hostStatePath = "/xyz/openbmc_project/state/host0";

    try
    {
        auto propVal = dbusHandler->getDbusPropertyVariant(
            hostStatePath, "OperatingSystemState", hostStateInterface);
        const auto& currHostState = std::get<std::string>(propVal);
        if ((currHostState != "xyz.openbmc_project.State.OperatingSystem."
                              "Status.OSStatus.Standby") &&
            (currHostState != "xyz.openbmc_project.State.OperatingSystem."
                              "Status.OSStatus.BootComplete"))
        {
            std::cout << "Host is not up. Current host state: "
                      << currHostState.c_str() << "\n";
            return;
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Error in getting current host state. Will still "
                     "continue to set the host effecter \n";
    }
    uint8_t newState{};
    try
    {
        newState =
            findNewStateValue(effecterInfoIndex, dbusInfoIndex, it->second);
    }
    catch (const std::out_of_range& e)
    {
        std::cerr << "new state not found in json"
                  << "\n";
        return;
    }

    std::vector<set_effecter_state_field> stateField;
    for (uint8_t i = 0; i < hostEffecterInfo[effecterInfoIndex].compEffecterCnt;
         i++)
    {
        if (i == dbusInfoIndex)
        {
            stateField.push_back({PLDM_REQUEST_SET, newState});
        }
        else
        {
            stateField.push_back({PLDM_NO_CHANGE, 0});
        }
    }
    int rc{};
    try
    {
        rc = setHostStateEffecter(effecterInfoIndex, stateField, effecterId);
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Could not set host state effecter \n";
        return;
    }
    if (rc != PLDM_SUCCESS)
    {
        std::cerr << "Could not set the host state effecter, rc= " << rc
                  << " \n";
    }
}

uint8_t
    HostEffecterParser::findNewStateValue(size_t effecterInfoIndex,
                                          size_t dbusInfoIndex,
                                          const PropertyValue& propertyValue)
{
    const auto& propValues = hostEffecterInfo[effecterInfoIndex]
                                 .dbusInfo[dbusInfoIndex]
                                 .propertyValues;
    auto it = std::find(propValues.begin(), propValues.end(), propertyValue);
    uint8_t newState{};
    if (it != propValues.end())
    {
        auto index = std::distance(propValues.begin(), it);
        newState = hostEffecterInfo[effecterInfoIndex]
                       .dbusInfo[dbusInfoIndex]
                       .state.states[index];
    }
    else
    {
        throw std::out_of_range("new state not found in json");
    }
    return newState;
}

int HostEffecterParser::setHostStateEffecter(
    size_t effecterInfoIndex, std::vector<set_effecter_state_field>& stateField,
    uint16_t effecterId)
{
    uint8_t& mctpEid = hostEffecterInfo[effecterInfoIndex].mctpEid;
    uint8_t& compEffCnt = hostEffecterInfo[effecterInfoIndex].compEffecterCnt;
    auto instanceId = requester->getInstanceId(mctpEid);

    std::vector<uint8_t> requestMsg(
        sizeof(pldm_msg_hdr) + sizeof(effecterId) + sizeof(compEffCnt) +
            sizeof(set_effecter_state_field) * compEffCnt,
        0);
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc = encode_set_state_effecter_states_req(
        instanceId, effecterId, compEffCnt, stateField.data(), request);

    if (rc != PLDM_SUCCESS)
    {
        std::cerr << "Message encode failure. PLDM error code = " << std::hex
                  << std::showbase << rc << "\n";
        requester->markFree(mctpEid, instanceId);
        return rc;
    }

    if (verbose)
    {
        if (requestMsg.size())
        {
            std::ostringstream tempStream;
            for (int byte : requestMsg)
            {
                tempStream << std::setfill('0') << std::setw(2) << std::hex
                           << byte << " ";
            }
            std::cout << tempStream.str() << std::endl;
        }
    }

    uint8_t* responseMsg = nullptr;
    size_t responseMsgSize{};

    rc = pldm_send_recv(mctpEid, sockFd, requestMsg.data(), requestMsg.size(),
                        &responseMsg, &responseMsgSize);
    std::unique_ptr<uint8_t, decltype(std::free)*> responseMsgPtr{responseMsg,
                                                                  std::free};
    requester->markFree(mctpEid, instanceId);

    if (rc != PLDM_REQUESTER_SUCCESS)
    {
        std::cerr << "Failed to send message/receive response. RC = " << rc
                  << ", errno = " << errno << " for setting host effecter "
                  << effecterId << "\n";
        pldm::utils::reportError(
            "xyz.openbmc_project.bmc.pldm.InternalFailure");
        return rc;
    }
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(responseMsgPtr.get());
    uint8_t completionCode{};
    rc = decode_set_state_effecter_states_resp(
        responsePtr, responseMsgSize - sizeof(pldm_msg_hdr), &completionCode);
    if (rc != PLDM_SUCCESS)
    {
        std::cerr << "Failed to decode setStateEffecterStates response, rc = "
                  << rc << "\n";
        return rc;
    }
    if (completionCode != PLDM_SUCCESS)
    {
        std::cerr << "Failed setStateEffecterStates for effecter " << effecterId
                  << ". Response from Host " << (uint32_t)completionCode
                  << "\n";
        pldm::utils::reportError(
            "xyz.openbmc_project.bmc.pldm.InternalFailure");
    }
    return rc;
}

void HostEffecterParser::createHostEffecterMatch(const std::string& objectPath,
                                                 const std::string& interface,
                                                 size_t effecterInfoIndex,
                                                 size_t dbusInfoIndex,
                                                 uint16_t effecterId)
{
    using namespace sdbusplus::bus::match::rules;
    effecterInfoMatch.emplace_back(
        std::make_unique<sdbusplus::bus::match::match>(
            pldm::utils::DBusHandler::getBus(),
            propertiesChanged(objectPath, interface),
            [this, effecterInfoIndex, dbusInfoIndex,
             effecterId](sdbusplus::message::message& msg) {
                DbusChgHostEffecterProps props;
                std::string iface;
                msg.read(iface, props);
                processHostEffecterChangeNotification(
                    props, effecterInfoIndex, dbusInfoIndex, effecterId);
            }));
}

} // namespace host_effecters
} // namespace pldm
