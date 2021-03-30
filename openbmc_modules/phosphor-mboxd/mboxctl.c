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

#include "config.h"
#include "dbus.h"

#define USAGE \
"\nUsage: %s [--silent | -s] <command> [args]\n\n" \
"\t\t--silent\t\t- no output on the command line\n\n" \
"\tCommands: (num args)\n" \
"\t\t--ping\t\t\t- ping the daemon (0)\n" \
"\t\t--daemon-state\t\t- check state of the daemon (0)\n" \
"\t\t--lpc-state\t\t- check the state of the lpc mapping (0)\n" \
"\t\t--kill\t\t\t- stop the daemon [no flush] (0)\n" \
"\t\t--reset\t\t\t- hard reset the daemon state (0)\n" \
"\t\t--point-to-flash\t- point the lpc mapping back to flash (0)\n" \
"\t\t--suspend\t\t- suspend the daemon to inhibit flash accesses (0)\n" \
"\t\t--resume\t\t- resume the daemon (1)\n" \
"\t\t\targ[0]: < \"clean\" | \"modified\" >\n" \
"\t\t--clear-cache\t- tell the daemon to discard any caches (0)\n"

#define NAME		"Mailbox Control"

static bool silent;

#define MSG_OUT(...)	do { if (!silent) { \
				fprintf(stdout, __VA_ARGS__); } \
			} while (0)
#define MSG_ERR(...)	do { if (!silent) { \
				fprintf(stderr, __VA_ARGS__); } \
			} while (0)

struct mboxctl_context {
	sd_bus *bus;
};

static void usage(char *name)
{
	MSG_OUT(USAGE, name);
	exit(0);
}

static const char *dbus_err_str[] = {
	"Success",
	"Failed - Internal Error",
	"Failed - Invalid Command or Request",
	"Failed - Request Rejected by Daemon",
	"Failed - BMC Hardware Error",
	"Failed - Insufficient Memory for Allocation Request"
};

static int init_mboxctl_dbus(struct mboxctl_context *context)
{
	int rc;

	rc = sd_bus_default_system(&context->bus);
	if (rc < 0) {
		MSG_ERR("Failed to connect to the system bus: %s\n",
			strerror(-rc));
	}

	return rc;
}

static int send_dbus_msg(struct mboxctl_context *context,
			 struct mbox_dbus_msg *msg,
			 struct mbox_dbus_msg *resp)
{
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *n = NULL;
	uint8_t *buf;
	size_t sz;
	int rc;

	/* Generate the bus message */
	rc = sd_bus_message_new_method_call(context->bus, &m, DBUS_NAME,
					    DOBJ_NAME, DBUS_NAME, "cmd");
	if (rc < 0) {
		MSG_ERR("Failed to init method call: %s\n",
			strerror(-rc));
		rc = -E_DBUS_INTERNAL;
		goto out;
	}

	/* Add the command */
	rc = sd_bus_message_append(m, "y", msg->cmd);
	if (rc < 0) {
		MSG_ERR("Failed to add cmd to message: %s\n",
			strerror(-rc));
		rc = -E_DBUS_INTERNAL;
		goto out;
	}

	/* Add the args */
	rc = sd_bus_message_append_array(m, 'y', msg->args, msg->num_args);
	if (rc < 0) {
		MSG_ERR("Failed to add args to message: %s\n",
			strerror(-rc));
		rc = -E_DBUS_INTERNAL;
		goto out;
	}

	/* Send the message */
	rc = sd_bus_call(context->bus, m, 0, &error, &n);
	if (rc < 0) {
		MSG_ERR("Failed to post message: %s\n", strerror(-rc));
		rc = -E_DBUS_INTERNAL;
		goto out;
	}

	/* Read response code */
	rc = sd_bus_message_read(n, "y", &resp->cmd);
	if (rc < 0) {
		MSG_ERR("Failed to read response code: %s\n",
			strerror(-rc));
		rc = -E_DBUS_INTERNAL;
		goto out;
	}

	/* Read response args */
	rc = sd_bus_message_read_array(n, 'y', (const void **) &buf, &sz);
	if (rc < 0) {
		MSG_ERR("Failed to read response args: %s\n",
			strerror(-rc));
		rc = -E_DBUS_INTERNAL;
		goto out;
	}

	if (sz < resp->num_args) {
		MSG_ERR("Command returned insufficient response args\n");
		rc = -E_DBUS_INTERNAL;
		goto out;
	}

