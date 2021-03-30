/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef MBOX_DBUS_H
#define MBOX_DBUS_H

#define DBUS_NAME		"org.openbmc.mboxd"
#define DOBJ_NAME		"/org/openbmc/mboxd"

/* Commands */
#define DBUS_C_PING		0x00
#define	DBUS_C_DAEMON_STATE	0x01
#define DBUS_C_RESET		0x02
#define DBUS_C_SUSPEND		0x03
#define DBUS_C_RESUME		0x04
#define DBUS_C_MODIFIED		0x05
#define DBUS_C_KILL		0x06
#define DBUS_C_LPC_STATE	0x07
#define NUM_DBUS_CMDS		(DBUS_C_LPC_STATE + 1)

/* Command Args */
/* Resume */
#define RESUME_NUM_ARGS		1
#define RESUME_NOT_MODIFIED	0x00
#define RESUME_FLASH_MODIFIED	0x01

/* Return Values */
#define DBUS_SUCCESS		0x00 /* Command Succeded */
#define E_DBUS_INTERNAL		0x01 /* Internal DBUS Error */
#define E_DBUS_INVAL		0x02 /* Invalid Command */
#define E_DBUS_REJECTED		0x03 /* Daemon Rejected Request */
#define E_DBUS_HARDWARE		0x04 /* BMC Hardware Error */
#define E_DBUS_NO_MEM		0x05 /* Failed Memory Allocation */

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

struct mbox_dbus_msg {
	uint8_t cmd;
	size_t num_args;
	uint8_t *args;
};

#endif /* MBOX_DBUS_H */
