#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "pdr.h"

/* Maximum size for request */
#define PLDM_SET_STATE_EFFECTER_STATES_REQ_BYTES 19
#define PLDM_GET_STATE_SENSOR_READINGS_REQ_BYTES 4
#define PLDM_GET_NUMERIC_EFFECTER_VALUE_REQ_BYTES 2
#define PLDM_GET_SENSOR_READING_REQ_BYTES 4
/* Response lengths are inclusive of completion code */
#define PLDM_SET_STATE_EFFECTER_STATES_RESP_BYTES 1

#define PLDM_SET_NUMERIC_EFFECTER_VALUE_RESP_BYTES 1
#define PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES 4

#define PLDM_GET_PDR_REQ_BYTES 13
/* Minimum response length */
#define PLDM_GET_PDR_MIN_RESP_BYTES 12
#define PLDM_GET_NUMERIC_EFFECTER_VALUE_MIN_RESP_BYTES 5
#define PLDM_GET_SENSOR_READING_MIN_RESP_BYTES 8
#define PLDM_GET_STATE_SENSOR_READINGS_MIN_RESP_BYTES 2

/* Minimum length for PLDM PlatformEventMessage request */
#define PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES 3
#define PLDM_PLATFORM_EVENT_MESSAGE_STATE_SENSOR_STATE_REQ_BYTES 6
#define PLDM_PLATFORM_EVENT_MESSAGE_RESP_BYTES 2

/* Minumum length of senson event data */
#define PLDM_SENSOR_EVENT_DATA_MIN_LENGTH 5
#define PLDM_SENSOR_EVENT_SENSOR_OP_STATE_DATA_LENGTH 2
#define PLDM_SENSOR_EVENT_STATE_SENSOR_STATE_DATA_LENGTH 3
#define PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_MIN_DATA_LENGTH 4
#define PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_MAX_DATA_LENGTH 7
#define PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_8BIT_DATA_LENGTH 4
#define PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_16BIT_DATA_LENGTH 5
#define PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_32BIT_DATA_LENGTH 7

/* Minimum length of data for pldmPDRRepositoryChgEvent */
#define PLDM_PDR_REPOSITORY_CHG_EVENT_MIN_LENGTH 2
#define PLDM_PDR_REPOSITORY_CHANGE_RECORD_MIN_LENGTH 2

#define PLDM_INVALID_EFFECTER_ID 0xFFFF

enum pldm_effecter_data_size {
	PLDM_EFFECTER_DATA_SIZE_UINT8,
	PLDM_EFFECTER_DATA_SIZE_SINT8,
	PLDM_EFFECTER_DATA_SIZE_UINT16,
	PLDM_EFFECTER_DATA_SIZE_SINT16,
	PLDM_EFFECTER_DATA_SIZE_UINT32,
	PLDM_EFFECTER_DATA_SIZE_SINT32
};

enum pldm_range_field_format {
	PLDM_RANGE_FIELD_FORMAT_UINT8,
	PLDM_RANGE_FIELD_FORMAT_SINT8,
	PLDM_RANGE_FIELD_FORMAT_UINT16,
	PLDM_RANGE_FIELD_FORMAT_SINT16,
	PLDM_RANGE_FIELD_FORMAT_UINT32,
	PLDM_RANGE_FIELD_FORMAT_SINT32,
	PLDM_RANGE_FIELD_FORMAT_REAL32
};

enum set_request { PLDM_NO_CHANGE = 0x00, PLDM_REQUEST_SET = 0x01 };

enum effecter_state { PLDM_INVALID_VALUE = 0xFF };

enum pldm_sensor_present_state {
	PLDM_SENSOR_UNKNOWN = 0x0,
	PLDM_SENSOR_NORMAL = 0x01,
	PLDM_SENSOR_WARNING = 0x02,
	PLDM_SENSOR_CRITICAL = 0x03,
	PLDM_SENSOR_FATAL = 0x04,
	PLDM_SENSOR_LOWERWARNING = 0x05,
	PLDM_SENSOR_LOWERCRITICAL = 0x06,
	PLDM_SENSOR_LOWERFATAL = 0x07,
	PLDM_SENSOR_UPPERWARNING = 0x08,
	PLDM_SENSOR_UPPERCRITICAL = 0x09,
	PLDM_SENSOR_UPPERFATAL = 0x0a
};

enum pldm_sensor_event_message_enable {
	PLDM_NO_EVENT_GENERATION,
	PLDM_EVENTS_DISABLED,
	PLDM_EVENTS_ENABLED,
	PLDM_OP_EVENTS_ONLY_ENABLED,
	PLDM_STATE_EVENTS_ONLY_ENABLED
};

enum pldm_effecter_oper_state {
	EFFECTER_OPER_STATE_ENABLED_UPDATEPENDING,
	EFFECTER_OPER_STATE_ENABLED_NOUPDATEPENDING,
	EFFECTER_OPER_STATE_DISABLED,
	EFFECTER_OPER_STATE_UNAVAILABLE,
	EFFECTER_OPER_STATE_STATUSUNKNOWN,
	EFFECTER_OPER_STATE_FAILED,
	EFFECTER_OPER_STATE_INITIALIZING,
	EFFECTER_OPER_STATE_SHUTTINGDOWN,
	EFFECTER_OPER_STATE_INTEST
};

enum pldm_platform_commands {
	PLDM_GET_SENSOR_READING = 0x11,
	PLDM_GET_STATE_SENSOR_READINGS = 0x21,
	PLDM_SET_NUMERIC_EFFECTER_VALUE = 0x31,
	PLDM_GET_NUMERIC_EFFECTER_VALUE = 0x32,
	PLDM_SET_STATE_EFFECTER_STATES = 0x39,
	PLDM_GET_PDR = 0x51,
	PLDM_PLATFORM_EVENT_MESSAGE = 0x0A
};

/** @brief PLDM PDR types
 */