	memcpy(resp->args, buf, resp->num_args);
	rc = 0;

out:
	sd_bus_error_free(&error);
	sd_bus_message_unref(m);
	sd_bus_message_unref(n);

	return rc;
}

static int handle_cmd_ping(struct mboxctl_context *context)
{
	struct mbox_dbus_msg msg = { 0 }, resp = { 0 };
	int rc;

	msg.cmd = DBUS_C_PING;

	rc = send_dbus_msg(context, &msg, &resp);
	if (rc < 0) {
		MSG_ERR("Failed to send ping command\n");
		return rc;
	}

	rc = -resp.cmd;
	MSG_OUT("Ping: %s\n", dbus_err_str[-rc]);

	return rc;
}

static int handle_cmd_daemon_state(struct mboxctl_context *context)
{
	struct mbox_dbus_msg msg = { 0 }, resp = { 0 };
	int rc;

	msg.cmd = DBUS_C_DAEMON_STATE;
	resp.num_args = DAEMON_STATE_NUM_ARGS;
	resp.args = calloc(resp.num_args, sizeof(*resp.args));
	if (!resp.args) {
		MSG_ERR("Memory allocation failed\n");
		return -E_DBUS_NO_MEM;
	}

	rc = send_dbus_msg(context, &msg, &resp);
	if (rc < 0) {
		MSG_ERR("Failed to send daemon state command\n");
		goto out;
	}

	rc = -resp.cmd;
	if (resp.cmd != DBUS_SUCCESS) {
		MSG_ERR("Daemon state command failed\n");
		goto out;
	}

	MSG_OUT("Daemon State: %s\n", resp.args[0] == DAEMON_STATE_ACTIVE ?
				      "Active" : "Suspended");

out:
	free(resp.args);
	return rc;
}

static int handle_cmd_lpc_state(struct mboxctl_context *context)
{
	struct mbox_dbus_msg msg = { 0 }, resp = { 0 };
	int rc;

	msg.cmd = DBUS_C_LPC_STATE;
	resp.num_args = LPC_STATE_NUM_ARGS;
	resp.args = calloc(resp.num_args, sizeof(*resp.args));
	if (!resp.args) {
		MSG_ERR("Memory allocation failed\n");
		return -E_DBUS_NO_MEM;
	}

	rc = send_dbus_msg(context, &msg, &resp);
	if (rc < 0) {
		MSG_ERR("Failed to send lpc state command\n");
		goto out;
	}

	rc = -resp.cmd;
	if (resp.cmd != DBUS_SUCCESS) {
		MSG_ERR("LPC state command failed\n");
		goto out;
	}

	MSG_OUT("LPC Bus Maps: %s\n", resp.args[0] == LPC_STATE_MEM ?
				      "BMC Memory" :
				      (resp.args[0] == LPC_STATE_FLASH ?
				       "Flash Device" :
				       "Invalid System State"));

out:
	free(resp.args);
	return rc;
}

static int handle_cmd_kill(struct mboxctl_context *context)
{
	struct mbox_dbus_msg msg = { 0 }, resp = { 0 };
	int rc;

	msg.cmd = DBUS_C_KILL;

	rc = send_dbus_msg(context, &msg, &resp);
	if (rc < 0) {
		MSG_ERR("Failed to send kill command\n");
		return rc;
	}

	rc = -resp.cmd;
	MSG_OUT("Kill: %s\n", dbus_err_str[-rc]);

	return rc;
}

static int handle_cmd_reset(struct mboxctl_context *context)
{
	struct mbox_dbus_msg msg = { 0 }, resp = { 0 };
	int rc;

	msg.cmd = DBUS_C_RESET;

	rc = send_dbus_msg(context, &msg, &resp);
	if (rc < 0) {
		MSG_ERR("Failed to send reset command\n");
		return rc;
	}

	rc = -resp.cmd;
	MSG_OUT("Reset: %s\n", dbus_err_str[-rc]);

	return rc;
}

static int handle_cmd_suspend(struct mboxctl_context *context)
{
	struct mbox_dbus_msg msg = { 0 }, resp = { 0 };
	int rc;

	msg.cmd = DBUS_C_SUSPEND;

	rc = send_dbus_msg(context, &msg, &resp);
	if (rc < 0) {
		MSG_ERR("Failed to send suspend command\n");
		return rc;
	}

	rc = -resp.cmd;
	MSG_OUT("Suspend: %s\n", dbus_err_str[-rc]);

	return rc;
}

