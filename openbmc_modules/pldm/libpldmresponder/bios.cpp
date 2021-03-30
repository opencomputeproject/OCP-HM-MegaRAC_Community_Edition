#include "bios.hpp"

#include "common/utils.hpp"

#include <time.h>

#include <array>
#include <chrono>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace pldm
{

namespace responder
{

namespace utils
{

void epochToBCDTime(uint64_t timeSec, uint8_t& seconds, uint8_t& minutes,
                    uint8_t& hours, uint8_t& day, uint8_t& month,
                    uint16_t& year)
{
    auto t = time_t(timeSec);
    auto time = localtime(&t);

    seconds = pldm::utils::decimalToBcd(time->tm_sec);
    minutes = pldm::utils::decimalToBcd(time->tm_min);
    hours = pldm::utils::decimalToBcd(time->tm_hour);
    day = pldm::utils::decimalToBcd(time->tm_mday);
    month = pldm::utils::decimalToBcd(time->tm_mon +
                                      1); // The number of months in the range
                                          // 0 to 11.PLDM expects range 1 to 12
    year = pldm::utils::decimalToBcd(time->tm_year +
                                     1900); // The number of years since 1900
}

std::time_t timeToEpoch(uint8_t seconds, uint8_t minutes, uint8_t hours,
                        uint8_t day, uint8_t month, uint16_t year)
{
    struct std::tm stm;

    stm.tm_year = year - 1900;
    stm.tm_mon = month - 1;
    stm.tm_mday = day;
    stm.tm_hour = hours;
    stm.tm_min = minutes;
    stm.tm_sec = seconds;
    stm.tm_isdst = -1;

    // It will get the time in seconds since
    // Epoch, 1970.1.1 00:00:00 +0000,UTC.
    return timegm(&stm);
}

} // namespace utils

namespace bios
{

using EpochTimeUS = uint64_t;

DBusHandler dbusHandler;

Handler::Handler() : biosConfig(BIOS_JSONS_DIR, BIOS_TABLES_DIR, &dbusHandler)
{
    biosConfig.removeTables();
    biosConfig.buildTables();

    handlers.emplace(PLDM_SET_DATE_TIME,
                     [this](const pldm_msg* request, size_t payloadLength) {
                         return this->setDateTime(request, payloadLength);
                     });
    handlers.emplace(PLDM_GET_DATE_TIME,
                     [this](const pldm_msg* request, size_t payloadLength) {
                         return this->getDateTime(request, payloadLength);
                     });
    handlers.emplace(PLDM_GET_BIOS_TABLE,
                     [this](const pldm_msg* request, size_t payloadLength) {
                         return this->getBIOSTable(request, payloadLength);
                     });
    handlers.emplace(PLDM_GET_BIOS_ATTRIBUTE_CURRENT_VALUE_BY_HANDLE,
                     [this](const pldm_msg* request, size_t payloadLength) {
                         return this->getBIOSAttributeCurrentValueByHandle(
                             request, payloadLength);
                     });
    handlers.emplace(PLDM_SET_BIOS_ATTRIBUTE_CURRENT_VALUE,
                     [this](const pldm_msg* request, size_t payloadLength) {
                         return this->setBIOSAttributeCurrentValue(
                             request, payloadLength);
                     });
}

Response Handler::getDateTime(const pldm_msg* request, size_t /*payloadLength*/)
{
    uint8_t seconds = 0;
    uint8_t minutes = 0;
    uint8_t hours = 0;
    uint8_t day = 0;
    uint8_t month = 0;
    uint16_t year = 0;

    constexpr auto timeInterface = "xyz.openbmc_project.Time.EpochTime";
    constexpr auto bmcTimePath = "/xyz/openbmc_project/time/bmc";
    Response response(sizeof(pldm_msg_hdr) + PLDM_GET_DATE_TIME_RESP_BYTES, 0);
    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());
    EpochTimeUS timeUsec;

    try
    {
        timeUsec = pldm::utils::DBusHandler().getDbusProperty<EpochTimeUS>(
            bmcTimePath, "Elapsed", timeInterface);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Error getting time, PATH=" << bmcTimePath
                  << " TIME INTERACE=" << timeInterface << "\n";

        return CmdHandler::ccOnlyResponse(request, PLDM_ERROR);
    }

    uint64_t timeSec = std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::microseconds(timeUsec))
                           .count();

    pldm::responder::utils::epochToBCDTime(timeSec, seconds, minutes, hours,
                                           day, month, year);

    auto rc = encode_get_date_time_resp(request->hdr.instance_id, PLDM_SUCCESS,
                                        seconds, minutes, hours, day, month,
                                        year, responsePtr);
    if (rc != PLDM_SUCCESS)
    {
        return ccOnlyResponse(request, rc);
    }

    return response;
}

Response Handler::setDateTime(const pldm_msg* request, size_t payloadLength)
{
    uint8_t seconds = 0;
    uint8_t minutes = 0;
    uint8_t hours = 0;
    uint8_t day = 0;
    uint8_t month = 0;
    uint16_t year = 0;
    std::time_t timeSec;

    constexpr auto setTimeInterface = "xyz.openbmc_project.Time.EpochTime";
    constexpr auto setTimePath = "/xyz/openbmc_project/time/bmc";
    constexpr auto timeSetPro = "Elapsed";

    auto rc = decode_set_date_time_req(request, payloadLength, &seconds,
                                       &minutes, &hours, &day, &month, &year);
    if (rc != PLDM_SUCCESS)
    {
        return ccOnlyResponse(request, rc);
    }
    timeSec = pldm::responder::utils::timeToEpoch(seconds, minutes, hours, day,
                                                  month, year);
    uint64_t timeUsec = std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::seconds(timeSec))
                            .count();
    PropertyValue value{timeUsec};
    try
    {
        DBusMapping dbusMapping{setTimePath, setTimeInterface, timeSetPro,
                                "uint64_t"};
        pldm::utils::DBusHandler().setDbusProperty(dbusMapping, value);
    }
    catch (std::exception& e)
    {

        std::cerr << "Error Setting time,PATH=" << setTimePath
                  << "TIME INTERFACE=" << setTimeInterface
                  << "ERROR=" << e.what() << "\n";

        return ccOnlyResponse(request, PLDM_ERROR);
    }

    return ccOnlyResponse(request, PLDM_SUCCESS);
}

