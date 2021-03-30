// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#include "config.h"

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <systemd/sd-bus.h>

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
"\t\t--clear-cache\t- tell the daemon to discard any caches (0)\n" \
"\t\t--backend <vpnor|mtd[:PATH]|file:PATH>\n"

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

static int mboxctl_directive(struct mboxctl_context *context, const char *cmd)
{
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL;
	int rc;

	rc = sd_bus_message_new_method_call(context->bus, &m,
					    MBOX_DBUS_NAME,
					    MBOX_DBUS_OBJECT,
					    MBOX_DBUS_CONTROL_IFACE,
					    cmd);
	if (rc < 0) {
		MSG_ERR("Failed to init method call: %s\n",
			strerror(-rc));
		goto out;
	}

	rc = sd_bus_call(context->bus, m, 0, &error, NULL);
	if (rc < 0) {
		MSG_ERR("Failed to post message: %s\n", strerror(-rc));
	}

out:
	sd_bus_error_free(&error);
	sd_bus_message_unref(m);

	return rc < 0 ? rc : 0;
}

static int mboxctl_getter(struct mboxctl_context *context,
			  const char *property, uint8_t *resp)
{
	sd_bus_error error = SD_BUS_ERROR_NULL;

	return sd_bus_get_property_trivial(context->bus, MBOX_DBUS_NAME,
					   MBOX_DBUS_OBJECT,
					   MBOX_DBUS_CONTROL_IFACE,
					   property, &error, 'y', resp);
}

static int handle_cmd_ping(struct mboxctl_context *context)
{
	int rc;

	rc = mboxctl_directive(context, "Ping");
	MSG_OUT("Ping: %s\n", rc ? strerror(-rc) : "Success");

	return rc;
}

static int handle_cmd_daemon_state(struct mboxctl_context *context)
{
	uint8_t resp;
	int rc;

	rc = mboxctl_getter(context, "DaemonState", &resp);
	if (rc < 0)
		return rc;

	MSG_OUT("Daemon State: %s\n", resp == DAEMON_STATE_ACTIVE ?
				      "Active" : "Suspended");
	return 0;
}

static int handle_cmd_lpc_state(struct mboxctl_context *context)
{
	uint8_t resp;
	int rc;

	rc = mboxctl_getter(context, "LpcState", &resp);
	if (rc < 0)
		return rc;

	MSG_OUT("LPC Bus Maps: %s\n", resp == LPC_STATE_MEM ?
				      "BMC Memory" :
				      (resp == LPC_STATE_FLASH ?
				       "Flash Device" :
				       "Invalid System State"));

	return 0;
}

static int handle_cmd_kill(struct mboxctl_context *context)
{
	int rc;

	rc = mboxctl_directive(context, "Kill");
	MSG_OUT("Kill: %s\n", rc ? strerror(-rc) : "Success");

	return rc;
}

static int handle_cmd_reset(struct mboxctl_context *context)
{
	int rc;

	rc = mboxctl_directive(context, "Reset");
	MSG_OUT("Reset: %s\n", rc ? strerror(-rc) : "Success");

	return rc;
}

static int handle_cmd_suspend(struct mboxctl_context *context)
{
	int rc;

	rc = mboxctl_directive(context, "Suspend");
	MSG_OUT("Suspend: %s\n", rc ? strerror(-rc) : "Success");

	return rc;
}

