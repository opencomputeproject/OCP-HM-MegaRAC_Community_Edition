/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef DBUS_H
#define DBUS_H

#include <stdint.h>
#include <stddef.h>

/*
 * "mbox" will become an inappropriate name for the protocol/daemon, so claim a
 * different name on the public interface.
 *
 * "hiomapd" expands to "Host I/O Map Daemon"
 *
 * TODO: The Great Rename
 */
#define MBOX_DBUS_NAME			"xyz.openbmc_project.Hiomapd"
#define MBOX_DBUS_OBJECT		"/xyz/openbmc_project/Hiomapd"
#define MBOX_DBUS_CONTROL_IFACE		"xyz.openbmc_project.Hiomapd.Control"
#define MBOX_DBUS_PROTOCOL_IFACE	"xyz.openbmc_project.Hiomapd.Protocol"
#define MBOX_DBUS_PROTOCOL_IFACE_V2	MBOX_DBUS_PROTOCOL_IFACE ".V2"

/* Legacy interface */
#define MBOX_DBUS_LEGACY_NAME		"org.openbmc.mboxd"
#define MBOX_DBUS_LEGACY_OBJECT		"/org/openbmc/mboxd"

/* Command Args */
/* Resume */
#define RESUME_NUM_ARGS		1
#define RESUME_NOT_MODIFIED	0x00
#define RESUME_FLASH_MODIFIED	0x01

/* Response Args */
/* Status */
#define DAEMON_STATE_NUM_ARGS	1
#define DAEMON_STATE_ACTIVE	0x00 /* Daemon Active */
#define DAEMON_STATE_SUSPENDED	0x01 /* Daemon Suspended */

/* LPC State */
#define LPC_STATE_NUM_ARGS	1
#define LPC_STATE_INVALID	0x00 /* Invalid State */
#define LPC_STATE_FLASH		0x01 /* LPC Maps Flash Directly */
#define LPC_STATE_MEM		0x02 /* LPC Maps Memory */

#endif /* MBOX_DBUS_H */
