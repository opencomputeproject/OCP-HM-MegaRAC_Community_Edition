#include "libpldm/entity.h"
#include "libpldm/state_set.h"

#include "common/types.hpp"
#include "pldm_cmd_helper.hpp"

namespace pldmtool
{

namespace platform
{

namespace
{

using namespace pldmtool::helper;
std::vector<std::unique_ptr<CommandInterface>> commands;

} // namespace

class GetPDR : public CommandInterface
{
  public:
    ~GetPDR() = default;
    GetPDR() = delete;
    GetPDR(const GetPDR&) = delete;
    GetPDR(GetPDR&&) = default;
    GetPDR& operator=(const GetPDR&) = delete;
    GetPDR& operator=(GetPDR&&) = default;

    using CommandInterface::CommandInterface;

    explicit GetPDR(const char* type, const char* name, CLI::App* app) :
        CommandInterface(type, name, app)
    {
        app->add_option(
               "-d,--data", recordHandle,
               "retrieve individual PDRs from a PDR Repository\n"
               "eg: The recordHandle value for the PDR to be retrieved and 0 "
               "means get first PDR in the repository.")
            ->required();
    }

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) +
                                        PLDM_GET_PDR_REQ_BYTES);
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

        auto rc =
            encode_get_pdr_req(instanceId, recordHandle, 0, PLDM_GET_FIRSTPART,
                               UINT16_MAX, 0, request, PLDM_GET_PDR_REQ_BYTES);
        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t completionCode = 0;
        uint8_t recordData[UINT16_MAX] = {0};
        uint32_t nextRecordHndl = 0;
        uint32_t nextDataTransferHndl = 0;
        uint8_t transferFlag = 0;
        uint16_t respCnt = 0;
        uint8_t transferCRC = 0;

        auto rc = decode_get_pdr_resp(
            responsePtr, payloadLength, &completionCode, &nextRecordHndl,
            &nextDataTransferHndl, &transferFlag, &respCnt, recordData,
            sizeof(recordData), &transferCRC);