static int handle_cmd_resume(struct mboxctl_context *context, char *sarg)
{
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *n = NULL;
	uint8_t arg;
	int rc;

	if (!sarg) {
		MSG_ERR("Resume command takes an argument\n");
		return -EINVAL;
	}

	rc = sd_bus_message_new_method_call(context->bus, &m,
					    MBOX_DBUS_NAME,
					    MBOX_DBUS_OBJECT,
					    MBOX_DBUS_CONTROL_IFACE,
					    "Resume");
	if (rc < 0) {
		MSG_ERR("Failed to init method call: %s\n",
			strerror(-rc));
		goto out;
	}

	if (!strncmp(sarg, "clean", strlen("clean"))) {
		arg = RESUME_NOT_MODIFIED;
	} else if (!strncmp(sarg, "modified", strlen("modified"))) {
		arg = RESUME_FLASH_MODIFIED;
	} else {
		MSG_ERR("Resume command takes argument < \"clean\" | "
			"\"modified\" >\n");
		rc = -EINVAL;
		goto out;
	}

	rc = sd_bus_message_append(m, "b", arg);
	if (rc < 0) {
		MSG_ERR("Failed to add args to message: %s\n",
			strerror(-rc));
		goto out;
	}

	rc = sd_bus_call(context->bus, m, 0, &error, &n);
	if (rc < 0) {
		MSG_ERR("Failed to post message: %s\n", strerror(-rc));
		goto out;
	}

	MSG_OUT("Resume: %s\n", rc < 0 ? strerror(-rc) : "Success");

out:
	sd_bus_error_free(&error);
	sd_bus_message_unref(m);
	sd_bus_message_unref(n);

	return rc < 0 ? rc : 0;
}

static int handle_cmd_modified(struct mboxctl_context *context)
{
	int rc;

	rc = mboxctl_directive(context, "MarkFlashModified");
	MSG_OUT("Clear Cache: %s\n", rc ? strerror(-rc) : "Success");

	return rc;
}

static int handle_cmd_backend(struct mboxctl_context *context, char *sarg)
{
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *n = NULL;
	char *delim = NULL;
	char *strv[2];
	int rc;

	if (!sarg) {
		MSG_ERR("Backend command takes an argument\n");
		return -EINVAL;
	}

	rc = sd_bus_message_new_method_call(context->bus, &m,
					    MBOX_DBUS_NAME,
					    MBOX_DBUS_OBJECT,
					    MBOX_DBUS_CONTROL_IFACE,
					    "SetBackend");
	if (rc < 0) {
		MSG_ERR("Failed to init method call: %s\n",
			strerror(-rc));
		goto out;
	}

	if (!strncmp(sarg, "vpnor", strlen("vpnor"))) {
		if (strchr(sarg, ':')) {
			MSG_ERR("Path parameter not supported for vpnor\n");
			rc = -EINVAL;
			goto out;
		}

		rc = sd_bus_message_append(m, "s", "vpnor");
		if (rc < 0)
			goto out;
	} else if (!strncmp(sarg, "mtd", strlen("mtd"))) {
		rc = sd_bus_message_append(m, "s", "mtd");
		if (rc < 0)
			goto out;
	} else if (!strncmp(sarg, "file", strlen("file"))) {
		rc = sd_bus_message_append(m, "s", "file");
		if (rc < 0)
			goto out;
	} else {
		rc = -EINVAL;
		goto out;
	}

	delim = strchr(sarg, ':');
	if (delim) {
		char *path;

		path = realpath(delim + 1, NULL);
		if (!path) {
			MSG_ERR("Failed to resolve path: %s\n",
					strerror(errno));
			rc = -errno;
			goto out;
		}

		strv[0] = path;
		strv[1] = NULL;

		rc = sd_bus_message_append_strv(m, &strv[0]);
		free(path);
		if (rc < 0)
			goto out;
	} else {
		strv[0] = NULL;
		strv[1] = NULL;
		rc = sd_bus_message_append_strv(m, &strv[0]);
		if (rc < 0)
			goto out;
	}

	rc = sd_bus_call(context->bus, m, 0, &error, &n);
	if (rc < 0) {
		MSG_ERR("Failed to post message: %s\n", strerror(-rc));
		goto out;
	}

	MSG_OUT("SetBackend: %s\n", rc < 0 ? strerror(-rc) : "Success");

out:
	sd_bus_error_free(&error);
	sd_bus_message_unref(m);

	return rc < 0 ? rc : 0;
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
		{ "backend",		required_argument,	0, 'b' },
		{ "version",		no_argument,		0, 'v' },
		{ "help",		no_argument,		0, 'h' },
		{ 0,			0,			0, 0   }
	};

	if (argc <= 1) {
		usage(argv[0]);
		return -EINVAL;
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
		case 'b':
			rc = handle_cmd_backend(context, optarg);
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
			rc = -EINVAL;
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