Response Handler::getBIOSTable(const pldm_msg* request, size_t payloadLength)
{
    uint32_t transferHandle{};
    uint8_t transferOpFlag{};
    uint8_t tableType{};

    auto rc = decode_get_bios_table_req(request, payloadLength, &transferHandle,
                                        &transferOpFlag, &tableType);
    if (rc != PLDM_SUCCESS)
    {
        return ccOnlyResponse(request, rc);
    }

    auto table =
        biosConfig.getBIOSTable(static_cast<pldm_bios_table_types>(tableType));
    if (!table)
    {
        return ccOnlyResponse(request, PLDM_BIOS_TABLE_UNAVAILABLE);
    }

    Response response(sizeof(pldm_msg_hdr) +
                      PLDM_GET_BIOS_TABLE_MIN_RESP_BYTES + table->size());
    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());

    rc = encode_get_bios_table_resp(
        request->hdr.instance_id, PLDM_SUCCESS, 0 /* nxtTransferHandle */,
        PLDM_START_AND_END, table->data(), response.size(), responsePtr);
    if (rc != PLDM_SUCCESS)
    {
        return ccOnlyResponse(request, rc);
    }

    return response;
}

Response Handler::getBIOSAttributeCurrentValueByHandle(const pldm_msg* request,
                                                       size_t payloadLength)
{
    uint32_t transferHandle;
    uint8_t transferOpFlag;
    uint16_t attributeHandle;

    auto rc = decode_get_bios_attribute_current_value_by_handle_req(
        request, payloadLength, &transferHandle, &transferOpFlag,
        &attributeHandle);
    if (rc != PLDM_SUCCESS)
    {
        return ccOnlyResponse(request, rc);
    }

    auto table = biosConfig.getBIOSTable(PLDM_BIOS_ATTR_VAL_TABLE);
    if (!table)
    {
        return ccOnlyResponse(request, PLDM_BIOS_TABLE_UNAVAILABLE);
    }

    auto entry = pldm_bios_table_attr_value_find_by_handle(
        table->data(), table->size(), attributeHandle);
    if (entry == nullptr)
    {
        return ccOnlyResponse(request, PLDM_INVALID_BIOS_ATTR_HANDLE);
    }

    auto entryLength = pldm_bios_table_attr_value_entry_length(entry);
    Response response(sizeof(pldm_msg_hdr) +
                          PLDM_GET_BIOS_ATTR_CURR_VAL_BY_HANDLE_MIN_RESP_BYTES +
                          entryLength,
                      0);
    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());
    rc = encode_get_bios_current_value_by_handle_resp(
        request->hdr.instance_id, PLDM_SUCCESS, 0, PLDM_START_AND_END,
        reinterpret_cast<const uint8_t*>(entry), entryLength, responsePtr);
    if (rc != PLDM_SUCCESS)
    {
        return ccOnlyResponse(request, rc);
    }

    return response;
}

Response Handler::setBIOSAttributeCurrentValue(const pldm_msg* request,
                                               size_t payloadLength)
{
    uint32_t transferHandle;
    uint8_t transferOpFlag;
    variable_field attributeField;

    auto rc = decode_set_bios_attribute_current_value_req(
        request, payloadLength, &transferHandle, &transferOpFlag,
        &attributeField);
    if (rc != PLDM_SUCCESS)
    {
        return ccOnlyResponse(request, rc);
    }

    rc = biosConfig.setAttrValue(attributeField.ptr, attributeField.length);

    Response response(
        sizeof(pldm_msg_hdr) + PLDM_SET_BIOS_ATTR_CURR_VAL_RESP_BYTES, 0);
    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());

    encode_set_bios_attribute_current_value_resp(request->hdr.instance_id, rc,
                                                 0, responsePtr);

    return response;
}

} // namespace bios
} // namespace responder
} // namespace pldm
