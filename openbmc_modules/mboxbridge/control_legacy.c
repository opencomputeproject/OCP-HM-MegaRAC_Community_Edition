// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#include <errno.h>
#include <stdlib.h>

#include "common.h"
#include "dbus.h"
#include "control_dbus.h"
#include "mboxd.h"

/* Command IDs (Legacy interface) */
#define DBUS_C_PING            0x00
#define DBUS_C_DAEMON_STATE    0x01
#define DBUS_C_RESET           0x02
#define DBUS_C_SUSPEND         0x03
#define DBUS_C_RESUME          0x04
#define DBUS_C_MODIFIED        0x05
#define DBUS_C_KILL            0x06
#define DBUS_C_LPC_STATE       0x07
#define NUM_DBUS_CMDS          (DBUS_C_LPC_STATE + 1)

/* Return Values (Legacy interface) */
#define DBUS_SUCCESS           0x00 /* Command Succeded */
#define E_DBUS_INTERNAL        0x01 /* Internal DBUS Error */
#define E_DBUS_INVAL           0x02 /* Invalid Command */
#define E_DBUS_REJECTED        0x03 /* Daemon Rejected Request */
#define E_DBUS_HARDWARE        0x04 /* BMC Hardware Error */
#define E_DBUS_NO_MEM          0x05 /* Failed Memory Allocation */

struct mbox_dbus_msg {
       uint8_t cmd;
       size_t num_args;
       uint8_t *args;
};

/*
 * Command: DBUS Ping
 * Ping the daemon
 *
 * Args: NONE
 * Resp: NONE
 */
static int control_legacy_ping(struct mbox_context *context,
			   struct mbox_dbus_msg *req,
			   struct mbox_dbus_msg *resp)
{
	return control_ping(context);
}

/*
 * Command: DBUS Status
 * Get the status of the daemon
 *
 * Args: NONE
 * Resp[0]: Status Code
 */
static int control_legacy_daemon_state(struct mbox_context *context,
					  struct mbox_dbus_msg *req,
					  struct mbox_dbus_msg *resp)
{
	resp->num_args = DAEMON_STATE_NUM_ARGS;
	resp->args = calloc(resp->num_args, sizeof(*resp->args));
	resp->args[0] = control_daemon_state(context);

	return 0;
}

/*
 * Command: DBUS LPC State
 * Get the state of the lpc bus mapping (whether it points to memory or flash
 *
 * Args: NONE
 * Resp[0]: LPC Bus State Code
 */
static int control_legacy_lpc_state(struct mbox_context *context,
				       struct mbox_dbus_msg *req,
				       struct mbox_dbus_msg *resp)
{
	resp->num_args = LPC_STATE_NUM_ARGS;
	resp->args = calloc(resp->num_args, sizeof(*resp->args));
	resp->args[0] = control_lpc_state(context);

	return 0;
}

/*
 * Command: DBUS Reset
 * Reset the daemon state, final operation TBA.
 * For now we just point the lpc mapping back at the flash.
 *
 * Args: NONE
 * Resp: NONE
 */
static int control_legacy_reset(struct mbox_context *context,
				   struct mbox_dbus_msg *req,
				   struct mbox_dbus_msg *resp)
{
	int rc;

	rc = control_reset(context);

	/* Map return codes for compatibility */
	if (rc == -EBUSY) {
		return -E_DBUS_REJECTED;
	} else if (rc < 0) {
		return -E_DBUS_HARDWARE;
	}

	return rc;
}

/*
 * Command: DBUS Kill
 * Stop the daemon
 *
 * Args: NONE
 * Resp: NONE
 */
static int control_legacy_kill(struct mbox_context *context,
				  struct mbox_dbus_msg *req,
				  struct mbox_dbus_msg *resp)
{
	return control_kill(context);
}

/*
 * Command: DBUS Flash Modified
 * Used to notify the daemon that the flash has been modified out from under
 * it - We need to reset all out windows to ensure flash will be reloaded
 * when a new window is opened.
 * Note: We don't flush any previously opened windows
 *
 * Args: NONE
 * Resp: NONE
 */
static int control_legacy_modified(struct mbox_context *context,
				      struct mbox_dbus_msg *req,
				      struct mbox_dbus_msg *resp)
{
	return control_modified(context);
}

/*
 * Command: DBUS Suspend
 * Suspend the daemon to inhibit it from performing flash accesses.
 * This is used to synchronise access to the flash between the daemon and
 * directly from the BMC.
 *
 * Args: NONE
 * Resp: NONE
 */
static int control_legacy_suspend(struct mbox_context *context,
				     struct mbox_dbus_msg *req,
				     struct mbox_dbus_msg *resp)
{
	int rc;

	rc = control_suspend(context);
	if (rc < 0) {
		/* Map return codes for compatibility */
		return -E_DBUS_HARDWARE;
	}

	return rc;
}

