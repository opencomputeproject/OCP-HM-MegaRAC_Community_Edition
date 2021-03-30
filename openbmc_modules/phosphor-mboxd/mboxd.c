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
#include <sys/signalfd.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <systemd/sd-bus.h>

#include "config.h"
#include "mbox.h"
#include "common.h"
#include "dbus.h"
#include "mboxd_dbus.h"
#include "mboxd_flash.h"
#include "mboxd_lpc.h"
#include "mboxd_msg.h"
#include "mboxd_windows.h"
#include "vpnor/mboxd_pnor_partition_table.h"

#define USAGE \
"\nUsage: %s [-V | --version] [-h | --help] [-v[v] | --verbose] [-s | --syslog]\n" \
"\t\t[-n | --window-num <num>]\n" \
"\t\t[-w | --window-size <size>M]\n" \
"\t\t-f | --flash <size>[K|M]\n\n" \
"\t-v | --verbose\t\tBe [more] verbose\n" \
"\t-s | --syslog\t\tLog output to syslog (pointless without -v)\n" \
"\t-n | --window-num\tThe number of windows\n" \
"\t\t\t\t(default: fill the reserved memory region)\n" \
"\t-w | --window-size\tThe window size (power of 2) in MB\n" \
"\t\t\t\t(default: 1MB)\n" \
"\t-f | --flash\t\tSize of flash in [K|M] bytes\n\n"

static int poll_loop(struct mbox_context *context)
{
	int rc = 0, i;

	/* Set POLLIN on polling file descriptors */
	for (i = 0; i < POLL_FDS; i++) {
		context->fds[i].events = POLLIN;
	}

	while (1) {
		rc = poll(context->fds, POLL_FDS, -1);

		if (rc < 0) { /* Error */
			MSG_ERR("Error from poll(): %s\n", strerror(errno));
			break; /* This should mean we clean up nicely */
		}

		/* Event on Polled File Descriptor - Handle It */
		if (context->fds[SIG_FD].revents & POLLIN) { /* Signal */
			struct signalfd_siginfo info = { 0 };

			rc = read(context->fds[SIG_FD].fd, (void *) &info,
				  sizeof(info));
			if (rc != sizeof(info)) {
				MSG_ERR("Error reading signal event: %s\n",
					strerror(errno));
			}

			MSG_DBG("Received signal: %d\n", info.ssi_signo);
			switch (info.ssi_signo) {
			case SIGINT:
			case SIGTERM:
				MSG_INFO("Caught Signal - Exiting...\n");
				context->terminate = true;
				break;
			case SIGHUP:
				/* Host didn't request reset -> Notify it */
				reset_all_windows(context, SET_BMC_EVENT);
				rc = reset_lpc(context);
				if (rc < 0) {
					MSG_ERR("WARNING: Failed to point the "
						"LPC bus back to flash on "
						"SIGHUP\nIf the host requires "
						"this expect problems...\n");
				}
				break;
			default:
				MSG_ERR("Unhandled Signal: %d\n",
					info.ssi_signo);
				break;
			}
		}
		if (context->fds[DBUS_FD].revents & POLLIN) { /* DBUS */
			while ((rc = sd_bus_process(context->bus, NULL)) > 0) {
				MSG_DBG("DBUS Event\n");
			}
			if (rc < 0) {
				MSG_ERR("Error handling DBUS event: %s\n",
						strerror(-rc));
			}
		}
		if (context->terminate) {
			break; /* This should mean we clean up nicely */
		}
		if (context->fds[MBOX_FD].revents & POLLIN) { /* MBOX */
			MSG_DBG("MBOX Event\n");
			rc = dispatch_mbox(context);
			if (rc < 0) {
				MSG_ERR("Error handling MBOX event\n");
			}
		}
	}

	/* Best to reset windows and the lpc mapping for safety */
	/* Host didn't request reset -> Notify it */
	reset_all_windows(context, SET_BMC_EVENT);
	rc = reset_lpc(context);
	/* Not much we can do if this fails */
	if (rc < 0) {
		MSG_ERR("WARNING: Failed to point the LPC bus back to flash\n"
			"If the host requires this expect problems...\n");
	}

	return rc;
}

static int init_signals(struct mbox_context *context, sigset_t *set)
{
	int rc;

	/* Block SIGHUPs, SIGTERMs and SIGINTs */
	sigemptyset(set);
	sigaddset(set, SIGHUP);
	sigaddset(set, SIGINT);
	sigaddset(set, SIGTERM);
	rc = sigprocmask(SIG_BLOCK, set, NULL);
	if (rc < 0) {
		MSG_ERR("Failed to set SIG_BLOCK mask %s\n", strerror(errno));
		return rc;
	}

	/* Get Signal File Descriptor */
	rc = signalfd(-1, set, SFD_NONBLOCK);
	if (rc < 0) {
		MSG_ERR("Failed to get signalfd %s\n", strerror(errno));
		return rc;
	}

	context->fds[SIG_FD].fd = rc;
	return 0;
}

static void usage(const char *name)
{
	printf(USAGE, name);
}