enum pldm_pdr_types {
	PLDM_TERMINUS_LOCATOR_PDR = 1,
	PLDM_NUMERIC_SENSOR_PDR = 2,
	PLDM_NUMERIC_SENSOR_INITIALIZATION_PDR = 3,
	PLDM_STATE_SENSOR_PDR = 4,
	PLDM_STATE_SENSOR_INITIALIZATION_PDR = 5,
	PLDM_SENSOR_AUXILIARY_NAMES_PDR = 6,
	PLDM_OEM_UNIT_PDR = 7,
	PLDM_OEM_STATE_SET_PDR = 8,
	PLDM_NUMERIC_EFFECTER_PDR = 9,
	PLDM_NUMERIC_EFFECTER_INITIALIZATION_PDR = 10,
	PLDM_STATE_EFFECTER_PDR = 11,
	PLDM_STATE_EFFECTER_INITIALIZATION_PDR = 12,
	PLDM_EFFECTER_AUXILIARY_NAMES_PDR = 13,
	PLDM_EFFECTER_OEM_SEMANTIC_PDR = 14,
	PLDM_PDR_ENTITY_ASSOCIATION = 15,
	PLDM_ENTITY_AUXILIARY_NAMES_PDR = 16,
	PLDM_OEM_ENTITY_ID_PDR = 17,
	PLDM_INTERRUPT_ASSOCIATION_PDR = 18,
	PLDM_EVENT_LOG_PDR = 19,
	PLDM_PDR_FRU_RECORD_SET = 20,
	PLDM_OEM_DEVICE_PDR = 126,
	PLDM_OEM_PDR = 127,
};

/** @brief PLDM effecter initialization schemes
 */
enum pldm_effecter_init {
	PLDM_NO_INIT,
	PLDM_USE_INIT_PDR,
	PLDM_ENABLE_EFFECTER,
	PLDM_DISABLE_EFECTER
};

/** @brief PLDM Platform M&C completion codes
 */
enum pldm_platform_completion_codes {
	PLDM_PLATFORM_INVALID_EFFECTER_ID = 0x80,
	PLDM_PLATFORM_INVALID_STATE_VALUE = 0x81,

	PLDM_PLATFORM_INVALID_DATA_TRANSFER_HANDLE = 0x80,
	PLDM_PLATFORM_INVALID_TRANSFER_OPERATION_FLAG = 0x81,
	PLDM_PLATFORM_INVALID_RECORD_HANDLE = 0x82,
	PLDM_PLATFORM_INVALID_RECORD_CHANGE_NUMBER = 0x83,
	PLDM_PLATFORM_TRANSFER_TIMEOUT = 0x84,

	PLDM_PLATFORM_SET_EFFECTER_UNSUPPORTED_SENSORSTATE = 0x82,
};

/** @brief PLDM Event types
 */
enum pldm_event_types {
	PLDM_SENSOR_EVENT = 0x00,
	PLDM_EFFECTER_EVENT = 0x01,
	PLDM_REDFISH_TASK_EXECUTED_EVENT = 0x02,
	PLDM_REDFISH_MESSAGE_EVENT = 0x03,
	PLDM_PDR_REPOSITORY_CHG_EVENT = 0x04,
	PLDM_MESSAGE_POLL_EVENT = 0x05,
	PLDM_HEARTBEAT_TIMER_ELAPSED_EVENT = 0x06
};

/** @brief PLDM sensorEventClass states
 */
enum sensor_event_class_states {
	PLDM_SENSOR_OP_STATE,
	PLDM_STATE_SENSOR_STATE,
	PLDM_NUMERIC_SENSOR_STATE
};

/** @brief PLDM sensor supported states
 */
enum pldm_sensor_operational_state {
	PLDM_SENSOR_ENABLED,
	PLDM_SENSOR_DISABLED,
	PLDM_SENSOR_UNAVAILABLE,
	PLDM_SENSOR_STATUSUNKOWN,
	PLDM_SENSOR_FAILED,
	PLDM_SENSOR_INITIALIZING,
	PLDM_SENSOR_SHUTTINGDOWN,
	PLDM_SENSOR_INTEST
};

/** @brief PLDM pldmPDRRepositoryChgEvent class eventData format
 */
enum pldm_pdr_repository_chg_event_data_format {
	REFRESH_ENTIRE_REPOSITORY,
	FORMAT_IS_PDR_TYPES,
	FORMAT_IS_PDR_HANDLES
};

/** @brief PLDM pldmPDRRepositoryChgEvent class changeRecord format
 * eventDataOperation
 */
enum pldm_pdr_repository_chg_event_change_record_event_data_operation {
	PLDM_REFRESH_ALL_RECORDS,
	PLDM_RECORDS_DELETED,
	PLDM_RECORDS_ADDED,
	PLDM_RECORDS_MODIFIED
};

/** @brief PLDM NumericSensorStatePresentReading data type
 */
enum pldm_sensor_readings_data_type {
	PLDM_SENSOR_DATA_SIZE_UINT8,
	PLDM_SENSOR_DATA_SIZE_SINT8,
	PLDM_SENSOR_DATA_SIZE_UINT16,
	PLDM_SENSOR_DATA_SIZE_SINT16,
	PLDM_SENSOR_DATA_SIZE_UINT32,
	PLDM_SENSOR_DATA_SIZE_SINT32
};

/** @brief PLDM PlatformEventMessage response status
 */
enum pldm_platform_event_status {
	PLDM_EVENT_NO_LOGGING = 0x00,
	PLDM_EVENT_LOGGING_DISABLED = 0x01,
	PLDM_EVENT_LOG_FULL = 0x02,
	PLDM_EVENT_ACCEPTED_FOR_LOGGING = 0x03,
	PLDM_EVENT_LOGGED = 0x04,
	PLDM_EVENT_LOGGING_REJECTED = 0x05
};

/** @struct pldm_pdr_hdr
 *
 *  Structure representing PLDM common PDR header
 */
struct pldm_pdr_hdr {
	uint32_t record_handle;
	uint8_t version;
	uint8_t type;
	uint16_t record_change_num;
	uint16_t length;
} __attribute__((packed));

/** @struct pldm_pdr_entity_association
 *
 *  Structure representing PLDM Entity Association PDR
 */
struct pldm_pdr_entity_association {
	uint16_t container_id;
	uint8_t association_type;
	pldm_entity container;
	uint8_t num_children;
	pldm_entity children[1];
} __attribute__((packed));

/** @struct pldm_pdr_fru_record_set
 *
 *  Structure representing PLDM FRU record set PDR
 */
struct pldm_pdr_fru_record_set {
	uint16_t terminus_handle;
	uint16_t fru_rsi;
	uint16_t entity_type;
	uint16_t entity_instance_num;
	uint16_t container_id;
} __attribute__((packed));

/** @struct pldm_state_sensor_pdr
 *
 *  Structure representing PLDM state sensor PDR
 */
struct pldm_state_sensor_pdr {
	struct pldm_pdr_hdr hdr;
	uint16_t terminus_handle;
	uint16_t sensor_id;
	uint16_t entity_type;
	uint16_t entity_instance;
	uint16_t container_id;
	uint8_t sensor_init;
	bool8_t sensor_auxiliary_names_pdr;
	uint8_t composite_sensor_count;
	uint8_t possible_states[1];
} __attribute__((packed));