        if (rc != PLDM_SUCCESS || completionCode != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)completionCode
                      << std::endl;
            return;
        }

        printPDRMsg(nextRecordHndl, respCnt, recordData);
    }

  private:
    const std::map<pldm::pdr::EntityType, std::string> entityType = {
        {PLDM_ENTITY_COMM_CHANNEL, "Communication Channel"},
        {PLDM_ENTITY_SYS_FIRMWARE, "System Firmware"},
        {PLDM_ENTITY_VIRTUAL_MACHINE_MANAGER, "Virtual Machine Manager"},
        {PLDM_ENTITY_SYSTEM_CHASSIS, "System chassis (main enclosure)"},
        {PLDM_ENTITY_SYS_BOARD, "System Board"},
        {PLDM_ENTITY_MEMORY_MODULE, "Memory Module"},
        {PLDM_ENTITY_PROC_MODULE, "Processor Module"},
        {PLDM_ENTITY_CHASSIS_FRONT_PANEL_BOARD,
         "Chassis front panel board (control panel)"},
        {PLDM_ENTITY_POWER_CONVERTER, "Power converter"},
        {PLDM_ENTITY_PROC, "Processor"},
        {PLDM_ENTITY_MGMT_CONTROLLER, "Management Controller"},
        {PLDM_ENTITY_CONNECTOR, "Connector"},
        {PLDM_ENTITY_POWER_SUPPLY, "Power Supply"},
        {11521, "System (logical)"},
    };

    const std::map<uint16_t, std::string> stateSet = {
        {PLDM_STATE_SET_HEALTH_STATE, "Health State"},
        {PLDM_STATE_SET_AVAILABILITY, "Availability"},
        {PLDM_STATE_SET_OPERATIONAL_STATUS, "Operational Status"},
        {PLDM_STATE_SET_OPERATIONAL_RUNNING_STATUS,
         "Operational Running Status"},
        {PLDM_STATE_SET_PRESENCE, "Presence"},
        {PLDM_STATE_SET_CONFIGURATION_STATE, "Configuration State"},
        {PLDM_STATE_SET_LINK_STATE, "Link State"},
        {PLDM_STATE_SET_SW_TERMINATION_STATUS, "Software Termination Status"},
        {PLDM_STATE_SET_BOOT_RESTART_CAUSE, "Boot/Restart Cause"},
        {PLDM_STATE_SET_BOOT_PROGRESS, "Boot Progress"},
        {PLDM_STATE_SET_SYSTEM_POWER_STATE, "System Power State"},
    };

    const std::array<std::string_view, 4> sensorInit = {
        "noInit", "useInitPDR", "enableSensor", "disableSensor"};

    const std::array<std::string_view, 4> effecterInit = {
        "noInit", "useInitPDR", "enableEffecter", "disableEffecter"};

    const std::map<uint8_t, std::string> pdrType = {
        {PLDM_TERMINUS_LOCATOR_PDR, "Terminus Locator PDR"},
        {PLDM_NUMERIC_SENSOR_PDR, "Numeric Sensor PDR"},
        {PLDM_NUMERIC_SENSOR_INITIALIZATION_PDR,
         "Numeric Sensor Initialization PDR"},
        {PLDM_STATE_SENSOR_PDR, "State Sensor PDR"},
        {PLDM_STATE_SENSOR_INITIALIZATION_PDR,
         "State Sensor Initialization PDR"},
        {PLDM_SENSOR_AUXILIARY_NAMES_PDR, "Sensor Auxiliary Names PDR"},
        {PLDM_OEM_UNIT_PDR, "OEM Unit PDR"},
        {PLDM_OEM_STATE_SET_PDR, "OEM State Set PDR"},
        {PLDM_NUMERIC_EFFECTER_PDR, "Numeric Effecter PDR"},
        {PLDM_NUMERIC_EFFECTER_INITIALIZATION_PDR,
         "Numeric Effecter Initialization PDR"},
        {PLDM_STATE_EFFECTER_PDR, "State Effecter PDR"},
        {PLDM_STATE_EFFECTER_INITIALIZATION_PDR,
         "State Effecter Initialization PDR"},
        {PLDM_EFFECTER_AUXILIARY_NAMES_PDR, "Effecter Auxiliary Names PDR"},
        {PLDM_EFFECTER_OEM_SEMANTIC_PDR, "Effecter OEM Semantic PDR"},
        {PLDM_PDR_ENTITY_ASSOCIATION, "Entity Association PDR"},
        {PLDM_ENTITY_AUXILIARY_NAMES_PDR, "Entity Auxiliary Names PDR"},
        {PLDM_OEM_ENTITY_ID_PDR, "OEM Entity ID PDR"},
        {PLDM_INTERRUPT_ASSOCIATION_PDR, "Interrupt Association PDR"},
        {PLDM_EVENT_LOG_PDR, "PLDM Event Log PDR"},
        {PLDM_PDR_FRU_RECORD_SET, "FRU Record Set PDR"},
        {PLDM_OEM_DEVICE_PDR, "OEM Device PDR"},
        {PLDM_OEM_PDR, "OEM PDR"},
    };

    std::string getEntityName(pldm::pdr::EntityType type)
    {
        try
        {
            return entityType.at(type);
        }
        catch (const std::out_of_range& e)
        {
            return std::to_string(static_cast<unsigned>(type)) + "(OEM)";
        }
    }

    std::string getStateSetName(uint16_t id)
    {
        auto typeString = std::to_string(id);
        try
        {
            return stateSet.at(id) + "(" + typeString + ")";
        }
        catch (const std::out_of_range& e)
        {
            return typeString;
        }
    }

    std::string getPDRType(uint8_t type)
    {
        auto typeString = std::to_string(type);
        try
        {
            return pdrType.at(type) + "(" + typeString + ")";
        }
        catch (const std::out_of_range& e)
        {
            return typeString;
        }
    }

    void printCommonPDRHeader(const pldm_pdr_hdr* hdr)
    {
        std::cout << "recordHandle: " << hdr->record_handle << std::endl;
        std::cout << "PDRHeaderVersion: " << unsigned(hdr->version)
                  << std::endl;
        std::cout << "PDRType: " << getPDRType(hdr->type) << std::endl;
        std::cout << "recordChangeNumber: " << hdr->record_change_num
                  << std::endl;
        std::cout << "dataLength: " << hdr->length << std::endl << std::endl;
    }

    void printPossibleStates(uint8_t possibleStatesSize,
                             const bitfield8_t* states)
    {
        uint8_t possibleStatesPos{};
        auto printStates = [&possibleStatesPos](const bitfield8_t& val) {
            for (int i = 0; i < CHAR_BIT; i++)
            {
                if (val.byte & (1 << i))
                {
                    std::cout << " " << (possibleStatesPos * CHAR_BIT + i);
                }
            }
            possibleStatesPos++;
        };
        std::for_each(states, states + possibleStatesSize, printStates);
    }

    void printStateSensorPDR(const uint8_t* data)
    {
        auto pdr = reinterpret_cast<const pldm_state_sensor_pdr*>(data);

        std::cout << "PLDMTerminusHandle: " << pdr->terminus_handle
                  << std::endl;
        std::cout << "sensorID: " << pdr->sensor_id << std::endl;
        std::cout << "entityType: " << getEntityName(pdr->entity_type)
                  << std::endl;
        std::cout << "entityInstanceNumber: " << pdr->entity_instance
                  << std::endl;
        std::cout << "containerID: " << pdr->container_id << std::endl;
        std::cout << "sensorInit: " << sensorInit[pdr->sensor_init]
                  << std::endl;
        std::cout << "sensorAuxiliaryNamesPDR: "
                  << (pdr->sensor_auxiliary_names_pdr ? "true" : "false")
                  << std::endl;
        std::cout << "compositeSensorCount: "
                  << unsigned(pdr->composite_sensor_count) << std::endl;

        auto statesPtr = pdr->possible_states;
        auto compositeSensorCount = pdr->composite_sensor_count;

        while (compositeSensorCount--)
        {
            auto state = reinterpret_cast<const state_sensor_possible_states*>(
                statesPtr);
            std::cout << "stateSetID: " << getStateSetName(state->state_set_id)
                      << std::endl;
            std::cout << "possibleStatesSize: "
                      << unsigned(state->possible_states_size) << std::endl;
            std::cout << "possibleStates:";
            printPossibleStates(state->possible_states_size, state->states);
            std::cout << std::endl;

            if (compositeSensorCount)
            {
                statesPtr += sizeof(state_sensor_possible_states) +
                             state->possible_states_size - 1;
            }
        }
    }

    void printPDRFruRecordSet(uint8_t* data)
    {
        if (data == NULL)
        {
            return;
        }

        data += sizeof(pldm_pdr_hdr);
        pldm_pdr_fru_record_set* pdr =
            reinterpret_cast<pldm_pdr_fru_record_set*>(data);

        std::cout << "PLDMTerminusHandle: " << pdr->terminus_handle
                  << std::endl;
        std::cout << "FRURecordSetIdentifier: " << pdr->fru_rsi << std::endl;
        std::cout << "entityType: " << getEntityName(pdr->entity_type)
                  << std::endl;
        std::cout << "entityInstanceNumber: " << pdr->entity_instance_num
                  << std::endl;
        std::cout << "containerID: " << pdr->container_id << std::endl;
    }

    void printPDREntityAssociation(uint8_t* data)
    {
        const std::map<uint8_t, const char*> assocationType = {
            {PLDM_ENTITY_ASSOCIAION_PHYSICAL, "Physical"},
            {PLDM_ENTITY_ASSOCIAION_LOGICAL, "Logical"},
        };

        if (data == NULL)
        {
            return;
        }

        data += sizeof(pldm_pdr_hdr);
        pldm_pdr_entity_association* pdr =
            reinterpret_cast<pldm_pdr_entity_association*>(data);

        std::cout << "containerID: " << pdr->container_id << std::endl;
        std::cout << "associationType: "
                  << assocationType.at(pdr->association_type) << std::endl
                  << std::endl;

        std::cout << "containerEntityType: "
                  << getEntityName(pdr->container.entity_type) << std::endl;
        std::cout << "containerEntityInstanceNumber: "
                  << pdr->container.entity_instance_num << std::endl;
        std::cout << "containerEntityContainerID: "
                  << pdr->container.entity_container_id << std::endl;

        std::cout << "containedEntityCount: "
                  << static_cast<unsigned>(pdr->num_children) << std::endl
                  << std::endl;

        auto child = reinterpret_cast<pldm_entity*>(&pdr->children[0]);
        for (int i = 0; i < pdr->num_children; ++i)
        {
            std::cout << "containedEntityType[" << i + 1
                      << "]: " << getEntityName(child->entity_type)
                      << std::endl;
            std::cout << "containedEntityInstanceNumber[" << i + 1
                      << "]: " << child->entity_instance_num << std::endl;
            std::cout << "containedEntityContainerID[" << i + 1
                      << "]: " << child->entity_container_id << std::endl
                      << std::endl;
            ++child;
        }
    }

    void printNumericEffecterPDR(uint8_t* data)
    {
        struct pldm_numeric_effecter_value_pdr* pdr =
            (struct pldm_numeric_effecter_value_pdr*)data;
        std::cout << "PLDMTerminusHandle: " << pdr->terminus_handle
                  << std::endl;
        std::cout << "effecterID: " << pdr->effecter_id << std::endl;
        std::cout << "entityType: " << pdr->entity_type << std::endl;
        std::cout << "entityInstanceNumber: " << pdr->entity_instance
                  << std::endl;
        std::cout << "containerID: " << pdr->container_id << std::endl;
        std::cout << "effecterSemanticID: " << pdr->effecter_semantic_id
                  << std::endl;
        std::cout << "effecterInit: " << unsigned(pdr->effecter_init)
                  << std::endl;
        std::cout << "effecterAuxiliaryNames: "
                  << (unsigned(pdr->effecter_auxiliary_names) ? "true"
                                                              : "false")
                  << std::endl;
        std::cout << "baseUnit: " << unsigned(pdr->base_unit) << std::endl;
        std::cout << "unitModifier: " << unsigned(pdr->unit_modifier)
                  << std::endl;
        std::cout << "rateUnit: " << unsigned(pdr->rate_unit) << std::endl;
        std::cout << "baseOEMUnitHandle: "
                  << unsigned(pdr->base_oem_unit_handle) << std::endl;
        std::cout << "auxUnit: " << unsigned(pdr->aux_unit) << std::endl;
        std::cout << "auxUnitModifier: " << unsigned(pdr->aux_unit_modifier)
                  << std::endl;
        std::cout << "auxrateUnit: " << unsigned(pdr->aux_rate_unit)
                  << std::endl;
        std::cout << "auxOEMUnitHandle: " << unsigned(pdr->aux_oem_unit_handle)
                  << std::endl;
        std::cout << "isLinear: "
                  << (unsigned(pdr->is_linear) ? "true" : "false") << std::endl;
        std::cout << "effecterDataSize: " << unsigned(pdr->effecter_data_size)
                  << std::endl;
        std::cout << "resolution: " << pdr->resolution << std::endl;
        std::cout << "offset: " << pdr->offset << std::endl;
        std::cout << "accuracy: " << pdr->accuracy << std::endl;
        std::cout << "plusTolerance: " << unsigned(pdr->plus_tolerance)
                  << std::endl;
        std::cout << "minusTolerance: " << unsigned(pdr->minus_tolerance)
                  << std::endl;
        std::cout << "stateTransitionInterval: "
                  << pdr->state_transition_interval << std::endl;
        std::cout << "TransitionInterval: " << pdr->transition_interval
                  << std::endl;
        switch (pdr->effecter_data_size)
        {
            case PLDM_EFFECTER_DATA_SIZE_UINT8:
                std::cout << "maxSettable: "
                          << unsigned(pdr->max_set_table.value_u8) << std::endl;
                std::cout << "minSettable: "
                          << unsigned(pdr->min_set_table.value_u8) << std::endl;
                break;
            case PLDM_EFFECTER_DATA_SIZE_SINT8:
                std::cout << "maxSettable: "
                          << unsigned(pdr->max_set_table.value_s8) << std::endl;
                std::cout << "minSettable: "
                          << unsigned(pdr->min_set_table.value_s8) << std::endl;
                break;
            case PLDM_EFFECTER_DATA_SIZE_UINT16:
                std::cout << "maxSettable: " << pdr->max_set_table.value_u16
                          << std::endl;
                std::cout << "minSettable: " << pdr->min_set_table.value_u16
                          << std::endl;
                break;
            case PLDM_EFFECTER_DATA_SIZE_SINT16:
                std::cout << "maxSettable: " << pdr->max_set_table.value_s16
                          << std::endl;
                std::cout << "minSettable: " << pdr->min_set_table.value_s16
                          << std::endl;
                break;
            case PLDM_EFFECTER_DATA_SIZE_UINT32:
                std::cout << "maxSettable: " << pdr->max_set_table.value_u32
                          << std::endl;
                std::cout << "minSettable: " << pdr->min_set_table.value_u32
                          << std::endl;
                break;
            case PLDM_EFFECTER_DATA_SIZE_SINT32:
                std::cout << "maxSettable: " << pdr->max_set_table.value_s32
                          << std::endl;
                std::cout << "minSettable: " << pdr->min_set_table.value_s32
                          << std::endl;
                break;
            default:
                break;
        }
        std::cout << "rangeFieldFormat: " << unsigned(pdr->range_field_format)
                  << std::endl;
        std::cout << "rangeFieldSupport: "
                  << unsigned(pdr->range_field_support.byte) << std::endl;
        switch (pdr->range_field_format)
        {
            case PLDM_RANGE_FIELD_FORMAT_UINT8:
                std::cout << "nominalValue: "
                          << unsigned(pdr->nominal_value.value_u8) << std::endl;
                std::cout << "normalMax: " << unsigned(pdr->normal_max.value_u8)
                          << std::endl;
                std::cout << "normalMin: " << unsigned(pdr->normal_min.value_u8)
                          << std::endl;
                std::cout << "ratedMax: " << unsigned(pdr->rated_max.value_u8)
                          << std::endl;
                std::cout << "ratedMin: " << unsigned(pdr->rated_min.value_u8)
                          << std::endl;
                break;
            case PLDM_RANGE_FIELD_FORMAT_SINT8:
                std::cout << "nominalValue: "
                          << unsigned(pdr->nominal_value.value_s8) << std::endl;
                std::cout << "normalMax: " << unsigned(pdr->normal_max.value_s8)
                          << std::endl;
                std::cout << "normalMin: " << unsigned(pdr->normal_min.value_s8)
                          << std::endl;
                std::cout << "ratedMax: " << unsigned(pdr->rated_max.value_s8)
                          << std::endl;
                std::cout << "ratedMin: " << unsigned(pdr->rated_min.value_s8)
                          << std::endl;
                break;
            case PLDM_RANGE_FIELD_FORMAT_UINT16:
                std::cout << "nominalValue: " << pdr->nominal_value.value_u16
                          << std::endl;
                std::cout << "normalMax: " << pdr->normal_max.value_u16
                          << std::endl;
                std::cout << "normalMin: " << pdr->normal_min.value_u16
                          << std::endl;
                std::cout << "ratedMax: " << pdr->rated_max.value_u16
                          << std::endl;
                std::cout << "ratedMin: " << pdr->rated_min.value_u16
                          << std::endl;
                break;
            case PLDM_RANGE_FIELD_FORMAT_SINT16:
                std::cout << "nominalValue: " << pdr->nominal_value.value_s16
                          << std::endl;
                std::cout << "normalMax: " << pdr->normal_max.value_s16
                          << std::endl;
                std::cout << "normalMin: " << pdr->normal_min.value_s16
                          << std::endl;
                std::cout << "ratedMax: " << pdr->rated_max.value_s16
                          << std::endl;
                std::cout << "ratedMin: " << pdr->rated_min.value_s16
                          << std::endl;
                break;
            case PLDM_RANGE_FIELD_FORMAT_UINT32:
                std::cout << "nominalValue: " << pdr->nominal_value.value_u32
                          << std::endl;
                std::cout << "normalMax: " << pdr->normal_max.value_u32
                          << std::endl;
                std::cout << "normalMin: " << pdr->normal_min.value_u32
                          << std::endl;
                std::cout << "ratedMax: " << pdr->rated_max.value_u32
                          << std::endl;
                std::cout << "ratedMin: " << pdr->rated_min.value_u32
                          << std::endl;
                break;
            case PLDM_RANGE_FIELD_FORMAT_SINT32:
                std::cout << "nominalValue: " << pdr->nominal_value.value_s32
                          << std::endl;
                std::cout << "normalMax: " << pdr->normal_max.value_s32
                          << std::endl;
                std::cout << "normalMin: " << pdr->normal_min.value_s32
                          << std::endl;
                std::cout << "ratedMax: " << pdr->rated_max.value_s32
                          << std::endl;
                std::cout << "ratedMin: " << pdr->rated_min.value_s32
                          << std::endl;
                break;
            case PLDM_RANGE_FIELD_FORMAT_REAL32:
                std::cout << "nominalValue: " << pdr->nominal_value.value_f32
                          << std::endl;
                std::cout << "normalMax: " << pdr->normal_max.value_f32
                          << std::endl;
                std::cout << "normalMin: " << pdr->normal_min.value_f32
                          << std::endl;
                std::cout << "ratedMax: " << pdr->rated_max.value_f32
                          << std::endl;
                std::cout << "ratedMin: " << pdr->rated_min.value_f32
                          << std::endl;
                break;
            default:
                break;
        }
    }

    void printStateEffecterPDR(const uint8_t* data)
    {
        auto pdr = reinterpret_cast<const pldm_state_effecter_pdr*>(data);

        std::cout << "PLDMTerminusHandle: " << pdr->terminus_handle
                  << std::endl;
        std::cout << "effecterID: " << pdr->effecter_id << std::endl;
        std::cout << "entityType: " << getEntityName(pdr->entity_type)
                  << std::endl;
        std::cout << "entityInstanceNumber: " << pdr->entity_instance
                  << std::endl;
        std::cout << "containerID: " << pdr->container_id << std::endl;
        std::cout << "effecterSemanticID: " << pdr->effecter_semantic_id
                  << std::endl;
        std::cout << "effecterInit: " << effecterInit[pdr->effecter_init]
                  << std::endl;
        std::cout << "effecterDescriptionPDR: "
                  << (pdr->has_description_pdr ? "true" : "false") << std::endl;
        std::cout << "compositeEffecterCount: "
                  << unsigned(pdr->composite_effecter_count) << std::endl;

        auto statesPtr = pdr->possible_states;
        auto compositeEffecterCount = pdr->composite_effecter_count;

        while (compositeEffecterCount--)
        {
            auto state =
                reinterpret_cast<const state_effecter_possible_states*>(
                    statesPtr);
            std::cout << "stateSetID: " << getStateSetName(state->state_set_id)
                      << std::endl;
            std::cout << "possibleStatesSize: "
                      << unsigned(state->possible_states_size) << std::endl;
            std::cout << "possibleStates:";
            printPossibleStates(state->possible_states_size, state->states);
            std::cout << std::endl;

            if (compositeEffecterCount)
            {
                statesPtr += sizeof(state_effecter_possible_states) +
                             state->possible_states_size - 1;
            }
        }
    }

    void printPDRMsg(const uint32_t nextRecordHndl, const uint16_t respCnt,
                     uint8_t* data)
    {
        if (data == NULL)
        {
            return;
        }

        std::cout << "nextRecordHandle: " << nextRecordHndl << std::endl;
        std::cout << "responseCount: " << respCnt << std::endl;

        struct pldm_pdr_hdr* pdr = (struct pldm_pdr_hdr*)data;
        printCommonPDRHeader(pdr);
        switch (pdr->type)
        {
            case PLDM_STATE_SENSOR_PDR:
                printStateSensorPDR(data);
                break;
            case PLDM_NUMERIC_EFFECTER_PDR:
                printNumericEffecterPDR(data);
                break;
            case PLDM_STATE_EFFECTER_PDR:
                printStateEffecterPDR(data);
                break;
            case PLDM_PDR_ENTITY_ASSOCIATION:
                printPDREntityAssociation(data);
                break;
            case PLDM_PDR_FRU_RECORD_SET:
                printPDRFruRecordSet(data);
                break;
            default:
                break;
        }
    }

  private:
    uint32_t recordHandle;
};

