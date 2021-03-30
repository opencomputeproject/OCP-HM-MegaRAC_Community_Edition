/**
 * Copyright © 2016 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <systemd/sd-bus.h>

#define DBUS_MAX_NAME_LEN 256

const char *objectmapper_service_name =  "xyz.openbmc_project.ObjectMapper";
const char *objectmapper_object_name  =  "/xyz/openbmc_project/object_mapper";
const char *objectmapper_intf_name    =  "xyz.openbmc_project.ObjectMapper";

typedef struct {
	int fan_num;
	int cpu_num;
	int core_num;
	int dimm_num;
	sd_bus *bus;
	char sensor_service[DBUS_MAX_NAME_LEN];
	char inventory_service[DBUS_MAX_NAME_LEN];
} fan_info_t;

/* Get an object's bus name from ObjectMapper */
int get_connection(sd_bus *bus, char *connection, const char *obj_path)
{
	sd_bus_error bus_error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL;
	char *temp_buf = NULL, *intf = NULL;
	int rc;

	rc = sd_bus_call_method(bus,
				objectmapper_service_name,
				objectmapper_object_name,
				objectmapper_intf_name,
				"GetObject",
				&bus_error,
				&m,
				"ss",
				obj_path,
				"");
	if (rc < 0) {
		fprintf(stderr,
			"Failed to GetObject: %s\n", bus_error.message);
		goto finish;
	}

	/* Get the key, aka, the bus name */
	sd_bus_message_read(m, "a{sas}", 1, &temp_buf, 1, &intf);
	strncpy(connection, temp_buf, DBUS_MAX_NAME_LEN);

finish:
	sd_bus_error_free(&bus_error);
	sd_bus_message_unref(m);
	sd_bus_flush(bus);

	return rc;
}


int set_dbus_sensor(sd_bus *bus, const char *obj_path, int val)
{
	char connection[DBUS_MAX_NAME_LEN];
	sd_bus_error bus_error = SD_BUS_ERROR_NULL;
	sd_bus_message *response = NULL;
	int rc;

	if (!bus || !obj_path)
		return -1;

	rc = get_connection(bus, connection, obj_path);
	if (rc < 0) {
		fprintf(stderr,
			"fanctl: Failed to get bus name for %s\n", obj_path);
		goto finish;
	}

	rc = sd_bus_set_property(bus,
				connection,
				obj_path,
				"xyz.openbmc_project.Control.FanSpeed",
				"Target",
				&bus_error,
				"i",
				val);
	if (rc < 0)
		fprintf(stderr,
			"fanctl: Failed to set sensor %s:[%s]\n",
			obj_path, strerror(-rc));

finish:
	sd_bus_error_free(&bus_error);
	sd_bus_message_unref(response);
	sd_bus_flush(bus);

	return rc;
}

/* Read sensor value from "xyz.openbmc_projects.Sensors" */
int read_dbus_sensor(sd_bus *bus, const char *obj_path, const char *property)
{
	char connection[DBUS_MAX_NAME_LEN];
	sd_bus_error bus_error = SD_BUS_ERROR_NULL;
	sd_bus_message *response = NULL;
	int rc;
	int val = 0;

	if (!bus || !obj_path)
		return 0;

	rc = get_connection(bus, connection, obj_path);
	if (rc < 0) {
		val = 0;
		fprintf(stderr,
			"fanctl: Failed to get bus name for %s\n", obj_path);
		goto finish;
	}

	rc = sd_bus_get_property(bus,
				connection,
				obj_path,
				"xyz.openbmc_project.Sensor.Value",
				property,
				&bus_error,
				&response,
				"i");
	if (rc < 0) {
		val = 0;
		fprintf(stderr,
			"fanctl: Failed to read sensor value from %s:[%s]\n",
			obj_path, strerror(-rc));
		goto finish;
	}

	rc = sd_bus_message_read(response, "i", &val);
	if (rc < 0) {
		val = 0;
		fprintf(stderr,
			"fanctl: Failed to parse sensor value "
			"response message from %s:[%s]\n",
			obj_path, strerror(-rc));
	}

finish:
	sd_bus_error_free(&bus_error);
	sd_bus_message_unref(response);
	sd_bus_flush(bus);

	return val;
}

/* set fan speed with /xyz/openbmc_project/sensors/fan_tach/fan* object */
static int fan_set_speed(sd_bus *bus, int fan_id, uint8_t fan_speed)
{
	char obj_path[DBUS_MAX_NAME_LEN];
	int rc;

	if (!bus)
		return -1;

	snprintf(obj_path, sizeof(obj_path),
		"/xyz/openbmc_project/sensors/fan_tach/fan%d_0", fan_id);
	rc = set_dbus_sensor(bus, obj_path, fan_speed);
	if (rc < 0)
		fprintf(stderr, "fanctl: Failed to set fan[%d] speed[%d]\n",
				fan_id, fan_speed);

	return rc;
}