/** @struct state_sensor_possible_states
 *
 *  Structure representing state enums for state sensor
 */
struct state_sensor_possible_states {
	uint16_t state_set_id;
	uint8_t possible_states_size;
	bitfield8_t states[1];
} __attribute__((packed));

/** @struct pldm_state_effecter_pdr
 *
 *  Structure representing PLDM state effecter PDR
 */
struct pldm_state_effecter_pdr {
	struct pldm_pdr_hdr hdr;
	uint16_t terminus_handle;
	uint16_t effecter_id;
	uint16_t entity_type;
	uint16_t entity_instance;
	uint16_t container_id;
	uint16_t effecter_semantic_id;
	uint8_t effecter_init;
	bool8_t has_description_pdr;
	uint8_t composite_effecter_count;
	uint8_t possible_states[1];
} __attribute__((packed));

/** @brief Encode PLDM state sensor PDR
 *
 * @param[in/out] sensor                 Structure to encode. All members of
 * sensor, except those mentioned in the @note below, should be initialized by
 * the caller.
 * @param[in]     allocation_size        Size of sensor allocation in bytes
 * @param[in]     possible_states        Possible sensor states
 * @param[in]     possible_states_size   Size of possible sensor states in bytes
 * @param[out]    actual_size            Size of sensor PDR. Set to 0 on error.
 * @return int    pldm_completion_codes
 *                PLDM_SUCCESS/PLDM_ERROR/PLDM_ERROR_INVALID_LENGTH
 *
 * @note The sensor parameter will be encoded in place.
 * @note Caller is responsible for allocation of the sensor parameter. Caller
 *       must allocate enough space for the base structure and the
 *       sensor->possible_states array, otherwise the function will fail.
 * @note sensor->hdr.length, .type, and .version will be set appropriately.
 */
int encode_state_sensor_pdr(
    struct pldm_state_sensor_pdr *sensor, size_t allocation_size,
    const struct state_sensor_possible_states *possible_states,
    size_t possible_states_size, size_t *actual_size);

/** @union union_effecter_data_size
 *
 *  The bit width and format of reading and threshold values that the effecter
 *  returns.
 *  Refer to: DSP0248_1.2.0: 28.11 Table 87
 */
typedef union {
	uint8_t value_u8;
	int8_t value_s8;
	uint16_t value_u16;
	int16_t value_s16;
	uint32_t value_u32;
	int32_t value_s32;
} union_effecter_data_size;

/** @union union_range_field_format
 *
 *  Indicates the format used for the nominalValue, normalMax, and normalMin
 *  fields.
 *  Refer to: DSP0248_1.2.0: 28.11 Table 87
 */
typedef union {
	uint8_t value_u8;
	int8_t value_s8;
	uint16_t value_u16;
	int16_t value_s16;
	uint32_t value_u32;
	int32_t value_s32;
	real32_t value_f32;
} union_range_field_format;

/** @struct pldm_numeric_effecter_value_pdr
 *
 *  Structure representing PLDM numeric effecter value PDR
 */
struct pldm_numeric_effecter_value_pdr {
	struct pldm_pdr_hdr hdr;
	uint16_t terminus_handle;
	uint16_t effecter_id;
	uint16_t entity_type;
	uint16_t entity_instance;
	uint16_t container_id;
	uint16_t effecter_semantic_id;
	uint8_t effecter_init;
	bool8_t effecter_auxiliary_names;
	uint8_t base_unit;
	int8_t unit_modifier;
	uint8_t rate_unit;
	uint8_t base_oem_unit_handle;
	uint8_t aux_unit;
	int8_t aux_unit_modifier;
	uint8_t aux_rate_unit;
	uint8_t aux_oem_unit_handle;
	bool8_t is_linear;
	uint8_t effecter_data_size;
	real32_t resolution;
	real32_t offset;
	uint16_t accuracy;
	uint8_t plus_tolerance;
	uint8_t minus_tolerance;
	real32_t state_transition_interval;
	real32_t transition_interval;
	union_effecter_data_size max_set_table;
	union_effecter_data_size min_set_table;
	uint8_t range_field_format;
	bitfield8_t range_field_support;
	union_range_field_format nominal_value;
	union_range_field_format normal_max;
	union_range_field_format normal_min;
	union_range_field_format rated_max;
	union_range_field_format rated_min;
} __attribute__((packed));

/** @struct state_effecter_possible_states
 *
 *  Structure representing state enums for state effecter
 */
struct state_effecter_possible_states {
	uint16_t state_set_id;
	uint8_t possible_states_size;
	bitfield8_t states[1];
} __attribute__((packed));

/** @brief Encode PLDM state effecter PDR
 *
 * @param[in/out] effecter               Structure to encode. All members of
 *                                       effecter, except those mentioned in
 *                                       the @note below, should be initialized
 *                                       by the caller.
 * @param[in]     allocation_size        Size of effecter allocation in bytes
 * @param[in]     possible_states        Possible effecter states
 * @param[in]     possible_states_size   Size of possible effecter states in
 *                                       bytes
 * @param[out]    actual_size            Size of effecter PDR. Set to 0 on
 *                                       error.
 * @return int    pldm_completion_codes
 *                PLDM_SUCCESS/PLDM_ERROR/PLDM_ERROR_INVALID_LENGTH
 *
 * @note The effecter parameter will be encoded in place.
 * @note Caller is responsible for allocation of the effecter parameter. Caller
 *       must allocate enough space for the base structure and the
 *       effecter->possible_states array, otherwise the function will fail.
 * @note effecter->hdr.length, .type, and .version will be set appropriately.
 */
int encode_state_effecter_pdr(
    struct pldm_state_effecter_pdr *effecter, size_t allocation_size,
    const struct state_effecter_possible_states *possible_states,
    size_t possible_states_size, size_t *actual_size);

/** @struct set_effecter_state_field
 *
 *  Structure representing a stateField in SetStateEffecterStates command */

typedef struct state_field_for_state_effecter_set {
	uint8_t set_request;	//!< Whether to change the state
	uint8_t effecter_state; //!< Expected state of the effecter
} __attribute__((packed)) set_effecter_state_field;

/** @struct get_sensor_readings_field
 *
 *  Structure representing a stateField in GetStateSensorReadings command */

typedef struct state_field_for_get_state_sensor_readings {
	uint8_t sensor_op_state; //!< The state of the sensor itself
	uint8_t present_state;	 //!< Return a state value
	uint8_t previous_state; //!< The state that the presentState was entered
				//! from. This must be different from the
				//! present state
	uint8_t event_state;	//!< Return a state value from a PLDM State Set
			     //! that is associated with the sensor
} __attribute__((packed)) get_sensor_state_field;