class SetStateEffecter : public CommandInterface
{
  public:
    ~SetStateEffecter() = default;
    SetStateEffecter() = delete;
    SetStateEffecter(const SetStateEffecter&) = delete;
    SetStateEffecter(SetStateEffecter&&) = default;
    SetStateEffecter& operator=(const SetStateEffecter&) = delete;
    SetStateEffecter& operator=(SetStateEffecter&&) = default;

    // compositeEffecterCount(value: 0x01 to 0x08) * stateField(2)
    static constexpr auto maxEffecterDataSize = 16;

    // compositeEffecterCount(value: 0x01 to 0x08)
    static constexpr auto minEffecterCount = 1;
    static constexpr auto maxEffecterCount = 8;
    explicit SetStateEffecter(const char* type, const char* name,
                              CLI::App* app) :
        CommandInterface(type, name, app)
    {
        app->add_option(
               "-i, --id", effecterId,
               "A handle that is used to identify and access the effecter")
            ->required();
        app->add_option("-c, --count", effecterCount,
                        "The number of individual sets of effecter information")
            ->required();
        app->add_option(
               "-d,--data", effecterData,
               "Set effecter state data\n"
               "eg: requestSet0 effecterState0 noChange1 dummyState1 ...")
            ->required();
    }

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(
            sizeof(pldm_msg_hdr) + PLDM_SET_STATE_EFFECTER_STATES_REQ_BYTES);
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