static int handle_cmd_resume(struct mboxctl_context *context, char *arg)
{
	struct mbox_dbus_msg msg = { 0 }, resp = { 0 };
	int rc;

	if (!arg) {
		MSG_ERR("Resume command takes an argument\n");
		return -E_DBUS_INVAL;
	}

	msg.cmd = DBUS_C_RESUME;
	msg.num_args = RESUME_NUM_ARGS;
	msg.args = calloc(msg.num_args, sizeof(*msg.args));
	if (!msg.args) {
		MSG_ERR("Memory allocation failed\n");
		return -E_DBUS_NO_MEM;
	}

	if (!strncmp(arg, "clean", strlen("clean"))) {
		msg.args[0] = RESUME_NOT_MODIFIED;
	} else if (!strncmp(arg, "modified", strlen("modified"))) {
		msg.args[0] = RESUME_FLASH_MODIFIED;
	} else {
		MSG_ERR("Resume command takes argument < \"clean\" | "
			"\"modified\" >\n");
		rc = -E_DBUS_INVAL;
		goto out;
	}

	rc = send_dbus_msg(context, &msg, &resp);
	if (rc < 0) {
		MSG_ERR("Failed to send resume command\n");
		goto out;
	}

	rc = -resp.cmd;
	MSG_OUT("Resume: %s\n", dbus_err_str[-rc]);

out:
	free(msg.args);
	return rc;
}

static int handle_cmd_modified(struct mboxctl_context *context)
{
	struct mbox_dbus_msg msg = { 0 }, resp = { 0 };
	int rc;

	msg.cmd = DBUS_C_MODIFIED;

	rc = send_dbus_msg(context, &msg, &resp);
	if (rc < 0) {
		MSG_ERR("Failed to send flash modified command\n");
		return rc;
	}

	rc = -resp.cmd;
	MSG_OUT("Clear Cache: %s\n", dbus_err_str[-rc]);

	return rc;
}

static int parse_cmdline(struct mboxctl_context *context, int argc, char **argv)
{
	int opt, rc = -1;

	static const struct option long_options[] = {
		{ "silent",		no_argument,		0, 's' },
		{ "ping",		no_argument,		0, 'p' },
		{ "daemon-state",	no_argument,		0, 'd' },
		{ "lpc-state",		no_argument,		0, 'l' },
		{ "kill",		no_argument,		0, 'k' },
		{ "reset",		no_argument,		0, 'r' },
		{ "point-to-flash",	no_argument,		0, 'f' },
		{ "suspend",		no_argument,		0, 'u' },
		{ "resume",		required_argument,	0, 'e' },
		{ "clear-cache",	no_argument,		0, 'c' },
		{ "version",		no_argument,		0, 'v' },
		{ "help",		no_argument,		0, 'h' },
		{ 0,			0,			0, 0   }
	};

	if (argc <= 1) {
		usage(argv[0]);
		return -E_DBUS_INVAL;
	}

	while ((opt = getopt_long(argc, argv, "spdlkrfue:cvh", long_options,
				  NULL)) != -1) {
		switch (opt) {
		case 's':
			silent = true;
			continue;
		case 'p':
			rc = handle_cmd_ping(context);
			break;
		case 'd':
			rc = handle_cmd_daemon_state(context);
			break;
		case 'l':
			rc = handle_cmd_lpc_state(context);
			break;
		case 'k':
			rc = handle_cmd_kill(context);
			break;
		case 'r': /* These are the same for now (reset may change) */
		case 'f':
			rc = handle_cmd_reset(context);
			break;
		case 'u':
			rc = handle_cmd_suspend(context);
			break;
		case 'e':
			rc = handle_cmd_resume(context, optarg);
			break;
		case 'c':
			rc = handle_cmd_modified(context);
			break;
		case 'v':
			MSG_OUT("%s V%s\n", NAME, PACKAGE_VERSION);
			rc = 0;
			break;
		case 'h':
			usage(argv[0]);
			rc = 0;
			break;
		default:
			usage(argv[0]);
			rc = -E_DBUS_INVAL;
			break;
		}
	}

	return rc;
}

int main(int argc, char **argv)
{
	struct mboxctl_context context;
	int rc;

	silent = false;

	rc = init_mboxctl_dbus(&context);
	if (rc < 0) {
		MSG_ERR("Failed to init dbus\n");
		return rc;
	}

	rc = parse_cmdline(&context, argc, argv);

	return rc;
}