static int fan_set_max_speed(fan_info_t *info)
{
	int i;
	int rc = -1;

	if (!info)
		return -1;
	for (i = 0; i < info->fan_num; i++) {
		rc = fan_set_speed(info->bus, i, 255);
		if (rc < 0)
			break;
		fprintf(stderr, "fanctl: Set fan%d to max speed\n", i);
	}

	return rc;
}
/*
 * FAN_TACH_OFFSET is specific to Barreleye.
 * Barreleye uses NTC7904D HW Monitor as Fan tachometoer.
 * The 13-bit FANIN value is made up Higher part: [12:5],
 * and Lower part: [4:0], which are read from two sensors.
 * see: https://www.nuvoton.com/resource-files/NCT7904D_Datasheet_V1.44.pdf
 */
#define FAN_TACH_OFFSET 5
static int fan_get_speed(sd_bus *bus, int fan_id)
{
	int fan_tach_H = 0, fan_tach_L = 0;
	char obj_path[DBUS_MAX_NAME_LEN];
	int fan_speed;

	/* get fan tach */
	snprintf(obj_path, sizeof(obj_path),
		"/xyz/openbmc_project/sensors/fan_tach/fan%d_0", fan_id);
	fan_tach_H = read_dbus_sensor(bus, obj_path, "MaxValue");
	fan_tach_L = read_dbus_sensor(bus, obj_path, "MinValue");

	/* invalid sensor value is -1 */
	if (fan_tach_H <= 0 || fan_tach_L <= 0)
		fan_speed = 0;
	else
		fan_speed = fan_tach_H << FAN_TACH_OFFSET | fan_tach_L;

	fprintf(stderr, "fan%d speed: %d\n", fan_id, fan_speed);
	return fan_speed;
}

/* set Fan Inventory 'Present' status */
int fan_set_present(sd_bus *bus, int fan_id, int val)
{
	sd_bus_error bus_error = SD_BUS_ERROR_NULL;
	sd_bus_message *response = NULL;
	int rc;
	char obj_path[DBUS_MAX_NAME_LEN];
	char connection[DBUS_MAX_NAME_LEN];

	snprintf(obj_path, sizeof(obj_path),
		"/xyz/openbmc_project/inventory/system/chassis/motherboard/fan%d",
		fan_id);

	rc = get_connection(bus, connection, obj_path);
	if (rc < 0) {
		fprintf(stderr,
			"fanctl: Failed to get bus name for %s\n", obj_path);
		goto finish;
	}

	rc = sd_bus_set_property(bus,
				connection,
				obj_path,
				"xyz.openbmc_project.Inventory.Item",
				"Present",
				&bus_error,
				"b",
				val);
	if(rc < 0)
		fprintf(stderr,
			"fanctl: Failed to update fan presence via dbus: %s\n",
			bus_error.message);

	fprintf(stderr, "fanctl: Set fan%d present status to: %s\n",
			fan_id, (val == 1 ? "True" : "False"));

finish:
	sd_bus_error_free(&bus_error);
	sd_bus_message_unref(response);
	sd_bus_flush(bus);

	return rc;
}

/*
 * Update Fan Invertory 'Present' status by first reading fan speed.
 * If fan speed is '0', the fan is considerred not 'Present'.
 */
static int fan_update_present(fan_info_t *info)
{
	int i;
	int rc = -1;
	int fan_speed;

	if (!info)
		return -1;

	for (i = 0; i < info->fan_num; i++) {
		fan_speed = fan_get_speed(info->bus, i);
		if (fan_speed > 0)
			rc = fan_set_present(info->bus, i, 1);
		else
			rc = fan_set_present(info->bus, i, 0);

		if (rc < 0) {
			fprintf(stderr,
				"fanctl: Failed to set fan present status\n");
			break;
		}
	}

	return rc;
}
/*
 * Router function for any FAN operations that come via dbus
 */
static int fan_function_router(sd_bus_message *msg, void *user_data,
		sd_bus_error *ret_error)
{
	/* Generic error reporter. */
	int rc = -1;
	fan_info_t *info = user_data;

	/* Get the Operation. */
	const char *fan_function = sd_bus_message_get_member(msg);
	if (fan_function == NULL) {
		fprintf(stderr, "fanctl: Null FAN function specified\n");
		return sd_bus_reply_method_return(msg, "i", rc);
	}

	/* Route the user action to appropriate handlers. */
	if ((strcmp(fan_function, "setMax") == 0)) {
		rc = fan_set_max_speed(info);
		return sd_bus_reply_method_return(msg, "i", rc);
	}
	if ((strcmp(fan_function, "updatePresent") == 0)) {
		rc = fan_update_present(info);
		return sd_bus_reply_method_return(msg, "i", rc);
	}

	return sd_bus_reply_method_return(msg, "i", rc);
}