/** @struct PLDM_SetStateEffecterStates_Request
 *
 *  Structure representing PLDM set state effecter states request.
 */
struct pldm_set_state_effecter_states_req {
	uint16_t effecter_id;
	uint8_t comp_effecter_count;
	set_effecter_state_field field[8];
} __attribute__((packed));

/** @struct pldm_get_pdr_resp
 *
 *  structure representing GetPDR response packet
 *  transfer CRC is not part of the structure and will be
 *  added at the end of last packet in multipart transfer
 */
struct pldm_get_pdr_resp {
	uint8_t completion_code;
	uint32_t next_record_handle;
	uint32_t next_data_transfer_handle;
	uint8_t transfer_flag;
	uint16_t response_count;
	uint8_t record_data[1];
} __attribute__((packed));

/** @struct pldm_get_pdr_req
 *
 *  structure representing GetPDR request packet
 */
struct pldm_get_pdr_req {
	uint32_t record_handle;
	uint32_t data_transfer_handle;
	uint8_t transfer_op_flag;
	uint16_t request_count;
	uint16_t record_change_number;
} __attribute__((packed));

/** @struct pldm_set_numeric_effecter_value_req
 *
 *  structure representing SetNumericEffecterValue request packet
 */
struct pldm_set_numeric_effecter_value_req {
	uint16_t effecter_id;
	uint8_t effecter_data_size;
	uint8_t effecter_value[1];
} __attribute__((packed));

/** @struct pldm_get_state_sensor_readings_req
 *
 *  Structure representing PLDM get state sensor readings request.
 */
struct pldm_get_state_sensor_readings_req {
	uint16_t sensor_id;
	bitfield8_t sensor_rearm;
	uint8_t reserved;
} __attribute__((packed));

/** @struct pldm_get_state_sensor_readings_resp
 *
 *  Structure representing PLDM get state sensor readings response.
 */
struct pldm_get_state_sensor_readings_resp {
	uint8_t completion_code;
	uint8_t comp_sensor_count;
	get_sensor_state_field field[1];
} __attribute__((packed));

/** @struct pldm_sensor_event
 *
 *  structure representing sensorEventClass
 */
struct pldm_sensor_event_data {
	uint16_t sensor_id;
	uint8_t sensor_event_class_type;
	uint8_t event_class[1];
} __attribute__((packed));

/** @struct pldm_state_sensor_state
 *
 *  structure representing sensorEventClass for stateSensorState
 */
struct pldm_sensor_event_state_sensor_state {
	uint8_t sensor_offset;
	uint8_t event_state;
	uint8_t previous_event_state;
} __attribute__((packed));

/** @struct pldm_sensor_event_numeric_sensor_state
 *
 *  structure representing sensorEventClass for stateSensorState
 */
struct pldm_sensor_event_numeric_sensor_state {
	uint8_t event_state;
	uint8_t previous_event_state;
	uint8_t sensor_data_size;
	uint8_t present_reading[1];
} __attribute__((packed));

/** @struct pldm_sensor_event_sensor_op_state
 *
 *  structure representing sensorEventClass for SensorOpState
 */
struct pldm_sensor_event_sensor_op_state {
	uint8_t present_op_state;
	uint8_t previous_op_state;
} __attribute__((packed));

/** @struct pldm_platform_event_message_req
 *
 *  structure representing PlatformEventMessage command request data
 */
struct pldm_platform_event_message_req {
	uint8_t format_version;
	uint8_t tid;
	uint8_t event_class;
	uint8_t event_data[1];
} __attribute__((packed));

/** @struct pldm_platform_event_message_response
 *
 *  structure representing PlatformEventMessage command response data
 */
struct pldm_platform_event_message_resp {
	uint8_t completion_code;
	uint8_t platform_event_status;
} __attribute__((packed));

/** @struct pldm_pdr_repository_chg_event_data
 *
 *  structure representing pldmPDRRepositoryChgEvent class eventData
 */
struct pldm_pdr_repository_chg_event_data {
	uint8_t event_data_format;
	uint8_t number_of_change_records;
	uint8_t change_records[1];
} __attribute__((packed));

/** @struct pldm_pdr_repository_chg_event_change_record_data
 *
 *  structure representing pldmPDRRepositoryChgEvent class eventData's change
 * record data
 */
struct pldm_pdr_repository_change_record_data {
	uint8_t event_data_operation;
	uint8_t number_of_change_entries;
	uint32_t change_entry[1];
} __attribute__((packed));

/** @struct pldm_get_numeric_effecter_value_req
 *
 *  structure representing GetNumericEffecterValue request packet
 */
struct pldm_get_numeric_effecter_value_req {
	uint16_t effecter_id;
} __attribute__((packed));

/** @struct pldm_get_numeric_effecter_value_resp
 *
 *  structure representing GetNumericEffecterValue response packet
 */
struct pldm_get_numeric_effecter_value_resp {
	uint8_t completion_code;
	uint8_t effecter_data_size;
	uint8_t effecter_oper_state;
	uint8_t pending_and_present_values[1];
} __attribute__((packed));

/** @struct pldm_get_sensor_reading_req
 *
 *  Structure representing PLDM get sensor reading request
 */
struct pldm_get_sensor_reading_req {
	uint16_t sensor_id;
	bool8_t rearm_event_state;
} __attribute__((packed));

/** @struct pldm_get_sensor_reading_resp
 *
 *  Structure representing PLDM get sensor reading response
 */
struct pldm_get_sensor_reading_resp {
	uint8_t completion_code;
	uint8_t sensor_data_size;
	uint8_t sensor_operational_state;
	uint8_t sensor_event_message_enable;
	uint8_t present_state;
	uint8_t previous_state;
	uint8_t event_state;
	uint8_t present_reading[1];
} __attribute__((packed));

/* Responder */

/* SetNumericEffecterValue */

/** @brief Decode SetNumericEffecterValue request data
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] effecter_id - used to identify and access the effecter
 *  @param[out] effecter_data_size - The bit width and format of the setting
 * 				value for the effecter.
 * 				value:{uint8,sint8,uint16,sint16,uint32,sint32}
 *  @param[out] effecter_value - The setting value of numeric effecter being
 * 				requested.
 *  @return pldm_completion_codes
 */
int decode_set_numeric_effecter_value_req(const struct pldm_msg *msg,
					  size_t payload_length,
					  uint16_t *effecter_id,
					  uint8_t *effecter_data_size,
					  uint8_t *effecter_value);

