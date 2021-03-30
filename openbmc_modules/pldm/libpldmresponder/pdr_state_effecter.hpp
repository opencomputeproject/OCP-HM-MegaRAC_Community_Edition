#pragma once

#include "libpldm/platform.h"

#include "libpldmresponder/pdr_utils.hpp"

namespace pldm
{

namespace responder
{

namespace pdr_state_effecter
{

using Json = nlohmann::json;

static const Json empty{};

/** @brief Parse PDR JSON file and generate state effecter PDR structure
 *
 *  @param[in] json - the JSON Object with the state effecter PDR
 *  @param[out] handler - the Parser of PLDM command handler
 *  @param[out] repo - pdr::RepoInterface
 *
 */
template <class DBusInterface, class Handler>
void generateStateEffecterPDR(const DBusInterface& dBusIntf, const Json& json,
                              Handler& handler, pdr_utils::RepoInterface& repo)
{
    static const std::vector<Json> emptyList{};
    auto entries = json.value("entries", emptyList);
    for (const auto& e : entries)
    {
        size_t pdrSize = 0;
        auto effecters = e.value("effecters", emptyList);
        for (const auto& effecter : effecters)
        {
            auto set = effecter.value("set", empty);
            auto statesSize = set.value("size", 0);
            if (!statesSize)
            {
                std::cerr << "Malformed PDR JSON return "
                             "pdrEntry;- no state set "
                             "info, TYPE="
                          << PLDM_STATE_EFFECTER_PDR << "\n";
                throw InternalFailure();
            }
            pdrSize += sizeof(state_effecter_possible_states) -
                       sizeof(bitfield8_t) + (sizeof(bitfield8_t) * statesSize);
        }
        pdrSize += sizeof(pldm_state_effecter_pdr) - sizeof(uint8_t);

        std::vector<uint8_t> entry{};
        entry.resize(pdrSize);

        pldm_state_effecter_pdr* pdr =
            reinterpret_cast<pldm_state_effecter_pdr*>(entry.data());
        pdr->hdr.record_handle = 0;
        pdr->hdr.version = 1;
        pdr->hdr.type = PLDM_STATE_EFFECTER_PDR;
        pdr->hdr.record_change_num = 0;
        pdr->hdr.length = pdrSize - sizeof(pldm_pdr_hdr);

        pdr->terminus_handle = 0;
        pdr->effecter_id = handler.getNextEffecterId();
        pdr->entity_type = e.value("type", 0);
        pdr->entity_instance = e.value("instance", 0);
        pdr->container_id = e.value("container", 0);
        pdr->effecter_semantic_id = 0;
        pdr->effecter_init = PLDM_NO_INIT;
        pdr->has_description_pdr = false;
        pdr->composite_effecter_count = effecters.size();

        DbusMappings dbusMappings{};
        DbusValMaps dbusValMaps{};
        uint8_t* start =
            entry.data() + sizeof(pldm_state_effecter_pdr) - sizeof(uint8_t);
        for (const auto& effecter : effecters)
        {
            auto set = effecter.value("set", empty);
            state_effecter_possible_states* possibleStates =
                reinterpret_cast<state_effecter_possible_states*>(start);
            possibleStates->state_set_id = set.value("id", 0);
            possibleStates->possible_states_size = set.value("size", 0);

            start += sizeof(possibleStates->state_set_id) +
                     sizeof(possibleStates->possible_states_size);
            static const std::vector<uint8_t> emptyStates{};
            PossibleValues stateValues;
            auto states = set.value("states", emptyStates);
            for (const auto& state : states)
            {
                auto index = state / 8;
                auto bit = state - (index * 8);
                bitfield8_t* bf = reinterpret_cast<bitfield8_t*>(start + index);
                bf->byte |= 1 << bit;
                stateValues.emplace_back(std::move(state));
            }
            start += possibleStates->possible_states_size;

            auto dbusEntry = effecter.value("dbus", empty);
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
            dbusMappings.emplace_back(std::move(dbusMapping));

            Json propValues = dbusEntry["property_values"];
            StatestoDbusVal dbusIdToValMap =
                populateMapping(propertyType, propValues, stateValues);
            if (!dbusIdToValMap.empty())
            {
                dbusValMaps.emplace_back(std::move(dbusIdToValMap));
            }
        }
        handler.addDbusObjMaps(
            pdr->effecter_id,
            std::make_tuple(std::move(dbusMappings), std::move(dbusValMaps)));
        PdrEntry pdrEntry{};
        pdrEntry.data = entry.data();
        pdrEntry.size = pdrSize;
        repo.addRecord(pdrEntry);
    }
}

} // namespace pdr_state_effecter
} // namespace responder
} // namespace pldm