static bool parse_cmdline(int argc, char **argv,
			  struct mbox_context *context)
{
	char *endptr;
	int opt;

	static const struct option long_options[] = {
		{ "flash",		required_argument,	0, 'f' },
		{ "window-size",	optional_argument,	0, 'w' },
		{ "window-num",		optional_argument,	0, 'n' },
		{ "verbose",		no_argument,		0, 'v' },
		{ "syslog",		no_argument,		0, 's' },
		{ "version",		no_argument,		0, 'V' },
		{ "help",		no_argument,		0, 'h' },
		{ 0,			0,			0, 0   }
	};

	verbosity = MBOX_LOG_NONE;
	mbox_vlog = &mbox_log_console;

	context->current = NULL; /* No current window */

	while ((opt = getopt_long(argc, argv, "f:w::n::vsVh", long_options, NULL))
			!= -1) {
		switch (opt) {
		case 0:
			break;
		case 'f':
			context->flash_size = strtol(optarg, &endptr, 10);
			if (optarg == endptr) {
				fprintf(stderr, "Unparseable flash size\n");
				return false;
			}
			switch (*endptr) {
			case '\0':
				break;
			case 'M':
				context->flash_size <<= 10;
			case 'K':
				context->flash_size <<= 10;
				break;
			default:
				fprintf(stderr, "Unknown units '%c'\n",
					*endptr);
				return false;
			}
			break;
		case 'n':
			context->windows.num = strtol(argv[optind], &endptr,
						      10);
			if (optarg == endptr || *endptr != '\0') {
				fprintf(stderr, "Unparseable window num\n");
				return false;
			}
			break;
		case 'w':
			context->windows.default_size = strtol(argv[optind],
							       &endptr, 10);
			context->windows.default_size <<= 20; /* Given in MB */
			if (optarg == endptr || (*endptr != '\0' &&
						 *endptr != 'M')) {
				fprintf(stderr, "Unparseable window size\n");
				return false;
			}
			if (!is_power_of_2(context->windows.default_size)) {
				fprintf(stderr, "Window size not power of 2\n");
				return false;
			}
			break;
		case 'v':
			verbosity++;
			break;
		case 's':
			/* Avoid a double openlog() */
			if (mbox_vlog != &vsyslog) {
				openlog(PREFIX, LOG_ODELAY, LOG_DAEMON);
				mbox_vlog = &vsyslog;
			}
			break;
		case 'V':
			printf("%s V%s\n", THIS_NAME, PACKAGE_VERSION);
			exit(0);
		case 'h':
			return false; /* This will print the usage message */
		default:
			return false;
		}
	}

	if (!context->flash_size) {
		fprintf(stderr, "Must specify a non-zero flash size\n");
		return false;
	}

	MSG_INFO("Flash size: 0x%.8x\n", context->flash_size);

	if (verbosity) {
		MSG_INFO("%s logging\n", verbosity == MBOX_LOG_DEBUG ? "Debug" :
					"Verbose");
	}

	return true;
}

int main(int argc, char **argv)
{
	struct mbox_context *context;
	char *name = argv[0];
	sigset_t set;
	int rc, i;

	context = calloc(1, sizeof(*context));
	if (!context) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(1);
	}

	if (!parse_cmdline(argc, argv, context)) {
		usage(name);
		free(context);
		exit(0);
	}

	for (i = 0; i < TOTAL_FDS; i++) {
		context->fds[i].fd = -1;
	}

	MSG_INFO("Starting Daemon\n");

	rc = init_signals(context, &set);
	if (rc) {
		goto finish;
	}

	rc = init_mbox_dev(context);
	if (rc) {
		goto finish;
	}

	rc = init_lpc_dev(context);
	if (rc) {
		goto finish;
	}

	/* We've found the reserved memory region -> we can assign to windows */
	rc = init_windows(context);
	if (rc) {
		goto finish;
	}

	rc = init_flash_dev(context);
	if (rc) {
		goto finish;
	}

	rc = init_mboxd_dbus(context);
	if (rc) {
		goto finish;
	}

#ifdef VIRTUAL_PNOR_ENABLED
	init_vpnor(context);
#endif

	/* Set the LPC bus mapping */
	rc = reset_lpc(context);
	if (rc) {
		MSG_ERR("LPC configuration failed, RESET required: %d\n", rc);
	}

	rc = set_bmc_events(context, BMC_EVENT_DAEMON_READY, SET_BMC_EVENT);
	if (rc) {
		goto finish;
	}

	MSG_INFO("Entering Polling Loop\n");
	rc = poll_loop(context);

	MSG_INFO("Exiting Poll Loop: %d\n", rc);

finish:
	MSG_INFO("Daemon Exiting...\n");
	clr_bmc_events(context, BMC_EVENT_DAEMON_READY, SET_BMC_EVENT);

	free_mboxd_dbus(context);
	free_flash_dev(context);
	free_lpc_dev(context);
	free_mbox_dev(context);
	free_windows(context);
#ifdef VIRTUAL_PNOR_ENABLED
	destroy_vpnor(context);
#endif
	free(context);

	return rc;
}
