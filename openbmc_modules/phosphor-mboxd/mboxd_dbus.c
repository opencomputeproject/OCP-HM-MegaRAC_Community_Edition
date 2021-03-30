// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <systemd/sd-bus.h>

#include "mbox.h"
#include "common.h"
#include "dbus.h"
#include "mboxd_dbus.h"
#include "mboxd_windows.h"
#include "mboxd_msg.h"
#include "mboxd_lpc.h"
#include "mboxd_flash.h"

typedef int (*mboxd_dbus_handler)(struct mbox_context *, struct mbox_dbus_msg *,
				  struct mbox_dbus_msg *);

/* DBUS Functions */

/*
 * Command: DBUS Ping
 * Ping the daemon
 *
 * Args: NONE
 * Resp: NONE
 */
static int dbus_handle_ping(struct mbox_context *context,
			    struct mbox_dbus_msg *req,
			    struct mbox_dbus_msg *resp)
{
	return 0;
}

/*
 * Command: DBUS Status
 * Get the status of the daemon
 *
 * Args: NONE
 * Resp[0]: Status Code
 */
static int dbus_handle_daemon_state(struct mbox_context *context,
				    struct mbox_dbus_msg *req,
				    struct mbox_dbus_msg *resp)
{
	resp->num_args = DAEMON_STATE_NUM_ARGS;
	resp->args = calloc(resp->num_args, sizeof(*resp->args));
	resp->args[0] = (context->state & STATE_SUSPENDED) ?
			DAEMON_STATE_SUSPENDED : DAEMON_STATE_ACTIVE;

	return 0;
}

/*
 * Command: DBUS LPC State
 * Get the state of the lpc bus mapping (whether it points to memory or flash
 *
 * Args: NONE
 * Resp[0]: LPC Bus State Code
 */
static int dbus_handle_lpc_state(struct mbox_context *context,
				 struct mbox_dbus_msg *req,
				 struct mbox_dbus_msg *resp)
{
	resp->num_args = LPC_STATE_NUM_ARGS;
	resp->args = calloc(resp->num_args, sizeof(*resp->args));
	if ((context->state & MAPS_MEM) && !(context->state & MAPS_FLASH)) {
		resp->args[0] = LPC_STATE_MEM;
	} else if (!(context->state & MAPS_MEM) &&
		   (context->state & MAPS_FLASH)) {
		resp->args[0] = LPC_STATE_FLASH;
	} else {
		resp->args[0] = LPC_STATE_INVALID;
	}

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
static int dbus_handle_reset(struct mbox_context *context,
			     struct mbox_dbus_msg *req,
			     struct mbox_dbus_msg *resp)
{
	int rc;

	/* We don't let the host access flash if the daemon is suspened */
	if (context->state & STATE_SUSPENDED) {
		return -E_DBUS_REJECTED;
	}

	/*
	 * This will close (and flush) the current window and reset the lpc bus
	 * mapping back to flash, or memory in case we're using a virtual pnor.
	 * Better set the bmc event to notify the host of this.
	 */
	reset_all_windows(context, SET_BMC_EVENT);
	rc = reset_lpc(context);
	if (rc < 0) {
		return -E_DBUS_HARDWARE;
	}

	return 0;
}

/*
 * Command: DBUS Kill
 * Stop the daemon
 *
 * Args: NONE
 * Resp: NONE
 */
static int dbus_handle_kill(struct mbox_context *context,
			    struct mbox_dbus_msg *req,
			    struct mbox_dbus_msg *resp)
{
	context->terminate = 1;

	MSG_INFO("DBUS Kill - Exiting...\n");

	return 0;
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
static int dbus_handle_modified(struct mbox_context *context,
				struct mbox_dbus_msg *req,
				struct mbox_dbus_msg *resp)
{
	/* Flash has been modified - can no longer trust our erased bytemap */
	set_flash_bytemap(context, 0, context->flash_size, FLASH_DIRTY);

	/* Force daemon to reload all windows -> Set BMC event to notify host */
	reset_all_windows(context, SET_BMC_EVENT);

	return 0;
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
static int dbus_handle_suspend(struct mbox_context *context,
			       struct mbox_dbus_msg *req,
			       struct mbox_dbus_msg *resp)
{
	int rc;

	if (context->state & STATE_SUSPENDED) {
		/* Already Suspended */
		return DBUS_SUCCESS;
	}

	/* Nothing to check - Just set the bit to notify the host */
	rc = set_bmc_events(context, BMC_EVENT_FLASH_CTRL_LOST, SET_BMC_EVENT);
	if (rc < 0) {
		return -E_DBUS_HARDWARE;
	}

	context->state |= STATE_SUSPENDED;

	return rc;
}

/*
 * Command: DBUS Resume
 * Resume the daemon to let it perform flash accesses again.
 *
 * Args[0]: Flash Modified (0 - no | 1 - yes)
 * Resp: NONE
 */
static int dbus_handle_resume(struct mbox_context *context,
			      struct mbox_dbus_msg *req,
			      struct mbox_dbus_msg *resp)
{
	int rc;

	if (req->num_args != 1) {
		return -E_DBUS_INVAL;
	}

	if (!(context->state & STATE_SUSPENDED)) {
		/* We weren't suspended... */
		return DBUS_SUCCESS;
	}

	if (req->args[0] == RESUME_FLASH_MODIFIED) {
		/* Call the flash modified handler */
		dbus_handle_modified(context, req, resp);
	}

	/* Clear the bit and send the BMC Event to the host */
	rc = clr_bmc_events(context, BMC_EVENT_FLASH_CTRL_LOST, SET_BMC_EVENT);

	if (rc < 0) {
		rc = -E_DBUS_HARDWARE;
	}
	context->state &= ~STATE_SUSPENDED;

	return rc;
}

static const mboxd_dbus_handler dbus_handlers[NUM_DBUS_CMDS] = {
	dbus_handle_ping,
	dbus_handle_daemon_state,
	dbus_handle_reset,
	dbus_handle_suspend,
	dbus_handle_resume,
	dbus_handle_modified,
	dbus_handle_kill,
	dbus_handle_lpc_state
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
	if (rc < 0)
		MSG_ERR("sd_bus_send failed: %d\n", rc);

cleanup:
	free(resp.args);
	return rc;
}

static const sd_bus_vtable mboxd_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("cmd", "yay", "yay", &method_cmd,
		      SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_VTABLE_END
};

int init_mboxd_dbus(struct mbox_context *context)
{
	int rc;

	rc = sd_bus_default_system(&context->bus);
	if (rc < 0) {
		MSG_ERR("Failed to connect to the system bus: %s\n",
			strerror(-rc));
		return rc;
	}

	rc = sd_bus_add_object_vtable(context->bus, NULL, DOBJ_NAME, DBUS_NAME,
				      mboxd_vtable, context);
	if (rc < 0) {
		MSG_ERR("Failed to register vtable: %s\n", strerror(-rc));
		return rc;
	}

	rc = sd_bus_request_name(context->bus, DBUS_NAME,
				 SD_BUS_NAME_ALLOW_REPLACEMENT |
				 SD_BUS_NAME_REPLACE_EXISTING);
	if (rc < 0) {
		MSG_ERR("Failed to acquire service name: %s\n", strerror(-rc));
		return rc;
	}

	rc = sd_bus_get_fd(context->bus);
	if (rc < 0) {
		MSG_ERR("Failed to get bus fd: %s\n", strerror(-rc));
		return rc;
	}

	context->fds[DBUS_FD].fd = rc;

	return 0;
}

void free_mboxd_dbus(struct mbox_context *context)
{
	sd_bus_unref(context->bus);
}