        if (effecterCount > maxEffecterCount ||
            effecterCount < minEffecterCount)
        {
            std::cerr << "Request Message Error: effecterCount size "
                      << effecterCount << "is invalid\n";
            auto rc = PLDM_ERROR_INVALID_DATA;
            return {rc, requestMsg};
        }

        if (effecterData.size() > maxEffecterDataSize)
        {
            std::cerr << "Request Message Error: effecterData size "
                      << effecterData.size() << "is invalid\n";
            auto rc = PLDM_ERROR_INVALID_DATA;
            return {rc, requestMsg};
        }

        auto stateField = parseEffecterData(effecterData, effecterCount);
        if (!stateField)
        {
            std::cerr << "Failed to parse effecter data, effecterCount size "
                      << effecterCount << "\n";
            auto rc = PLDM_ERROR_INVALID_DATA;
            return {rc, requestMsg};
        }

        auto rc = encode_set_state_effecter_states_req(
            instanceId, effecterId, effecterCount, stateField->data(), request);
        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t completionCode = 0;
        auto rc = decode_set_state_effecter_states_resp(
            responsePtr, payloadLength, &completionCode);

        if (rc != PLDM_SUCCESS || completionCode != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)completionCode << "\n";
            return;
        }

        std::cout << "SetStateEffecterStates: SUCCESS" << std::endl;
    }

  private:
    uint16_t effecterId;
    uint8_t effecterCount;
    std::vector<uint8_t> effecterData;
};

