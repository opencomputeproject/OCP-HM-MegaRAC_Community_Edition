#include <endian.h>
#include <string.h>

#include "platform.h"

int encode_state_effecter_pdr(
    struct pldm_state_effecter_pdr *const effecter,
    const size_t allocation_size,
    const struct state_effecter_possible_states *const possible_states,
    const size_t possible_states_size, size_t *const actual_size)
{
	// Encode possible states

	size_t calculated_possible_states_size = 0;

	{
		char *states_ptr = (char *)possible_states;
		char *const begin_states_ptr = states_ptr;

		for (int i = 0; i < effecter->composite_effecter_count; ++i) {
			struct state_effecter_possible_states *states =
			    (struct state_effecter_possible_states *)states_ptr;

			HTOLE16(states->state_set_id);

			states_ptr +=
			    (sizeof(*states) - sizeof(states->states) +
			     states->possible_states_size);
		}

		calculated_possible_states_size = states_ptr - begin_states_ptr;
	}

	// Check lengths

	if (possible_states_size != calculated_possible_states_size) {
		*actual_size = 0;
		return PLDM_ERROR;
	}

	*actual_size =
	    (sizeof(struct pldm_state_effecter_pdr) + possible_states_size -
	     sizeof(effecter->possible_states));

	if (allocation_size < *actual_size) {
		*actual_size = 0;
		return PLDM_ERROR_INVALID_LENGTH;
	}

	// Encode rest of PDR

	effecter->hdr.version = 1;
	effecter->hdr.type = PLDM_STATE_EFFECTER_PDR;
	effecter->hdr.length = *actual_size - sizeof(struct pldm_pdr_hdr);

	memcpy(effecter->possible_states, possible_states,
	       possible_states_size);

	// Convert effecter PDR body
	HTOLE16(effecter->terminus_handle);
	HTOLE16(effecter->effecter_id);
	HTOLE16(effecter->entity_type);
	HTOLE16(effecter->entity_instance);
	HTOLE16(effecter->container_id);
	HTOLE16(effecter->effecter_semantic_id);

	// Convert header
	HTOLE32(effecter->hdr.record_handle);
	HTOLE16(effecter->hdr.record_change_num);
	HTOLE16(effecter->hdr.length);

	return PLDM_SUCCESS;
}

int encode_state_sensor_pdr(
    struct pldm_state_sensor_pdr *const sensor, const size_t allocation_size,
    const struct state_sensor_possible_states *const possible_states,
    const size_t possible_states_size, size_t *const actual_size)
{
	// Encode possible states

	size_t calculated_possible_states_size = 0;

	{
		char *states_ptr = (char *)possible_states,
		     *const begin_states_ptr = states_ptr;

		for (int i = 0; i < sensor->composite_sensor_count; ++i) {
			struct state_sensor_possible_states *states =
			    (struct state_sensor_possible_states *)states_ptr;

			HTOLE16(states->state_set_id);

			states_ptr +=
			    (sizeof(*states) - sizeof(states->states) +
			     states->possible_states_size);
		}

		calculated_possible_states_size = states_ptr - begin_states_ptr;
	}

	// Check lengths

	if (possible_states_size != calculated_possible_states_size) {
		*actual_size = 0;
		return PLDM_ERROR;
	}

	*actual_size = (sizeof(struct pldm_state_sensor_pdr) +
			possible_states_size - sizeof(sensor->possible_states));

	if (allocation_size < *actual_size) {
		*actual_size = 0;
		return PLDM_ERROR_INVALID_LENGTH;
	}

	// Encode rest of PDR

	sensor->hdr.version = 1;
	sensor->hdr.type = PLDM_STATE_SENSOR_PDR;
	sensor->hdr.length = *actual_size - sizeof(struct pldm_pdr_hdr);

	memcpy(sensor->possible_states, possible_states, possible_states_size);

	// Convert sensor PDR body
	HTOLE16(sensor->terminus_handle);
	HTOLE16(sensor->sensor_id);
	HTOLE16(sensor->entity_type);
	HTOLE16(sensor->entity_instance);
	HTOLE16(sensor->container_id);

	// Convert header
	HTOLE32(sensor->hdr.record_handle);
	HTOLE16(sensor->hdr.record_change_num);
	HTOLE16(sensor->hdr.length);

	return PLDM_SUCCESS;
}

int encode_set_state_effecter_states_resp(uint8_t instance_id,
					  uint8_t completion_code,
					  struct pldm_msg *msg)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	msg->payload[0] = completion_code;

	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_SET_STATE_EFFECTER_STATES;

	rc = pack_pldm_header(&header, &(msg->hdr));

	return rc;
}

int encode_set_state_effecter_states_req(uint8_t instance_id,
					 uint16_t effecter_id,
					 uint8_t comp_effecter_count,
					 set_effecter_state_field *field,
					 struct pldm_msg *msg)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	header.msg_type = PLDM_REQUEST;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_SET_STATE_EFFECTER_STATES;

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	if (comp_effecter_count < 0x1 || comp_effecter_count > 0x8 ||
	    field == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_set_state_effecter_states_req *request =
	    (struct pldm_set_state_effecter_states_req *)msg->payload;
	effecter_id = htole16(effecter_id);
	request->effecter_id = effecter_id;
	request->comp_effecter_count = comp_effecter_count;
	memcpy(request->field, field,
	       (sizeof(set_effecter_state_field) * comp_effecter_count));

	return PLDM_SUCCESS;
}