/* Dbus Services offered by this FAN controller */
static const sd_bus_vtable fan_control_vtable[] =
{
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("setMax", "", "i", &fan_function_router,
			SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("updatePresent", "", "i", &fan_function_router,
			SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_VTABLE_END,
};

int start_fan_services(fan_info_t *info)
{
	/* Generic error reporter. */
	int rc = -1;
	/* slot where we are offering the FAN dbus service. */
	sd_bus_slot *fan_slot = NULL;
	const char *fan_object = "/org/openbmc/control/fans";

	info->bus = NULL;
	/* Get a hook onto system bus. */
	rc = sd_bus_open_system(&info->bus);
	if (rc < 0) {
		fprintf(stderr,"fanctl: Error opening system bus.\n");
		return rc;
	}

	/* Install the object */
	rc = sd_bus_add_object_vtable(info->bus,
			&fan_slot,
			fan_object, /* object path */
			"org.openbmc.control.Fans", /* interface name */
			fan_control_vtable,
			info);
	if (rc < 0) {
		fprintf(stderr, "fanctl: Failed to add object to dbus: %s\n",
				strerror(-rc));
		return rc;
	}

	/* If we had success in adding the providers, request for a bus name. */
	rc = sd_bus_request_name(info->bus,
			"org.openbmc.control.Fans", 0);
	if (rc < 0) {
		fprintf(stderr, "fanctl: Failed to acquire service name: %s\n",
				strerror(-rc));
		return rc;
	}

	for (;;) {
		/* Process requests */
		rc = sd_bus_process(info->bus, NULL);
		if (rc < 0) {
			fprintf(stderr, "fanctl: Failed to process bus: %s\n",
					strerror(-rc));
			break;
		}
		if (rc > 0) {
			continue;
		}

		rc = sd_bus_wait(info->bus, (uint64_t) - 1);
		if (rc < 0) {
			fprintf(stderr, "fanctl: Failed to wait on bus: %s\n",
				strerror(-rc));
			break;
		}
	}

	sd_bus_slot_unref(fan_slot);
	sd_bus_unref(info->bus);

	return rc;
}

static int str_to_int(char *str)
{
	long val;
	char *temp;

	val = strtol(str, &temp, 10);
	if (temp == str || *temp != '\0' ||
		((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE))
		return -1;
	if (val < 0)
		return -1;

	return (int)val;
}

static int parse_argument(int argc, char **argv, fan_info_t *info)
{
	int c;
	struct option long_options[] =
	{
		{"fan_num",  required_argument, 0, 'f'},
		{"core_num",    required_argument, 0, 'c'},
		{"cpu_num",    required_argument, 0, 'p'},
		{"dimm_num",    required_argument, 0, 'd'},
		{0, 0, 0, 0}
	};

	while (1) {
		c = getopt_long (argc, argv, "c:d:f:p:", long_options, NULL);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
		case 'f':
			info->fan_num = str_to_int(optarg);
			if (info->fan_num == -1) {
				fprintf(stderr, "fanctl: Wrong fan_num: %s\n", optarg);
				return -1;
			}
			break;
		case 'c':
			info->core_num = str_to_int(optarg);
			if (info->core_num == -1) {
				fprintf(stderr, "fanctl: Wrong core_num: %s\n", optarg);
				return -1;
			}
			break;
		case 'p':
			info->cpu_num = str_to_int(optarg);
			if (info->cpu_num == -1) {
				fprintf(stderr, "fanctl: Wrong cpu_num: %s\n", optarg);
				return -1;
			}
			break;
		case 'd':
			info->dimm_num = str_to_int(optarg);
			if (info->dimm_num == -1) {
				fprintf(stderr, "fanctl: Wrong dimm_num: %s\n", optarg);
				return -1;
			}
			break;
		default:
			fprintf(stderr, "fanctl: Wrong argument\n");
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int rc = 0;
	fan_info_t fan_info;

	memset(&fan_info, 0, sizeof(fan_info));
	rc = parse_argument(argc, argv, &fan_info);
	if (rc < 0) {
		fprintf(stderr, "fanctl: Error parse argument\n");
		return rc;
	}
	/* This call is not supposed to return. If it does, then an error */
	rc = start_fan_services(&fan_info);
	if (rc < 0) {
		fprintf(stderr, "fanctl: Error starting FAN Services. Exiting");
	}

	return rc;
}
