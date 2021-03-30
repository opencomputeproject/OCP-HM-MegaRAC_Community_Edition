#pragma once

#include "config.h"

#include "libpldm/platform.h"
#include "libpldm/states.h"

#include "common/utils.hpp"
#include "libpldmresponder/pdr.hpp"
#include "pdr_utils.hpp"
#include "pldmd/handler.hpp"

#include <math.h>
#include <stdint.h>

#include <map>
#include <optional>

using namespace pldm::responder::pdr;
using namespace pldm::utils;

namespace pldm
{
namespace responder
{
namespace platform_numeric_effecter
{

/** @brief Function to get the effecter value by PDR factor coefficient, etc.
 *  @param[in] pdr - The structure of pldm_numeric_effecter_value_pdr.
 *  @tparam[in] effecterValue - effecter value.
 *
 *  @return - std::pair<int, std::optional<PropertyValue>> - rc:Success or
 *          failure, PropertyValue: The value to be set
 */
template <typename T>
std::pair<int, std::optional<PropertyValue>>
    getEffecterRawValue(const pldm_numeric_effecter_value_pdr* pdr,
                        T& effecterValue)
{
    // X = Round [ (Y - B) / m ]
    // refer to DSP0248_1.2.0 27.8
    int rc = 0;
    PropertyValue value;
    switch (pdr->effecter_data_size)
    {
        case PLDM_EFFECTER_DATA_SIZE_UINT8:
        {
            auto rawValue = static_cast<uint8_t>(
                round(effecterValue - pdr->offset) / pdr->resolution);
            if (pdr->min_set_table.value_u8 < pdr->max_set_table.value_u8 &&
                (rawValue < pdr->min_set_table.value_u8 ||
                 rawValue > pdr->max_set_table.value_u8))
            {
                rc = PLDM_ERROR_INVALID_DATA;
            }
            value = rawValue;
            break;
        }
        case PLDM_EFFECTER_DATA_SIZE_SINT8:
        {
            auto rawValue = static_cast<int8_t>(
                round(effecterValue - pdr->offset) / pdr->resolution);
            if (pdr->min_set_table.value_s8 < pdr->max_set_table.value_s8 &&
                (rawValue < pdr->min_set_table.value_s8 ||
                 rawValue > pdr->max_set_table.value_s8))
            {
                rc = PLDM_ERROR_INVALID_DATA;
            }
            value = rawValue;
            break;
        }
        case PLDM_EFFECTER_DATA_SIZE_UINT16:
        {
            auto rawValue = static_cast<uint16_t>(
                round(effecterValue - pdr->offset) / pdr->resolution);
            if (pdr->min_set_table.value_u16 < pdr->max_set_table.value_u16 &&
                (rawValue < pdr->min_set_table.value_u16 ||
                 rawValue > pdr->max_set_table.value_u16))
            {
                rc = PLDM_ERROR_INVALID_DATA;
            }
            value = rawValue;
            break;
        }
        case PLDM_EFFECTER_DATA_SIZE_SINT16:
        {
            auto rawValue = static_cast<int16_t>(
                round(effecterValue - pdr->offset) / pdr->resolution);
            if (pdr->min_set_table.value_s16 < pdr->max_set_table.value_s16 &&
                (rawValue < pdr->min_set_table.value_s16 ||
                 rawValue > pdr->max_set_table.value_s16))
            {
                rc = PLDM_ERROR_INVALID_DATA;
            }
            value = rawValue;
            break;
        }
        case PLDM_EFFECTER_DATA_SIZE_UINT32:
        {
            auto rawValue = static_cast<uint32_t>(
                round(effecterValue - pdr->offset) / pdr->resolution);
            if (pdr->min_set_table.value_u32 < pdr->max_set_table.value_u32 &&
                (rawValue < pdr->min_set_table.value_u32 ||
                 rawValue > pdr->max_set_table.value_u32))
            {
                rc = PLDM_ERROR_INVALID_DATA;
            }
            value = rawValue;
            break;
        }
        case PLDM_EFFECTER_DATA_SIZE_SINT32:
        {
            auto rawValue = static_cast<int32_t>(
                round(effecterValue - pdr->offset) / pdr->resolution);
            if (pdr->min_set_table.value_s32 < pdr->max_set_table.value_s32 &&
                (rawValue < pdr->min_set_table.value_s32 ||
                 rawValue > pdr->max_set_table.value_s32))
            {
                rc = PLDM_ERROR_INVALID_DATA;
            }
            value = rawValue;
            break;
        }
    }

    return {rc, std::make_optional(std::move(value))};
}

/** @brief Function to convert the D-Bus value by PDR factor and effecter value.
 *  @param[in] pdr - The structure of pldm_numeric_effecter_value_pdr.
 *  @param[in] effecterDataSize - effecter value size.
 *  @param[in,out] effecterValue - effecter value.
 *
 *  @return std::pair<int, std::optional<PropertyValue>> - rc:Success or
 *          failure, PropertyValue: The value to be set
 */
std::pair<int, std::optional<PropertyValue>>
    convertToDbusValue(const pldm_numeric_effecter_value_pdr* pdr,
                       uint8_t effecterDataSize, uint8_t* effecterValue)
{
    if (effecterDataSize == PLDM_EFFECTER_DATA_SIZE_UINT8)
    {
        uint8_t currentValue = *(reinterpret_cast<uint8_t*>(&effecterValue[0]));
        return getEffecterRawValue<uint8_t>(pdr, currentValue);
    }
    else if (effecterDataSize == PLDM_EFFECTER_DATA_SIZE_SINT8)
    {
        int8_t currentValue = *(reinterpret_cast<int8_t*>(&effecterValue[0]));
        return getEffecterRawValue<int8_t>(pdr, currentValue);
    }
    else if (effecterDataSize == PLDM_EFFECTER_DATA_SIZE_UINT16)
    {
        uint16_t currentValue =
            *(reinterpret_cast<uint16_t*>(&effecterValue[0]));
        return getEffecterRawValue<uint16_t>(pdr, currentValue);
    }
    else if (effecterDataSize == PLDM_EFFECTER_DATA_SIZE_SINT16)
    {
        int16_t currentValue = *(reinterpret_cast<int16_t*>(&effecterValue[0]));
        return getEffecterRawValue<int16_t>(pdr, currentValue);
    }
    else if (effecterDataSize == PLDM_EFFECTER_DATA_SIZE_UINT32)
    {
        uint32_t currentValue =
            *(reinterpret_cast<uint32_t*>(&effecterValue[0]));
        return getEffecterRawValue<uint32_t>(pdr, currentValue);
    }
    else if (effecterDataSize == PLDM_EFFECTER_DATA_SIZE_SINT32)
    {
        int32_t currentValue = *(reinterpret_cast<int32_t*>(&effecterValue[0]));
        return getEffecterRawValue<int32_t>(pdr, currentValue);
    }
    else
    {
        std::cerr << "Wrong field effecterDataSize...\n";
        return {PLDM_ERROR, {}};
    }
}

/** @brief Function to set the effecter value requested by pldm requester
 *  @tparam[in] DBusInterface - DBus interface type
 *  @tparam[in] Handler - pldm::responder::platform::Handler
 *  @param[in] dBusIntf - The interface object of DBusInterface
 *  @param[in] handler - The interface object of
 *             pldm::responder::platform::Handler
 *  @param[in] effecterId - Effecter ID sent by the requester to act on
 *  @param[in] effecterDataSize - The bit width and format of the setting
 * 				value for the effecter
 *  @param[in] effecter_value - The setting value of numeric effecter being
 * 				requested.
 *  @param[in] effecterValueLength - The setting value length of numeric
 *              effecter being requested.
 *  @return - Success or failure in setting the states. Returns failure in
 * terms of PLDM completion codes if atleast one state fails to be set
 */
template <class DBusInterface, class Handler>
int setNumericEffecterValueHandler(const DBusInterface& dBusIntf,
                                   Handler& handler, uint16_t effecterId,
                                   uint8_t effecterDataSize,
                                   uint8_t* effecterValue,
                                   size_t effecterValueLength)
{
    constexpr auto effecterValueArrayLength = 4;
    pldm_numeric_effecter_value_pdr* pdr = nullptr;

    std::unique_ptr<pldm_pdr, decltype(&pldm_pdr_destroy)>
        numericEffecterPdrRepo(pldm_pdr_init(), pldm_pdr_destroy);
    Repo numericEffecterPDRs(numericEffecterPdrRepo.get());
    getRepoByType(handler.getRepo(), numericEffecterPDRs,
                  PLDM_NUMERIC_EFFECTER_PDR);
    if (numericEffecterPDRs.empty())
    {
        std::cerr << "The Numeric Effecter PDR repo is empty." << std::endl;
        return PLDM_ERROR;
    }

    // Get the pdr structure of pldm_numeric_effecter_value_pdr according
    // to the effecterId
    PdrEntry pdrEntry{};
    auto pdrRecord = numericEffecterPDRs.getFirstRecord(pdrEntry);
    while (pdrRecord)
    {
        pdr = reinterpret_cast<pldm_numeric_effecter_value_pdr*>(pdrEntry.data);
        if (pdr->effecter_id != effecterId)
        {
            pdr = nullptr;
            pdrRecord = numericEffecterPDRs.getNextRecord(pdrRecord, pdrEntry);
            continue;
        }

        break;
    }

    if (!pdr)
    {
        return PLDM_PLATFORM_INVALID_EFFECTER_ID;
    }

    if (effecterValueLength != effecterValueArrayLength)
    {
        std::cerr << "effecter data size is incorrect.\n";
        return PLDM_ERROR_INVALID_DATA;
    }

    // convert to dbus effectervalue according to the factor
    auto [rc, dbusValue] =
        convertToDbusValue(pdr, effecterDataSize, effecterValue);
    if (rc != PLDM_SUCCESS)
    {
        return rc;
    }

    const auto& [dbusMappings, dbusValMaps] =
        handler.getDbusObjMaps(effecterId);
    DBusMapping dbusMapping{
        dbusMappings[0].objectPath, dbusMappings[0].interface,
        dbusMappings[0].propertyName, dbusMappings[0].propertyType};
    try
    {
        dBusIntf.setDbusProperty(dbusMapping, dbusValue.value());
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error setting property, ERROR=" << e.what()
                  << " PROPERTY=" << dbusMapping.propertyName << " INTERFACE="
                  << dbusMapping.interface << " PATH=" << dbusMapping.objectPath
                  << "\n";
        return PLDM_ERROR;
    }

    return PLDM_SUCCESS;
}

} // namespace platform_numeric_effecter
} // namespace responder
} // namespace pldm