int decode_set_state_effecter_states_resp(const struct pldm_msg *msg,
					  size_t payload_length,
					  uint8_t *completion_code)
{
	if (msg == NULL || completion_code == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length > PLDM_SET_STATE_EFFECTER_STATES_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	return PLDM_SUCCESS;
}

int decode_set_state_effecter_states_req(const struct pldm_msg *msg,
					 size_t payload_length,
					 uint16_t *effecter_id,
					 uint8_t *comp_effecter_count,
					 set_effecter_state_field *field)
{
	if (msg == NULL || effecter_id == NULL || comp_effecter_count == NULL ||
	    field == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length > PLDM_SET_STATE_EFFECTER_STATES_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_set_state_effecter_states_req *request =
	    (struct pldm_set_state_effecter_states_req *)msg->payload;

	*effecter_id = le16toh(request->effecter_id);
	*comp_effecter_count = request->comp_effecter_count;
	memcpy(field, request->field,
	       (sizeof(set_effecter_state_field) * (*comp_effecter_count)));

	return PLDM_SUCCESS;
}

int decode_get_pdr_req(const struct pldm_msg *msg, size_t payload_length,
		       uint32_t *record_hndl, uint32_t *data_transfer_hndl,
		       uint8_t *transfer_op_flag, uint16_t *request_cnt,
		       uint16_t *record_chg_num)
{
	if (msg == NULL || record_hndl == NULL || data_transfer_hndl == NULL ||
	    transfer_op_flag == NULL || request_cnt == NULL ||
	    record_chg_num == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (payload_length != PLDM_GET_PDR_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_pdr_req *request =
	    (struct pldm_get_pdr_req *)msg->payload;
	*record_hndl = le32toh(request->record_handle);
	*data_transfer_hndl = le32toh(request->data_transfer_handle);
	*transfer_op_flag = request->transfer_op_flag;
	*request_cnt = le16toh(request->request_count);
	*record_chg_num = le16toh(request->record_change_number);

	return PLDM_SUCCESS;
}

int encode_get_pdr_resp(uint8_t instance_id, uint8_t completion_code,
			uint32_t next_record_hndl,
			uint32_t next_data_transfer_hndl, uint8_t transfer_flag,
			uint16_t resp_cnt, const uint8_t *record_data,
			uint8_t transfer_crc, struct pldm_msg *msg)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_get_pdr_resp *response =
	    (struct pldm_get_pdr_resp *)msg->payload;

	response->completion_code = completion_code;

	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_GET_PDR;
	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	if (response->completion_code == PLDM_SUCCESS) {
		response->next_record_handle = htole32(next_record_hndl);
		response->next_data_transfer_handle =
		    htole32(next_data_transfer_hndl);
		response->transfer_flag = transfer_flag;
		response->response_count = htole16(resp_cnt);
		if (record_data != NULL && resp_cnt > 0) {
			memcpy(response->record_data, record_data, resp_cnt);
		}
		if (transfer_flag == PLDM_END) {
			uint8_t *dst = msg->payload;
			dst +=
			    (sizeof(struct pldm_get_pdr_resp) - 1) + resp_cnt;
			*dst = transfer_crc;
		}
	}

	return PLDM_SUCCESS;
}

int encode_get_pdr_req(uint8_t instance_id, uint32_t record_hndl,
		       uint32_t data_transfer_hndl, uint8_t transfer_op_flag,
		       uint16_t request_cnt, uint16_t record_chg_num,
		       struct pldm_msg *msg, size_t payload_length)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_get_pdr_req *request =
	    (struct pldm_get_pdr_req *)msg->payload;

	header.msg_type = PLDM_REQUEST;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_GET_PDR;

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	if (payload_length != PLDM_GET_PDR_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	request->record_handle = htole32(record_hndl);
	request->data_transfer_handle = htole32(data_transfer_hndl);
	request->transfer_op_flag = transfer_op_flag;
	request->request_count = htole16(request_cnt);
	request->record_change_number = htole16(record_chg_num);

	return PLDM_SUCCESS;
}

int decode_get_pdr_resp(const struct pldm_msg *msg, size_t payload_length,
			uint8_t *completion_code, uint32_t *next_record_hndl,
			uint32_t *next_data_transfer_hndl,
			uint8_t *transfer_flag, uint16_t *resp_cnt,
			uint8_t *record_data, size_t record_data_length,
			uint8_t *transfer_crc)
{
	if (msg == NULL || completion_code == NULL ||
	    next_record_hndl == NULL || next_data_transfer_hndl == NULL ||
	    transfer_flag == NULL || resp_cnt == NULL || transfer_crc == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length < PLDM_GET_PDR_MIN_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_pdr_resp *response =
	    (struct pldm_get_pdr_resp *)msg->payload;

	*next_record_hndl = le32toh(response->next_record_handle);
	*next_data_transfer_hndl = le32toh(response->next_data_transfer_handle);
	*transfer_flag = response->transfer_flag;
	*resp_cnt = le16toh(response->response_count);

	if (*transfer_flag != PLDM_END &&
	    (int)payload_length != PLDM_GET_PDR_MIN_RESP_BYTES + *resp_cnt) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	if (*transfer_flag == PLDM_END &&
	    (int)payload_length !=
		PLDM_GET_PDR_MIN_RESP_BYTES + *resp_cnt + 1) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	if (*resp_cnt > 0 && record_data != NULL) {
		if (record_data_length < *resp_cnt) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		memcpy(record_data, response->record_data, *resp_cnt);
	}

	if (*transfer_flag == PLDM_END) {
		*transfer_crc =
		    msg->payload[PLDM_GET_PDR_MIN_RESP_BYTES + *resp_cnt];
	}

	return PLDM_SUCCESS;
}

int decode_set_numeric_effecter_value_req(const struct pldm_msg *msg,
					  size_t payload_length,
					  uint16_t *effecter_id,
					  uint8_t *effecter_data_size,
					  uint8_t *effecter_value)
{
	if (msg == NULL || effecter_id == NULL || effecter_data_size == NULL ||
	    effecter_value == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length < PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_set_numeric_effecter_value_req *request =
	    (struct pldm_set_numeric_effecter_value_req *)msg->payload;
	*effecter_id = le16toh(request->effecter_id);
	*effecter_data_size = request->effecter_data_size;

	if (*effecter_data_size > PLDM_EFFECTER_DATA_SIZE_SINT32) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (*effecter_data_size == PLDM_EFFECTER_DATA_SIZE_UINT8 ||
	    *effecter_data_size == PLDM_EFFECTER_DATA_SIZE_SINT8) {

		if (payload_length !=
		    PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES) {
			return PLDM_ERROR_INVALID_LENGTH;
		}

		*effecter_value = request->effecter_value[0];
	}

	if (*effecter_data_size == PLDM_EFFECTER_DATA_SIZE_UINT16 ||
	    *effecter_data_size == PLDM_EFFECTER_DATA_SIZE_SINT16) {

		if (payload_length !=
		    PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 1) {
			return PLDM_ERROR_INVALID_LENGTH;
		}

		memcpy(effecter_value, request->effecter_value, 2);
		uint16_t *val = (uint16_t *)(effecter_value);
		*val = le16toh(*val);
	}

	if (*effecter_data_size == PLDM_EFFECTER_DATA_SIZE_UINT32 ||
	    *effecter_data_size == PLDM_EFFECTER_DATA_SIZE_SINT32) {

		if (payload_length !=
		    PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 3) {
			return PLDM_ERROR_INVALID_LENGTH;
		}

		memcpy(effecter_value, request->effecter_value, 4);
		uint32_t *val = (uint32_t *)(effecter_value);
		*val = le32toh(*val);
	}

	return PLDM_SUCCESS;
}

int encode_set_numeric_effecter_value_resp(uint8_t instance_id,
					   uint8_t completion_code,
					   struct pldm_msg *msg,
					   size_t payload_length)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_SET_NUMERIC_EFFECTER_VALUE_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	msg->payload[0] = completion_code;

	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_SET_NUMERIC_EFFECTER_VALUE;

	rc = pack_pldm_header(&header, &(msg->hdr));

	return rc;
}

int encode_set_numeric_effecter_value_req(
    uint8_t instance_id, uint16_t effecter_id, uint8_t effecter_data_size,
    uint8_t *effecter_value, struct pldm_msg *msg, size_t payload_length)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	if (msg == NULL || effecter_value == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_set_numeric_effecter_value_req *request =
	    (struct pldm_set_numeric_effecter_value_req *)msg->payload;

	header.msg_type = PLDM_REQUEST;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_SET_NUMERIC_EFFECTER_VALUE;

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	if (effecter_data_size > PLDM_EFFECTER_DATA_SIZE_SINT32) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (effecter_data_size == PLDM_EFFECTER_DATA_SIZE_UINT8 ||
	    effecter_data_size == PLDM_EFFECTER_DATA_SIZE_SINT8) {
		if (payload_length !=
		    PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		request->effecter_value[0] = *effecter_value;
	} else if (effecter_data_size == PLDM_EFFECTER_DATA_SIZE_UINT16 ||
		   effecter_data_size == PLDM_EFFECTER_DATA_SIZE_SINT16) {
		if (payload_length !=
		    PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 1) {
			return PLDM_ERROR_INVALID_LENGTH;
		}

		uint16_t val = *(uint16_t *)(effecter_value);
		val = htole16(val);
		memcpy(request->effecter_value, &val, sizeof(uint16_t));

	} else if (effecter_data_size == PLDM_EFFECTER_DATA_SIZE_UINT32 ||
		   effecter_data_size == PLDM_EFFECTER_DATA_SIZE_SINT32) {
		if (payload_length !=
		    PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 3) {
			return PLDM_ERROR_INVALID_LENGTH;
		}

		uint32_t val = *(uint32_t *)(effecter_value);
		val = htole32(val);
		memcpy(request->effecter_value, &val, sizeof(uint32_t));
	}

	request->effecter_id = htole16(effecter_id);
	request->effecter_data_size = effecter_data_size;

	return PLDM_SUCCESS;
}

int decode_set_numeric_effecter_value_resp(const struct pldm_msg *msg,
					   size_t payload_length,
					   uint8_t *completion_code)
{
	if (msg == NULL || completion_code == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_SET_NUMERIC_EFFECTER_VALUE_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*completion_code = msg->payload[0];

	return PLDM_SUCCESS;
}

int encode_get_state_sensor_readings_resp(uint8_t instance_id,
					  uint8_t completion_code,
					  uint8_t comp_sensor_count,
					  get_sensor_state_field *field,
					  struct pldm_msg *msg)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_GET_STATE_SENSOR_READINGS;

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	if (comp_sensor_count < 0x1 || comp_sensor_count > 0x8) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_get_state_sensor_readings_resp *response =
	    (struct pldm_get_state_sensor_readings_resp *)msg->payload;

	response->completion_code = completion_code;
	response->comp_sensor_count = comp_sensor_count;
	memcpy(response->field, field,
	       (sizeof(get_sensor_state_field) * comp_sensor_count));

	return rc;
}

int encode_get_state_sensor_readings_req(uint8_t instance_id,
					 uint16_t sensor_id,
					 bitfield8_t sensor_rearm,
					 uint8_t reserved, struct pldm_msg *msg)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	header.msg_type = PLDM_REQUEST;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_GET_STATE_SENSOR_READINGS;

	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_get_state_sensor_readings_req *request =
	    (struct pldm_get_state_sensor_readings_req *)msg->payload;

	request->sensor_id = htole16(sensor_id);
	request->reserved = reserved;
	request->sensor_rearm = sensor_rearm;

	return PLDM_SUCCESS;
}

int decode_get_state_sensor_readings_resp(const struct pldm_msg *msg,
					  size_t payload_length,
					  uint8_t *completion_code,
					  uint8_t *comp_sensor_count,
					  get_sensor_state_field *field)
{
	if (msg == NULL || completion_code == NULL ||
	    comp_sensor_count == NULL || field == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length >
	    PLDM_GET_STATE_SENSOR_READINGS_MIN_RESP_BYTES +
		sizeof(get_sensor_state_field) * *comp_sensor_count) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_state_sensor_readings_resp *response =
	    (struct pldm_get_state_sensor_readings_resp *)msg->payload;

	if (response->comp_sensor_count < 0x1 ||
	    response->comp_sensor_count > 0x8) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (response->comp_sensor_count > *comp_sensor_count) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	*comp_sensor_count = response->comp_sensor_count;

	memcpy(field, response->field,
	       (sizeof(get_sensor_state_field) * (*comp_sensor_count)));

	return PLDM_SUCCESS;
}

int decode_get_state_sensor_readings_req(const struct pldm_msg *msg,
					 size_t payload_length,
					 uint16_t *sensor_id,
					 bitfield8_t *sensor_rearm,
					 uint8_t *reserved)
{
	if (msg == NULL || sensor_id == NULL || sensor_rearm == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_GET_STATE_SENSOR_READINGS_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_state_sensor_readings_req *request =
	    (struct pldm_get_state_sensor_readings_req *)msg->payload;

	*sensor_id = le16toh(request->sensor_id);
	*reserved = request->reserved;
	memcpy(&(sensor_rearm->byte), &(request->sensor_rearm.byte),
	       sizeof(request->sensor_rearm.byte));

	return PLDM_SUCCESS;
}

int encode_sensor_event_data(
    struct pldm_sensor_event_data *const event_data,
    const size_t event_data_size, const uint16_t sensor_id,
    const enum sensor_event_class_states sensor_event_class,
    const uint8_t sensor_offset, const uint8_t event_state,
    const uint8_t previous_event_state, size_t *const actual_event_data_size)
{
	*actual_event_data_size =
	    (sizeof(*event_data) - sizeof(event_data->event_class) +
	     sizeof(struct pldm_sensor_event_state_sensor_state));

	if (!event_data) {
		return PLDM_SUCCESS;
	}

	if (event_data_size < *actual_event_data_size) {
		*actual_event_data_size = 0;
		return PLDM_ERROR_INVALID_LENGTH;
	}

	event_data->sensor_id = htole32(sensor_id);
	event_data->sensor_event_class_type = sensor_event_class;

	struct pldm_sensor_event_state_sensor_state *const state_data =
	    (struct pldm_sensor_event_state_sensor_state *)
		event_data->event_class;

	state_data->sensor_offset = sensor_offset;
	state_data->event_state = event_state;
	state_data->previous_event_state = previous_event_state;

	return PLDM_SUCCESS;
}

int decode_platform_event_message_req(const struct pldm_msg *msg,
				      size_t payload_length,
				      uint8_t *format_version, uint8_t *tid,
				      uint8_t *event_class,
				      size_t *event_data_offset)
{

	if (msg == NULL || format_version == NULL || tid == NULL ||
	    event_class == NULL || event_data_offset == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length <= PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_platform_event_message_req *response =
	    (struct pldm_platform_event_message_req *)msg->payload;

	*format_version = response->format_version;
	*tid = response->tid;
	*event_class = response->event_class;
	*event_data_offset =
	    sizeof(*format_version) + sizeof(*tid) + sizeof(*event_class);

	return PLDM_SUCCESS;
}

int encode_platform_event_message_resp(uint8_t instance_id,
				       uint8_t completion_code,
				       uint8_t platform_event_status,
				       struct pldm_msg *msg)
{
	int rc = PLDM_SUCCESS;

	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (platform_event_status > PLDM_EVENT_LOGGING_REJECTED) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_platform_event_message_resp *response =
	    (struct pldm_platform_event_message_resp *)msg->payload;
	response->completion_code = completion_code;
	response->platform_event_status = platform_event_status;

	struct pldm_header_info header = {0};
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_PLATFORM_EVENT_MESSAGE;

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}
	return PLDM_SUCCESS;
}

int encode_platform_event_message_req(
    uint8_t instance_id, uint8_t format_version, uint8_t tid,
    uint8_t event_class, const uint8_t *event_data, size_t event_data_length,
    struct pldm_msg *msg, size_t payload_length)

{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	header.msg_type = PLDM_REQUEST;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_PLATFORM_EVENT_MESSAGE;

	if (format_version != 1) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (msg == NULL || event_data == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (event_data_length == 0) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length !=
	    PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES + event_data_length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	if (event_class > PLDM_HEARTBEAT_TIMER_ELAPSED_EVENT &&
	    !(event_class >= 0xF0 && event_class <= 0xFE)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_platform_event_message_req *request =
	    (struct pldm_platform_event_message_req *)msg->payload;
	request->format_version = format_version;
	request->tid = tid;
	request->event_class = event_class;
	memcpy(request->event_data, event_data, event_data_length);

	return PLDM_SUCCESS;
}

int decode_platform_event_message_resp(const struct pldm_msg *msg,
				       size_t payload_length,
				       uint8_t *completion_code,
				       uint8_t *platform_event_status)
{
	if (msg == NULL || completion_code == NULL ||
	    platform_event_status == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}
	if (payload_length != PLDM_PLATFORM_EVENT_MESSAGE_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_platform_event_message_resp *response =
	    (struct pldm_platform_event_message_resp *)msg->payload;
	*platform_event_status = response->platform_event_status;

	if (*platform_event_status > PLDM_EVENT_LOGGING_REJECTED) {
		return PLDM_ERROR_INVALID_DATA;
	}

	return PLDM_SUCCESS;
}

int decode_sensor_event_data(const uint8_t *event_data,
			     size_t event_data_length, uint16_t *sensor_id,
			     uint8_t *sensor_event_class_type,
			     size_t *event_class_data_offset)
{
	if (event_data == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (event_data_length < PLDM_SENSOR_EVENT_DATA_MIN_LENGTH) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	size_t event_class_data_length =
	    event_data_length - PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES;

	struct pldm_sensor_event_data *sensor_event_data =
	    (struct pldm_sensor_event_data *)event_data;
	*sensor_id = sensor_event_data->sensor_id;
	*sensor_event_class_type = sensor_event_data->sensor_event_class_type;
	if (sensor_event_data->sensor_event_class_type ==
	    PLDM_SENSOR_OP_STATE) {
		if (event_class_data_length !=
		    PLDM_SENSOR_EVENT_SENSOR_OP_STATE_DATA_LENGTH) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
	} else if (sensor_event_data->sensor_event_class_type ==
		   PLDM_STATE_SENSOR_STATE) {
		if (event_class_data_length !=
		    PLDM_SENSOR_EVENT_STATE_SENSOR_STATE_DATA_LENGTH) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
	} else if (sensor_event_data->sensor_event_class_type ==
		   PLDM_NUMERIC_SENSOR_STATE) {
		if (event_class_data_length <
			PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_MIN_DATA_LENGTH ||
		    event_class_data_length >
			PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_MAX_DATA_LENGTH) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
	} else {
		return PLDM_ERROR_INVALID_DATA;
	}
	*event_class_data_offset =
	    sizeof(*sensor_id) + sizeof(*sensor_event_class_type);
	return PLDM_SUCCESS;
}

int decode_sensor_op_data(const uint8_t *sensor_data, size_t sensor_data_length,
			  uint8_t *present_op_state, uint8_t *previous_op_state)
{
	if (sensor_data == NULL || present_op_state == NULL ||
	    previous_op_state == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (sensor_data_length !=
	    PLDM_SENSOR_EVENT_SENSOR_OP_STATE_DATA_LENGTH) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_sensor_event_sensor_op_state *sensor_op_data =
	    (struct pldm_sensor_event_sensor_op_state *)sensor_data;
	*present_op_state = sensor_op_data->present_op_state;
	*previous_op_state = sensor_op_data->previous_op_state;
	return PLDM_SUCCESS;
}

int decode_state_sensor_data(const uint8_t *sensor_data,
			     size_t sensor_data_length, uint8_t *sensor_offset,
			     uint8_t *event_state,
			     uint8_t *previous_event_state)
{
	if (sensor_data == NULL || sensor_offset == NULL ||
	    event_state == NULL || previous_event_state == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (sensor_data_length !=
	    PLDM_SENSOR_EVENT_STATE_SENSOR_STATE_DATA_LENGTH) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_sensor_event_state_sensor_state *sensor_state_data =
	    (struct pldm_sensor_event_state_sensor_state *)sensor_data;
	*sensor_offset = sensor_state_data->sensor_offset;
	*event_state = sensor_state_data->event_state;
	*previous_event_state = sensor_state_data->previous_event_state;
	return PLDM_SUCCESS;
}

int decode_numeric_sensor_data(const uint8_t *sensor_data,
			       size_t sensor_data_length, uint8_t *event_state,
			       uint8_t *previous_event_state,
			       uint8_t *sensor_data_size,
			       uint32_t *present_reading)
{
	if (sensor_data == NULL || sensor_data_size == NULL ||
	    event_state == NULL || previous_event_state == NULL ||
	    present_reading == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (sensor_data_length <
		PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_MIN_DATA_LENGTH ||
	    sensor_data_length >
		PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_MAX_DATA_LENGTH) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_sensor_event_numeric_sensor_state *numeric_sensor_data =
	    (struct pldm_sensor_event_numeric_sensor_state *)sensor_data;
	*event_state = numeric_sensor_data->event_state;
	*previous_event_state = numeric_sensor_data->previous_event_state;
	*sensor_data_size = numeric_sensor_data->sensor_data_size;
	uint8_t *present_reading_ptr = numeric_sensor_data->present_reading;

	switch (*sensor_data_size) {
	case PLDM_SENSOR_DATA_SIZE_UINT8:
	case PLDM_SENSOR_DATA_SIZE_SINT8:
		if (sensor_data_length !=
		    PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_8BIT_DATA_LENGTH) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		*present_reading = present_reading_ptr[0];
		break;
	case PLDM_SENSOR_DATA_SIZE_UINT16:
	case PLDM_SENSOR_DATA_SIZE_SINT16:
		if (sensor_data_length !=
		    PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_16BIT_DATA_LENGTH) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		*present_reading = le16toh(present_reading_ptr[1] |
					   (present_reading_ptr[0] << 8));
		break;
	case PLDM_SENSOR_DATA_SIZE_UINT32:
	case PLDM_SENSOR_DATA_SIZE_SINT32:
		if (sensor_data_length !=
		    PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_32BIT_DATA_LENGTH) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		*present_reading = le32toh(present_reading_ptr[3] |
					   (present_reading_ptr[2] << 8) |
					   (present_reading_ptr[1] << 16) |
					   (present_reading_ptr[0] << 24));
		break;
	default:
		return PLDM_ERROR_INVALID_DATA;
	}
	return PLDM_SUCCESS;
}

int encode_get_numeric_effecter_value_req(uint8_t instance_id,
					  uint16_t effecter_id,
					  struct pldm_msg *msg)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_get_numeric_effecter_value_req *request =
	    (struct pldm_get_numeric_effecter_value_req *)msg->payload;

	header.msg_type = PLDM_REQUEST;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_GET_NUMERIC_EFFECTER_VALUE;

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	request->effecter_id = htole16(effecter_id);

	return PLDM_SUCCESS;
}

int encode_get_numeric_effecter_value_resp(
    uint8_t instance_id, uint8_t completion_code, uint8_t effecter_data_size,
    uint8_t effecter_oper_state, uint8_t *pending_value, uint8_t *present_value,
    struct pldm_msg *msg, size_t payload_length)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	if (msg == NULL || pending_value == NULL || present_value == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (effecter_data_size > PLDM_EFFECTER_DATA_SIZE_SINT32) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (effecter_oper_state > EFFECTER_OPER_STATE_INTEST) {
		return PLDM_ERROR_INVALID_DATA;
	}

	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_GET_NUMERIC_EFFECTER_VALUE;

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_get_numeric_effecter_value_resp *response =
	    (struct pldm_get_numeric_effecter_value_resp *)msg->payload;

	response->completion_code = completion_code;
	response->effecter_data_size = effecter_data_size;
	response->effecter_oper_state = effecter_oper_state;

	if (effecter_data_size == PLDM_EFFECTER_DATA_SIZE_UINT8 ||
	    effecter_data_size == PLDM_EFFECTER_DATA_SIZE_SINT8) {
		if (payload_length !=
		    PLDM_GET_NUMERIC_EFFECTER_VALUE_MIN_RESP_BYTES) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		response->pending_and_present_values[0] = *pending_value;
		response->pending_and_present_values[1] = *present_value;

	} else if (effecter_data_size == PLDM_EFFECTER_DATA_SIZE_UINT16 ||
		   effecter_data_size == PLDM_EFFECTER_DATA_SIZE_SINT16) {
		if (payload_length !=
		    PLDM_GET_NUMERIC_EFFECTER_VALUE_MIN_RESP_BYTES + 2) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		uint16_t val_pending = *(uint16_t *)pending_value;
		val_pending = htole16(val_pending);
		memcpy(response->pending_and_present_values, &val_pending,
		       sizeof(uint16_t));
		uint16_t val_present = *(uint16_t *)present_value;
		val_present = htole16(val_present);
		memcpy(
		    (response->pending_and_present_values + sizeof(uint16_t)),
		    &val_present, sizeof(uint16_t));

	} else if (effecter_data_size == PLDM_EFFECTER_DATA_SIZE_UINT32 ||
		   effecter_data_size == PLDM_EFFECTER_DATA_SIZE_SINT32) {
		if (payload_length !=
		    PLDM_GET_NUMERIC_EFFECTER_VALUE_MIN_RESP_BYTES + 6) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		uint32_t val_pending = *(uint32_t *)pending_value;
		val_pending = htole32(val_pending);
		memcpy(response->pending_and_present_values, &val_pending,
		       sizeof(uint32_t));
		uint32_t val_present = *(uint32_t *)present_value;
		val_present = htole32(val_present);
		memcpy(
		    (response->pending_and_present_values + sizeof(uint32_t)),
		    &val_present, sizeof(uint32_t));
	}
	return PLDM_SUCCESS;
}

int decode_get_numeric_effecter_value_req(const struct pldm_msg *msg,
					  size_t payload_length,
					  uint16_t *effecter_id)
{
	if (msg == NULL || effecter_id == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_GET_NUMERIC_EFFECTER_VALUE_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_numeric_effecter_value_req *request =
	    (struct pldm_get_numeric_effecter_value_req *)msg->payload;

	*effecter_id = le16toh(request->effecter_id);

	return PLDM_SUCCESS;
}

int decode_get_numeric_effecter_value_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint8_t *effecter_data_size, uint8_t *effecter_oper_state,
    uint8_t *pending_value, uint8_t *present_value)
{
	if (msg == NULL || effecter_data_size == NULL ||
	    effecter_oper_state == NULL || pending_value == NULL ||
	    present_value == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length < PLDM_GET_NUMERIC_EFFECTER_VALUE_MIN_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_numeric_effecter_value_resp *response =
	    (struct pldm_get_numeric_effecter_value_resp *)msg->payload;

	*effecter_data_size = response->effecter_data_size;
	*effecter_oper_state = response->effecter_oper_state;

	if (*effecter_data_size > PLDM_EFFECTER_DATA_SIZE_SINT32) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (*effecter_oper_state > EFFECTER_OPER_STATE_INTEST) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (*effecter_data_size == PLDM_EFFECTER_DATA_SIZE_UINT8 ||
	    *effecter_data_size == PLDM_EFFECTER_DATA_SIZE_SINT8) {
		if (payload_length !=
		    PLDM_GET_NUMERIC_EFFECTER_VALUE_MIN_RESP_BYTES) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		memcpy(pending_value, response->pending_and_present_values, 1);
		memcpy(present_value, &response->pending_and_present_values[1],
		       1);

	} else if (*effecter_data_size == PLDM_EFFECTER_DATA_SIZE_UINT16 ||
		   *effecter_data_size == PLDM_EFFECTER_DATA_SIZE_SINT16) {
		if (payload_length !=
		    PLDM_GET_NUMERIC_EFFECTER_VALUE_MIN_RESP_BYTES + 2) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		memcpy(pending_value, response->pending_and_present_values,
		       sizeof(uint16_t));
		uint16_t *val_pending = (uint16_t *)pending_value;
		*val_pending = le16toh(*val_pending);
		memcpy(
		    present_value,
		    (response->pending_and_present_values + sizeof(uint16_t)),
		    sizeof(uint16_t));
		uint16_t *val_present = (uint16_t *)present_value;
		*val_present = le16toh(*val_present);

	} else if (*effecter_data_size == PLDM_EFFECTER_DATA_SIZE_UINT32 ||
		   *effecter_data_size == PLDM_EFFECTER_DATA_SIZE_SINT32) {
		if (payload_length !=
		    PLDM_GET_NUMERIC_EFFECTER_VALUE_MIN_RESP_BYTES + 6) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		memcpy(pending_value, response->pending_and_present_values,
		       sizeof(uint32_t));
		uint32_t *val_pending = (uint32_t *)pending_value;
		*val_pending = le32toh(*val_pending);
		memcpy(
		    present_value,
		    (response->pending_and_present_values + sizeof(uint32_t)),
		    sizeof(uint32_t));
		uint32_t *val_present = (uint32_t *)present_value;
		*val_present = le32toh(*val_present);
	}
	return PLDM_SUCCESS;
}

int encode_pldm_pdr_repository_chg_event_data(
    uint8_t event_data_format, uint8_t number_of_change_records,
    const uint8_t *event_data_operations,
    const uint8_t *numbers_of_change_entries,
    const uint32_t *const *change_entries,
    struct pldm_pdr_repository_chg_event_data *event_data,
    size_t *actual_change_records_size, size_t max_change_records_size)
{
	if (event_data_operations == NULL ||
	    numbers_of_change_entries == NULL || change_entries == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	size_t expected_size =
	    sizeof(event_data_format) + sizeof(number_of_change_records);

	expected_size +=
	    sizeof(*event_data_operations) * number_of_change_records;
	expected_size +=
	    sizeof(*numbers_of_change_entries) * number_of_change_records;

	for (uint8_t i = 0; i < number_of_change_records; ++i) {
		expected_size +=
		    sizeof(*change_entries[0]) * numbers_of_change_entries[i];
	}

	*actual_change_records_size = expected_size;

	if (event_data == NULL) {
		return PLDM_SUCCESS;
	}

	if (max_change_records_size < expected_size) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	event_data->event_data_format = event_data_format;
	event_data->number_of_change_records = number_of_change_records;

	struct pldm_pdr_repository_change_record_data *record_data =
	    (struct pldm_pdr_repository_change_record_data *)
		event_data->change_records;

	for (uint8_t i = 0; i < number_of_change_records; ++i) {
		record_data->event_data_operation = event_data_operations[i];
		record_data->number_of_change_entries =
		    numbers_of_change_entries[i];

		for (uint8_t j = 0; j < record_data->number_of_change_entries;
		     ++j) {
			record_data->change_entry[j] =
			    htole32(change_entries[i][j]);
		}

		record_data = (struct pldm_pdr_repository_change_record_data
				   *)(record_data->change_entry +
				      record_data->number_of_change_entries);
	}

	return PLDM_SUCCESS;
}

int decode_pldm_pdr_repository_chg_event_data(const uint8_t *event_data,
					      size_t event_data_size,
					      uint8_t *event_data_format,
					      uint8_t *number_of_change_records,
					      size_t *change_record_data_offset)
{
	if (event_data == NULL || event_data_format == NULL ||
	    number_of_change_records == NULL ||
	    change_record_data_offset == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (event_data_size < PLDM_PDR_REPOSITORY_CHG_EVENT_MIN_LENGTH) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_pdr_repository_chg_event_data
	    *pdr_repository_chg_event_data =
		(struct pldm_pdr_repository_chg_event_data *)event_data;

	*event_data_format = pdr_repository_chg_event_data->event_data_format;
	*number_of_change_records =
	    pdr_repository_chg_event_data->number_of_change_records;
	*change_record_data_offset =
	    sizeof(*event_data_format) + sizeof(*number_of_change_records);

	return PLDM_SUCCESS;
}

int decode_pldm_pdr_repository_change_record_data(
    const uint8_t *change_record_data, size_t change_record_data_size,
    uint8_t *event_data_operation, uint8_t *number_of_change_entries,
    size_t *change_entry_data_offset)
{
	if (change_record_data == NULL || event_data_operation == NULL ||
	    number_of_change_entries == NULL ||
	    change_entry_data_offset == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (change_record_data_size <
	    PLDM_PDR_REPOSITORY_CHANGE_RECORD_MIN_LENGTH) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_pdr_repository_change_record_data
	    *pdr_repository_change_record_data =
		(struct pldm_pdr_repository_change_record_data *)
		    change_record_data;

	*event_data_operation =
	    pdr_repository_change_record_data->event_data_operation;
	*number_of_change_entries =
	    pdr_repository_change_record_data->number_of_change_entries;
	*change_entry_data_offset =
	    sizeof(*event_data_operation) + sizeof(*number_of_change_entries);

	return PLDM_SUCCESS;
}

int encode_get_sensor_reading_req(uint8_t instance_id, uint16_t sensor_id,
				  uint8_t rearm_event_state,
				  struct pldm_msg *msg)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	header.msg_type = PLDM_REQUEST;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_GET_SENSOR_READING;

	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_get_sensor_reading_req *request =
	    (struct pldm_get_sensor_reading_req *)msg->payload;

	request->sensor_id = htole16(sensor_id);
	request->rearm_event_state = rearm_event_state;

	return PLDM_SUCCESS;
}

int decode_get_sensor_reading_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint8_t *sensor_data_size, uint8_t *sensor_operational_state,
    uint8_t *sensor_event_message_enable, uint8_t *present_state,
    uint8_t *previous_state, uint8_t *event_state, uint8_t *present_reading)
{
	if (msg == NULL || completion_code == NULL ||
	    sensor_data_size == NULL || sensor_operational_state == NULL ||
	    sensor_event_message_enable == NULL || present_state == NULL ||
	    previous_state == NULL || event_state == NULL ||
	    present_reading == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length < PLDM_GET_SENSOR_READING_MIN_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_sensor_reading_resp *response =
	    (struct pldm_get_sensor_reading_resp *)msg->payload;

	if (response->sensor_data_size > PLDM_SENSOR_DATA_SIZE_SINT32) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (response->sensor_data_size > *sensor_data_size) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*sensor_data_size = response->sensor_data_size;
	*sensor_operational_state = response->sensor_operational_state;
	*sensor_event_message_enable = response->sensor_event_message_enable;
	*present_state = response->present_state;
	*previous_state = response->previous_state;
	*event_state = response->event_state;

	if (*sensor_data_size == PLDM_EFFECTER_DATA_SIZE_UINT8 ||
	    *sensor_data_size == PLDM_EFFECTER_DATA_SIZE_SINT8) {
		if (payload_length != PLDM_GET_SENSOR_READING_MIN_RESP_BYTES) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		*present_reading = response->present_reading[0];

	} else if (*sensor_data_size == PLDM_EFFECTER_DATA_SIZE_UINT16 ||
		   *sensor_data_size == PLDM_EFFECTER_DATA_SIZE_SINT16) {
		if (payload_length !=
		    PLDM_GET_SENSOR_READING_MIN_RESP_BYTES + 1) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		memcpy(present_reading, response->present_reading, 2);
		uint16_t *val = (uint16_t *)(present_reading);
		*val = le16toh(*val);

	} else if (*sensor_data_size == PLDM_EFFECTER_DATA_SIZE_UINT32 ||
		   *sensor_data_size == PLDM_EFFECTER_DATA_SIZE_SINT32) {
		if (payload_length !=
		    PLDM_GET_SENSOR_READING_MIN_RESP_BYTES + 3) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		memcpy(present_reading, response->present_reading, 4);
		uint32_t *val = (uint32_t *)(present_reading);
		*val = le32toh(*val);
	}

	return PLDM_SUCCESS;
}

int encode_get_sensor_reading_resp(
    uint8_t instance_id, uint8_t completion_code, uint8_t sensor_data_size,
    uint8_t sensor_operational_state, uint8_t sensor_event_message_enable,
    uint8_t present_state, uint8_t previous_state, uint8_t event_state,
    uint8_t *present_reading, struct pldm_msg *msg, size_t payload_length)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	if (msg == NULL || present_reading == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (sensor_data_size > PLDM_EFFECTER_DATA_SIZE_SINT32) {
		return PLDM_ERROR_INVALID_DATA;
	}

	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_PLATFORM;
	header.command = PLDM_GET_SENSOR_READING;

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_get_sensor_reading_resp *response =
	    (struct pldm_get_sensor_reading_resp *)msg->payload;

	response->completion_code = completion_code;
	response->sensor_data_size = sensor_data_size;
	response->sensor_operational_state = sensor_operational_state;
	response->sensor_event_message_enable = sensor_event_message_enable;
	response->present_state = present_state;
	response->previous_state = previous_state;
	response->event_state = event_state;

	if (sensor_data_size == PLDM_EFFECTER_DATA_SIZE_UINT8 ||
	    sensor_data_size == PLDM_EFFECTER_DATA_SIZE_SINT8) {
		if (payload_length != PLDM_GET_SENSOR_READING_MIN_RESP_BYTES) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		response->present_reading[0] = *present_reading;

	} else if (sensor_data_size == PLDM_EFFECTER_DATA_SIZE_UINT16 ||
		   sensor_data_size == PLDM_EFFECTER_DATA_SIZE_SINT16) {
		if (payload_length !=
		    PLDM_GET_SENSOR_READING_MIN_RESP_BYTES + 1) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		uint16_t val = *(uint16_t *)present_reading;
		val = htole16(val);
		memcpy(response->present_reading, &val, 2);

	} else if (sensor_data_size == PLDM_EFFECTER_DATA_SIZE_UINT32 ||
		   sensor_data_size == PLDM_EFFECTER_DATA_SIZE_SINT32) {
		if (payload_length !=
		    PLDM_GET_SENSOR_READING_MIN_RESP_BYTES + 3) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
		uint32_t val = *(uint32_t *)present_reading;
		val = htole32(val);
		memcpy(response->present_reading, &val, 4);
	}

	return PLDM_SUCCESS;
}

int decode_get_sensor_reading_req(const struct pldm_msg *msg,
				  size_t payload_length, uint16_t *sensor_id,
				  uint8_t *rearm_event_state)
{
	if (msg == NULL || sensor_id == NULL || rearm_event_state == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_GET_SENSOR_READING_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_sensor_reading_req *request =
	    (struct pldm_get_sensor_reading_req *)msg->payload;

	*sensor_id = le16toh(request->sensor_id);
	*rearm_event_state = request->rearm_event_state;

	return PLDM_SUCCESS;
}