/** @brief Create a PLDM response message for SetNumericEffecterValue
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - PLDM completion code
 *  @param[out] msg - Message will be written to this
 *  @param[in] payload_length - Length of request message payload
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.body.payload'
 */
int encode_set_numeric_effecter_value_resp(uint8_t instance_id,
					   uint8_t completion_code,
					   struct pldm_msg *msg,
					   size_t payload_length);

/* SetStateEffecterStates */

/** @brief Create a PLDM response message for SetStateEffecterStates
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - PLDM completion code
 *  @param[out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.body.payload'
 */

int encode_set_state_effecter_states_resp(uint8_t instance_id,
					  uint8_t completion_code,
					  struct pldm_msg *msg);

/** @brief Decode SetStateEffecterStates request data
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] effecter_id - used to identify and access the effecter
 *  @param[out] comp_effecter_count - number of individual sets of effecter
 *         information. Upto eight sets of state effecter info can be accessed
 *         for a given effecter.
 *  @param[out] field - each unit is an instance of the stateFileld structure
 *         that is used to set the requested state for a particular effecter
 *         within the state effecter. This field holds the starting address of
 *         the stateField values. The user is responsible to allocate the
 *         memory prior to calling this command. Since the state field count is
 *         not known in advance, the user should allocate the maximum size
 *         always, which is 8 in number.
 *  @return pldm_completion_codes
 */

int decode_set_state_effecter_states_req(const struct pldm_msg *msg,
					 size_t payload_length,
					 uint16_t *effecter_id,
					 uint8_t *comp_effecter_count,
					 set_effecter_state_field *field);

/* GetPDR */

/** @brief Create a PLDM response message for GetPDR
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - PLDM completion code
 *  @param[in] next_record_hndl - The recordHandle for the PDR that is next in
 *        the PDR Repository
 *  @param[in] next_data_transfer_hndl - A handle that identifies the next
 *        portion of the PDR data to be transferred, if any
 *  @param[in] transfer_flag - Indicates the portion of PDR data being
 *        transferred
 *  @param[in] resp_cnt - The number of recordData bytes returned in this
 *        response
 *  @param[in] record_data - PDR data bytes of length resp_cnt
 *  @param[in] transfer_crc - A CRC-8 for the overall PDR. This is present only
 *        in the last part of a PDR being transferred
 *  @param[out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_pdr_resp(uint8_t instance_id, uint8_t completion_code,
			uint32_t next_record_hndl,
			uint32_t next_data_transfer_hndl, uint8_t transfer_flag,
			uint16_t resp_cnt, const uint8_t *record_data,
			uint8_t transfer_crc, struct pldm_msg *msg);

/** @brief Decode GetPDR request data
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] record_hndl - The recordHandle value for the PDR to be retrieved
 *  @param[out] data_transfer_hndl - Handle used to identify a particular
 *         multipart PDR data transfer operation
 *  @param[out] transfer_op_flag - Flag to indicate the first or subsequent
 *         portion of transfer
 *  @param[out] request_cnt - The maximum number of record bytes requested
 *  @param[out] record_chg_num - Used to determine whether the PDR has changed
 *        while PDR transfer is going on
 *  @return pldm_completion_codes
 */

int decode_get_pdr_req(const struct pldm_msg *msg, size_t payload_length,
		       uint32_t *record_hndl, uint32_t *data_transfer_hndl,
		       uint8_t *transfer_op_flag, uint16_t *request_cnt,
		       uint16_t *record_chg_num);

/* GetStateSensorReadings */

/** @brief Decode GetStateSensorReadings request data
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] sensor_id - used to identify and access the simple or composite
 *         sensor
 *  @param[out] sensor_rearm - Each bit location in this field corresponds to a
 *         particular sensor within the state sensor, where bit [0] corresponds
 *         to the first state sensor (sensor offset 0) and bit [7] corresponds
 *         to the eighth sensor (sensor offset 7), sequentially.
 *  @param[out] reserved - value: 0x00
 *  @return pldm_completion_codes
 */

int decode_get_state_sensor_readings_req(const struct pldm_msg *msg,
					 size_t payload_length,
					 uint16_t *sensor_id,
					 bitfield8_t *sensor_rearm,
					 uint8_t *reserved);

/** @brief Encode GetStateSensorReadings response data
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - PLDM completion code
 *  @param[out] comp_sensor_count - The number of individual sets of sensor
 *         information that this command accesses
 *  @param[out] field - Each stateField is an instance of a stateField structure
 *         that is used to return the present operational state setting and the
 *         present state and event state for a particular set of sensor
 *         information contained within the state sensor
 *  @param[out] msg - Message will be written to this
 *  @return pldm_completion_codes
 */

int encode_get_state_sensor_readings_resp(uint8_t instance_id,
					  uint8_t completion_code,
					  uint8_t comp_sensor_count,
					  get_sensor_state_field *field,
					  struct pldm_msg *msg);

/* GetNumericEffecterValue */

/** @brief Decode GetNumericEffecterValue request data
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] effecter_id - used to identify and access the effecter
 *  @return pldm_completion_codes
 */
int decode_get_numeric_effecter_value_req(const struct pldm_msg *msg,
					  size_t payload_length,
					  uint16_t *effecter_id);

/** @brief Create a PLDM response message for GetNumericEffecterValue
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - PLDM completion code
 *  @param[in] effecter_data_size - The bit width and format of the setting
 *             value for the effecter.
 * 	       value:{uint8,sint8,uint16,sint16,uint32,sint32}
 *  @param[in] effecter_oper_state - The state of the effecter itself
 *  @param[in] pending_value - The pending numeric value setting of the
 *             effecter. The effecterDataSize field indicates the number of
 *             bits used for this field
 *  @param[in] present_value - The present numeric value setting of the
 *             effecter. The effecterDataSize indicates the number of bits
 *             used for this field
 *  @param[out] msg - Message will be written to this
 *  @param[in] payload_length - Length of request message payload
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_numeric_effecter_value_resp(
    uint8_t instance_id, uint8_t completion_code, uint8_t effecter_data_size,
    uint8_t effecter_oper_state, uint8_t *pending_value, uint8_t *present_value,
    struct pldm_msg *msg, size_t payload_length);

/* GetSensorReading */

/** @brief Decode GetSensorReading request data
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] sensor_id - A handle that is used to identify and access
 *         the sensor
 *  @param[out] rearm_event_state - true =  manually re-arm EventState after
 *         responding to this request, false = no manual re-arm
 *  @return pldm_completion_codes
 */

int decode_get_sensor_reading_req(const struct pldm_msg *msg,
				  size_t payload_length, uint16_t *sensor_id,
				  bool8_t *rearm_event_state);

