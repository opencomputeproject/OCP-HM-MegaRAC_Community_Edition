// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

#include "common.h"
#include "dbus.h"
#include "control_dbus.h"
#include "mboxd.h"

typedef int (*control_action)(struct mbox_context *context);

static int control_dbus_directive(sd_bus_message *m, void *userdata,
					sd_bus_error *ret_error,
					control_action action)
{
	struct mbox_context *context;
	sd_bus_message *n;
	int rc;

	if (!action) {
		MSG_ERR("No action provided\n");
		return -EINVAL; 
	}

	context = (struct mbox_context *) userdata;
	if (!context) {
		MSG_ERR("DBUS Internal Error\n");
		return -EINVAL;
	}

	rc = action(context);
	if (rc < 0) {
		MSG_ERR("Action failed: %d\n", rc);
		return rc;
	}

	rc = sd_bus_message_new_method_return(m, &n);
	if (rc < 0) {
		MSG_ERR("sd_bus_message_new_method_return failed: %d\n", rc);
		return rc;
	}

	rc = sd_bus_send(NULL, n, NULL);
	sd_bus_message_unref(n);
	return rc;
}

static int control_dbus_ping(sd_bus_message *m, void *userdata,
				   sd_bus_error *ret_error)
{
	return control_dbus_directive(m, userdata, ret_error, control_ping);
}

static int control_dbus_reset(sd_bus_message *m, void *userdata,
				    sd_bus_error *ret_error)
{
	return control_dbus_directive(m, userdata, ret_error, control_reset);
}

static int control_dbus_kill(sd_bus_message *m, void *userdata,
				   sd_bus_error *ret_error)
{
	return control_dbus_directive(m, userdata, ret_error, control_kill);
}

static int control_dbus_modified(sd_bus_message *m, void *userdata,
				       sd_bus_error *ret_error)
{
	return control_dbus_directive(m, userdata, ret_error, control_modified);
}

static int control_dbus_suspend(sd_bus_message *m, void *userdata,
				      sd_bus_error *ret_error)
{
	return control_dbus_directive(m, userdata, ret_error, control_suspend);
}

static int control_dbus_resume(sd_bus_message *m, void *userdata,
				     sd_bus_error *ret_error)
{
	struct mbox_context *context;
	sd_bus_message *n;
	bool modified;
	int rc;

	context = (struct mbox_context *) userdata;
	if (!context) {
		MSG_ERR("DBUS Internal Error\n");
		return -EINVAL;
	}

	rc = sd_bus_message_read_basic(m, 'b', &modified);
	if (rc < 0) {
		MSG_ERR("DBUS error reading message: %s\n", strerror(-rc));
		return rc;
	}

	rc = control_resume(context, modified);
	if (rc < 0)
		return rc;

	rc = sd_bus_message_new_method_return(m, &n);
	if (rc < 0) {
		MSG_ERR("sd_bus_message_new_method_return failed: %d\n", rc);
		return rc;
	}

	return sd_bus_send(NULL, n, NULL);
}

static int control_dbus_set_backend(sd_bus_message *m, void *userdata,
				    sd_bus_error *ret_error)
{
	struct mbox_context *context;
	struct backend backend;
	sd_bus_message *n;
	const char *name;
	int rc;

	context = (struct mbox_context *) userdata;
	if (!context) {
		MSG_ERR("DBUS Internal Error\n");
		return -EINVAL;
	}

	rc = sd_bus_message_read_basic(m, 's', &name);
	if (rc < 0) {
		MSG_ERR("DBUS error reading message: %s\n", strerror(-rc));
		return rc;
	}

	if (!strcmp(name, "vpnor")) {
		struct vpnor_partition_paths paths;

		vpnor_default_paths(&paths);
		backend = backend_get_vpnor();
		rc = control_set_backend(context, &backend, &paths);
		if (rc < 0)
			return rc;
	} else if (!strcmp(name, "mtd")) {
		char **paths = NULL;
		char *path = NULL;

		rc = sd_bus_message_read_strv(m, &paths);
		if (rc < 0)
			return rc;

		if (paths && *paths)
			path = *paths;
		else
			path = get_dev_mtd();

		backend = backend_get_mtd();

		rc = control_set_backend(context, &backend, path);
		if (rc < 0)
			return rc;

		free(path);
		free(paths);
	} else if (!strcmp(name, "file")) {
		char **paths = NULL;
		char *path = NULL;

		rc = sd_bus_message_read_strv(m, &paths);
		if (rc < 0)
			return rc;

		if (!(paths && *paths))
			return -EINVAL;

		path = *paths;

		backend = backend_get_file();

		rc = control_set_backend(context, &backend, path);
		if (rc < 0)
			return rc;

		free(path);
		free(paths);
	} else {
		return -EINVAL;
	}

	rc = sd_bus_message_new_method_return(m, &n);
	if (rc < 0) {
		MSG_ERR("sd_bus_message_new_method_return failed: %d\n", rc);
		return rc;
	}

	rc = sd_bus_send(NULL, n, NULL);
	sd_bus_message_unref(n);
	return rc;
}

static int control_dbus_get_u8(sd_bus *bus, const char *path,
			       const char *interface, const char *property,
			       sd_bus_message *reply, void *userdata,
			       sd_bus_error *ret_error)
{
	struct mbox_context *context = userdata;
	uint8_t value;

	assert(!strcmp(MBOX_DBUS_OBJECT, path));

	if (!strcmp("DaemonState", property)) {
		value = control_daemon_state(context);
	} else if (!strcmp("LpcState", property)) {
		value = control_lpc_state(context);
	} else {
		MSG_ERR("Unknown DBus property: %s\n", property);
		return -EINVAL;
	}

	return sd_bus_message_append(reply, "y", value);
}

static const sd_bus_vtable mboxd_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("Ping", NULL, NULL, &control_dbus_ping,
		      SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("Reset", NULL, NULL, &control_dbus_reset,
		      SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("Kill", NULL, NULL, &control_dbus_kill,
		      SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("MarkFlashModified", NULL, NULL, &control_dbus_modified,
		      SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("Suspend", NULL, NULL, &control_dbus_suspend,
		      SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("Resume", "b", NULL, &control_dbus_resume,
		      SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("SetBackend", "sas", NULL, &control_dbus_set_backend,
		      SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_PROPERTY("DaemonState", "y", &control_dbus_get_u8, 0,
		        SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
	SD_BUS_PROPERTY("LpcState", "y", &control_dbus_get_u8, 0,
		        SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
	SD_BUS_VTABLE_END
};

int control_dbus_init(struct mbox_context *context)
{
	return sd_bus_add_object_vtable(context->bus, NULL,
					MBOX_DBUS_OBJECT,
					MBOX_DBUS_CONTROL_IFACE,
					mboxd_vtable, context);
}

#define __unused __attribute__((unused))
void control_dbus_free(struct mbox_context *context __unused)
{
	return;
}
