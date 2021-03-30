#pragma once

#include "libpldm/platform.h"

#include "libpldmresponder/pdr_utils.hpp"

namespace pldm
{

namespace responder
{

namespace pdr_numeric_effecter
{

using Json = nlohmann::json;

static const Json empty{};

/** @brief Parse PDR JSON file and generate numeric effecter PDR structure
 *
 *  @param[in] json - the JSON Object with the numeric effecter PDR
 *  @param[out] handler - the Parser of PLDM command handler
 *  @param[out] repo - pdr::RepoInterface
 *
 */
template <class DBusInterface, class Handler>
void generateNumericEffecterPDR(const DBusInterface& dBusIntf, const Json& json,
                                Handler& handler,
                                pdr_utils::RepoInterface& repo)
{
    static const std::vector<Json> emptyList{};
    auto entries = json.value("entries", emptyList);
    for (const auto& e : entries)
    {
        std::vector<uint8_t> entry{};
        entry.resize(sizeof(pldm_numeric_effecter_value_pdr));

        pldm_numeric_effecter_value_pdr* pdr =
            reinterpret_cast<pldm_numeric_effecter_value_pdr*>(entry.data());
        pdr->hdr.record_handle = 0;
        pdr->hdr.version = 1;
        pdr->hdr.type = PLDM_NUMERIC_EFFECTER_PDR;
        pdr->hdr.record_change_num = 0;
        pdr->hdr.length =
            sizeof(pldm_numeric_effecter_value_pdr) - sizeof(pldm_pdr_hdr);

        pdr->terminus_handle = e.value("terminus_handle", 0);
        pdr->effecter_id = handler.getNextEffecterId();
        pdr->entity_type = e.value("entity_type", 0);
        pdr->entity_instance = e.value("entity_instance", 0);
        pdr->container_id = e.value("container_id", 0);
        pdr->effecter_semantic_id = e.value("effecter_semantic_id", 0);
        pdr->effecter_init = e.value("effecter_init", PLDM_NO_INIT);
        pdr->effecter_auxiliary_names = e.value("effecter_init", false);
        pdr->base_unit = e.value("base_unit", 0);
        pdr->unit_modifier = e.value("unit_modifier", 0);
        pdr->rate_unit = e.value("rate_unit", 0);
        pdr->base_oem_unit_handle = e.value("base_oem_unit_handle", 0);
        pdr->aux_unit = e.value("aux_unit", 0);
        pdr->aux_unit_modifier = e.value("aux_unit_modifier", 0);
        pdr->aux_oem_unit_handle = e.value("aux_oem_unit_handle", 0);
        pdr->aux_rate_unit = e.value("aux_rate_unit", 0);
        pdr->is_linear = e.value("is_linear", true);
        pdr->effecter_data_size =
            e.value("effecter_data_size", PLDM_EFFECTER_DATA_SIZE_UINT8);
        pdr->resolution = e.value("effecter_resolution_init", 1.00);
        pdr->offset = e.value("offset", 0.00);
        pdr->accuracy = e.value("accuracy", 0);
        pdr->plus_tolerance = e.value("plus_tolerance", 0);
        pdr->minus_tolerance = e.value("minus_tolerance", 0);
        pdr->state_transition_interval =
            e.value("state_transition_interval", 0.00);
        pdr->transition_interval = e.value("transition_interval", 0.00);
        switch (pdr->effecter_data_size)
        {
            case PLDM_EFFECTER_DATA_SIZE_UINT8:
                pdr->max_set_table.value_u8 = e.value("max_set_table", 0);
                pdr->min_set_table.value_u8 = e.value("min_set_table", 0);
                break;
            case PLDM_EFFECTER_DATA_SIZE_SINT8:
                pdr->max_set_table.value_s8 = e.value("max_set_table", 0);
                pdr->min_set_table.value_s8 = e.value("min_set_table", 0);
                break;
            case PLDM_EFFECTER_DATA_SIZE_UINT16:
                pdr->max_set_table.value_u16 = e.value("max_set_table", 0);
                pdr->min_set_table.value_u16 = e.value("min_set_table", 0);
                break;
            case PLDM_EFFECTER_DATA_SIZE_SINT16:
                pdr->max_set_table.value_s16 = e.value("max_set_table", 0);
                pdr->min_set_table.value_s16 = e.value("min_set_table", 0);
                break;
            case PLDM_EFFECTER_DATA_SIZE_UINT32:
                pdr->max_set_table.value_u32 = e.value("max_set_table", 0);
                pdr->min_set_table.value_u32 = e.value("min_set_table", 0);
                break;
            case PLDM_EFFECTER_DATA_SIZE_SINT32:
                pdr->max_set_table.value_s32 = e.value("max_set_table", 0);
                pdr->min_set_table.value_s32 = e.value("min_set_table", 0);
                break;
            default:
                break;
        }

        pdr->range_field_format =
            e.value("range_field_format", PLDM_RANGE_FIELD_FORMAT_UINT8);
        pdr->range_field_support.byte = e.value("range_field_support", 0);
        switch (pdr->range_field_format)
        {
            case PLDM_RANGE_FIELD_FORMAT_UINT8:
                pdr->nominal_value.value_u8 = e.value("nominal_value", 0);
                pdr->normal_max.value_u8 = e.value("normal_max", 0);
                pdr->normal_min.value_u8 = e.value("normal_min", 0);
                pdr->rated_max.value_u8 = e.value("rated_max", 0);
                pdr->rated_min.value_u8 = e.value("rated_min", 0);
                break;
            case PLDM_RANGE_FIELD_FORMAT_SINT8:
                pdr->nominal_value.value_s8 = e.value("nominal_value", 0);
                pdr->normal_max.value_s8 = e.value("normal_max", 0);
                pdr->normal_min.value_s8 = e.value("normal_min", 0);
                pdr->rated_max.value_s8 = e.value("rated_max", 0);
                pdr->rated_min.value_s8 = e.value("rated_min", 0);
                break;
            case PLDM_RANGE_FIELD_FORMAT_UINT16:
                pdr->nominal_value.value_u16 = e.value("nominal_value", 0);
                pdr->normal_max.value_u16 = e.value("normal_max", 0);
                pdr->normal_min.value_u16 = e.value("normal_min", 0);
                pdr->rated_max.value_u16 = e.value("rated_max", 0);
                pdr->rated_min.value_u16 = e.value("rated_min", 0);
                break;
            case PLDM_RANGE_FIELD_FORMAT_SINT16:
                pdr->nominal_value.value_s16 = e.value("nominal_value", 0);
                pdr->normal_max.value_s16 = e.value("normal_max", 0);
                pdr->normal_min.value_s16 = e.value("normal_min", 0);
                pdr->rated_max.value_s16 = e.value("rated_max", 0);
                pdr->rated_min.value_s16 = e.value("rated_min", 0);
                break;
            case PLDM_RANGE_FIELD_FORMAT_UINT32:
                pdr->nominal_value.value_u32 = e.value("nominal_value", 0);
                pdr->normal_max.value_u32 = e.value("normal_max", 0);
                pdr->normal_min.value_u32 = e.value("normal_min", 0);
                pdr->rated_max.value_u32 = e.value("rated_max", 0);
                pdr->rated_min.value_u32 = e.value("rated_min", 0);
                break;
            case PLDM_RANGE_FIELD_FORMAT_SINT32:
                pdr->nominal_value.value_s32 = e.value("nominal_value", 0);
                pdr->normal_max.value_s32 = e.value("normal_max", 0);
                pdr->normal_min.value_s32 = e.value("normal_min", 0);
                pdr->rated_max.value_s32 = e.value("rated_max", 0);
                pdr->rated_min.value_s32 = e.value("rated_min", 0);
                break;
            case PLDM_RANGE_FIELD_FORMAT_REAL32:
                pdr->nominal_value.value_f32 = e.value("nominal_value", 0);
                pdr->normal_max.value_f32 = e.value("normal_max", 0);
                pdr->normal_min.value_f32 = e.value("normal_min", 0);
                pdr->rated_max.value_f32 = e.value("rated_max", 0);
                pdr->rated_min.value_f32 = e.value("rated_min", 0);
                break;
            default:
                break;
        }

        auto dbusEntry = e.value("dbus", empty);
        auto objectPath = dbusEntry.value("path", "");
        auto interface = dbusEntry.value("interface", "");
        auto propertyName = dbusEntry.value("property_name", "");
        auto propertyType = dbusEntry.value("property_type", "");

        try
        {
            auto service =
                dBusIntf.getService(objectPath.c_str(), interface.c_str());
        }
        catch (const std::exception& e)
        {
            continue;
        }

        pldm::utils::DBusMapping dbusMapping{objectPath, interface,
                                             propertyName, propertyType};
        DbusMappings dbusMappings{};
        DbusValMaps dbusValMaps{};
        dbusMappings.emplace_back(std::move(dbusMapping));
        handler.addDbusObjMaps(
            pdr->effecter_id,
            std::make_tuple(std::move(dbusMappings), std::move(dbusValMaps)));

        pdr_utils::PdrEntry pdrEntry{};
        pdrEntry.data = entry.data();
        pdrEntry.size = sizeof(pldm_numeric_effecter_value_pdr);
        repo.addRecord(pdrEntry);
    }
}

} // namespace pdr_numeric_effecter
} // namespace responder
} // namespace pldm
