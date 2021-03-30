#include "utils.hpp"

#include "libpldm/pdr.h"
#include "libpldm/pldm_types.h"

#include <xyz/openbmc_project/Common/error.hpp>

#include <array>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace pldm
{
namespace utils
{

constexpr auto mapperBusName = "xyz.openbmc_project.ObjectMapper";
constexpr auto mapperPath = "/xyz/openbmc_project/object_mapper";
constexpr auto mapperInterface = "xyz.openbmc_project.ObjectMapper";
constexpr auto eidPath = "/usr/share/pldm/host_eid";

std::vector<std::vector<uint8_t>> findStateEffecterPDR(uint8_t /*tid*/,
                                                       uint16_t entityID,
                                                       uint16_t stateSetId,
                                                       const pldm_pdr* repo)
{
    uint8_t* outData = nullptr;
    uint32_t size{};
    const pldm_pdr_record* record{};
    std::vector<std::vector<uint8_t>> pdrs;
    try
    {
        do
        {
            record = pldm_pdr_find_record_by_type(repo, PLDM_STATE_EFFECTER_PDR,
                                                  record, &outData, &size);
            if (record)
            {
                auto pdr = reinterpret_cast<pldm_state_effecter_pdr*>(outData);
                auto compositeEffecterCount = pdr->composite_effecter_count;
                auto possible_states_start = pdr->possible_states;

                for (auto effecters = 0x00; effecters < compositeEffecterCount;
                     effecters++)
                {
                    auto possibleStates =
                        reinterpret_cast<state_effecter_possible_states*>(
                            possible_states_start);
                    auto setId = possibleStates->state_set_id;
                    auto possibleStateSize =
                        possibleStates->possible_states_size;

                    if (pdr->entity_type == entityID && setId == stateSetId)
                    {
                        std::vector<uint8_t> effecter_pdr(&outData[0],
                                                          &outData[size]);
                        pdrs.emplace_back(std::move(effecter_pdr));
                        break;
                    }
                    possible_states_start += possibleStateSize + sizeof(setId) +
                                             sizeof(possibleStateSize);
                }
            }

        } while (record);
    }
    catch (const std::exception& e)
    {
        std::cerr << " Failed to obtain a record. ERROR =" << e.what()
                  << std::endl;
    }

    return pdrs;
}

std::vector<std::vector<uint8_t>> findStateSensorPDR(uint8_t /*tid*/,
                                                     uint16_t entityID,
                                                     uint16_t stateSetId,
                                                     const pldm_pdr* repo)
{
    uint8_t* outData = nullptr;
    uint32_t size{};
    const pldm_pdr_record* record{};
    std::vector<std::vector<uint8_t>> pdrs;
    try
    {
        do
        {
            record = pldm_pdr_find_record_by_type(repo, PLDM_STATE_SENSOR_PDR,
                                                  record, &outData, &size);
            if (record)
            {
                auto pdr = reinterpret_cast<pldm_state_sensor_pdr*>(outData);
                auto compositeSensorCount = pdr->composite_sensor_count;
                auto possible_states_start = pdr->possible_states;

                for (auto sensors = 0x00; sensors < compositeSensorCount;
                     sensors++)
                {
                    auto possibleStates =
                        reinterpret_cast<state_sensor_possible_states*>(
                            possible_states_start);
                    auto setId = possibleStates->state_set_id;
                    auto possibleStateSize =
                        possibleStates->possible_states_size;

                    if (pdr->entity_type == entityID && setId == stateSetId)
                    {
                        std::vector<uint8_t> sensor_pdr(&outData[0],
                                                        &outData[size]);
                        pdrs.emplace_back(std::move(sensor_pdr));
                        break;
                    }
                    possible_states_start += possibleStateSize + sizeof(setId) +
                                             sizeof(possibleStateSize);
                }
            }

        } while (record);
    }
    catch (const std::exception& e)
    {
        std::cerr << " Failed to obtain a record. ERROR =" << e.what()
                  << std::endl;
    }

    return pdrs;
}

uint8_t readHostEID()
{
    uint8_t eid{};
    std::ifstream eidFile{eidPath};
    if (!eidFile.good())
    {
        std::cerr << "Could not open host EID file"
                  << "\n";
    }
    else
    {
        std::string eidStr;
        eidFile >> eidStr;
        if (!eidStr.empty())
        {
            eid = atoi(eidStr.c_str());
        }
        else
        {
            std::cerr << "Host EID file was empty"
                      << "\n";
        }
    }

    return eid;
}

uint8_t getNumPadBytes(uint32_t data)
{
    uint8_t pad;
    pad = ((data % 4) ? (4 - data % 4) : 0);
    return pad;
} // end getNumPadBytes

bool uintToDate(uint64_t data, uint16_t* year, uint8_t* month, uint8_t* day,
                uint8_t* hour, uint8_t* min, uint8_t* sec)
{
    constexpr uint64_t max_data = 29991231115959;
    constexpr uint64_t min_data = 19700101000000;
    if (data < min_data || data > max_data)
    {
        return false;
    }

    *year = data / 10000000000;
    data = data % 10000000000;
    *month = data / 100000000;
    data = data % 100000000;
    *day = data / 1000000;
    data = data % 1000000;
    *hour = data / 10000;
    data = data % 10000;
    *min = data / 100;
    *sec = data % 100;

    return true;
}

std::optional<std::vector<set_effecter_state_field>>
    parseEffecterData(const std::vector<uint8_t>& effecterData,
                      uint8_t effecterCount)
{
    std::vector<set_effecter_state_field> stateField;

    if (effecterData.size() != effecterCount * 2)
    {
        return std::nullopt;
    }

    for (uint8_t i = 0; i < effecterCount; ++i)
    {
        uint8_t set_request = effecterData[i * 2] == PLDM_REQUEST_SET
                                  ? PLDM_REQUEST_SET
                                  : PLDM_NO_CHANGE;
        set_effecter_state_field filed{set_request, effecterData[i * 2 + 1]};
        stateField.emplace_back(std::move(filed));
    }

    return std::make_optional(std::move(stateField));
}

std::string DBusHandler::getService(const char* path,
                                    const char* interface) const
{
    using DbusInterfaceList = std::vector<std::string>;
    std::map<std::string, std::vector<std::string>> mapperResponse;
    auto& bus = DBusHandler::getBus();

    auto mapper = bus.new_method_call(mapperBusName, mapperPath,
                                      mapperInterface, "GetObject");
    mapper.append(path, DbusInterfaceList({interface}));

    auto mapperResponseMsg = bus.call(mapper);
    mapperResponseMsg.read(mapperResponse);
    return mapperResponse.begin()->first;
}

void reportError(const char* errorMsg)
{
    static constexpr auto logObjPath = "/xyz/openbmc_project/logging";
    static constexpr auto logInterface = "xyz.openbmc_project.Logging.Create";

    auto& bus = pldm::utils::DBusHandler::getBus();

    try
    {
        auto service = DBusHandler().getService(logObjPath, logInterface);
        using namespace sdbusplus::xyz::openbmc_project::Logging::server;
        auto severity =
            sdbusplus::xyz::openbmc_project::Logging::server::convertForMessage(
                sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level::
                    Error);
        auto method = bus.new_method_call(service.c_str(), logObjPath,
                                          logInterface, "Create");
        std::map<std::string, std::string> addlData{};
        method.append(errorMsg, severity, addlData);
        bus.call_noreply(method);
    }
    catch (const std::exception& e)
    {
        std::cerr << "failed to make a d-bus call to create error log, ERROR="
                  << e.what() << "\n";
    }
}

void DBusHandler::setDbusProperty(const DBusMapping& dBusMap,
                                  const PropertyValue& value) const
{
    auto setDbusValue = [&dBusMap, this](const auto& variant) {
        auto& bus = getBus();
        auto service =
            getService(dBusMap.objectPath.c_str(), dBusMap.interface.c_str());
        auto method = bus.new_method_call(
            service.c_str(), dBusMap.objectPath.c_str(), dbusProperties, "Set");
        method.append(dBusMap.interface.c_str(), dBusMap.propertyName.c_str(),
                      variant);
        bus.call_noreply(method);
    };

    if (dBusMap.propertyType == "uint8_t")
    {
        std::variant<uint8_t> v = std::get<uint8_t>(value);
        setDbusValue(v);
    }
    else if (dBusMap.propertyType == "bool")
    {
        std::variant<bool> v = std::get<bool>(value);
        setDbusValue(v);
    }
    else if (dBusMap.propertyType == "int16_t")
    {
        std::variant<int16_t> v = std::get<int16_t>(value);
        setDbusValue(v);
    }
    else if (dBusMap.propertyType == "uint16_t")
    {
        std::variant<uint16_t> v = std::get<uint16_t>(value);
        setDbusValue(v);
    }
    else if (dBusMap.propertyType == "int32_t")
    {
        std::variant<int32_t> v = std::get<int32_t>(value);
        setDbusValue(v);
    }
    else if (dBusMap.propertyType == "uint32_t")
    {
        std::variant<uint32_t> v = std::get<uint32_t>(value);
        setDbusValue(v);
    }
    else if (dBusMap.propertyType == "int64_t")
    {
        std::variant<int64_t> v = std::get<int64_t>(value);
        setDbusValue(v);
    }
    else if (dBusMap.propertyType == "uint64_t")
    {
        std::variant<uint64_t> v = std::get<uint64_t>(value);
        setDbusValue(v);
    }
    else if (dBusMap.propertyType == "double")
    {
        std::variant<double> v = std::get<double>(value);
        setDbusValue(v);
    }
    else if (dBusMap.propertyType == "string")
    {
        std::variant<std::string> v = std::get<std::string>(value);
        setDbusValue(v);
    }
    else
    {
        throw std::invalid_argument("UnSpported Dbus Type");
    }
}

PropertyValue DBusHandler::getDbusPropertyVariant(
    const char* objPath, const char* dbusProp, const char* dbusInterface) const
{
    auto& bus = DBusHandler::getBus();
    auto service = getService(objPath, dbusInterface);
    auto method =
        bus.new_method_call(service.c_str(), objPath, dbusProperties, "Get");
    method.append(dbusInterface, dbusProp);
    PropertyValue value{};
    auto reply = bus.call(method);
    reply.read(value);
    return value;
}

PropertyValue jsonEntryToDbusVal(std::string_view type,
                                 const nlohmann::json& value)
{
    PropertyValue propValue{};
    if (type == "uint8_t")
    {
        propValue = static_cast<uint8_t>(value);
    }
    else if (type == "uint16_t")
    {
        propValue = static_cast<uint16_t>(value);
    }
    else if (type == "uint32_t")
    {
        propValue = static_cast<uint32_t>(value);
    }
    else if (type == "uint64_t")
    {
        propValue = static_cast<uint64_t>(value);
    }
    else if (type == "int16_t")
    {
        propValue = static_cast<int16_t>(value);
    }
    else if (type == "int32_t")
    {
        propValue = static_cast<int32_t>(value);
    }
    else if (type == "int64_t")
    {
        propValue = static_cast<int64_t>(value);
    }
    else if (type == "bool")
    {
        propValue = static_cast<bool>(value);
    }
    else if (type == "double")
    {
        propValue = static_cast<double>(value);
    }
    else if (type == "string")
    {
        propValue = static_cast<std::string>(value);
    }
    else
    {
        std::cerr << "Unknown D-Bus property type, TYPE=" << type << "\n";
    }

    return propValue;
}

uint16_t findStateEffecterId(const pldm_pdr* pdrRepo, uint16_t entityType,
                             uint16_t entityInstance, uint16_t containerId,
                             uint16_t stateSetId, bool localOrRemote)
{
    uint8_t* pdrData = nullptr;
    uint32_t pdrSize{};
    const pldm_pdr_record* record{};
    do
    {
        record = pldm_pdr_find_record_by_type(pdrRepo, PLDM_STATE_EFFECTER_PDR,
                                              record, &pdrData, &pdrSize);
        if (record && (localOrRemote ^ pldm_pdr_record_is_remote(record)))
        {
            auto pdr = reinterpret_cast<pldm_state_effecter_pdr*>(pdrData);
            auto compositeEffecterCount = pdr->composite_effecter_count;
            auto possible_states_start = pdr->possible_states;

            for (auto effecters = 0x00; effecters < compositeEffecterCount;
                 effecters++)
            {
                auto possibleStates =
                    reinterpret_cast<state_effecter_possible_states*>(
                        possible_states_start);
                auto setId = possibleStates->state_set_id;
                auto possibleStateSize = possibleStates->possible_states_size;

                if (entityType == pdr->entity_type &&
                    entityInstance == pdr->entity_instance &&
                    containerId == pdr->container_id && stateSetId == setId)
                {
                    return pdr->effecter_id;
                }
                possible_states_start += possibleStateSize + sizeof(setId) +
                                         sizeof(possibleStateSize);
            }
        }
    } while (record);

    return PLDM_INVALID_EFFECTER_ID;
}

int emitStateSensorEventSignal(uint8_t tid, uint16_t sensorId,
                               uint8_t sensorOffset, uint8_t eventState,
                               uint8_t previousEventState)
{
    try
    {
        auto& bus = DBusHandler::getBus();
        auto msg = bus.new_signal("/xyz/openbmc_project/pldm",
                                  "xyz.openbmc_project.PLDM.Event",
                                  "StateSensorEvent");
        msg.append(tid, sensorId, sensorOffset, eventState, previousEventState);

        msg.signal_send();
    }
    catch (std::exception& e)
    {
        std::cerr << "Error emitting pldm event signal:"
                  << "ERROR=" << e.what() << "\n";
        return PLDM_ERROR;
    }

    return PLDM_SUCCESS;
}

} // namespace utils
} // namespace pldm