/** @brief Encode GetSensorReading response data
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - PLDM completion code
 *  @param[out] sensor_data_size - The bit width and format of reading and
 *         threshold values
 *  @param[out] sensor_operational_state - The state of the sensor itself
 *  @param[out] sensor_event_message_enable - value: { noEventGeneration,
 *         eventsDisabled, eventsEnabled, opEventsOnlyEnabled,
 *         stateEventsOnlyEnabled }
 *  @param[out] present_state - The most recently assessed state value monitored
 *         by the sensor
 *  @param[out] previous_state - The state that the presentState was entered
 *         from
 *  @param[out] event_state - Indicates which threshold crossing assertion
 *         events have been detected
 *  @param[out] present_reading - The present value indicated by the sensor
 *  @param[out] msg - Message will be written to this
 *  @param[in] payload_length - Length of request message payload
 *  @return pldm_completion_codes
 */

int encode_get_sensor_reading_resp(
    uint8_t instance_id, uint8_t completion_code, uint8_t sensor_data_size,
    uint8_t sensor_operational_state, uint8_t sensor_event_message_enable,
    uint8_t present_state, uint8_t previous_state, uint8_t event_state,
    uint8_t *present_reading, struct pldm_msg *msg, size_t payload_length);

/* Requester */

/* GetPDR */

/** @brief Create a PLDM request message for GetPDR
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] record_hndl - The recordHandle value for the PDR to be retrieved
 *  @param[in] data_transfer_hndl - Handle used to identify a particular
 *         multipart PDR data transfer operation
 *  @param[in] transfer_op_flag - Flag to indicate the first or subsequent
 *         portion of transfer
 *  @param[in] request_cnt - The maximum number of record bytes requested
 *  @param[in] record_chg_num - Used to determine whether the PDR has changed
 *        while PDR transfer is going on
 *  @param[out] msg - Message will be written to this
 *  @param[in] payload_length - Length of request message payload
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_pdr_req(uint8_t instance_id, uint32_t record_hndl,
		       uint32_t data_transfer_hndl, uint8_t transfer_op_flag,
		       uint16_t request_cnt, uint16_t record_chg_num,
		       struct pldm_msg *msg, size_t payload_length);

/** @brief Decode GetPDR response data
 *
 *  Note:
 *  * If the return value is not PLDM_SUCCESS, it represents a
 * transport layer error.
 *  * If the completion_code value is not PLDM_SUCCESS, it represents a
 * protocol layer error and all the out-parameters are invalid.
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] completion_code - PLDM completion code
 *  @param[out] next_record_hndl - The recordHandle for the PDR that is next in
 *        the PDR Repository
 *  @param[out] next_data_transfer_hndl - A handle that identifies the next
 *        portion of the PDR data to be transferred, if any
 *  @param[out] transfer_flag - Indicates the portion of PDR data being
 *        transferred
 *  @param[out] resp_cnt - The number of recordData bytes returned in this
 *        response
 *  @param[out] record_data - PDR data bytes of length resp_cnt, or NULL to
 *        skip the copy and place the actual length in resp_cnt.
 *  @param[in] record_data_length - Length of record_data
 *  @param[out] transfer_crc - A CRC-8 for the overall PDR. This is present only
 *        in the last part of a PDR being transferred
 *  @return pldm_completion_codes
 */
int decode_get_pdr_resp(const struct pldm_msg *msg, size_t payload_length,
			uint8_t *completion_code, uint32_t *next_record_hndl,
			uint32_t *next_data_transfer_hndl,
			uint8_t *transfer_flag, uint16_t *resp_cnt,
			uint8_t *record_data, size_t record_data_length,
			uint8_t *transfer_crc);

/* SetStateEffecterStates */

/** @brief Create a PLDM request message for SetStateEffecterStates
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] effecter_id - used to identify and access the effecter
 *  @param[in] comp_effecter_count - number of individual sets of effecter
 *         information. Upto eight sets of state effecter info can be accessed
 *         for a given effecter.
 *  @param[in] field - each unit is an instance of the stateField structure
 *         that is used to set the requested state for a particular effecter
 *         within the state effecter. This field holds the starting address of
 *         the stateField values. The user is responsible to allocate the
 *         memory prior to calling this command. The user has to allocate the
 *         field parameter as sizeof(set_effecter_state_field) *
 *         comp_effecter_count
 *  @param[out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */

int encode_set_state_effecter_states_req(uint8_t instance_id,
					 uint16_t effecter_id,
					 uint8_t comp_effecter_count,
					 set_effecter_state_field *field,
					 struct pldm_msg *msg);

/** @brief Decode SetStateEffecterStates response data
 *
 *  Note:
 *  * If the return value is not PLDM_SUCCESS, it represents a
 * transport layer error.
 *  * If the completion_code value is not PLDM_SUCCESS, it represents a
 * protocol layer error and all the out-parameters are invalid.
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] completion_code - PLDM completion code
 *  @return pldm_completion_codes
 */
int decode_set_state_effecter_states_resp(const struct pldm_msg *msg,
					  size_t payload_length,
					  uint8_t *completion_code);

/* SetNumericEffecterValue */

/** @brief Create a PLDM request message for SetNumericEffecterValue
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] effecter_id - used to identify and access the effecter
 *  @param[in] effecter_data_size - The bit width and format of the setting
 * 				value for the effecter.
 * 				value:{uint8,sint8,uint16,sint16,uint32,sint32}
 *  @param[in] effecter_value - The setting value of numeric effecter being
 * 				requested.
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_set_numeric_effecter_value_req(
    uint8_t instance_id, uint16_t effecter_id, uint8_t effecter_data_size,
    uint8_t *effecter_value, struct pldm_msg *msg, size_t payload_length);

/** @brief Decode SetNumericEffecterValue response data
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] completion_code - PLDM completion code
 *  @return pldm_completion_codes
 */
int decode_set_numeric_effecter_value_resp(const struct pldm_msg *msg,
					   size_t payload_length,
					   uint8_t *completion_code);