class SetNumericEffecterValue : public CommandInterface
{
  public:
    ~SetNumericEffecterValue() = default;
    SetNumericEffecterValue() = delete;
    SetNumericEffecterValue(const SetNumericEffecterValue&) = delete;
    SetNumericEffecterValue(SetNumericEffecterValue&&) = default;
    SetNumericEffecterValue& operator=(const SetNumericEffecterValue&) = delete;
    SetNumericEffecterValue& operator=(SetNumericEffecterValue&&) = default;

    explicit SetNumericEffecterValue(const char* type, const char* name,
                                     CLI::App* app) :
        CommandInterface(type, name, app)
    {
        app->add_option(
               "-i, --id", effecterId,
               "A handle that is used to identify and access the effecter")
            ->required();
        app->add_option("-s, --size", effecterDataSize,
                        "The bit width and format of the setting value for the "
                        "effecter. enum value: {uint8, sint8, uint16, sint16, "
                        "uint32, sint32}\n")
            ->required();
        app->add_option("-d,--data", maxEffecterValue,
                        "The setting value of numeric effecter being "
                        "requested\n")
            ->required();
    }

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(
            sizeof(pldm_msg_hdr) +
            PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 3);

        uint8_t* effecterValue = (uint8_t*)&maxEffecterValue;

        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
        size_t payload_length = PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES;