/*
 * Command: DBUS Resume
 * Resume the daemon to let it perform flash accesses again.
 *
 * Args[0]: Flash Modified (0 - no | 1 - yes)
 * Resp: NONE
 */
static int control_legacy_resume(struct mbox_context *context,
				    struct mbox_dbus_msg *req,
				    struct mbox_dbus_msg *resp)
{
	int rc;

	if (req->num_args != 1) {
		return -E_DBUS_INVAL;
	}

	rc = control_resume(context, req->args[0] == RESUME_FLASH_MODIFIED);
	if (rc < 0) {
		/* Map return codes for compatibility */
		rc = -E_DBUS_HARDWARE;
	}

	return rc;
}

typedef int (*control_action)(struct mbox_context *context,
				 struct mbox_dbus_msg *req,
				 struct mbox_dbus_msg *resp);
static const control_action dbus_handlers[NUM_DBUS_CMDS] = {
	control_legacy_ping,
	control_legacy_daemon_state,
	control_legacy_reset,
	control_legacy_suspend,
	control_legacy_resume,
	control_legacy_modified,
	control_legacy_kill,
	control_legacy_lpc_state
};

static int method_cmd(sd_bus_message *m, void *userdata,
		      sd_bus_error *ret_error)
{
	struct mbox_dbus_msg req = { 0 }, resp = { 0 };
	struct mbox_context *context;
	sd_bus_message *n;
	int rc, i;

	context = (struct mbox_context *) userdata;
	if (!context) {
		MSG_ERR("DBUS Internal Error\n");
		rc = -E_DBUS_INTERNAL;
		goto out;
	}

	/* Read the command */
	rc = sd_bus_message_read(m, "y", &req.cmd);
	if (rc < 0) {
		MSG_ERR("DBUS error reading message: %s\n", strerror(-rc));
		rc = -E_DBUS_INTERNAL;
		goto out;
	}
	MSG_DBG("DBUS request: %u\n", req.cmd);

	/* Read the args */
	rc = sd_bus_message_read_array(m, 'y', (const void **) &req.args,
				       &req.num_args);
	if (rc < 0) {
		MSG_ERR("DBUS error reading message: %s\n", strerror(-rc));
		rc = -E_DBUS_INTERNAL;
		goto out;
	}
	MSG_DBG("DBUS num_args: %u\n", (unsigned) req.num_args);
	for (i = 0; i < req.num_args; i++) {
		MSG_DBG("DBUS arg[%d]: %u\n", i, req.args[i]);
	}

	/* Handle the command */
	if (req.cmd >= NUM_DBUS_CMDS) {
		rc = -E_DBUS_INVAL;
		MSG_ERR("Received unknown dbus cmd: %d\n", req.cmd);
	} else {
		rc = dbus_handlers[req.cmd](context, &req, &resp);
	}

out:
	if (rc < 0) {
		resp.cmd = -rc;
	}
	rc = sd_bus_message_new_method_return(m, &n); /* Generate response */
	if (rc < 0) {
		MSG_ERR("sd_bus_message_new_method_return failed: %d\n", rc);
		goto cleanup;
	}

	rc = sd_bus_message_append(n, "y", resp.cmd); /* Set return code */
	if (rc < 0) {
		MSG_ERR("sd_bus_message_append failed: %d\n", rc);
		goto cleanup;
	}

	rc = sd_bus_message_append_array(n, 'y', resp.args, resp.num_args);
	if (rc < 0) {
		MSG_ERR("sd_bus_message_append_array failed: %d\n", rc);
		goto cleanup;
	}

	MSG_DBG("DBUS response: %u\n", resp.cmd);
	MSG_DBG("DBUS num_args: %u\n", (unsigned) resp.num_args);
	for (i = 0; i < resp.num_args; i++) {
		MSG_DBG("DBUS arg[%d]: %u\n", i, resp.args[i]);
	}

	rc = sd_bus_send(NULL, n, NULL); /* Send response */
	sd_bus_message_unref(n);
	if (rc < 0)
		MSG_ERR("sd_bus_send failed: %d\n", rc);

cleanup:
	free(resp.args);
	return rc;
}

static const sd_bus_vtable control_legacy_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("cmd", "yay", "yay", &method_cmd,
		      SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_VTABLE_END
};

int control_legacy_init(struct mbox_context *context)
{
	int rc;

	rc = sd_bus_add_object_vtable(context->bus, NULL,
				      MBOX_DBUS_LEGACY_OBJECT,
				      MBOX_DBUS_LEGACY_NAME,
				      control_legacy_vtable, context);
	if (rc < 0) {
		MSG_ERR("Failed to register vtable: %s\n", strerror(-rc));
		return rc;
	}

	return sd_bus_request_name(context->bus, MBOX_DBUS_LEGACY_NAME,
				 SD_BUS_NAME_ALLOW_REPLACEMENT |
				 SD_BUS_NAME_REPLACE_EXISTING);
}

void control_legacy_free(struct mbox_context *context __attribute__((unused)))
{
	return;
}