/** @brief Create a PLDM request message for GetStateSensorReadings
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] sensor_id - used to identify and access the simple or composite
 *         sensor
 *  @param[in] sensorRearm - Each bit location in this field corresponds to a
 *         particular sensor within the state sensor, where bit [0] corresponds
 *         to the first state sensor (sensor offset 0) and bit [7] corresponds
 *         to the eighth sensor (sensor offset 7), sequentially
 *  @param[in] reserved - value: 0x00
 *  @param[out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_state_sensor_readings_req(uint8_t instance_id,
					 uint16_t sensor_id,
					 bitfield8_t sensor_rearm,
					 uint8_t reserved,
					 struct pldm_msg *msg);

/** @brief Decode GetStateSensorReadings response data
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] completion_code - PLDM completion code
 *  @param[in,out] comp_sensor_count - The number of individual sets of sensor
 *         information that this command accesses
 *  @param[out] field - Each stateField is an instance of a stateField structure
 *         that is used to return the present operational state setting and the
 *         present state and event state for a particular set of sensor
 *         information contained within the state sensor
 *  @return pldm_completion_codes
 */

int decode_get_state_sensor_readings_resp(const struct pldm_msg *msg,
					  size_t payload_length,
					  uint8_t *completion_code,
					  uint8_t *comp_sensor_count,
					  get_sensor_state_field *field);

/* PlatformEventMessage */

/** @brief Decode PlatformEventMessage request data
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] format_version - Version of the event format
 *  @param[out] tid - Terminus ID for the terminus that originated the event
 * message
 *  @param[out] event_class - The class of event being sent
 *  @param[out] event_data_offset - Offset where the event data should be read
 * from pldm msg
 *  @return pldm_completion_codes
 */
int decode_platform_event_message_req(const struct pldm_msg *msg,
				      size_t payload_length,
				      uint8_t *format_version, uint8_t *tid,
				      uint8_t *event_class,
				      size_t *event_data_offset);

/** @brief Encode PlatformEventMessage response data
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - PLDM completion code
 *  @param[in] platform_event_status - Response status of the event message
 * command
 *  @param[out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_platform_event_message_resp(uint8_t instance_id,
				       uint8_t completion_code,
				       uint8_t platform_event_status,
				       struct pldm_msg *msg);

/** @brief Encode PlatformEventMessage request data
 * @param[in] instance_id - Message's instance id
 * @param[in] format_version - Version of the event format
 * @param[in] tid - Terminus ID for the terminus that originated the event
 * message
 * @param[in] event_class - The class of event being sent
 * @param[in] event_data - the event data should be read from pldm msg
 * @param[in] event_data_length - Length of the event data
 * @param[out] msg - Request message
 * @return pldm_completion_codes
 * @note Caller is responsible for memory alloc and dealloc of param
 * 'msg.payload'
 */
int encode_platform_event_message_req(
    uint8_t instance_id, uint8_t format_version, uint8_t tid,
    uint8_t event_class, const uint8_t *event_data, size_t event_data_length,
    struct pldm_msg *msg, size_t payload_length);

/** @brief Decode PlatformEventMessage response data
 * @param[in] msg - Request message
 * @param[in] payload_length - Length of Response message payload
 * @param[out] completion_code - PLDM completion code
 * @param[out] platform_event_status - Response status of the event message
 * command
 * @return pldm_completion_codes
 */
int decode_platform_event_message_resp(const struct pldm_msg *msg,
				       size_t payload_length,
				       uint8_t *completion_code,
				       uint8_t *platform_event_status);

/** @brief Decode sensorEventData response data
 *
 *  @param[in] event_data - event data from the response message
 *  @param[in] event_data_length - length of the event data
 *  @param[out] sensor_id -  sensorID value of the sensor
 *  @param[out] sensor_event_class_type - Type of sensor event class
 *  @param[out] event_class_data_offset - Offset where the event class data
 * should be read from event data
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'event_data'
 */
int decode_sensor_event_data(const uint8_t *event_data,
			     size_t event_data_length, uint16_t *sensor_id,
			     uint8_t *sensor_event_class_type,
			     size_t *event_class_data_offset);

/** @brief Decode sensorOpState response data
 *
 *  @param[in] sensor_data - sensor_data for sensorEventClass = sensorOpState
 *  @param[in] sensor_data_length - Length of sensor_data
 *  @param[out] present_op_state - The sensorOperationalState value from the
 * state change that triggered the event message
 *  @param[out] previous_op_state - The sensorOperationalState value for the
 * state from which the present state was entered
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'sensor_data'
 */
int decode_sensor_op_data(const uint8_t *sensor_data, size_t sensor_data_length,
			  uint8_t *present_op_state,
			  uint8_t *previous_op_state);

/** @brief Decode stateSensorState response data
 *
 *  @param[in] sensor_data - sensor_data for sensorEventClass = stateSensorState
 *  @param[in] sensor_data_length - Length of sensor_data
 *  @param[out] sensor_offset - Identifies which state sensor within a composite
 * state sensor the event is being returned for
 *  @param[out] event_state - The event state value from the state change that
 * triggered the event message
 *  @param[out] previous_event_state - The event state value for the state from
 * which the present event state was entered
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'sensor_data'
 */
int decode_state_sensor_data(const uint8_t *sensor_data,
			     size_t sensor_data_length, uint8_t *sensor_offset,
			     uint8_t *event_state,
			     uint8_t *previous_event_state);

/** @brief Decode numericSensorState response data
 *
 *  @param[in] sensor_data - sensor_data for sensorEventClass =
 * numericSensorState
 *  @param[in] sensor_data_length - Length of sensor_data
 *  @param[out] event_state - The eventState value from the state change that
 * triggered the event message
 *  @param[out] previous_event_state - The eventState value for the state from
 * which the present state was entered
 *  @param[out] sensor_data_size - The bit width and format of reading and
 * threshold values that the sensor returns
 *  @param[out] present_reading - The present value indicated by the sensor
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'sensor_data'
 */
int decode_numeric_sensor_data(const uint8_t *sensor_data,
			       size_t sensor_data_length, uint8_t *event_state,
			       uint8_t *previous_event_state,
			       uint8_t *sensor_data_size,
			       uint32_t *present_reading);

/* GetNumericEffecterValue */

/** @brief Create a PLDM request message for GetNumericEffecterValue
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] effecter_id - used to identify and access the effecter
 *  @param[out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_numeric_effecter_value_req(uint8_t instance_id,
					  uint16_t effecter_id,
					  struct pldm_msg *msg);

/** @brief Create a PLDM response message for GetNumericEffecterValue
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] completion_code - PLDM completion code
 *  @param[out] effecter_data_size - The bit width and format of the setting
 *		value for the effecter.
 *		value:{uint8,sint8,uint16,sint16,uint32,sint32}
 *  @param[out] effecter_oper_state - The state of the effecter itself
 *  @param[out] pending_value - The pending numeric value setting of the
 *              effecter. The effecterDataSize field indicates the number of
 *              bits used for this field
 *  @param[out] present_value - The present numeric value setting of the
 *              effecter. The effecterDataSize indicates the number of bits
 *              used for this field
 *  @return pldm_completion_codes
 */