        if (effecterDataSize == PLDM_EFFECTER_DATA_SIZE_UINT16 ||
            effecterDataSize == PLDM_EFFECTER_DATA_SIZE_SINT16)
        {
            payload_length = PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 1;
        }
        if (effecterDataSize == PLDM_EFFECTER_DATA_SIZE_UINT32 ||
            effecterDataSize == PLDM_EFFECTER_DATA_SIZE_SINT32)
        {
            payload_length = PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 3;
        }
        auto rc = encode_set_numeric_effecter_value_req(
            0, effecterId, effecterDataSize, effecterValue, request,
            payload_length);

        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t completionCode = 0;
        auto rc = decode_set_numeric_effecter_value_resp(
            responsePtr, payloadLength, &completionCode);

        if (rc != PLDM_SUCCESS || completionCode != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)completionCode
                      << std::endl;
            return;
        }

        std::cout << "SetNumericEffecterValue: SUCCESS" << std::endl;
    }

  private:
    uint16_t effecterId;
    uint8_t effecterDataSize;
    uint64_t maxEffecterValue;
};

void registerCommand(CLI::App& app)
{
    auto platform = app.add_subcommand("platform", "platform type command");
    platform->require_subcommand(1);

    auto getPDR =
        platform->add_subcommand("GetPDR", "get platform descriptor records");
    commands.push_back(std::make_unique<GetPDR>("platform", "getPDR", getPDR));

    auto setStateEffecterStates = platform->add_subcommand(
        "SetStateEffecterStates", "set effecter states");
    commands.push_back(std::make_unique<SetStateEffecter>(
        "platform", "setStateEffecterStates", setStateEffecterStates));

    auto setNumericEffecterValue = platform->add_subcommand(
        "SetNumericEffecterValue", "set the value for a PLDM Numeric Effecter");
    commands.push_back(std::make_unique<SetNumericEffecterValue>(
        "platform", "setNumericEffecterValue", setNumericEffecterValue));
}

} // namespace platform
} // namespace pldmtool