int decode_get_numeric_effecter_value_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint8_t *effecter_data_size, uint8_t *effecter_oper_state,
    uint8_t *pending_value, uint8_t *present_value);

/** @brief Decode pldmPDRRepositoryChgEvent response data
 *
 *  @param[in] event_data - eventData for pldmPDRRepositoryChgEvent
 *  @param[in] event_data_size - Length of event_data
 *  @param[out] event_data_format - This field indicates if the changedRecords
 * are of PDR Types or PDR Record Handles
 *  @param[out] number_of_change_records - The number of changeRecords following
 * this field
 *  @param[out] change_record_data_offset - Identifies where changeRecord data
 * is located within event_data
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'event_data'
 */
int decode_pldm_pdr_repository_chg_event_data(
    const uint8_t *event_data, size_t event_data_size,
    uint8_t *event_data_format, uint8_t *number_of_change_records,
    size_t *change_record_data_offset);

/** @brief Encode PLDM PDR Repository Change eventData
 *  @param[in] event_data_format - Format of this event data (e.g.
 * FORMAT_IS_PDR_HANDLES)
 *  @param[in] number_of_change_records - Number of changeRecords in this
 * eventData
 *  @param[in] event_data_operations - Array of eventDataOperations
 *      (e.g. RECORDS_ADDED) for each changeRecord in this eventData. This array
 * should contain number_of_change_records elements.
 *  @param[in] numbers_of_change_entries - Array of numbers of changeEntrys
 *      for each changeRecord in this eventData. This array should contain
 *      number_of_change_records elements.
 *  @param[in] change_entries - 2-dimensional array of arrays of changeEntrys,
 *      one array per changeRecord in this eventData. The toplevel array should
 *      contain number_of_change_records elements. Each subarray [i] should
 *      contain numbers_of_change_entries[i] elements.
 *  @param[in] event_data - The eventData will be encoded into this. This entire
 *      structure must be max_change_records_size long. It must be large enough
 *      to accomodate the data to be encoded. The caller is responsible for
 *      allocating and deallocating it, including the variable-size
 *      'event_data.change_records' field. If this parameter is NULL,
 *      PLDM_SUCCESS will be returned and actual_change_records_size will be set
 *      to reflect the required size of the structure.
 *  @param[out] actual_change_records_size - The actual number of meaningful
 *      encoded bytes in event_data. The caller can over-allocate memory and use
 *      this output to determine the real size of the structure.
 *  @param[in] max_change_records_size - The size of event_data in bytes. If the
 *      encoded message would be larger than this value, an error is returned.
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 * 'event_data.change_records'
 */
int encode_pldm_pdr_repository_chg_event_data(
    uint8_t event_data_format, uint8_t number_of_change_records,
    const uint8_t *event_data_operations,
    const uint8_t *numbers_of_change_entries,
    const uint32_t *const *change_entries,
    struct pldm_pdr_repository_chg_event_data *event_data,
    size_t *actual_change_records_size, size_t max_change_records_size);

/** @brief Encode event data for a PLDM Sensor Event
 *
 *  @param[out] event_data              The object to store the encoded event in
 *  @param[in] event_data_size          Size of the allocation for event_data
 *  @param[in] sensor_id                Sensor ID
 *  @param[in] sensor_event_class       Sensor event class
 *  @param[in] sensor_offset            Offset
 *  @param[in] event_state              Event state
 *  @param[in] previous_event_state     Previous event state
 *  @param[out] actual_event_data_size  The real size in bytes of the event_data
 *  @return int pldm_completion_codes   PLDM_SUCCESS/PLDM_ERROR_INVALID_LENGTH
 *  @note If event_data is NULL, then *actual_event_data_size will be set to
 *        reflect the size of the event data, and PLDM_SUCCESS will be returned.
 *  @note The caller is responsible for allocating and deallocating the
 *        event_data
 */
int encode_sensor_event_data(struct pldm_sensor_event_data *event_data,
			     size_t event_data_size, uint16_t sensor_id,
			     enum sensor_event_class_states sensor_event_class,
			     uint8_t sensor_offset, uint8_t event_state,
			     uint8_t previous_event_state,
			     size_t *actual_event_data_size);

/** @brief Decode PldmPDRRepositoryChangeRecord response data
 *
 *  @param[in] change_record_data - changeRecordData for
 * pldmPDRRepositoryChgEvent
 *  @param[in] change_record_data_size - Length of change_record_data
 *  @param[out] event_data_operation - This field indicates the changeEntries
 * operation types
 *  @param[out] number_of_change_entries - The number of changeEntries following
 * this field
 *  @param[out] change_entry_data_offset - Identifies where changeEntries data
 * is located within change_record_data
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'change_record_data'
 */
int decode_pldm_pdr_repository_change_record_data(
    const uint8_t *change_record_data, size_t change_record_data_size,
    uint8_t *event_data_operation, uint8_t *number_of_change_entries,
    size_t *change_entry_data_offset);

/* GetSensorReading */

/** @brief Encode GetSensorReading request data
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] sensor_id - A handle that is used to identify and access the
 *         sensor
 *  @param[in] rearm_event_state - true =  manually re-arm EventState after
 *         responding to this request, false = no manual re-arm
 *  @param[out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note	Caller is responsible for memory alloc and dealloc of param
 * 		'msg.payload'
 */
int encode_get_sensor_reading_req(uint8_t instance_id, uint16_t sensor_id,
				  bool8_t rearm_event_state,
				  struct pldm_msg *msg);

/** @brief Decode GetSensorReading response data
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] completion_code - PLDM completion code
 *  @param[out] sensor_data_size - The bit width and format of reading and
 *         threshold values
 *  @param[out] sensor_operational_state - The state of the sensor itself
 *  @param[out] sensor_event_message_enable - value: { noEventGeneration,
 *         eventsDisabled, eventsEnabled, opEventsOnlyEnabled,
 *         stateEventsOnlyEnabled }
 *  @param[out] present_state - The most recently assessed state value monitored
 *         by the sensor
 *  @param[out] previous_state - The state that the presentState was entered
 *         from
 *  @param[out] event_state - Indicates which threshold crossing assertion
 *         events have been detected
 *  @param[out] present_reading - The present value indicated by the sensor
 *  @return pldm_completion_codes
 */

int decode_get_sensor_reading_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint8_t *sensor_data_size, uint8_t *sensor_operational_state,
    uint8_t *sensor_event_message_enable, uint8_t *present_state,
    uint8_t *previous_state, uint8_t *event_state, uint8_t *present_reading);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_H */
